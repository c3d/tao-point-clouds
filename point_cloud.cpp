// ****************************************************************************
//  point_cloud.cpp                                                Tao project
// ****************************************************************************
//
//   File Description:
//
//    Drawing a large number of points efficiently.
//
//
//
//
//
//
//
//
// ****************************************************************************
// This software is property of Taodyne SAS - Confidential
// Ce logiciel est la propriété de Taodyne SAS - Confidentiel
//  (C) 2012 Jerome Forissier <jerome@taodyne.com>
//  (C) 2012 Taodyne SAS
// ****************************************************************************

#include "tao/tao_gl.h"
#include "point_cloud.h"
#include "basics.h"      // From XLR
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QString>
#include <QTextStream>


// ============================================================================
//
//    Helpers
//
// ============================================================================

inline QString operator +(std::string s)
// ----------------------------------------------------------------------------
//   UTF-8 conversion from std::string to QString
// ----------------------------------------------------------------------------
{
    return QString::fromUtf8(s.data(), s.length());
}

inline std::string operator +(QString s)
// ----------------------------------------------------------------------------
//   UTF-8 conversion from QString to std::string
// ----------------------------------------------------------------------------
{
    return std::string(s.toUtf8().constData());
}


// ============================================================================
//
//    PointCloud
//
// ============================================================================

bool PointCloud::addPoint(const Point &p)
// ----------------------------------------------------------------------------
//   Add a new point to the cloud
// ----------------------------------------------------------------------------
{
    points.push_back(p);
    return true;
}



void PointCloud::removePoints(unsigned n)
// ----------------------------------------------------------------------------
//   Drop n points from the cloud
// ----------------------------------------------------------------------------
{
    if (n >= size())
        return clear();

    n -= size();
    while (n--)
        points.pop_back();
}


void PointCloud::draw()
// ----------------------------------------------------------------------------
//   Draw cloud
// ----------------------------------------------------------------------------
{
    if (points.size() == 0)
        return;

    // Activate current document color
    PointCloudFactory::tao->SetFillColor();

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Point), &points[0].x);
    glDrawArrays(GL_POINTS, 0, size());
    glDisableClientState(GL_VERTEX_ARRAY);
}


void PointCloud::clear()
// ----------------------------------------------------------------------------
//   Remove all points
// ----------------------------------------------------------------------------
{
    points.clear();
}


static float random01()
// ----------------------------------------------------------------------------
//   Return random float in [0.0, 1.0]
// ----------------------------------------------------------------------------
{
    return float(XL::xl_random(0.0, 1.0));
}


bool PointCloud::randomPoints(unsigned n)
// ----------------------------------------------------------------------------
//   Create a point cloud with n random points
// ----------------------------------------------------------------------------
{
    if (size() == n)
        return false;

    IFTRACE(pointcloud)
        debug() << "Points: " << size() << " requested: " << n << "\n";

    int delta = n - size();
    if (delta < 0)
        removePoints(-delta);
    else
        for (int i = 0; i < delta; ++i)
            addPoint(Point(random01(), random01(), random01()));

    return true;
}


bool PointCloud::loadData(text file, text sep, int xi, int yi, int zi)
// ----------------------------------------------------------------------------
//   Load points from a file
// ----------------------------------------------------------------------------
{
    if (file == this->file)
        return false;

    if (xi < 1 || yi < 1 || zi < 1)
    {
        error = "Invalid index value";
        return false;
    }

    text folder = PointCloudFactory::tao->currentDocumentFolder();
    QString qf = QString::fromUtf8(folder.data(), folder.length());
    QString qn = QString::fromUtf8(file.data(), file.length());
    QFileInfo inf(QDir(qf), qn);
    text path = +QDir::toNativeSeparators(inf.absoluteFilePath());
    QFile f(+path);
    if (!f.open(QIODevice::ReadOnly))
    {
        error = +QString("File not found or unreadable: $1\n"
                      "File path: %1").arg(+path);
        return false;
    }

    IFTRACE(pointcloud)
        debug() << "Loading " << path << "\n";

    clear();
    QTextStream t(&f);
    QString line;
    unsigned count = 0;
    QString separator = +sep;
    int max = qMax(qMax(xi, yi), zi);
    do
    {
        line = t.readLine();
        QStringList values = line.split(separator);
        if (values.size() < max)
            continue;
        bool xok, yok, zok;
        float x = values[xi-1].toFloat(&xok);
        float y = values[yi-1].toFloat(&yok);
        float z = values[zi-1].toFloat(&zok);
        if (xok && yok && zok)
        {
            addPoint(Point(x, y, z));
            count++;
        }
    }
    while (!line.isNull());
    f.close();

    this->file = file;
    IFTRACE(pointcloud)
        debug() << "Loaded " << count << " points\n";
    return true;
}


