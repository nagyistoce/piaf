# Project for Piaf API library
TEMPLATE = lib

TARGET = SwPluginCore
DEFINES += SWPLUGIN_SIDE

HEADERS = tools/inc/SwPluginCore.h

SOURCES = tools/src/SwPluginCore.cpp

INCLUDEPATH += tools/inc/ inc
DEPENDPATH += tools/inc/ inc/

OBJECTS_DIR = .obj-lib
MOC_DIR = .moc-lib


##INSTALLATION
target.path = /usr/local/lib/

head.path = /usr/local/include/SwPlugin/
head.files = tools/inc/*.h \
			inc/*.h

INSTALLS += target head

