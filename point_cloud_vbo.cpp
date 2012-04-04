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
