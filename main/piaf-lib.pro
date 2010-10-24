# Project for Piaf API library
TEMPLATE = lib

TARGET = SwPluginCore

SOURCES = tools/src/SwPluginCore.cpp
INCLUDEPATH += tools/inc/ inc
DEPENDPATH += tools/inc/ inc/

##INSTALLATION
target.path = /usr/local/lib/

head.path = /usr/local/include/SwPlugin/
head.files = tools/inc/*.h inc/*.h

INSTALLS += target head

