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


bool PointCloud::addPoint(const Point &p)
// ----------------------------------------------------------------------------
//   Add a new point to the cloud
// ----------------------------------------------------------------------------
{
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
        points.pop_back();
}


void PointCloud::draw()
// ----------------------------------------------------------------------------
//   Draw cloud
// ----------------------------------------------------------------------------
{
    if (points.size() == 0)
        return;

    // Activate current document color
    PointCloudFactory::tao->SetFillColor();

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Point), &points[0].x);
    glDrawArrays(GL_POINTS, 0, size());
    glDisableClientState(GL_VERTEX_ARRAY);
}


void PointCloud::clear()
// ----------------------------------------------------------------------------
//   Remove all points
// ----------------------------------------------------------------------------
{
    points.clear();
}


static float random01()
// ----------------------------------------------------------------------------
//   Return random float in [0.0, 1.0]
// ----------------------------------------------------------------------------
{
    return float(XL::xl_random(0.0, 1.0));
}


bool PointCloud::randomPoints(unsigned n)
// ----------------------------------------------------------------------------
//   Create a point cloud with n random points
// ----------------------------------------------------------------------------
{
    if (size() == n)
        return false;

    IFTRACE(pointcloud)
        debug() << "Points: " << size() << " requested: " << n << "\n";

    int delta = n - size();
    if (delta < 0)
        removePoints(-delta);
    else
        for (int i = 0; i < delta; ++i)
            addPoint(Point(random01(), random01(), random01()));

    return true;
}


bool PointCloud::loadData(text file, text sep, int xi, int yi, int zi)
// ----------------------------------------------------------------------------
//   Load points from a file
// ----------------------------------------------------------------------------
{
    if (file == this->file)
        return false;

    if (xi < 1 || yi < 1 || zi < 1)
    {
        error = "Invalid index value";
        return false;
    }

    text folder = PointCloudFactory::tao->currentDocumentFolder();
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
    int max = qMax(qMax(xi, yi), zi);
    do
    {
        line = t.readLine();
        QStringList values = line.split(separator);
        if (values.size() < max)
            continue;
        bool xok, yok, zok;
        float x = values[xi-1].toFloat(&xok);
        float y = values[yi-1].toFloat(&yok);
        float z = values[zi-1].toFloat(&zok);
        if (xok && yok && zok)
        {
            addPoint(Point(x, y, z));
            count++;
        }
    }
    while (!line.isNull());
    f.close();

    this->file = file;
    IFTRACE(pointcloud)
        debug() << "Loaded " << count << " points\n";
    return true;
}


std::ostream & PointCloud::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[PointCloud] \"" << name << "\" " << (void*)this << " ";
    return std::cerr;
}
