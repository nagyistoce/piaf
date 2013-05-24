# Generic build profile for OpenCV based plugins

SRCNAME = $$(SRCNAME)
message( Building $(SRCNAME))

TEMPLATE        =       app
CONFIG          =       warn_on release thread
LANGUAGE        =       C++
TARGET          =       $$(SRCNAME)
OBJECTS_DIR 	= 	.obj

# Include OpenCV definitions
include(../../main/opencv.pri)

unix: {
	LIBS += -L/usr/local/lib 
	LIBS += -lSwPluginCore -lfftw3 
	exists(/usr/lib/libsfftw.so) {
		DEFINES += HAS_SFFTW
		LIBS += -lsfftw -lsrfftw
	}
	DEFINES += VERSION __LINUX_VERSION__
}

linux-g++:TMAKE_CXXFLAGS += -g -Wall -O2 \
	-fexceptions -Wimplicit -Wreturn-type \
	-Wunused -Wswitch -Wcomment -Wuninitialized -Wparentheses  \
	-Wpointer-arith  -Wshadow

HEADERS =	
SOURCES =	$$(SRCNAME).cpp 

INCLUDEPATH +=          /usr/local/include/SwPlugin/

DEPENDPATH +=		.
DEPENDPATH +=		$$INCLUDEPATH


# local link if not installed
LIBS += -L../../piaflib 


