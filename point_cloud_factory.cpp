// *****************************************************************************
// point_cloud_factory.cpp                                         Tao3D project
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

#include "point_cloud_factory.h"
#include "point_cloud.h"
#include "point_cloud_vbo.h"
#include "graphic_state.h"
#include <QEvent>


PointCloudFactory * PointCloudFactory::factory = NULL;


PointCloudFactory::PointCloudFactory(const Tao::ModuleApi *tao)
// ----------------------------------------------------------------------------
//   Constructor
// ----------------------------------------------------------------------------
    : tao(tao)
{
    QString extensions((const char *)glGetString(GL_EXTENSIONS));
    vboSupported = extensions.contains("ARB_vertex_buffer_object");
    IFTRACE(pointcloud)
        sdebug() << "VBO supported: " << vboSupported << "\n";
}


PointCloudFactory * PointCloudFactory::instance(const Tao::ModuleApi *tao)
// ----------------------------------------------------------------------------
//   Return factory instance (singleton)
// ----------------------------------------------------------------------------
{
    if (!factory)
        factory = new PointCloudFactory(tao);
    return factory;
}


void PointCloudFactory::destroy()
// ----------------------------------------------------------------------------
//   Destroy factory instance
// ----------------------------------------------------------------------------
{
    if (!factory)
        return;
    delete factory;
    factory = NULL;
}


PointCloud * PointCloudFactory::cloud(text name, LookupMode mode)
// ----------------------------------------------------------------------------
//   Find point cloud by name, optionally create new one
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = NULL;
    cloud_map::iterator found = clouds.find(name);
    if (found != clouds.end())
    {
        cloud = (*found).second;
        if (mode & LM_CLEAR_OPTIMIZED)
        {
            if (cloud && cloud->isOptimized())
            {
                IFTRACE(pointcloud)
                    sdebug() << "Cloud " << (void *)cloud << " has been optimized, "
                                "allocating a new one\n";
                clouds.erase(name);
                delete cloud;
                cloud = new PointCloudVBO(name);
                clouds[name] = cloud;
            }
        }
    }
    else if (mode & LM_CREATE)
    {
        cloud = new PointCloudVBO(name);
        clouds[name] = cloud;
    }
    return cloud;
}


void PointCloudFactory::render_callback(void *arg)
// ----------------------------------------------------------------------------
//   Find point cloud by name and draw it
// ----------------------------------------------------------------------------
{
    text name = text((const char *)arg);
    PointCloud * cloud = PointCloudFactory::instance()->cloud(name);
    if (cloud)
        cloud->draw();
}


void PointCloudFactory::identify_callback(void *arg)
// ----------------------------------------------------------------------------
//   We can't click on point clouds
// ----------------------------------------------------------------------------
{
    (void) arg;
}


void PointCloudFactory::delete_callback(void *arg)
// ----------------------------------------------------------------------------
//   Delete point cloud name
// ----------------------------------------------------------------------------
{
    free(arg);
}


XL::Name_p PointCloudFactory::cloud_drop(text name)
// ----------------------------------------------------------------------------
//   Purge the given point cloud from memory
// ----------------------------------------------------------------------------
{
    PointCloudFactory * f = PointCloudFactory::instance();
    cloud_map::iterator found = f->clouds.find(name);
    if (found != f->clouds.end())
    {
        PointCloud *s = (*found).second;
        f->clouds.erase(found);
        delete s;
        return XL::xl_true;
    }
    return XL::xl_false;
}


XL::Name_p PointCloudFactory::cloud_only(text name)
// ----------------------------------------------------------------------------
//   Purge all other point clouds from memory
// ----------------------------------------------------------------------------
{
    PointCloudFactory * f = PointCloudFactory::instance();
    cloud_map::iterator n = f->clouds.begin();
    for (cloud_map::iterator v = f->clouds.begin();
         v != f->clouds.end();
         v = n)
    {
        if (name != (*v).first)
        {
            PointCloud *s = (*v).second;
            f->clouds.erase(v);
            delete s;
            n = f->clouds.begin();
        }
        else
        {
            n = ++v;
        }
    }
    return XL::xl_false;
}


XL::name_p PointCloudFactory::cloud_show(text name)
// ----------------------------------------------------------------------------
//   Show point cloud
// ----------------------------------------------------------------------------
{
    instance()->tao->AddToLayout2(PointCloudFactory::render_callback,
                                  PointCloudFactory::identify_callback,
                                  strdup(name.c_str()),
                                  PointCloudFactory::delete_callback);
    return XL::xl_true;
}


XL::name_p PointCloudFactory::cloud_optimize(text name)
// ----------------------------------------------------------------------------
//   Optimize point cloud data
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = instance()->cloud(name);
    if (!cloud)
        return XL::xl_false;

    bool opt = cloud->optimize();
    return opt ? XL::xl_true : XL::xl_false;
}


