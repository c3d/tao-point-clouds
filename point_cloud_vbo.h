#ifndef POINT_CLOUD_VBO_H
#define POINT_CLOUD_VBO_H
// ****************************************************************************
//  point_cloud_vbo.h                                              Tao project
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

#include "point_cloud.h"
#include "tao/tao_gl.h"


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
    virtual bool      addPoint(const Point &p);
    virtual void      removePoints(unsigned n);
    virtual void      draw();
    virtual bool      optimize();
    virtual bool      isOptimized() { return optimized; }
    virtual void      clear();
    virtual bool      randomPoints(unsigned n);
    virtual bool      loadData(text file, text sep, int xi, int yi, int zi);

protected:
    void  checkGLContext();
    bool  useVbo();
    void  updateVbo();
    void  genBuffer();
    void  delBuffer();


protected:
    virtual std::ostream &  debug();

protected:
    GLuint              vbo;
    bool                dirty;      // Point data modified, VBO not in sync
    bool                optimized;  // Point data only in VBO
    bool                dontOptimize;  // Data would be lost if context changes
    unsigned            nbPoints;   // When optimized == true
    const QGLContext *  context;

    // To re-create cloud from file
    text  sep;
    int   xi, yi, zi;
};

#endif // POINT_CLOUD_VBO_H