std::ostream & PointCloud::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[PointCloud] \"" << name << "\" " << (void*)this << " ";
    return std::cerr;
}



// ============================================================================
//
//    PointCloudVBO
//
// ============================================================================

PointCloudVBO::PointCloudVBO(text name)
// ----------------------------------------------------------------------------
//   Initialize object
// ----------------------------------------------------------------------------
    : PointCloud(name), dirty(false), optimized(false), dontOptimize(false),
      nbPoints(0), context(QGLContext::currentContext())
{
    genBuffer();
}


PointCloudVBO::~PointCloudVBO()
// ----------------------------------------------------------------------------
//   Destroy object
// ----------------------------------------------------------------------------
{
    delBuffer();
}


unsigned PointCloudVBO::size()
// ----------------------------------------------------------------------------
//   The number of points in the cloud
// ----------------------------------------------------------------------------
{
    if (optimized)
        return nbPoints;
    return PointCloud::size();
}


bool PointCloudVBO::addPoint(const Point &p)
// ----------------------------------------------------------------------------
//   Add a new point to the cloud
// ----------------------------------------------------------------------------
{
    if (optimized)
    {
        error = "Cannot add point to optimized cloud";
        return false;
    }

    PointCloud::addPoint(p);
    dirty = true;
    dontOptimize = true;
    return true;
}


void PointCloudVBO::removePoints(unsigned n)
// ----------------------------------------------------------------------------
//   Drop n points from the cloud
// ----------------------------------------------------------------------------
{
    Q_ASSERT(!optimized);

    PointCloud::removePoints(n);
    if (useVbo())
        updateVbo();
    dontOptimize = true;
}


void PointCloudVBO::draw()
// ----------------------------------------------------------------------------
//   Draw cloud
// ----------------------------------------------------------------------------
{
    if (!useVbo())
        return PointCloud::draw();

    checkGLContext();

    if (size() == 0)
        return;

    if (dirty)
        updateVbo();

    // Activate current document color
    PointCloudFactory::tao->SetFillColor();

    glEnableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexPointer(3, GL_FLOAT, sizeof(Point), 0);
    glDrawArrays(GL_POINTS, 0, size());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableClientState(GL_VERTEX_ARRAY);
}


bool PointCloudVBO::optimize()
// ----------------------------------------------------------------------------
//   Optimize point cloud data
// ----------------------------------------------------------------------------
{
    if (optimized || dontOptimize)
        return optimized;

    if (useVbo())
    {
        if (dirty)
            updateVbo();
        nbPoints = points.size();
        points.clear();
        optimized = true;
        IFTRACE(pointcloud)
            debug() << "Cloud optimized\n";
    }

    return true;
}


void PointCloudVBO::clear()
// ----------------------------------------------------------------------------
//   Remove all points
// ----------------------------------------------------------------------------
{
    if (size() == 0)
        return;

    if (optimized)
    {
        nbPoints = 0;
        optimized = false;
    }
    else
    {
        PointCloud::clear();
        if (useVbo())
            updateVbo();
    }
    file = "";
    nbRandom = 0;
}


bool PointCloudVBO::randomPoints(unsigned n)
// ----------------------------------------------------------------------------
//   Create a point cloud with n random points
// ----------------------------------------------------------------------------
{
    bool changed = PointCloud::randomPoints(n);
    if (useVbo() && changed)
    {
        updateVbo();
        dontOptimize = false;
    }
    return changed;
}


bool PointCloudVBO::loadData(text file, text sep, int xi, int yi, int zi)
// ----------------------------------------------------------------------------
//   Load points from a file
// ----------------------------------------------------------------------------
{
    bool changed = PointCloud::loadData(file, sep, xi, yi, zi);
    if (useVbo() && changed)
    {
        updateVbo();

        dontOptimize = false;
        this->sep = sep;
        this->xi = xi;
        this->yi = yi;
        this->zi = zi;
    }
    return changed;
}


