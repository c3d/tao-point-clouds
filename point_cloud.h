#ifndef POINT_CLOUD_H
#define POINT_CLOUD_H
// *****************************************************************************
// point_cloud.h                                                   Tao3D project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2012, Baptiste Soulisse <baptiste.soulisse@taodyne.com>
// (C) 2012, Catherine Burvelle <catherine@taodyne.com>
// (C) 2012,2014,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
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
#include "basics.h"  // From XLR
#include "tao/tao_gl.h"
#include "tao/module_api.h"
#include <QString>
#include <QRunnable>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <vector>


struct PointCloud : Runnable
// ----------------------------------------------------------------------------
//    Display a large number of points efficiently
// ----------------------------------------------------------------------------
{

public:
    PointCloud(text name);
    virtual ~PointCloud();

public:
    struct Point
    {
        Point(float x, float y, float z) : x(x), y(y), z(z) {}
        float x, y, z;
    };
    struct Color
    {
        Color() : r(-1.0), g(-1.0), b(-1.0), a(-1.0) {}
        Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
        bool isValid() { return r != -1.0; }
        float r, g, b, a;
    };
    struct LoadDataParm
    {
        LoadDataParm()
            : file(""), sep(""), xi(0), yi(0), zi(0), colorScale(0.0),
              ri(-1.0), gi(-1.0), bi(-1.0), ai(-1.0) {}
        LoadDataParm(text file, text sep, int xi, int yi, int zi,
                     float colorScale, float ri, float gi, float bi, float ai)
            : file(file), sep(sep), xi(xi), yi(yi), zi(zi),
              colorScale(colorScale), ri(ri), gi(gi), bi(bi), ai(ai) {}
        text  file, sep;
        int   xi, yi, zi;
        float colorScale, ri, gi, bi, ai;
    };

public:
    virtual unsigned  size();
    virtual bool      addPoint(const Point &p, Color c = Color());
    virtual void      removePoints(unsigned n);
    virtual void      draw();
    virtual bool      optimize() { return false; }
    virtual bool      isOptimized() { return false; }
    virtual void      clear();
    virtual bool      randomPoints(unsigned n, bool colored = false);
    virtual bool      loadData(text file, text sep, int xi, int yi, int zi,
                               float colorScale = 0.0,
                               float ri = -1.0, float gi = -1.0,
                               float bi = -1.0, float ai = -1.0,
                               bool async = false);
    virtual bool      colored() { return (colors.size() != 0); }
    virtual void      run();  // From Runnable

public:
    text       error;
    float      loaded;  // -1.0 default, [0.0..1.0[ loading, 1.0 loaded
    text       folder;  // When cloud is loaded from a file
    float      pointSize;
    bool       pointSprites;
    bool       pointProgrammableSize;

protected:
    typedef std::vector<Point>  point_vec;
    typedef std::vector<Color>  color_vec;

protected:
    virtual std::ostream &  debug();
    bool                    loadInProgress();
    void                    reload();
    void                    loadFromStream(QIODevice *io);
    void                    replyFinished(QNetworkReply *);

protected:
    static void             fileChanged(std::string path,
                                        std::string absolutePath,
                                        void * userData);


protected:
    text       name;
    point_vec  points;
    color_vec  colors;

    // When cloud is loaded from a file
    text       file;
    void     * fileMonitor;

    // When cloud is loaded from a URL
    QNetworkAccessManager *network;
    QNetworkReply         *networkReply;

    // When cloud is random
    unsigned   nbRandom;
    bool       coloredRandom;

    // Save loadData parameters to run in a thread
    LoadDataParm loadDataParm;
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

