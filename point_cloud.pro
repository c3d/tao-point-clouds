# ******************************************************************************
#  point_cloud.pro                                                  Tao project
# ******************************************************************************
# File Description:
# Qt build file for the PointCloud module
# ******************************************************************************
# This software is property of Taodyne SAS - Confidential
# Ce logiciel est la propriété de Taodyne SAS - Confidentiel
# (C) 2012 Jerome Forissier <jerome@taodyne.com>
# (C) 2012 Taodyne SAS
# ******************************************************************************

MODINSTDIR = point_cloud

include(../modules.pri)

HEADERS     = point_cloud.h
SOURCES     = point_cloud.cpp
TBL_SOURCES = point_cloud.tbl
!macx {
DEFINES += GLEW_STATIC
SOURCES += $${TAOTOPSRC}/tao/include/tao/GL/glew.c
}
OTHER_FILES = point_cloud.xl point_cloud.tbl traces.tbl
QT         += core opengl

# Icon is a picture of a point cloud rendering of the well-known "Standford
# Bunny" (http://graphics.stanford.edu/data/3Dscanrep/).
INSTALLS += thismod_icon
