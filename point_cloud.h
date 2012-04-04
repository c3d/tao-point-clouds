#ifndef POINT_CLOUD_H
#define POINT_CLOUD_H
// ****************************************************************************
//  point_cloud.h                                                  Tao project
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

#include "basics.h"  // From XLR
#include <QString>
#include <vector>


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



// ============================================================================
//
//    Helpers
//
// ============================================================================

inline QString operator +(std::string s)
// ----------------------------------------------------------------------------
//   UTF-8 conversion from std::string to QString
// ----------------------------------------------------------------------------
{
    return QString::fromUtf8(s.data(), s.length());
}

inline std::string operator +(QString s)
// ----------------------------------------------------------------------------
//   UTF-8 conversion from QString to std::string
// ----------------------------------------------------------------------------
{
    return std::string(s.toUtf8().constData());
}

#endif // POINT_CLOUD_H

