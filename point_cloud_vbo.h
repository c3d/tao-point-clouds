#ifndef POINT_CLOUD_VBO_H
#define POINT_CLOUD_VBO_H
// *****************************************************************************
// point_cloud_vbo.h                                               Tao3D project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2013, Baptiste Soulisse <baptiste.soulisse@taodyne.com>
// (C) 2013, Catherine Burvelle <catherine@taodyne.com>
// (C) 2012-2014,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012-2013, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of Tao3D
//
// Tao3D is free software: you can r redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tao3D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Tao3D, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

#include "point_cloud.h"
#include <QGLContext>


class PointCloudVBO : public PointCloud
// ----------------------------------------------------------------------------
//    Point cloud drawn using a Vertex Buffer Object for higher performance
// ----------------------------------------------------------------------------
{
public:
    PointCloudVBO(text name);
    virtual ~PointCloudVBO();

public:
    virtual unsigned  size();
    virtual bool      addPoint(const Point &p, Color c = Color());
    virtual void      removePoints(unsigned n);
    virtual void      draw();
    virtual bool      optimize();
    virtual bool      isOptimized() { return optimized; }
    virtual void      clear();
    virtual bool      randomPoints(unsigned n, bool colored);
    virtual bool      loadData(text file, text sep, int xi, int yi, int zi,
                               float colorScale = 0.0,
                               float ri = -1.0, float gi = -1.0,
                               float bi = -1.0, float ai = -1.0,
                               bool async = false);
    virtual bool      colored();

protected:
    void  checkGLContext();
    bool  useVbo();
    void  updateVbo();
    void  genPointBuffer();
    void  genColorBuffer();
    void  delBuffers();
    bool  dontOptimize() { return (noOptimize || loadInProgress()); }


protected:
    virtual std::ostream &  debug();

protected:
    GLuint              vbo, colorVbo;
    bool                dirty;      // Point data modified, VBOs not in sync
    bool                optimized;  // Point data only in VBOs
    bool                noOptimize; // Data would be lost if context changes
    unsigned            nbPoints;   // When optimized == true
    bool                is_colored; // When optimized == true
    const QGLContext *  context;

    // To re-create cloud from file
    text  sep;
    int   xi, yi, zi;
    float colorScale;
    float ri, gi, bi, ai;
};

#endif // POINT_CLOUD_VBO_H
