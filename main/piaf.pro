# Piaf GUI .pro
# Author : Christophe Seyve - cseyve@free.fr

TARGET = piaf

VERSION = "`date +%Y%m%d`"
QT += xml
CONFIG += qt \
    warn_on \
    debug \
    thread

win32:DEFINES += QT_DLL 

win32:LIBS += ../../release/qwt.lib
macx: DEFINES+= __MACOSX__

# This is the legacy version of Piaf, with Qt3support
DEFINES += PIAF_LEGACY
LANGUAGE = C++

include(v4l2.pri)
include(ffmpeg.pri)
include(opencv.pri)
include(openni.pri)


unix: {
	DEFINES += VERSION="`date +%Y%m%d`"
	LIBS += -L/usr/local/lib
}

linux-g++* {
	DEFINES += _LINUX
}

message("====== Checking supports for different libraries in DEFINES=$$DEFINES ======")
LEGACYPATH = .

contains(DEFINES, "HAS_FFMPEG") {
	message("    + Add file support through FFMPEG")
	HEADERS += \
		$$LEGACYPATH/acquisitions/video/inc/ffmpeg_file_acquisition.h \

	SOURCES += \
		$$LEGACYPATH/acquisitions/video/src/ffmpeg_file_acquisition.cpp \
}

contains(DEFINES, "HAS_V4L") {
	message("    + Add device support through V4L1")
	SOURCES += $$LEGACYPATH/acquisitions/video/src/v4lutils.c
	HEADERS += $$LEGACYPATH/acquisitions/video/inc/v4lutils.h
}

contains(DEFINES, "HAS_V4L2") {
	message("    + Add device support through V4L2")
	SOURCES += \
			$$LEGACYPATH/acquisitions/video/src/jdatasrc.c
	DEFINES += _V4L2

	HEADERS += $$LEGACYPATH/acquisitions/video/inc/V4L2Device.h \
		$$LEGACYPATH/acquisitions/video/inc/v4l2uvc.h \
		$$LEGACYPATH/acquisitions/video/inc/utils.h \
		$$LEGACYPATH/acquisitions/video/inc/uvccolor.h

	SOURCES += $$LEGACYPATH/acquisitions/video/src/V4L2Device.cpp \
		$$LEGACYPATH/acquisitions/video/src/v4l2uvc.c \
		$$LEGACYPATH/acquisitions/video/src/utils.c
}

contains(DEFINES, "HAS_OPENNI") {
# || contains(DEFINES, "HAS_OPENNI2") {
	message(" + Add support for OPENNI")

	# common
	# Live video source
	HEADERS += \
		$$LEGACYPATH/acquisitions/video/inc/openni_videoacquisition.h \
		$$LEGACYPATH/acquisitions/video/inc/openni_common.h
	SOURCES +=  $$LEGACYPATH/acquisitions/video/src/openni_videoacquisition.cpp \
		$$LEGACYPATH/acquisitions/video/src/openni_common.cpp
	!contains(DEFINES, "HAS_OPENNI2") {
	# Live video source
	HEADERS +=  $$LEGACYPATH/acquisitions/video/inc/openni_file_acquisition.h
	SOURCES +=  $$LEGACYPATH/acquisitions/video/src/openni_file_acquisition.cpp
	}

}


win32: {
	DEFINES += QT_DLL QWT_DLL
	#LIBS += ../../release/qwt.lib
}
TRANSLATIONS = piaf_French.ts
DEFINES += QT3_SUPPORT


INCLUDEPATH += inc/
INCLUDEPATH += tools/inc/
INCLUDEPATH += acquisitions/inc/
INCLUDEPATH += acquisitions/video/inc/
INCLUDEPATH += components/inc/


SOURCES += src/main.cpp \
    src/objectsexplorer.cpp \
    src/sw_component.cpp \
    src/sw_library.cpp \
    src/workshop.cpp \
	src/imageinfo.cpp \
	src/workshoplist.cpp \
	components/src/workshopcomponent.cpp \
    components/src/workshopmeasure.cpp \
    components/src/workshopimage.cpp \
    components/src/workshopmovie.cpp \
	tools/src/imgutils.cpp \
	tools/src/qimage_utils.cpp \
	tools/src/workshoptool.cpp \
	tools/src/previewimage.cpp \
    tools/src/workshopvideocapture.cpp \
    ../piaflib/src/SwPluginCore.cpp \
    tools/src/SwFilters.cpp \
    tools/src/workshopimagetool.cpp \
    tools/src/videoplayertool.cpp \
	#acquisitions/video/src/FileVideoAcquisition.cpp \
	acquisitions/video/src/videocapture.cpp \
	acquisitions/video/src/swvideodetector.cpp \
	acquisitions/video/src/uvccolor.c \
    tools/src/swtoolmainwindow.cpp \
    tools/src/OpenCVEncoder.cpp \
	tools/src/moviebookmarkform.cpp \
    acquisitions/video/src/virtualdeviceacquisition.cpp \
	acquisitions/video/src/opencvvideoacquisition.cpp \
    src/plugineditdialog.cpp \
	tools/src/timehistogramwidget.cpp \
    tools/src/batchfiltersmainwindow.cpp \
	tools/src/vidacqsettingswindow.cpp \
	tools/src/swimage_utils.cpp \
	tools/src/imagewidget.cpp \
	tools/src/time_histogram.cpp
