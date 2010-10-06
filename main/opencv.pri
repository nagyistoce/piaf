#this script try to locate opencv and set the right compilation options

# PG = opencv
# system(pkg-config --exists $$PG) {
#    message(opencv v$$system(pkg-config --modversion $$PG)found)
#    LIBS += $$system(pkg-config --libs $$PG)
#    INCLUDEPATH += $$system(pkg-config --cflags $$PG | sed s/-I//g)
# } else {
#   error($$PG "NOT FOUND => IT WILL NOT COMPILE")
#}

LIBSDIR =
LIBS_EXT = dylib


linux-g++: LIBS_EXT = so
unix: {
	# Test if OpenCV library is present
	exists( /usr/local/include/opencv/cv.hpp ) {
		#message("OpenCV found in /usr/local/include.")
		INCLUDEPATH += /usr/local/include/opencv

		CVINSTPATH = /usr/local
		CVINCPATH = /usr/local/include/opencv

		LIBS += -L/usr/local/lib
		LIBSDIR = /usr/local/lib
	} else {
		exists( /usr/include/opencv/cv.hpp )
		{
			#message("OpenCV found in /usr/include.")
			CVINSTPATH = /usr
			CVINCPATH = /usr/include/opencv
			INCLUDEPATH += /usr/include/opencv

			LIBS += -L/usr/lib
			LIBSDIR = /usr/lib
		} else {
			message ( "OpenCV NOT FOUND => IT WILL NOT COMPILE" )
			DEFINES += WITHOUT_OPENCV
		}
	}

	# on MacOS X with OpenCV 1, we must also link with cxcore
	#message( Dynamic libraries : '$$LIBS_EXT' )
	CXCORE_LIB = $$CVINSTPATH/lib/libcxcore.$$LIBS_EXT
	#message ( Testing CxCore lib = '$$CXCORE_LIB' )
	exists( $$CXCORE_LIB ) {
		#                      message( " => Linking with CxCore in '$$CVINSTPATH' ")
		LIBS += -lcxcore
	}


	# For Ubuntu 7.10 for example, the link option is -lcv instead of -lopencv
	CV_LIB = $$LIBSDIR/libcv.$$LIBS_EXT
	#message ( Testing CV lib = '$$CV_LIB' )
	exists( $$CV_LIB ) {
		#message( " => Linking with -lcv ('$$CV_LIB' exists)")
		LIBS += -lcv
	} else {
		CV_LIB = $$LIBSDIR/libopencv.$$LIBS_EXT
		exists($$CV_LIB ) {

			#message( " => Linking with -lopencv ('$$CV_LIB' does not exist)")
			LIBS += -lopencv
		}
	}
}


# Piaf use CVAux to capture images
unix: LIBS += -lcvaux -lhighgui



