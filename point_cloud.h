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


struct PointCloud
// ----------------------------------------------------------------------------
//    Display a large number of points efficiently
// ----------------------------------------------------------------------------
{
    struct Point {
        Point(float x, float y, float z) : x(x), y(y), z(z) {}
        float x, y, z;
    };
    typedef std::vector<Point>            point_vec;
    typedef std::map<text, PointCloud *>  cloud_map;

public:
    PointCloud(text name);
    virtual ~PointCloud();

public:
    void                        addPoint(float x, float y, float z);
    void                        pointsChanged();

public:
    static void                 init(const Tao::ModuleApi *api);
    static void                 render_callback(void *arg);
    static void                 delete_callback(void *arg);

    // XL interface
    static XL::Name_p           cloud_drop(text name);
    static XL::Name_p           cloud_only(text name);
    static XL::Name_p           cloud_show(text name);
    static XL::Name_p           cloud_random(text name, XL::Integer_p points);
    static XL::Name_p           cloud_add(text name,
                                          XL::Real_p x, XL::Real_p y,
                                          XL::Real_p z);

protected:
    text                        name;
    point_vec                   points;
    bool                        useVboIfAvailable;
    GLuint                      vbo;
    const QGLContext *          context;
    bool                        dirty;

protected:
    std::ostream &              debug();
    void                        Draw();
    bool                        useVbo();
    void                        checkGLContext();
    void                        genBuffer();

protected:
    static std::ostream &       sdebug();
    static PointCloud *         cloud(text name, bool create = false);
    static float                random01();

protected:
    static cloud_map            clouds;
    static bool                 vboSupported;

public:
    static const Tao::ModuleApi * tao;
};

#endif // POINT_CLOUD_H

