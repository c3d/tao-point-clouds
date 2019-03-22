#ifndef POINT_CLOUD_FACTORY_H
#define POINT_CLOUD_FACTORY_H
// *****************************************************************************
// point_cloud_factory.h                                           Tao3D project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2012-2013, Baptiste Soulisse <baptiste.soulisse@taodyne.com>
// (C) 2012-2013, Catherine Burvelle <catherine@taodyne.com>
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

#include "thread_pool.h"
#include "tree.h"
#include "tao/module_api.h"
#include <QFlags>
#include <map>

struct PointCloud;

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
    PointCloudFactory(const Tao::ModuleApi *tao = 0);
    virtual ~PointCloudFactory() {}

    PointCloud *  cloud(text name, LookupMode mode = LM_DEFAULT);

public:
    static PointCloudFactory * instance(const Tao::ModuleApi *tao = 0);
    static void                destroy();

    static void          render_callback(void *arg);
    static void          identify_callback(void *arg);
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
    static XL::Real_p    cloud_point_size(text name, float sz);
    static XL::Name_p    cloud_point_sprites(text name, bool enabled);
    static XL::Name_p    cloud_point_programmable_size(text name, bool enabled);

public:
    const Tao::ModuleApi *  tao;
    bool                    vboSupported;
    ThreadPool              pool;

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

