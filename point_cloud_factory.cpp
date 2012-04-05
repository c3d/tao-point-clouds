// ****************************************************************************
//  point_cloud_factory.cpp                                        Tao project
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

#include "point_cloud_factory.h"
#include "point_cloud.h"
#include "point_cloud_vbo.h"


const Tao::ModuleApi * PointCloudFactory::tao = NULL;
PointCloudFactory::cloud_map PointCloudFactory::clouds;
bool PointCloudFactory::vboSupported = false;



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


void PointCloudFactory::init(const Tao::ModuleApi *api)
// ----------------------------------------------------------------------------
//   Initialize static part of class
// ----------------------------------------------------------------------------
{
    tao = api;
    glewInit();
    QString extensions((const char *)glGetString(GL_EXTENSIONS));
    vboSupported = extensions.contains("ARB_vertex_buffer_object");
    IFTRACE(pointcloud)
        sdebug() << "VBO supported: " << vboSupported << "\n";
}


void PointCloudFactory::render_callback(void *arg)
// ----------------------------------------------------------------------------
//   Find point cloud by name and draw it
// ----------------------------------------------------------------------------
{
    text name = text((const char *)arg);
    PointCloud * cloud = PointCloudFactory::cloud(name);
    if (cloud)
        cloud->draw();
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
    cloud_map::iterator found = clouds.find(name);
    if (found != clouds.end())
    {
        PointCloud *s = (*found).second;
        clouds.erase(found);
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
    cloud_map::iterator n = clouds.begin();
    for (cloud_map::iterator v = clouds.begin(); v != clouds.end(); v = n)
    {
        if (name != (*v).first)
        {
            PointCloud *s = (*v).second;
            clouds.erase(v);
            delete s;
            n = clouds.begin();
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
    tao->addToLayout(PointCloudFactory::render_callback, strdup(name.c_str()),
                     PointCloudFactory::delete_callback);
    return XL::xl_true;
}


XL::name_p PointCloudFactory::cloud_optimize(text name)
// ----------------------------------------------------------------------------
//   Optimize point cloud data
// ----------------------------------------------------------------------------
{
    PointCloud *cloud = PointCloudFactory::cloud(name);
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
    PointCloud *cloud = PointCloudFactory::cloud(name, LM_CREATE);
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
    PointCloud *cloud = PointCloudFactory::cloud(name,
                                                 LM_CREATE |
                                                 LM_CLEAR_OPTIMIZED);
    if (!cloud)
        return XL::xl_false;

    PointCloud::Point point(x->value, y->value, z->value);
    PointCloud::Color color;
    if (r >= 0 && g >= 0 && b >= 0 && a >= 0)
        color = PointCloud::Color(r, g, b, a);
    bool changed = cloud->addPoint(point, color);
    if (!changed && cloud->error != "")
    {
        XL::Ooops(cloud->error, self);
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
    PointCloud *cloud = PointCloudFactory::cloud(name, LM_CREATE);
    if (!cloud)
        return XL::xl_false;

    bool changed = cloud->loadData(file, fmt, xi, yi, zi, colorScale,
                                   ri, gi, bi, ai);
    if (!changed && cloud->error != "")
    {
        XL::Ooops(cloud->error, self);
        cloud->error.clear();
    }

    return changed ? XL::xl_true : XL::xl_false;
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

int module_init(const Tao::ModuleApi *api, const Tao::ModuleInfo *mod)
// ----------------------------------------------------------------------------
//   Initialize the Tao module
// ----------------------------------------------------------------------------
{
    Q_UNUSED(mod);
    XL_INIT_TRACES();
    PointCloudFactory::init(api);
    return 0;
}


int module_exit()
// ----------------------------------------------------------------------------
//   Uninitialize the Tao module
// ----------------------------------------------------------------------------
{
    PointCloudFactory::cloud_only("");
    return 0;
}
