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

unix:DEFINES += VERSION __LINUX_VERSION__

INCLUDEPATH += /usr/local/include/SwPlugin/

LIBS += -L/usr/local/lib/ -lSwPluginCore

linux-g++:TMAKE_CXXFLAGS += -g -Wall -O2 \
	-fexceptions -Wimplicit -Wreturn-type \
	-Wunused -Wswitch -Wcomment -Wuninitialized -Wparentheses  \
	-Wpointer-arith  -Wshadow

HEADERS = 
SOURCES =	$$(SRCNAME).cpp \
	../../main/tools/src/swimage_utils.cpp

DEPENDPATH +=		.
DEPENDPATH +=		$$INCLUDEPATH