void PointCloudVBO::checkGLContext()
// ----------------------------------------------------------------------------
//   Do what's needed if GL context has changed
// ----------------------------------------------------------------------------
{
    if (QGLContext::currentContext() != context)
    {
        IFTRACE(pointcloud)
            debug() << "GL context changed\n";

        // Re-create VBO
        genBuffer();
        if (optimized)
        {
            IFTRACE(pointcloud)
                debug() << "GL context changed on optimized cloud\n";

            Q_ASSERT(file != "" || nbRandom != 0);

            if (file != "")
            {
                IFTRACE(pointcloud)
                    debug() << "Reloading file\n";
                text f = file;
                clear();
                loadData(f, sep, xi, yi, zi);
            }
            else if (nbRandom != 0)
            {
                IFTRACE(pointcloud)
                    debug() << "Re-creating random points\n";
                unsigned n = nbRandom;
                clear();
                randomPoints(n);
            }

            optimized = false;
            Q_ASSERT(!dirty);
        }
        else
            updateVbo();

        context = QGLContext::currentContext();
    }
}


bool PointCloudVBO::useVbo()
// ----------------------------------------------------------------------------
//   Should we use Vertex Buffer Objects?
// ----------------------------------------------------------------------------
{
    return PointCloudFactory::vboSupported;
}


void PointCloudVBO::updateVbo()
// ----------------------------------------------------------------------------
//   Take into account a change in point data
// ----------------------------------------------------------------------------
{
    Q_ASSERT(!optimized);

    IFTRACE(pointcloud)
        debug() << "Updating VBO #" << vbo << " (" << size() << " points)\n";

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size()*sizeof(Point), &points[0].x,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    dirty = false;
}


void PointCloudVBO::genBuffer()
// ----------------------------------------------------------------------------
//   Allocate new VBO
// ----------------------------------------------------------------------------
{
    glGenBuffers(1, &vbo);
    IFTRACE(pointcloud)
        debug() << "Will use VBO id: " << vbo << "\n";
}


void PointCloudVBO::delBuffer()
// ----------------------------------------------------------------------------
//   Release VBO
// ----------------------------------------------------------------------------
{
    IFTRACE(pointcloud)
        debug() << "Releasing VBO id: " << vbo << "\n";
    glDeleteBuffers(1, &vbo);
}


std::ostream & PointCloudVBO::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[PointCloudVBO] \"" << name << "\" " << (void*)this << " ";
    return std::cerr;
}



// ============================================================================
//
//    PointCloudFactory
//
// ============================================================================

const Tao::ModuleApi * PointCloudFactory::tao = NULL;
PointCloudFactory::cloud_map PointCloudFactory::clouds;
bool PointCloudFactory::vboSupported = false;



PointCloud * PointCloudFactory::cloud(text name, LookupMode mode)
// ----------------------------------------------------------------------------
//   Find point cloud by name, optionally create new one
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = NULL;
    cloud_map::iterator found = clouds.find(name);
    if (found != clouds.end())
    {
        cloud = (*found).second;
        if (mode & LM_CLEAR_OPTIMIZED)
        {
            if (cloud && cloud->isOptimized())
            {
                IFTRACE(pointcloud)
                    sdebug() << "Cloud " << (void *)cloud << " has been optimized, "
                                "allocating a new one\n";
                clouds.erase(name);
                delete cloud;
                cloud = new PointCloudVBO(name);
                clouds[name] = cloud;
            }
        }
    }
    else if (mode & LM_CREATE)
    {
        cloud = new PointCloudVBO(name);
        clouds[name] = cloud;
    }
    return cloud;
}


void PointCloudFactory::init(const Tao::ModuleApi *api)
// ----------------------------------------------------------------------------
//   Initialize static part of class
// ----------------------------------------------------------------------------
{
    tao = api;
    glewInit();
    QString extensions((const char *)glGetString(GL_EXTENSIONS));
    vboSupported = extensions.contains("ARB_vertex_buffer_object");
    IFTRACE(pointcloud)
        sdebug() << "VBO supported: " << vboSupported << "\n";
}


void PointCloudFactory::render_callback(void *arg)
// ----------------------------------------------------------------------------
//   Find point cloud by name and draw it
// ----------------------------------------------------------------------------
{
    text name = text((const char *)arg);
    PointCloud * cloud = PointCloudFactory::cloud(name);
    if (cloud)
        cloud->draw();
}


