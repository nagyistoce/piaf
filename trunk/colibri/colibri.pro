# -------------------------------------------------
# Project created by QtCreator 2009-08-10T21:22:13
# -------------------------------------------------
TARGET = Colibri

DEFINES += QT3_SUPPORT
QT += qt3support

include(../main/ffmpeg.pri)
include(../main/opencv.pri)

TEMPLATE = app
LANGUAGE = C++

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

macx: {
    message("MacOS X specific options =================================================")
    ICON = icons/Colibri128.icns
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
}

# TARGET = $$join(TARGET,,,_debug)
# DEFINES += "TRANSLATION_DIR=\"Colibri.app/Contents/\""
linux-g++ { 
    message("Linux specific options =================================================")
    DEFINES += "TRANSLATION_DIR=/usr/share/"
}

win32:TARGET = $$join(TARGET,,d)

# }

SOURCES += src/main.cpp \
    src/colibrimainwindow.cpp \
	src/imgutils.cpp \
	../main/tools/src/SwPluginCore.cpp \
	../main/tools/src/SwFilters.cpp \
	../main/src/imagewidget.cpp

HEADERS += inc/colibrimainwindow.h \
	inc/imgutils.h \
	../main/tools/inc/SwPluginCore.h \
	../main/tools/inc/SwFilters.h \
	../main/inc/imagewidget.h

FORMS += ui/colibrimainwindow.ui
INCLUDEPATH += inc
INCLUDEPATH += .
INCLUDEPATH += ../main/inc
INCLUDEPATH += ../main/tools/inc/

DEPENDPATH += $$INCLUDEPATH

RESOURCES += colibri.qrc

# # INSTALLATION
# target.path = /usr/local/colibri
# INSTALLS += target
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

