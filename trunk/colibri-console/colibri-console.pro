#-------------------------------------------------
#
# Project created by QtCreator 2013-09-29T21:45:19
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = colibri-console
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

DEFINES += CONSOLE

include(../main/opencv.pri)

TEMPLATE = app
LANGUAGE = C++

INCLUDEPATH += ../piaflib/inc
DEPENDPATH += ../piaflib/inc

CONFIG += qt thread \
	warn_on \
	debug_and_release


linux-g++:TMAKE_CXXFLAGS += -Wall \
	-g \
	-O2 \
	-fexceptions \
	-Wimplicit \
	-Wreturn-type \
	-Wunused \
	-Wswitch \
	-Wcomment \
	-Wuninitialized \
	-Wparentheses \
	-Wpointer-arith



INCLUDEPATH += inc
INCLUDEPATH += ../colibri/inc
INCLUDEPATH += .
INCLUDEPATH += ../main/inc
INCLUDEPATH += ../main/tools/inc/
INCLUDEPATH += ../main/acquisitions/video/inc/
# filters management
INCLUDEPATH += ../workflow/tools/inc/


DEPENDPATH += $$INCLUDEPATH
win32: {
	OBJECTS_DIR = obj
	MOC_DIR = moc
} else {
	OBJECTS_DIR = .obj
	MOC_DIR = .moc
}

# console includes
SOURCES += main.cpp \
	src/colibrithread.cpp

DEFINES += console

# generic includes for colibri
SOURCES += \
	../colibri/src/imgutils.cpp \
	../main/tools/src/swimage_utils.cpp

HEADERS += \
	../colibri/inc/imgutils.h \
	../main/tools/inc/swimage_utils.h

linux-g++* {
	HEADERS += \
		../piaflib/inc/SwPluginCore.h \
		../workflow/tools/inc/PiafFilter.h
	SOURCES += ../piaflib/src/SwPluginCore.cpp \
		../workflow/tools/src/PiafFilter.cpp \
		../main/acquisitions/video/src/swvideodetector.cpp \
		../main/tools/src/time_histogram.cpp
}




OTHER_FILES += \
    qtc_packaging/debian_harmattan/rules \
    qtc_packaging/debian_harmattan/README \
    qtc_packaging/debian_harmattan/manifest.aegis \
    qtc_packaging/debian_harmattan/copyright \
    qtc_packaging/debian_harmattan/control \
    qtc_packaging/debian_harmattan/compat \
    qtc_packaging/debian_harmattan/changelog

HEADERS += \
    inc/colibrithread.h

# FINAL CONFIGURATION ==================================================
message( "")
message( "")
message( "FINAL CONFIGURATION ================================================== ")
message( "Configuration : ")
message( " config : $$CONFIG ")
message( " defs : $$DEFINES ")
message( " libs : $$LIBS ")
message( "FINAL CONFIGURATION ================================================== ")
message( "")
message( "")
