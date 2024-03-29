# this script try to locate opencv and set the right compilation options
# PG = opencv
# system(pkg-config --exists $$PG) {
# message(opencv v$$system(pkg-config --modversion $$PG)found)
# LIBS += $$system(pkg-config --libs $$PG)
# INCLUDEPATH += $$system(pkg-config --cflags $$PG | sed s/-I//g)
# } else {
# error($$PG "NOT FOUND => IT WILL NOT COMPILE")
# }
# For MacOS X
LIBS_EXT = dylib

# Append avcodec
PG=libavcodec
system(pkg-config --exists $$PG) {

	message($$PG v$$system(pkg-config --modversion $$PG) found)
	LIBS += $$system(pkg-config --libs $$PG)
	INCLUDEPATH += $$system(pkg-config --cflags $$PG | sed s/-I//g)


	DEFINES += HAS_FFMPEG
} else {
	error($$PG "NOT FOUND => IT WILL NOT COMPILE")
}


# Append avformat
PG=libavformat
system(pkg-config --exists $$PG) {
	message($$PG v$$system(pkg-config --modversion $$PG) found)
	LIBS += $$system(pkg-config --libs $$PG)
	INCLUDEPATH += $$system(pkg-config --cflags $$PG | sed s/-I//g)


	DEFINES += HAS_FFMPEG
} else {
	error($$PG "NOT FOUND => IT WILL NOT COMPILE")
}


# Append swscale
PG=libswscale
system(pkg-config --exists $$PG) {

	message($$PG v$$system(pkg-config --modversion $$PG) found)
	LIBS += $$system(pkg-config --libs $$PG)
	INCLUDEPATH += $$system(pkg-config --cflags $$PG | sed "s/\-I//g")

	DEFINES += HAS_SWSCALE
} else {
	error($$PG "NOT FOUND => IT WILL NOT COMPILE")
}


linux-g++*:LIBS_EXT = so
INCLUDE_AVCODEC =
LIBSWSLIBDIR =
LIBSWSINCDIR = 
unix: {
	# Test if FFMEPG library is present
	exists( /usr/local/include/ffmpeg/avcodec.h ) {
				message("ffmpeg found in /usr/local/include.")
		INCLUDEPATH += /usr/local/include/ffmpeg
		INCLUDE_AVCODEC = /usr/local/include/ffmpeg
		
		LIBS += -L/usr/local/lib
		LIBSWSINCDIR = /usr/local/include/libswscale
		LIBSWSDIR = /usr/local/lib
	} else {
		exists( /usr/local/include/libavcodec/avcodec.h ) {
			# separated includes... damn ffmpeg daily modifications !!
			message("ffmpeg found in /usr/local/include/ffmpeg/libav*.")
			INCLUDEPATH += /usr/local/include/
                        INCLUDEPATH += /usr/local/include/libavutils

			INCLUDEPATH += /usr/local/include/libavcodec
			INCLUDEPATH += /usr/local/include/libavformat

			LIBSWSLIBDIR = /usr/local/lib
			LIBSWSINCDIR = /usr/local/include/libswscale
			LIBS += -L/usr/local/lib
		} else {
			exists( /usr/include/libavcodec/avcodec.h ) {
				message("ffmpeg found in /usr/include/libavcodec/libav*.")


				# separated includes... damn ffmpeg daily modifications !!
				INCLUDEPATH += /usr/include/libavcodec
				INCLUDEPATH += /usr/include/libavformat
				INCLUDEPATH += /usr/include/libavutils

				#INCLUDEPATH += /usr/include/ffmpeg
				LIBSWSLIBDIR = /usr/lib
				LIBSWSINCDIR = /usr/include/libswscale

				LIBS += -L/usr/lib 
			} else {
				exists( /usr/include/ffmpeg/avcodec.h ) {
					message("ffmpeg found in /usr/include.")
					INCLUDEPATH += /usr/include/ffmpeg
					LIBSWSLIBDIR = /usr/lib
					LIBSWSINCDIR = /usr/include/libswscale/
					LIBS += -L/usr/lib
				} else {
					message ( "ffmpeg NOT FOUND => IT WILL NOT COMPILE" )
				}
			}
		}
	}


	# Check if we need to link with libswscale
	exists($$LIBSWSINCDIR/swscale.h) {
		DEFINES += HAS_SWSCALE
		INCLUDEPATH += $$LIBSWSINCDIR
		LIBS += -lswscale
	}

	message("Testing if libs ate installed in $$LIBSWSLIBDIR/libavcodec.$$LIBS_EXT")
	exists($$LIBSWSLIBDIR/libavcodec.$$LIBS_EXT) { 
		DEFINES += HAS_FFMPEG
	}

	SWSCALE_H = $$LIBSWSLIBDIR/libswscale.$$LIBS_EXT
	message ( Testing SWScale lib = '$$SWSCALE_H' )

	LIBS += -lavutil -lavcodec -lavformat
}

win32: {
	DEFINES +=
	INCLUDEPATH += "C:\Program Files\ffmpeg\include\ffmpeg"
	LIBS += -L"C:\Program Files\ffmpeg\lib" \
		-L"C:\Program Files\ffmpeg\bin" \
		-lavcodec
}




message("FFMPEG: ")
message("     - Includes : $$INCLUDEPATH")