void PointCloudFactory::delete_callback(void *arg)
// ----------------------------------------------------------------------------
//   Delete point cloud name
// ----------------------------------------------------------------------------
{
    free(arg);
}


XL::Name_p PointCloudFactory::cloud_drop(text name)
// ----------------------------------------------------------------------------
//   Purge the given point cloud from memory
// ----------------------------------------------------------------------------
{
    cloud_map::iterator found = clouds.find(name);
    if (found != clouds.end())
    {
        PointCloud *s = (*found).second;
        clouds.erase(found);
        delete s;
        return XL::xl_true;
    }
    return XL::xl_false;
}


XL::Name_p PointCloudFactory::cloud_only(text name)
// ----------------------------------------------------------------------------
//   Purge all other point clouds from memory
// ----------------------------------------------------------------------------
{
    cloud_map::iterator n = clouds.begin();
    for (cloud_map::iterator v = clouds.begin(); v != clouds.end(); v = n)
    {
        if (name != (*v).first)
        {
            PointCloud *s = (*v).second;
            clouds.erase(v);
            delete s;
            n = clouds.begin();
        }
        else
        {
            n = ++v;
        }
    }
    return XL::xl_false;
}


XL::name_p PointCloudFactory::cloud_show(text name)
// ----------------------------------------------------------------------------
//   Show point cloud
// ----------------------------------------------------------------------------
{
    tao->addToLayout(PointCloudFactory::render_callback, strdup(name.c_str()),
                     PointCloudFactory::delete_callback);
    return XL::xl_true;
}


XL::name_p PointCloudFactory::cloud_optimize(text name)
// ----------------------------------------------------------------------------
//   Optimize point cloud data
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = PointCloudFactory::cloud(name);
    if (!cloud)
        return XL::xl_false;

    bool opt = cloud->optimize();
    return opt ? XL::xl_true : XL::xl_false;
}


XL::Name_p PointCloudFactory::cloud_random(text name, XL::Integer_p points)
// ----------------------------------------------------------------------------
//   Create a point cloud with n random points
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = PointCloudFactory::cloud(name, LM_CREATE);
    if (!cloud)
        return XL::xl_false;

    bool changed = cloud->randomPoints(points->value);
    return changed ? XL::xl_true : XL::xl_false;
}


XL::Name_p PointCloudFactory::cloud_add(XL::Tree_p self,
                                        text name, XL::Real_p x, XL::Real_p y,
                                        XL::Real_p z)
// ----------------------------------------------------------------------------
//   Add point to a cloud
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = PointCloudFactory::cloud(name,
                                                 LM_CREATE |
                                                 LM_CLEAR_OPTIMIZED);
    if (!cloud)
        return XL::xl_false;

    bool changed = cloud->addPoint(PointCloud::Point(x->value, y->value,
                                                     z->value));
    if (!changed && cloud->error != "")
    {
        XL::Ooops(cloud->error, self);
        cloud->error.clear();
    }

    return changed ? XL::xl_true : XL::xl_false;
}


XL::Name_p PointCloudFactory::cloud_load_data(XL::Tree_p self,
                                              text name, text file, text fmt,
                                              int xi, int yi, int zi)
// ----------------------------------------------------------------------------
//   Load points from a file
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = PointCloudFactory::cloud(name, LM_CREATE);
    if (!cloud)
        return XL::xl_false;

    bool changed = cloud->loadData(file, fmt, xi, yi, zi);
    if (!changed && cloud->error != "")
    {
        XL::Ooops(cloud->error, self);
        cloud->error.clear();
    }

    return changed ? XL::xl_true : XL::xl_false;
}


std::ostream & PointCloudFactory::sdebug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[PointCloudFactory] ";
    return std::cerr;
}


XL_DEFINE_TRACES

int module_init(const Tao::ModuleApi *api, const Tao::ModuleInfo *mod)
// ----------------------------------------------------------------------------
//   Initialize the Tao module
// ----------------------------------------------------------------------------
{
    Q_UNUSED(mod);
    XL_INIT_TRACES();
    PointCloudFactory::init(api);
    return 0;
}


int module_exit()
// ----------------------------------------------------------------------------
//   Uninitialize the Tao module
// ----------------------------------------------------------------------------
{
    PointCloudFactory::cloud_only("");
    return 0;
}
