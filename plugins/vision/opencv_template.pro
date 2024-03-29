# Generic build profile for OpenCV based plugins

SRCNAME = $$(SRCNAME)
message( Building $(SRCNAME))

TEMPLATE        =       app
CONFIG          =       warn_on release thread
TARGET          =       $$(SRCNAME)
OBJECTS_DIR 	= 	.obj

# Include OpenCV definitions
include(../../main/opencv.pri)


unix:LIBS += -L/usr/local/lib 

LIBS += -pg 

unix:DEFINES += VERSION __LINUX_VERSION__

INCLUDEPATH += /usr/local/include/SwPlugin/
# add path to swimage_utils.h for IplImage <-> SwImage conversion
INCLUDEPATH += ../../main/tools/inc

# local include
INCLUDEPATH += ../../piaflib/inc/


DEPENDPATH += $$INCLUDEPATH

LIBS += -L/usr/local/lib/ -lSwPluginCore

# in case the installation was not done, use local link
LIBS += -L../../piaflib

linux-g++:TMAKE_CXXFLAGS += -g -Wall -O2 -pg \
	-fexceptions -Wimplicit -Wreturn-type \
	-Wunused -Wswitch -Wcomment -Wuninitialized -Wparentheses  \
	-Wpointer-arith  -Wshadow

HEADERS = 
SOURCES =	$$(SRCNAME).cpp \
	../../main/tools/src/swimage_utils.cpp

DEPENDPATH +=		.
DEPENDPATH +=		$$INCLUDEPATH

