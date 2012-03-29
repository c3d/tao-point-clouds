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


using namespace XL;

const Tao::ModuleApi * PointCloud::tao = NULL;
PointCloud::cloud_map PointCloud::clouds;
bool PointCloud::vboSupported = false;


PointCloud::PointCloud(text name)
// ----------------------------------------------------------------------------
//   Create a point cloud
// ----------------------------------------------------------------------------
    : name(name), useVboIfAvailable(true), vbo(0), context(NULL), dirty(false)
{
    IFTRACE(pointcloud)
        debug() << "Creation\n";
    if (useVbo())
    {
        context = QGLContext::currentContext();
        genBuffer();
    }
}


PointCloud::~PointCloud()
// ----------------------------------------------------------------------------
//    Delete a point cloud
// ----------------------------------------------------------------------------
{
    IFTRACE(pointcloud)
        debug() << "Destroyed\n";
    if (vbo)
    {
        IFTRACE(pointcloud)
            debug() << "Deleting VBO id: " << vbo << "\n";
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
}


void PointCloud::genBuffer()
// ----------------------------------------------------------------------------
//   Allocate new VBO
// ----------------------------------------------------------------------------
{
    glGenBuffers(1, &vbo);
    IFTRACE(pointcloud)
        debug() << "Will use VBO id: " << vbo << "\n";
}


void PointCloud::addPoint(float x, float y, float z)
// ----------------------------------------------------------------------------
//   Add a new point to the cloud
// ----------------------------------------------------------------------------
{
    points.push_back(Point(x, y, z));
}


void PointCloud::pointsChanged()
// ----------------------------------------------------------------------------
//   Take into account a change in point data
// ----------------------------------------------------------------------------
{
    dirty = false;

    if (!useVbo())
    {
        // Nothing to do since point data are reloaded on each draw
        return;
    }

    IFTRACE(pointcloud)
        debug() << "Updating VBO\n";

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, points.size()*sizeof(Point), &points[0].x,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void PointCloud::Draw()
// ----------------------------------------------------------------------------
//   Draw cloud
// ----------------------------------------------------------------------------
{
    if (points.size() == 0)
        return;

    if (dirty)
        pointsChanged();

    // Activate current document color
    tao->SetFillColor();

    glEnableClientState(GL_VERTEX_ARRAY);

    if (useVbo())
    {
        checkGLContext();
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glVertexPointer(3, GL_FLOAT, sizeof(Point), 0);
    }
    else
    {
        glVertexPointer(3, GL_FLOAT, sizeof(Point), &points[0].x);
    }

    glDrawArrays(GL_POINTS, 0, points.size());

    if (useVbo())
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    glDisableClientState(GL_VERTEX_ARRAY);
}


bool PointCloud::useVbo()
// ----------------------------------------------------------------------------
//   Should we use Vertex Buffer Objects?
// ----------------------------------------------------------------------------
{
    return (useVboIfAvailable && vboSupported);
}


void PointCloud::checkGLContext()
// ----------------------------------------------------------------------------
//   Do what's needed if GL context has changed
// ----------------------------------------------------------------------------
{
    if (QGLContext::currentContext() != context)
    {
        IFTRACE(pointcloud)
            debug() << "GL context changed\n";

        // Re-create VBO and copy data
        genBuffer();
        pointsChanged();

        context = QGLContext::currentContext();
    }
}


std::ostream & PointCloud::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[PointCloud] \"" << name << "\" " << (void*)this << " ";
    return std::cerr;
}


std::ostream & PointCloud::sdebug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[PointCloud] ";
    return std::cerr;
}


PointCloud * PointCloud::cloud(text name, bool create)
// ----------------------------------------------------------------------------
//   Find point cloud by name, optionally create new one
// ----------------------------------------------------------------------------
{
    cloud_map::iterator found = clouds.find(name);
    if (found != clouds.end())
        return (*found).second;
    PointCloud *cloud = NULL;
    if (create)
    {
        cloud = new PointCloud(name);
        clouds[name] = cloud;
    }
    return cloud;
}


float PointCloud::random01()
// ----------------------------------------------------------------------------
//   Return random float in [0.0, 1.0]
// ----------------------------------------------------------------------------
{
    return float(xl_random(0.0, 1.0));
}


void PointCloud::init(const Tao::ModuleApi *api)
// ----------------------------------------------------------------------------
//   Initialize static part of class
// ----------------------------------------------------------------------------
{
    tao = api;
    glewInit();
    QString extensions((const char *)glGetString(GL_EXTENSIONS));
    vboSupported = extensions.contains("ARB_vertex_buffer_object");
    sdebug() << "VBO supported: " << vboSupported << "\n";
}


void PointCloud::render_callback(void *arg)
// ----------------------------------------------------------------------------
//   Find point cloud by name and draw it
// ----------------------------------------------------------------------------
{
    text name = text((const char *)arg);
    PointCloud * cloud = PointCloud::cloud(name);
    if (cloud)
        cloud->Draw();
}


void PointCloud::delete_callback(void *arg)
// ----------------------------------------------------------------------------
//   Delete point cloud name
// ----------------------------------------------------------------------------
{
    free(arg);
}


XL::Name_p PointCloud::cloud_drop(text name)
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


XL::Name_p PointCloud::cloud_only(text name)
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


XL::name_p PointCloud::cloud_show(text name)
// ----------------------------------------------------------------------------
//   Show point cloud
// ----------------------------------------------------------------------------
{
    tao->addToLayout(PointCloud::render_callback, strdup(name.c_str()),
                     PointCloud::delete_callback);
    return XL::xl_true;
}


XL::Name_p PointCloud::cloud_random(text name, XL::Integer_p points)
// ----------------------------------------------------------------------------
//   Create a point cloud with n random points
// ----------------------------------------------------------------------------
{
    Q_UNUSED(points);

    unsigned wanted = points->value;
    PointCloud *cloud = PointCloud::cloud(name);
    if (!cloud || cloud->points.size() != wanted)
    {
        if (!cloud)
            cloud = PointCloud::cloud(name, true);
        if (!cloud)
            return XL::xl_false;
        IFTRACE(pointcloud)
            cloud->debug() << "Points: " << cloud->points.size()
                           << " requested: " << wanted << "\n";
        if (cloud->points.size() != wanted)
        {
            while (cloud->points.size() > wanted)
                cloud->points.pop_back();
            for (unsigned i = cloud->points.size(); i < wanted; ++i)
                cloud->addPoint(random01(), random01(), random01());
            cloud->pointsChanged();
        }
    }
    return XL::xl_true;
}


XL::Name_p PointCloud::cloud_add(text name,
                                 XL::Real_p x, XL::Real_p y, XL::Real_p z)
// ----------------------------------------------------------------------------
//   Add point to a cloud
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = PointCloud::cloud(name, true);
    if (!cloud)
        return XL::xl_false;

    cloud->addPoint(x->value, y->value, z->value);
    // Need to reload vertex buffer before next draw
    cloud->dirty = true;
    return XL::xl_true;
}


XL_DEFINE_TRACES

int module_init(const Tao::ModuleApi *api, const Tao::ModuleInfo *mod)
// ----------------------------------------------------------------------------
//   Initialize the Tao module
// ----------------------------------------------------------------------------
{
    Q_UNUSED(mod);
    XL_INIT_TRACES();
    PointCloud::init(api);
    return 0;
}


int module_exit()
// ----------------------------------------------------------------------------
//   Uninitialize the Tao module
// ----------------------------------------------------------------------------
{
    PointCloud::cloud_only("");
    return 0;
}
