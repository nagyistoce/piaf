# .pri for video4linux2 / V4L2 support
exists(/usr/include/linux/videodev.h) {
	message("Support for V4L(1)")
	DEFINES += HAS_VIDEODEV HAS_V4L
}

exists(/usr/include/linux/videodev2.h) {
	DEFINES += HAS_VIDEODEV HAS_V4L2
        message("Support for V4L2 -> defines=$$DEFINES")
}



