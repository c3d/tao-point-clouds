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

#endif // POINT_CLOUD_FACTORY_H

