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
linux-g++:LIBS_EXT = so
INCLUDE_AVCODEC =
LIBSWSDIR =

unix: {
	# Test if FFMEPG library is present
	exists( /usr/local/include/ffmpeg/avcodec.h ) {
				message("ffmpeg found in /usr/local/include.")
		INCLUDEPATH += /usr/local/include/ffmpeg
		INCLUDE_AVCODEC = /usr/local/include/ffmpeg

		LIBS += -L/usr/local/lib
		LIBSWSDIR = /usr/local/include/ffmpeg
	} else {
		exists( /usr/local/include/libavcodec/avcodec.h ) {
			# separated includes... damn ffmpeg daily modifications !!
			message("ffmpeg found in /usr/local/include/ffmpeg/libav*.")
			INCLUDEPATH += /usr/local/include/
                        INCLUDEPATH += /usr/local/include/libavutils

			INCLUDEPATH += /usr/local/include/libavcodec
			INCLUDEPATH += /usr/local/include/libavformat

			LIBSWSDIR = /usr/local/include/libswscale
			LIBS += -L/usr/local/lib
		} else {
			exists( /usr/include/libavcodec/avcodec.h ) {
				message("ffmpeg found in /usr/include/ffmpeg/libav*.")
				# separated includes... damn ffmpeg daily modifications !!
				INCLUDEPATH += /usr/include/libavcodec
				INCLUDEPATH += /usr/include/libavformat
				INCLUDEPATH += /usr/include/libavutils

				#INCLUDEPATH += /usr/include/ffmpeg
				LIBSWSDIR = /usr/include/libswscale

				LIBS += -L/usr/lib -lswscale
			} else {
				exists( /usr/include/ffmpeg/avcodec.h ) {
					message("ffmpeg found in /usr/include.")
					INCLUDEPATH += /usr/include/ffmpeg
					LIBSWSDIR = /usr/include/ffmpeg
					LIBS += -L/usr/lib
				} else {
					message ( "ffmpeg NOT FOUND => IT WILL NOT COMPILE" )
				}
			}
		}
	}


	# Check if we need to link with libswscale
	SWSCALE_H = $$LIBSWSDIR/swscale.h
	message ( Testing SWScale lib = '$$SWSCALE_H' )
	exists( $$SWSCALE_H ) {
		message("Linking with libswscale")
		INCLUDEPATH += $$LIBSWSDIR
		LIBS += -lswscale
	}

	LIBS += -lavutil -lavcodec -lavformat
}

win32: {
	DEFINES +=
	INCLUDEPATH += "C:\Program Files\ffmpeg\include\ffmpeg"
	LIBS += -L"C:\Program Files\ffmpeg\lib" \
		-L"C:\Program Files\ffmpeg\bin" \
		-lavcodec
}
