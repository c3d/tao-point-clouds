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
// This software is licensed under the GNU General Public License v3.
// See file COPYING for details.
//  (C) 2012 Jerome Forissier <jerome@taodyne.com>
//  (C) 2012 Taodyne SAS
// ****************************************************************************

#include "point_cloud_vbo.h"
#include "point_cloud_factory.h"
#include "tao/graphic_state.h"
#include <QCoreApplication>
#include <QThread>

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
        GL.EnableClientState(GL_COLOR_ARRAY);
        GL.BindBuffer(GL_ARRAY_BUFFER, colorVbo);
        GL.ColorPointer(4, GL_FLOAT, sizeof(Color), 0);
    }
    else
    {
        // Activate current document color
        fact->tao->SetFillColor();
    }

    if (pointSize > 0)
    {
        glPushAttrib(GL_POINT_BIT);
        GL.PointSize(pointSize * fact->tao->DevicePixelRatio());
    }
    if (pointSprites)
    {
        GL.Enable(GL_POINT_SPRITE);
        GL.TexEnv(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
        GL.PointParameter(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
        fact->tao->SetTextures();
    }
    if (pointProgrammableSize)
        GL.Enable(GL_VERTEX_PROGRAM_POINT_SIZE);

    GL.EnableClientState(GL_VERTEX_ARRAY);
    GL.BindBuffer(GL_ARRAY_BUFFER, vbo);
    GL.VertexPointer(3, GL_FLOAT, sizeof(Point), 0);
    GL.DrawArrays(GL_POINTS, 0, size());
    GL.BindBuffer(GL_ARRAY_BUFFER, 0);
    GL.DisableClientState(GL_VERTEX_ARRAY);
    if (colored())
        GL.DisableClientState(GL_COLOR_ARRAY);

    if (pointProgrammableSize)
        GL.Disable(GL_VERTEX_PROGRAM_POINT_SIZE);
    if (pointSprites)
    {
        GL.Disable(GL_POINT_SPRITE);
        GL.TexEnv(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_FALSE);
        GL.PointParameter(GL_POINT_SPRITE_COORD_ORIGIN, GL_UPPER_LEFT);
    }
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
        noOptimize = false;
    return changed;
}


bool PointCloudVBO::loadData(text file, text sep, int xi, int yi, int zi,
                             float colorScale,
                             float ri, float gi, float bi, float ai,
                             bool async)
// ----------------------------------------------------------------------------
//   Load points from a file
// ----------------------------------------------------------------------------
{
    bool changed = PointCloud::loadData(file, sep, xi, yi, zi, colorScale,
                                        ri, gi, bi, ai, async);
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

    if (QThread::currentThread() != qApp->thread())
    {
        // OpenGL functions may be called only from the main thread, which
        // owns the GL context
        IFTRACE(pointcloud)
            debug() << "Not updating VBO (not main thread)\n";
        return;
    }

    IFTRACE(pointcloud)
        debug() << "Updating VBO #" << vbo << " (" << size() << " points)\n";

    GL.BindBuffer(GL_ARRAY_BUFFER, vbo);
    GL.BufferData(GL_ARRAY_BUFFER, size()*sizeof(Point), &points[0].x,
                 GL_STATIC_DRAW);
    GL.BindBuffer(GL_ARRAY_BUFFER, 0);

    if (colored())
    {
        if (colorVbo == 0)
            genColorBuffer();

        IFTRACE(pointcloud)
            debug() << "Updating VBO #" << colorVbo << " (" << size()
                    << " colors)\n";

        GL.BindBuffer(GL_ARRAY_BUFFER, colorVbo);
        GL.BufferData(GL_ARRAY_BUFFER, size()*sizeof(Color), &colors[0].r,
                     GL_STATIC_DRAW);
        GL.BindBuffer(GL_ARRAY_BUFFER, 0);
    }
    dirty = false;
}


void PointCloudVBO::genPointBuffer()
// ----------------------------------------------------------------------------
//   Allocate new VBO for point coordinates
// ----------------------------------------------------------------------------
{
    GL.GenBuffers(1, &vbo);
    IFTRACE(pointcloud)
        debug() << "Allocated VBO #" << vbo << " for point coordinates\n";
}


void PointCloudVBO::genColorBuffer()
// ----------------------------------------------------------------------------
//   Allocate new VBO for colors
// ----------------------------------------------------------------------------
{
    Q_ASSERT(colored());
    GL.GenBuffers(1, &colorVbo);
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
    GL.DeleteBuffers(1, &vbo);
    if (colorVbo)
    {
        IFTRACE(pointcloud)
            debug() << "Releasing VBO #" << colorVbo << "\n";
        GL.DeleteBuffers(1, &colorVbo);
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
