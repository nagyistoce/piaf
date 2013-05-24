# Test the installation of OpenNI 2 

exists(/usr/local/include/openni2) {
        message("The system knows OpenNI v2")
	DEFINES += HAS_OPENNI2
        INCLUDEPATH += /usr/local/include/openni2
        LIBS += -lOpenNI2
} else {
	exists(/usr/include/ni) {
	        message("The system knows OpenNI v1")
        	DEFINES += HAS_OPENNI
        	INCLUDEPATH += /usr/include/ni
        	LIBS += -lOpenNI
	} else {
		message("No support for OpenNI v1 or v2")
	}
}

