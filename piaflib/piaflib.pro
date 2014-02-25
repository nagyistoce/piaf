# Project for Piaf API library
TEMPLATE = lib

CONFIG -= qt

TARGET = SwPluginCore
DEFINES += SWPLUGIN_SIDE

HEADERS = inc/SwPluginCore.h

SOURCES = src/SwPluginCore.cpp

message("OpenCV inc before: $$INCLUDEPATH")
# include opencv to check if it's available
include(../main/opencv.pri)

message("OpenCV inc after: $$INCLUDEPATH")

INCLUDEPATH += inc
DEPENDPATH += inc

linux*arm* {
	DESTDIR = ../lib/arm
	OBJECTS_DIR = .obj-arm
        MOC_DIR = .moc-arm
} else {
	DESTDIR = ../lib/x86
	OBJECTS_DIR = .obj-lib
	MOC_DIR = .moc-lib
}


##INSTALLATION
target.path = /usr/local/lib/
target.files = *.so*


head.path = /usr/local/include/SwPlugin/
head.files = inc/*.h ../main/tools/inc/swimage_utils.h

INSTALLS += target head

# FIXME : check if OpenCV is installed
INCLUDEPATH += ../main/tools/inc
DEPENDPATH += ../main/tools/inc

SOURCES += ../main/tools/src/swimage_utils.cpp