XL::Name_p PointCloudFactory::cloud_random(text name, XL::Integer_p points,
                                           bool colored)
// ----------------------------------------------------------------------------
//   Create a point cloud with n random points
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = instance()->cloud(name, LM_CREATE);
    if (!cloud)
        return XL::xl_false;

    bool changed = cloud->randomPoints(points->value, colored);
    return changed ? XL::xl_true : XL::xl_false;
}


XL::Name_p PointCloudFactory::cloud_add(XL::Tree_p self,
                                        text name, XL::Real_p x, XL::Real_p y,
                                        XL::Real_p z,
                                        float r, float g, float b, float a)
// ----------------------------------------------------------------------------
//   Add point to a cloud
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = instance()->cloud(name, LM_CREATE | LM_CLEAR_OPTIMIZED);
    if (!cloud)
    {
        XL::Ooops("PointsCloud: No cloud named $2 for $1", self).Arg(name);
        return XL::xl_false;
    }

    PointCloud::Point point(x->value, y->value, z->value);
    PointCloud::Color color;
    if (r >= 0 && g >= 0 && b >= 0 && a >= 0)
        color = PointCloud::Color(r, g, b, a);
    else if (cloud->colored())
        color = PointCloud::Color(1,1,1,1);
    bool changed = cloud->addPoint(point, color);
    if (!changed && cloud->error != "")
    {
        XL::Ooops("PointsCloud: Error adding to cloud $2 in $1: $3", self)
            .Arg(name).Arg(cloud->error);
        cloud->error.clear();
    }

    return changed ? XL::xl_true : XL::xl_false;
}


XL::Name_p PointCloudFactory::cloud_load_data(XL::Tree_p self,
                                              text name, text file, text fmt,
                                              int xi, int yi, int zi,
                                              float colorScale,
                                              float ri, float gi, float bi,
                                              float ai)
// ----------------------------------------------------------------------------
//   Load points from a file
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = instance()->cloud(name, LM_CREATE);
    if (!cloud)
    {
        XL::Ooops("PointsCloud: No cloud named $2 for $1", self).Arg(name);
        return XL::xl_false;
    }

    if (cloud->folder == "")
        cloud->folder = instance()->tao->currentDocumentFolder();
    bool changed = cloud->loadData(file, fmt, xi, yi, zi, colorScale,
                                   ri, gi, bi, ai, true);
    if (!changed && cloud->error != "")
    {
        XL::Ooops("PointsCloud: Error loading cloud $2 from $3 in $1: $4",
                  self).Arg(name).Arg(file).Arg(cloud->error);
        cloud->error.clear();
    }

    return changed ? XL::xl_true : XL::xl_false;
}


XL::Real_p PointCloudFactory::cloud_loaded(text name)
// ----------------------------------------------------------------------------
//   How much of the file has been loaded by cloud_load_data (0.0 to 1.0)
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = instance()->cloud(name);
    if (!cloud)
        return new XL::Real(0.0);
    instance()->tao->refreshOn(QEvent::Timer, -1.0);
    double l = cloud->loaded;
    if (l < 0)
        l = 0;
    return new XL::Real(l);
}


XL::Real_p PointCloudFactory::cloud_point_size(text name, float size)
// ----------------------------------------------------------------------------
//   Sets the GL point size for the cloud
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = instance()->cloud(name);
    if (!cloud)
        return new XL::Real(0.0);
    cloud->pointSize = size;
    return new XL::Real(size);
}


XL::Name_p PointCloudFactory::cloud_point_sprites(text name, bool enabled)
// ----------------------------------------------------------------------------
//   Sets the GL point size for the cloud
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = instance()->cloud(name);
    if (!cloud)
        return XL::xl_false;
    cloud->pointSprites = enabled;
    return XL::xl_true;
}


XL::Name_p PointCloudFactory::cloud_point_programmable_size(text name, bool on)
// ----------------------------------------------------------------------------
//   Sets the GL point size for the cloud
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = instance()->cloud(name);
    if (!cloud)
        return XL::xl_false;
    cloud->pointProgrammableSize = on;
    return XL::xl_true;
}


std::ostream & PointCloudFactory::sdebug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[PointCloudFactory] ";
    return std::cerr;
}


XL_DEFINE_TRACES
DEFINE_MODULE_GL;

int module_init(const Tao::ModuleApi *api, const Tao::ModuleInfo *mod)
// ----------------------------------------------------------------------------
//   Initialize the Tao module
// ----------------------------------------------------------------------------
{
    Q_UNUSED(mod);
    XL_INIT_TRACES();
    PointCloudFactory::instance(api);
    return 0;
}


int module_exit()
// ----------------------------------------------------------------------------
//   Uninitialize the Tao module
// ----------------------------------------------------------------------------
{
    PointCloudFactory::instance()->pool.stopAll();
    PointCloudFactory::cloud_only("");
    return 0;
}
