// *****************************************************************************
// point_cloud.cpp                                                 Tao3D project
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
// (C) 2012-2013, Catherine Burvelle <catherine@taodyne.com>
// (C) 2012-2015,2019, Christophe de Dinechin <christophe@dinechin.org>
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

#include "point_cloud.h"
#include "point_cloud_factory.h"
#include "tao/tao_gl.h"
#include "tao/graphic_state.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QTextStream>


PointCloud::PointCloud(text name)
// ----------------------------------------------------------------------------
//   Constructor
// ----------------------------------------------------------------------------
    : loaded(-1.0), pointSize(-1.0), pointSprites(false), name(name),
      fileMonitor(0),
      network(NULL), networkReply(NULL),
      nbRandom(0), coloredRandom(false)
{}


PointCloud::~PointCloud()
// ----------------------------------------------------------------------------
//   Destructor
// ----------------------------------------------------------------------------
{
    interrupt();
    PointCloudFactory::instance()->tao->deleteFileMonitor(fileMonitor);
    if (network)
        network->deleteLater();
    if (networkReply)
        networkReply->deleteLater();
}


unsigned PointCloud::size()
// ----------------------------------------------------------------------------
//   Number of points in the cloud
// ----------------------------------------------------------------------------
{
    if (loadInProgress())
        return 0;
    if (colored())
    {
        XL_ASSERT(points.size() == colors.size());
    }
    return points.size();
}


bool PointCloud::addPoint(const Point &p, Color c)
// ----------------------------------------------------------------------------
//   Add a new point to the cloud
// ----------------------------------------------------------------------------
{
    if (c.isValid())
    {
        XL_ASSERT(colored() || size() == 0);
        colors.push_back(c);
    }
    points.push_back(p);
    return true;
}



void PointCloud::removePoints(unsigned n)
// ----------------------------------------------------------------------------
//   Drop n points from the cloud
// ----------------------------------------------------------------------------
{
    if (n >= size())
        return clear();

    while (n--)
    {
        points.pop_back();
        if (colored())
            colors.pop_back();
    }
}


