// ****************************************************************************
//  point_cloud_vbo.cpp                                            Tao project
// ****************************************************************************
//
//   File Description:
//
//    Drawing a large number of points efficiently, using Vertex Buffer
//    Objects.
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

#include "point_cloud_vbo.h"
#include "point_cloud_factory.h"


PointCloudVBO::PointCloudVBO(text name)
// ----------------------------------------------------------------------------
//   Initialize object
// ----------------------------------------------------------------------------
    : PointCloud(name), vbo(0), colorVbo(0),
      dirty(false), optimized(false), noOptimize(false),
      nbPoints(0), context(QGLContext::currentContext())
{
    genPointBuffer();
}


PointCloudVBO::~PointCloudVBO()
// ----------------------------------------------------------------------------
//   Destroy object
// ----------------------------------------------------------------------------
{
    interrupt();
    delBuffers();
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


bool PointCloudVBO::addPoint(const Point &p, Color c)
// ----------------------------------------------------------------------------
//   Add a new point to the cloud
// ----------------------------------------------------------------------------
{
    if (optimized)
    {
        error = "Cannot add point to optimized cloud";
        return false;
    }

    PointCloud::addPoint(p, c);
    dirty = true;
    noOptimize = true;
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
    noOptimize = true;
}


void PointCloudVBO::draw()
// ----------------------------------------------------------------------------
//   Draw cloud
// ----------------------------------------------------------------------------
{
    if (!useVbo())
        return PointCloud::draw();

    PointCloudFactory * fact = PointCloudFactory::instance();

    checkGLContext();

    if (size() == 0)
        return;

    if (dirty)
        updateVbo();

    if (colored())
    {
        glEnableClientState(GL_COLOR_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, colorVbo);
        glColorPointer(4, GL_FLOAT, sizeof(Color), 0);
    }
    else
    {
        // Activate current document color
        fact->tao->SetFillColor();
    }

    if (pointSize > 0)
    {
        glPushAttrib(GL_POINT_BIT);
        glPointSize(pointSize);
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexPointer(3, GL_FLOAT, sizeof(Point), 0);
    glDrawArrays(GL_POINTS, 0, size());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableClientState(GL_VERTEX_ARRAY);
    if (colored())
        glDisableClientState(GL_COLOR_ARRAY);

    if (pointSize > 0)
        glPopAttrib();

}


bool PointCloudVBO::optimize()
// ----------------------------------------------------------------------------
//   Optimize point cloud data
// ----------------------------------------------------------------------------
{
    if (optimized || dontOptimize())
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


bool PointCloudVBO::randomPoints(unsigned n, bool colored)
// ----------------------------------------------------------------------------
//   Create a point cloud with n random points
// ----------------------------------------------------------------------------
{
    bool changed = PointCloud::randomPoints(n, colored);
    if (useVbo() && changed)
    {
        updateVbo();
        noOptimize = false;
    }
    return changed;
}


bool PointCloudVBO::loadData(text file, text sep, int xi, int yi, int zi,
                             float colorScale,
                             float ri, float gi, float bi, float ai)
// ----------------------------------------------------------------------------
//   Load points from a file
// ----------------------------------------------------------------------------
{
    bool changed = PointCloud::loadData(file, sep, xi, yi, zi, colorScale,
                                        ri, gi, bi, ai);
    if (useVbo() && changed)
    {
        updateVbo();

        noOptimize = false;
        this->sep = sep;
        this->xi = xi;
        this->yi = yi;
        this->zi = zi;
        this->colorScale = colorScale;
        this->ri = ri;
        this->gi = gi;
        this->bi = bi;
        this->ai = ai;
    }
    return changed;
}


bool PointCloudVBO::colored()
// ----------------------------------------------------------------------------
//   Do we have color data for the point set?
// ----------------------------------------------------------------------------
{
    if (optimized)
        return is_colored;
     return (colors.size() != 0);
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

        // Re-create VBO(s)
        genPointBuffer();
        if (colored())
            genColorBuffer();

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
                loadData(f, sep, xi, yi, zi, colorScale, ri, gi, bi, ai);
            }
            else if (nbRandom != 0)
            {
                IFTRACE(pointcloud)
                    debug() << "Re-creating random points\n";
                unsigned n = nbRandom;
                clear();
                randomPoints(n, coloredRandom);
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
    return PointCloudFactory::instance()->vboSupported;
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

    if (colored())
    {
        if (colorVbo == 0)
            genColorBuffer();

        IFTRACE(pointcloud)
            debug() << "Updating VBO #" << colorVbo << " (" << size()
                    << " colors)\n";

        glBindBuffer(GL_ARRAY_BUFFER, colorVbo);
        glBufferData(GL_ARRAY_BUFFER, size()*sizeof(Color), &colors[0].r,
                     GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    dirty = false;
}


void PointCloudVBO::genPointBuffer()
// ----------------------------------------------------------------------------
//   Allocate new VBO for point coordinates
// ----------------------------------------------------------------------------
{
    glGenBuffers(1, &vbo);
    IFTRACE(pointcloud)
        debug() << "Allocated VBO #" << vbo << " for point coordinates\n";
}


void PointCloudVBO::genColorBuffer()
// ----------------------------------------------------------------------------
//   Allocate new VBO for colors
// ----------------------------------------------------------------------------
{
    Q_ASSERT(colored());
    glGenBuffers(1, &colorVbo);
    IFTRACE(pointcloud)
        debug() << "Allocated VBO #" << colorVbo << " for colors\n";
}


void PointCloudVBO::delBuffers()
// ----------------------------------------------------------------------------
//   Release VBO(s)
// ----------------------------------------------------------------------------
{
    IFTRACE(pointcloud)
        debug() << "Releasing VBO #" << vbo << "\n";
    glDeleteBuffers(1, &vbo);
    if (colorVbo)
    {
        IFTRACE(pointcloud)
            debug() << "Releasing VBO #" << colorVbo << "\n";
        glDeleteBuffers(1, &colorVbo);
    }
}


std::ostream & PointCloudVBO::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[PointCloudVBO] \"" << name << "\" " << (void*)this << " ";
    return std::cerr;
}
