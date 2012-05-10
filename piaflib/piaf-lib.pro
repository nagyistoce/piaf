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


##INSTALLATION
target.path = /usr/local/lib/

head.path = /usr/local/include/SwPlugin/
head.files = inc/*.h 

INSTALLS += target head

