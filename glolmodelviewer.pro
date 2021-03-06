######################################################################
# Automatically generated by qmake (2.01a) ?? 6? 30 07:46:40 2016
######################################################################

TEMPLATE = app
TARGET = glolmodelviewer
DEPENDPATH += . gl SOIL include src
INCLUDEPATH += . gl SOIL include src

CONFIG += gl iLmBase glu
LIBS += -lGL -lglut -lIex

#DEFINES += _G_TEST
QMAKE_CXXFLAGS += -std=c++0x

CONFIG += console
QT -= core gui
#MOC_DIR = ./moc
OBJECTS_DIR = ./obj

# Input
HEADERS += SOIL/image_DXT.h \
           SOIL/image_helper.h \
           src/lol_reader.h \
           SOIL/SOIL.h \
           SOIL/stb_image_aug.h \
           SOIL/stbi_DDS_aug.h \
           SOIL/stbi_DDS_aug_c.h \
					 include/gstd.h \
					 src/lol_render.h \
					 include/gutility.h \
					 include/imath_ext.h \
           gl/glut.h \
					 include/g_gui.h

SOURCES += SOIL/image_DXT.c \
SOIL/image_helper.c \
src/lol_reader.cpp \
main.cpp \
SOIL/SOIL.c \
SOIL/stb_image_aug.c \
src/lol_render.cpp \
src/lol_skn_reader.cpp \
src/lol_skl_reader.cpp \
src/lol_anm_reader.cpp \
src/lol_dds_reader.cpp
