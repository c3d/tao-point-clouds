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

#include "point_cloud.h"
#include "point_cloud_factory.h"
#include "tao/tao_gl.h"
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QTextStream>


PointCloud::PointCloud(text name)
// ----------------------------------------------------------------------------
//   Constructor
// ----------------------------------------------------------------------------
    : loaded(-1.0), pointSize(-1.0), name(name), nbRandom(0),
      coloredRandom(false)
{}


PointCloud::~PointCloud()
// ----------------------------------------------------------------------------
//   Destructor
// ----------------------------------------------------------------------------
{
    interrupt();
}


unsigned PointCloud::size()
// ----------------------------------------------------------------------------
//   Number of points in the cloud
// ----------------------------------------------------------------------------
{
    if (colored())
    {
        Q_ASSERT(points.size() == colors.size());
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
        Q_ASSERT(colored() || size() == 0);
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

    n -= size();
    while (n--)
    {
        points.pop_back();
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

    if (!colored())
    {
        // Activate current document color
        PointCloudFactory::instance()->tao->SetFillColor();
    }

    if (pointSize > 0)
    {
        glPushAttrib(GL_POINT_BIT);
        glPointSize(pointSize);
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Point), &points[0].x);
    glColorPointer(4, GL_FLOAT, sizeof(Color), &colors[0].r);
    glDrawArrays(GL_POINTS, 0, size());
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

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
    if (async)
    {
        loadDataParm = LoadDataParm(file, sep, xi, yi, zi, colorScale,
                                    ri, gi, bi, ai);
        PointCloudFactory::instance()->pool.start(this);
        return true;
    }
    if (xi < 1 || yi < 1 || zi < 1)
    {
        error = "Invalid coordinate index value";
        return false;
    }

    // colored attribute can't be changed if cloud already exists
    if (size())
        colorScale = colored() ? 1.0 : 0.0;

    Q_ASSERT(folder != "");
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

    IFTRACE(pointcloud)
        debug() << "Loading " << path << "\n";

    clear();
    QTextStream t(&f);
    QString line;
    unsigned count = 0;
    QString separator = +sep;
    int maxp = qMax(qMax(xi, yi), zi);
    float maxc = qMax(qMax(ri, gi), qMax(bi, ai));
    int max  = qMax(maxp, (int)maxc);
    double sz = f.size();
    double pos = 0.0;
    loaded = 0.0;
    do
    {
        if (interrupted())
        {
            IFTRACE(pointcloud)
                debug() << "loadData interrupted\n";
            return false;
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
        if (xok && yok && zok && colorok)
        {
            addPoint(Point(x, y, z), color);
            count++;
        }
    }
    while (!line.isNull());
    f.close();
    loaded = 1.0;

    this->file = file;
    IFTRACE(pointcloud)
        debug() << "Loaded " << count << " points\n";
    return true;
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
    return (loaded >= 0 && loaded < 1.0);
}
