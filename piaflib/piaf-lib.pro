# Project for Piaf API library
TEMPLATE = lib

TARGET = SwPluginCore
DEFINES += SWPLUGIN_SIDE

HEADERS = inc/SwPluginCore.h

SOURCES = src/SwPluginCore.cpp

INCLUDEPATH += inc
DEPENDPATH += inc

OBJECTS_DIR = .obj-lib
MOC_DIR = .moc-lib

# include opencv to check if it's available
include(../main/opencv.pri)

##INSTALLATION
target.path = /usr/local/lib/
target.files = *.so*


head.path = /usr/local/include/SwPlugin/
head.files = inc/*.h 

INSTALLS += target head

# FIXME : check if OpenCV is installed
INCLUDEPATH += ../main/tools/inc
DEPENDPATH += ../main/tools/inc

SOURCES += ../main/tools/src/swimage_utils.cpp