contains($$DEFINES, "HAS_FFMPEG") {
	message("Add ffmpeg classes")
	SOURCES += acquisitions/video/src/ffmpeg_file_acquisition.cpp 
	HEADERS += acquisitions/video/inc/ffmpeg_file_acquisition.h
}

# Replaced by OpenCVEncoder.cpp
# tools/src/FFMpegEncoder.cpp  
#    tools/src/SwMpegEncoder.cpp \
#    acquisitions/video/src/SwVideoAcquisition.cpp \
#    acquisitions/video/src/VirtualDevice.cpp \


# Difficulty to port Qwt3 to Qt4
# tools/src/previewplot2d.cpp \
# tools/src/workshopplot2d.cpp \
# ../otherlibs/qwt-sisell-0.1/src/qwt_buff_curve.cpp \
# ../otherlibs/qwt-sisell-0.1/src/qwt_ext_plot.cpp \
# ../otherlibs/qwt-sisell-0.1/src/qwt_ext_cursor.cpp \
# LIBS += -lqwt

# Porting to use CvCapture instead of V4L/V4L2 => obsolete classes
# acquisitions/video/src/v4l2uvc.c \
# acquisitions/video/src/utils.c \
# acquisitions/video/src/color.c \
# acquisitions/video/src/avilib.c \
# acquisitions/video/src/V4L2Device.cpp \
linux-g++:exists(/usr/lib/gcc/x86_64-linux-gnu) { 
    message( "64bit CPU => use C function for YUV conversion" )
    SOURCES += acquisitions/video/src/ccvt_c.c
}
else { 
    exists(/usr/include/linux/linkage.h) { 
        message( "32bit CPU => use MMX code for YUV conversion" )
        SOURCES += acquisitions/video/src/ccvt.S
    }
    else { 
        message( "no linkage.h => use ccvt_c.c" )
        SOURCES += acquisitions/video/src/ccvt_c.c
    }
}

INCLUDEPATH += ../piaflib/inc
DEPENDPATH += ../piaflib/

HEADERS += inc/workshop.h \
    inc/objectsexplorer.h \
    inc/sw_component.h \
    inc/sw_library.h \
    inc/sw_structure.h \
    ../piaflib/inc/sw_types.h \
    inc/workshoplist.h \
    ../piaflib/inc/SwTypes.h \
	../piaflib/inc/SwImage.h \
	inc/piaf-common.h \
	inc/imageinfo.h \
	components/inc/workshopcomponent.h \
    components/inc/workshopmeasure.h \
    components/inc/workshopimage.h \
    components/inc/workshopmovie.h \
	tools/inc/imgutils.h \
	tools/inc/workshoptool.h \
	tools/inc/previewimage.h \
    tools/inc/workshopvideocapture.h \
    ../piaflib/inc/SwPluginCore.h \
    tools/inc/SwFilters.h \
    tools/inc/workshopimagetool.h \
    tools/inc/qlistboxmarker.h \
    tools/inc/videoplayertool.h \
	tools/inc/OpenCVEncoder.h \
	tools/inc/moviebookmarkform.h \
	tools/inc/swimage_utils.h \
	acquisitions/video/inc/v4lutils.h \
    acquisitions/video/inc/ccvt.h \
    acquisitions/video/inc/SwVideoAcquisition.h \
    acquisitions/video/inc/FileVideoAcquisition.h \
	acquisitions/video/inc/ffmpeg_file_acquisition.h \
	acquisitions/video/inc/videocapture.h \
    acquisitions/video/inc/swvideodetector.h \
    tools/inc/swtoolmainwindow.h \
	acquisitions/video/inc/virtualdeviceacquisition.h \
    acquisitions/video/inc/opencvvideoacquisition.h \
    inc/plugineditdialog.h \
	tools/inc/timehistogramwidget.h \
	tools/inc/batchfiltersmainwindow.h \
	tools/inc/vidacqsettingswindow.h \
	inc/piaf-settings.h \
	inc/nolinux_videodev.h \
	tools/inc/imagewidget.h \
	tools/inc/time_histogram.h \