void PointCloud::draw()
// ----------------------------------------------------------------------------
//   Draw cloud
// ----------------------------------------------------------------------------
{
    if (points.size() == 0 || loadInProgress())
        return;

    PointCloudFactory * fact = PointCloudFactory::instance();
    if (!colored())
    {
        // Activate current document color
        fact->tao->SetFillColor();
    }

    if (pointSize > 0)
    {
        glPushAttrib(GL_POINT_BIT);
        GL.PointSize(pointSize * fact->tao->DevicePixelRatio());
    }
    if (pointSprites)
    {
        GL.Enable(GL_POINT_SPRITE);
        GL.TexEnv(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
        GL.PointParameter(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
        fact->tao->SetTextures();
    }
    if (pointProgrammableSize)
        GL.Enable(GL_VERTEX_PROGRAM_POINT_SIZE);

    GL.EnableClientState(GL_VERTEX_ARRAY);
    GL.EnableClientState(GL_COLOR_ARRAY);
    GL.VertexPointer(3, GL_FLOAT, sizeof(Point), &points[0].x);
    GL.ColorPointer(4, GL_FLOAT, sizeof(Color), &colors[0].r);
    GL.DrawArrays(GL_POINTS, 0, size());
    GL.DisableClientState(GL_VERTEX_ARRAY);
    GL.DisableClientState(GL_COLOR_ARRAY);

    if (pointProgrammableSize)
        GL.Disable(GL_VERTEX_PROGRAM_POINT_SIZE);
    if (pointSprites)
    {
        GL.Disable(GL_POINT_SPRITE);
        GL.TexEnv(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_FALSE);
        GL.PointParameter(GL_POINT_SPRITE_COORD_ORIGIN, GL_UPPER_LEFT);
    }
    if (pointSize > 0)
        glPopAttrib();
}


void PointCloud::clear()
// ----------------------------------------------------------------------------
//   Remove all points
// ----------------------------------------------------------------------------
{
    points.clear();
    colors.clear();
}


static float random01()
// ----------------------------------------------------------------------------
//   Return random float in [0.0, 1.0]
// ----------------------------------------------------------------------------
{
    return float(XL::xl_random(0.0, 1.0));
}


bool PointCloud::randomPoints(unsigned n, bool col)
// ----------------------------------------------------------------------------
//   Create a point cloud with n random points
// ----------------------------------------------------------------------------
{
    if (size() == n)
        return false;

    // colored attribute can't be changed if cloud already exists
    if (size() != 0)
        col = colored();

    IFTRACE(pointcloud)
        debug() << "Points: " << size() << " requested: " << n
                << " colored: " << (col ? "yes" : "no") << "\n";

    int delta = n - size();
    if (delta < 0)
    {
        removePoints(-delta);
    }
    else
    {
        for (int i = 0; i < delta; ++i)
        {
            Color color;
            if (col)
                color = Color(random01(), random01(), random01(), random01());
            Point point(random01(), random01(), random01());
            addPoint(point, color);
        }
    }

    return true;
}


bool PointCloud::loadData(text file, text sep, int xi, int yi, int zi,
                          float colorScale,
                          float ri, float gi, float bi, float ai, bool async)
// ----------------------------------------------------------------------------
//   Load points from a file (synchronously or in a thread)
// ----------------------------------------------------------------------------
{
    if (file == this->file)
        return false;
    loadDataParm = LoadDataParm(file, sep, xi, yi, zi, colorScale,
                                ri, gi, bi, ai);

    XL_ASSERT(folder != "");
    if (xi < 1 || yi < 1 || zi < 1)
    {
        error = "Invalid coordinate index value";
        return false;
    }

    if (file.find("://") != file.npos)
    {
        if (!network)
            network = new QNetworkAccessManager;
        if (!networkReply)
        {
            QUrl url(+file);
            QNetworkRequest req(url);
            networkReply = network->get(req);
        }
        return true;
    }

    QString qf = QString::fromUtf8(folder.data(), folder.length());
    QString qn = QString::fromUtf8(file.data(), file.length());
    QFileInfo inf(QDir(qf), qn);
    text path = +QDir::toNativeSeparators(inf.absoluteFilePath());
    QFile f(+path);
    if (!f.open(QIODevice::ReadOnly))
    {
        error = +QString("File not found or unreadable: $1\n"
                      "File path: %1").arg(+path);
        return false;
    }

    PointCloudFactory * fact = PointCloudFactory::instance();
    if (!fileMonitor)
    {
        fileMonitor = fact->tao->newFileMonitor(0, fileChanged, 0, this,
                                                "PointCloud:" + name);
        fact->tao->fileMonitorRemoveAllPaths(fileMonitor);
        fact->tao->fileMonitorAddPath(fileMonitor, path);
    }

    if (async)
    {
        fact->pool.start(this);
        return true;
    }

    // colored attribute can't be changed if cloud already exists
    if (size())
        colorScale = colored() ? 1.0 : 0.0;

    IFTRACE(pointcloud)
        debug() << "Loading " << path << "\n";

    loadFromStream(&f);

    this->file = file;
    f.close();

    return true;
}


void PointCloud::loadFromStream(QIODevice *io)
// ----------------------------------------------------------------------------
//   Load data from a given I/O device (file or network reply)
// ----------------------------------------------------------------------------
{
    clear();
    QTextStream t(io);
    QString line;
    unsigned count = 0;

    QString separator = +loadDataParm.sep;
    int xi = loadDataParm.xi;
    int yi = loadDataParm.yi;
    int zi = loadDataParm.zi;
    float ri = loadDataParm.ri;
    float gi = loadDataParm.gi;
    float bi = loadDataParm.bi;
    float ai = loadDataParm.ai;
    float colorScale = loadDataParm.colorScale;

    int maxp = qMax(qMax(xi, yi), zi);
    float maxc = qMax(qMax(ri, gi), qMax(bi, ai));
    int max  = qMax(maxp, (int)maxc);
    double sz = io->bytesAvailable();
    double pos = 0.0;
    loaded = 0.0;
    do
    {
        if (interrupted())
        {
            IFTRACE(pointcloud)
                debug() << "loadData interrupted\n";
            return;
        }

        line = t.readLine();
        pos += line.size() + 1;
        if (sz)
            loaded = pos/sz;
        QStringList values = line.split(separator);
        if (values.size() < max)
            continue;
        bool xok, yok, zok;
        float x = values[xi-1].toFloat(&xok);
        float y = values[yi-1].toFloat(&yok);
        float z = values[zi-1].toFloat(&zok);
        bool colorok = true;
        Color color;
        if (colorScale)
        {
            bool rok = true;
            bool gok = true;
            bool bok = true;
            bool aok = true;
            float r = (ri > 0) ? values[ri-1].toFloat(&rok) * colorScale : -ri;
            float g = (gi > 0) ? values[gi-1].toFloat(&gok) * colorScale : -gi;
            float b = (bi > 0) ? values[bi-1].toFloat(&bok) * colorScale : -bi;
            float a = (ai > 0) ? values[ai-1].toFloat(&aok) * colorScale : -ai;
            colorok = rok && gok && bok && aok;
            if (colorok)
                color = Color(r, g, b, a);
        }
        size();
        if (xok && yok && zok && colorok)
        {
            addPoint(Point(x, y, z), color);
            count++;
        }
        size();
    }
    while (!line.isNull());
    loaded = 1.0;

    IFTRACE(pointcloud)
        debug() << "Loaded " << count << " points\n";
}


void PointCloud::replyFinished(QNetworkReply *reply)
// ----------------------------------------------------------------------------
//   A network reply completed - Process it
// ----------------------------------------------------------------------------
{
    IFTRACE(pointcloud)
        debug() << "Loading from network reply\n";
    if (reply == networkReply)
        networkReply = NULL;
    loadFromStream(reply);
    reply->deleteLater();
}


void PointCloud::run()
// ----------------------------------------------------------------------------
//   Called in a separate thread to load data asynchronously
// ----------------------------------------------------------------------------
{
    LoadDataParm &p(loadDataParm);
    loadData(p.file, p.sep, p.xi, p.yi, p.zi, p.colorScale,
             p.ri, p.gi, p.bi, p.ai, false);
}


std::ostream & PointCloud::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[PointCloud] \"" << name << "\" " << (void*)this << " ";
    return std::cerr;
}

bool PointCloud::loadInProgress()
// ----------------------------------------------------------------------------
//   Is a file currently being loaded?
// ----------------------------------------------------------------------------
{
    if (networkReply)
    {
        if (networkReply->isRunning())
            return true;
        if (networkReply->isFinished())
        {
            replyFinished(networkReply);
            return false;
        }
    }

    return (loaded >= 0 && loaded < 1.0);
}


void PointCloud::reload()
// ----------------------------------------------------------------------------
//   Reload data from file
// ----------------------------------------------------------------------------
{
    IFTRACE(pointcloud)
        debug() << "Reloading\n";

    clear();
    file = ""; // Or loadData() would do nothing
    LoadDataParm &p(loadDataParm);
    loadData(p.file, p.sep, p.xi, p.yi, p.zi, p.colorScale,
             p.ri, p.gi, p.bi, p.ai, true);
}


void PointCloud::fileChanged(std::string path,
                             std::string absolutePath,
                             void * userData)
// ----------------------------------------------------------------------------
//   Callback, called when a file has changed
// ----------------------------------------------------------------------------
{
    Q_UNUSED(path);
    Q_UNUSED(absolutePath)

    PointCloud * cloud = (PointCloud *)userData;
    cloud->reload();
}
