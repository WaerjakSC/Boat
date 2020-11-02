QT          += core gui widgets

TEMPLATE    = app
CONFIG      += c++17

TARGET      = INNgine2019

PRECOMPILED_HEADER = innpch.h

INCLUDEPATH +=  ./GSL

HEADERS += \
    GSL/matrix2x2.h \
    GSL/matrix3x3.h \
    GSL/matrix4x4.h \
    GSL/vector2d.h \
    GSL/vector3d.h \
    GSL/vector4d.h \
    GSL/gsl_math.h \
    GSL/math_constants.h \
    boat.h \
    constants.h \
    renderwindow.h \
    shader.h \
    mainwindow.h \
    texture.h \
    vertex.h \
    visualobject.h \
    camera.h \
    gltypes.h \
    input.h \
    material.h \
    objmesh.h \
#    innpch.h \
    colorshader.h \
    textureshader.h \


SOURCES += main.cpp \
    GSL/matrix2x2.cpp \
    GSL/matrix3x3.cpp \
    GSL/matrix4x4.cpp \
    GSL/vector2d.cpp \
    GSL/vector3d.cpp \
    GSL/vector4d.cpp \
    GSL/gsl_math.cpp \
    boat.cpp \
    renderwindow.cpp \
    mainwindow.cpp \
    shader.cpp \
    texture.cpp \
    vertex.cpp \
    visualobject.cpp \
    camera.cpp \
    input.cpp \
    material.cpp \
    objmesh.cpp \
    colorshader.cpp \
    textureshader.cpp

FORMS += \
    mainwindow.ui

DISTFILES += \
    Shaders/plainshader.frag \
    Shaders/plainshader.vert \
    Shaders/textureshader.frag \
    GSL/README.md \
    README.md \
    Shaders/textureshader.vert
