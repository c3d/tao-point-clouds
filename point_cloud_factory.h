#ifndef POINT_CLOUD_FACTORY_H
#define POINT_CLOUD_FACTORY_H
// ****************************************************************************
//  point_cloud_factory.h                                          Tao project
// ****************************************************************************
//
//   File Description:
//
//    Create and manipulate point clouds.
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
#include <QFlags>
#include <QThreadPool>
#include <map>

class PointCloud;

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
    PointCloudFactory();
    virtual ~PointCloudFactory() {}

    PointCloud *  cloud(text name, LookupMode mode = LM_DEFAULT);

public:
    static PointCloudFactory * instance();
    static void                destroy();

    static void          render_callback(void *arg);
    static void          delete_callback(void *arg);

    // XL interface
    static XL::Name_p    cloud_drop(text name);
    static XL::Name_p    cloud_only(text name);
    static XL::Name_p    cloud_show(text name);
    static XL::Name_p    cloud_optimize(text name);
    static XL::Name_p    cloud_random(text name, XL::Integer_p points,
                                      bool colored = false);
    static XL::Name_p    cloud_add(XL::Tree_p self,
                                   text name,
                                   XL::Real_p x, XL::Real_p y,
                                   XL::Real_p z,
                                   float r = -1.0, float g = -1.0,
                                   float b = -1.0, float a = -1.0);
    static XL::Name_p    cloud_load_data(XL::Tree_p self,
                                         text name, text file, text fmt,
                                         int xi, int yi, int zi,
                                         float colorScale = 0.0,
                                         float ri = -1.0, float gi = -1.0,
                                         float bi = -1.0, float ai = -1.0);
    static XL::Real_p    cloud_loaded(text name);


public:
    const Tao::ModuleApi *  tao;
    bool                    vboSupported;
    QThreadPool             pool;

protected:
    static std::ostream &  sdebug();

protected:
    typedef std::map<text, PointCloud *>  cloud_map;

protected:
    cloud_map    clouds;

protected:
    static PointCloudFactory * factory;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PointCloudFactory::LookupMode)

#endif // POINT_CLOUD_FACTORY_H