exists(/usr/local/include/libfreenect/libfreenect.h) {
	message("The system knows Freenect ;)")
	DEFINES += HAS_FREENECT
	INCLUDEPATH += /usr/local/include/libfreenect
	LIBS += -L/usr/local/lib -lfreenect
	HEADERS +=  acquisitions/video/inc/freenectvideoacquisition.h
	SOURCES +=  acquisitions/video/src/freenectvideoacquisition.cpp
}


# Obsolete because of OpenCVEncoder
# tools/inc/FFMpegEncoder.h \ 
#    tools/inc/SwMpegEncoder.h \

# Obsolete because of OpenCV's capture API
# acquisitions/video/inc/VirtualDevice.h \
# tools/inc/previewplot2d.h \
# tools/inc/workshopplot2d.h \
# ../otherlibs/qwt-sisell-0.1/inc/qwt_buff_curve.h \
# ../otherlibs/qwt-sisell-0.1/inc/qwt_ext_plot.h \
# ../otherlibs/qwt-sisell-0.1/inc/qwt_ext_cursor.h \
# acquisitions/video/inc/v4l2uvc.h \
# acquisitions/video/inc/utils.h \
# acquisitions/video/inc/color.h \
# acquisitions/video/inc/avilib.h \
# acquisitions/video/inc/V4L2Device.h \



unix: {
	MOC_DIR = .moc
	OBJECTS_DIR = .obj
} else {
        MOC_DIR = moc
        OBJECTS_DIR = obj
}

DEPENDPATH += ./inc
DEPENDPATH += components/inc
DEPENDPATH += tools/inc
DEPENDPATH += acquisitions/video/inc
DEPENDPATH += ./ui

#no more QwT in Qt4 version of piaf DEPENDPATH += ../otherlibs/qwt-sisell-0.1/inc

macx: {
	DEFINES += __MACOSX__
}

linux-g++: {
	DEFINES += __LINUX__
	TMAKE_CXXFLAGS += -g \
		-Wall \
		-O0 \
		-fexceptions \
		-Wimplicit \
		-Wreturn-type \
		-Wunused \
		-Wswitch \
		-Wcomment \
		-Wuninitialized \
		-Wparentheses \
		-Wpointer-arith

	INCLUDEPATH += /usr/src/linux/include
}

# INCLUDEPATH += ../otherlibs/qwt-sisell-0.1/inc
INCLUDEPATH += ui tools/ui

# INCLUDEPATH += /usr/include/qwt-qt4
INCLUDEPATH += images/pixmaps
INCLUDEPATH += $$MAIN_INC_DIR
INCLUDEPATH += $$TOOL_INC_DIR
INCLUDEPATH += $$VIDEO_ACQ_INC_DIR
INCLUDEPATH += $$COMP_INC_DIR
INCLUDEPATH += $$QWTS_INC_DIR
DEPENDPATH += $$INCLUDEPATH

message( "Installation directory = $(PWD) ")
unix: { 
    INSTALL_PIAF = $(PIAF_DIR)
    message( Reading installation directory : '$$INSTALL_PIAF')
    count( $$INSTALL_PIAF , 0 ):message("Installation directory is undefined !! Installing in '/usr/local/piaf'.")
    else { 
        message( Installing in : $$INSTALL_PIAF)
        DEFINES += BASE_DIRECTORY=$$INSTALL_PIAF
        message( defines : $$DEFINES )
    }
}

INCLUDEPATH += .

FORMS += tools/ui/swtoolmainwindow.ui \
		tools/ui/imagetoavidialog.ui \
		tools/ui/moviebookmarkform.ui \
    ui/plugineditdialog.ui \
	tools/ui/timehistogramwidget.ui \
	tools/ui/batchfiltersmainwindow.ui \
	tools/ui/vidacqsettingswindow.ui

HEADERS += tools/inc/imagetoavidialog.h

SOURCES += tools/src/imagetoavidialog.cpp

# since we get image info, use the Exiv2 library
LIBS += -lexiv2


# The following line was changed from FORMS to FORMS3 by qt3to4
#FORMS3 = ui/randsignalform.ui \
#    ui/marker_dialog.ui \
#    ui/curve_properties.ui
FORMS3 = ui/piafconfigurationdialog.ui
#    ui/pluginlistdialog.ui
TEMPLATE = app

# The following line was inserted by qt3to4
QT += qt3support

# The following line was inserted by qt3to4
CONFIG += uic3


###########################################################
## INSTALLATION
target.path = /usr/local/piaf/

images.path = /usr/local/piaf/images/
images.files = pixmaps/

INSTALLS += target images



###########################################################
## SUMMARY

message( "Config: ==========================================")
message( "INCLUDEPATH = $$INCLUDEPATH" )
message( "DEFINES = $$DEFINES" )
message( "LIBS = $$LIBS" )

RESOURCES += \
    piaf.qrc




