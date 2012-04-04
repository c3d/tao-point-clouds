#ifndef POINT_CLOUD_H
#define POINT_CLOUD_H
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

#include "tree.h"
#include "tao/module_api.h"
#include <qgl.h>
#include <map>


class PointCloud
// ----------------------------------------------------------------------------
//    Display a large number of points efficiently
// ----------------------------------------------------------------------------
{

public:
    PointCloud(text name) : name(name), nbRandom(0) {}
    virtual ~PointCloud() {}

public:
    struct Point
    {
        Point(float x, float y, float z) : x(x), y(y), z(z) {}
        float x, y, z;
    };

public:
    virtual unsigned  size() { return points.size(); }
    virtual bool      addPoint(const Point &p);
    virtual void      removePoints(unsigned n);
    virtual void      draw();
    virtual bool      optimize() { return false; }
    virtual bool      isOptimized() { return false; }
    virtual void      clear();
    virtual bool      randomPoints(unsigned n);
    virtual bool      loadData(text file, text sep, int xi, int yi, int zi);

public:
    text       error;

protected:
    typedef std::vector<Point>  point_vec;

protected:
    virtual std::ostream &  debug();

protected:
    text       name;
    point_vec  points;

    // When cloud is loaded from a file
    text       file;

    // When cloud is random
    unsigned   nbRandom;
};


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


class PointCloudFactory
// ----------------------------------------------------------------------------
//    Manage cache of cloud objects, implement Tao primitives and callbacks
// ----------------------------------------------------------------------------
{
public:
    enum LookupModeFlag {
        LM_DEFAULT = 0x0,
        LM_CREATE = 0x1,          // Create if not exists
        LM_CLEAR_OPTIMIZED = 0x2  // Re-create if exists and is optimized
    };
    Q_DECLARE_FLAGS(LookupMode, LookupModeFlag)

public:
    PointCloudFactory() {}
    virtual ~PointCloudFactory() {}

public:
    static PointCloud *  cloud(text name, LookupMode mode = LM_DEFAULT);

    static void          init(const Tao::ModuleApi *api);
    static void          render_callback(void *arg);
    static void          delete_callback(void *arg);

    // XL interface
    static XL::Name_p    cloud_drop(text name);
    static XL::Name_p    cloud_only(text name);
    static XL::Name_p    cloud_show(text name);
    static XL::Name_p    cloud_optimize(text name);
    static XL::Name_p    cloud_random(text name, XL::Integer_p points);
    static XL::Name_p    cloud_add(XL::Tree_p self,
                                   text name,
                                   XL::Real_p x, XL::Real_p y,
                                   XL::Real_p z);
    static XL::Name_p    cloud_load_data(XL::Tree_p self,
                                         text name, text file, text fmt,
                                         int xi, int yi, int zi);

protected:
    typedef std::map<text, PointCloud *>  cloud_map;

protected:
    static std::ostream &  sdebug();

protected:
    static cloud_map            clouds;

public:
    static bool                   vboSupported;
    static const Tao::ModuleApi * tao;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PointCloudFactory::LookupMode)

#endif // POINT_CLOUD_H

