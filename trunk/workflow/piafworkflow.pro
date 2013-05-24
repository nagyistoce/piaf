# -------------------------------------------------
# Project created by QtCreator 2009-07-01T21:37:51
# -------------------------------------------------
TEMPLATE = app

QT += xml


# Use lowercase name for Linux
linux-g++: {
	TARGET = piafworkflow
} else {
	TARGET = PiafWorkflow
}

CONFIG += debug

DEFINES += PIAFWORKFLOW
PIAFLIB = ../piaflib
INCLUDEPATH += ../piaflib/inc

unix: {
    DEFINES += VERSION_YY="`date +%Y`" \
        VERSION_MM="`date +%m | sed 's/0//'`" \
        VERSION_DD="`date +%d | sed 's/0//'`" \
	__LINUX__ LINUX _LINUX
}

win32: {
    DEFINES += VERSION_YY="2011" \
	VERSION_MM="08" \
	VERSION_DD="29"
    DEFINES += __WINDOWS__ WINDOWS
}

# icon
# reference : file:///usr/share/qt4/doc/html/appicon.html
# mac::ICON = icon/Ema.icns
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
mac:ICON = icons/ema-icon.icns

# and an uppercase first letter for Mac & Windows
mac::TARGET = PiafWorkflow
win32::TARGET = PiafWorkflow

include(../main/v4l2.pri)
include(../main/opencv.pri)


LIBS += -lexiv2
DEPENDPATH += . \
    inc \
    src \
    ui
INCLUDEPATH += . \
    inc \
    ui
OBJECTS_DIR = .obj-simple

#DEFINES += QT3_SUPPORT

TRANSLATIONS = piafworkflow_French.ts


INCLUDEPATH += tools/inc
INCLUDEPATH += ../main/tools/inc

RESOURCES += ema.qrc

################################################################################
# INCLUDES FROM PIAF OLD GUI
################################################################################

VERSION = "`date +%Y%m%d`"

CONFIG += qt \
	warn_on \
	debug \
	thread

win32:DEFINES += QT_DLL

win32:LIBS += ../../release/qwt.lib
macx: DEFINES+= __MACOSX__


LANGUAGE = C++

unix: {
	DEFINES += VERSION="`date +%Y%m%d`"
	LIBS += -L/usr/local/lib
}

linux-g++: {
	DEFINES += _LINUX
}

LEGACYPATH=../main


win32: {
	DEFINES += QT_DLL QWT_DLL
	#LIBS += ../../release/qwt.lib
}
TRANSLATIONS = piaf_French.ts

include($$LEGACYPATH/ffmpeg.pri)
include($$LEGACYPATH/opencv.pri)
include($$LEGACYPATH/openni.pri)

INCLUDEPATH += $$LEGACYPATH/inc/
INCLUDEPATH += $$LEGACYPATH/tools/inc/
INCLUDEPATH += $$LEGACYPATH/acquisitions/inc/
INCLUDEPATH += $$LEGACYPATH/acquisitions/video/inc/
INCLUDEPATH += $$LEGACYPATH/components/inc/

exists(tototatatutu) {
	DEFINES += QT3_SUPPORT

	# The following line was inserted by qt3to4
	QT += qt3support
	HEADERS += $$LEGACYPATH/inc/workshop.h \
		$$LEGACYPATH/inc/objectsexplorer.h \
		$$LEGACYPATH/inc/sw_component.h \
		$$LEGACYPATH/inc/sw_library.h \
		$$LEGACYPATH/inc/sw_structure.h \
		$$LEGACYPATH/inc/imageinfo.h \
		$$LEGACYPATH/inc/workshoplist.h \
		$$LEGACYPATH/tools/inc/workshoptool.h \
		$$LEGACYPATH/tools/inc/previewimage.h \
		$$LEGACYPATH/tools/inc/workshopvideocapture.h \
		$$LEGACYPATH/tools/inc/SwFilters.h \
		$$LEGACYPATH/tools/inc/swtoolmainwindow.h \
		$$LEGACYPATH/tools/inc/workshopimagetool.h \
		$$LEGACYPATH/tools/inc/videoplayertool.h \
	$$LEGACYPATH/components/inc/workshopcomponent.h \
	$$LEGACYPATH/components/inc/workshopmeasure.h \
	$$LEGACYPATH/components/inc/workshopimage.h \
	$$LEGACYPATH/components/inc/workshopmovie.h \

	SOURCES += \
		$$LEGACYPATH/src/objectsexplorer.cpp \
		$$LEGACYPATH/src/sw_component.cpp \
		$$LEGACYPATH/src/sw_library.cpp \
		$$LEGACYPATH/src/imageinfo.cpp \
		$$LEGACYPATH/src/workshop.cpp \
		$$LEGACYPATH/src/workshoplist.cpp \
		$$LEGACYPATH/tools/src/workshoptool.cpp \
		$$LEGACYPATH/tools/src/previewimage.cpp \
		$$LEGACYPATH/tools/src/workshopvideocapture.cpp \
		$$LEGACYPATH/tools/src/SwFilters.cpp \
		$$LEGACYPATH/tools/src/workshopimagetool.cpp \
		$$LEGACYPATH/tools/src/videoplayertool.cpp \
		$$LEGACYPATH/tools/src/swtoolmainwindow.cpp \
	$$LEGACYPATH/components/src/workshopcomponent.cpp \
	$$LEGACYPATH/components/src/workshopmeasure.cpp \
	$$LEGACYPATH/components/src/workshopimage.cpp \
	$$LEGACYPATH/components/src/workshopmovie.cpp \

	FORMS += $$LEGACYPATH/tools/ui/swtoolmainwindow.ui \
}

message("====== Checking supports for different libraries in DEFINES=$$DEFINES ======")

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

contains(DEFINES, "HAS_OPENNI")|contains(DEFINES, "HAS_OPENNI2") {
	message(" + Add support for OPENNI")
	# common functions
	HEADERS +=  $$LEGACYPATH/acquisitions/video/inc/openni_common.h
	SOURCES +=  $$LEGACYPATH/acquisitions/video/src/openni_common.cpp

	# File video source
	HEADERS +=  $$LEGACYPATH/acquisitions/video/inc/openni_file_acquisition.h
	SOURCES +=  $$LEGACYPATH/acquisitions/video/src/openni_file_acquisition.cpp

	# Live video source
	!contains(DEFINES, "HAS_OPENNI2") {
		HEADERS += $$LEGACYPATH/acquisitions/video/inc/openni_videoacquisition.h
		SOURCES += $$LEGACYPATH/acquisitions/video/src/openni_videoacquisition.cpp
	} else {
		message("No support for live in OpenNI2 for the moment");
	}
}

contains(DEFINES, "HAS_HIGHGUI") {
	message("    + Add file replay support through HighGUI")
	# File video source
	HEADERS +=  $$LEGACYPATH/acquisitions/video/inc/opencv_file_acquisition.h
	SOURCES +=  $$LEGACYPATH/acquisitions/video/src/opencv_file_acquisition.cpp
}



# $$LEGACYPATH/src/main.cpp \
SOURCES += \
	$$PIAFLIB/src/SwPluginCore.cpp \
#	$$LEGACYPATH/acquisitions/video/src/FileVideoAcquisition.cpp \
	$$LEGACYPATH/acquisitions/video/src/videocapture.cpp \
	$$LEGACYPATH/acquisitions/video/src/swvideodetector.cpp \
	$$LEGACYPATH/acquisitions/video/src/uvccolor.c \
	$$LEGACYPATH/src/imageinfo.cpp \
	$$LEGACYPATH/tools/src/OpenCVEncoder.cpp \
	tools/src/moviebookmarkform.cpp \
	$$LEGACYPATH/acquisitions/video/src/virtualdeviceacquisition.cpp \
	$$LEGACYPATH/acquisitions/video/src/opencvvideoacquisition.cpp \
	$$LEGACYPATH/src/plugineditdialog.cpp \
	$$LEGACYPATH/tools/src/vidacqsettingswindow.cpp \
	$$LEGACYPATH/tools/src/swimage_utils.cpp \


HEADERS += \
	$$PIAFLIB/inc/sw_types.h \
	$$PIAFLIB/inc/SwTypes.h \
	$$PIAFLIB/inc/SwImage.h \
	$$PIAFLIB/inc/SwPluginCore.h \
	$$PIAFLIB/inc/swopencv.h \
	$$LEGACYPATH/inc/piaf-common.h \
	$$LEGACYPATH/inc/imageinfo.h \
	$$LEGACYPATH/tools/inc/qlistboxmarker.h \
	$$LEGACYPATH/tools/inc/OpenCVEncoder.h \
	tools/inc/moviebookmarkform.h \
	$$LEGACYPATH/acquisitions/video/inc/ccvt.h \
	$$LEGACYPATH/acquisitions/video/inc/SwVideoAcquisition.h \
	$$LEGACYPATH/acquisitions/video/inc/FileVideoAcquisition.h \
	$$LEGACYPATH/acquisitions/video/inc/videocapture.h \
	$$LEGACYPATH/acquisitions/video/inc/swvideodetector.h \
	$$LEGACYPATH/acquisitions/video/inc/virtualdeviceacquisition.h \
	$$LEGACYPATH/acquisitions/video/inc/opencvvideoacquisition.h \
	$$LEGACYPATH/inc/plugineditdialog.h \
	$$LEGACYPATH/tools/inc/vidacqsettingswindow.h \
	$$LEGACYPATH/inc/piaf-settings.h \
	$$LEGACYPATH/inc/nolinux_videodev.h \
	$$LEGACYPATH/tools/inc/swimage_utils.h \
	$$LEGACYPATH/acquisitions/video/inc/file_video_acquisition_factory.h \
    ../main/acquisitions/video/inc/openni_common.h


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
# acquisitions/video/src/V4L2Device.cpp 

linux-g++: exists(/usr/lib/gcc/x86_64-linux-gnu) {
	message( "64bit CPU => use C function for YUV conversion" )
	SOURCES += $$LEGACYPATH/acquisitions/video/src/ccvt_c.c
}
else {
	exists(/usr/include/linux/linkage.h) {
		message( "32bit CPU => use MMX code for YUV conversion" )
		SOURCES += $$LEGACYPATH/acquisitions/video/src/ccvt.S
	}
	else {
		message( "no linkage.h => use ccvt_c.c" )
		SOURCES += $$LEGACYPATH/acquisitions/video/src/ccvt_c.c
	}
}


exists(/usr/local/include/libfreenect/libfreenect.h) {
	message("The system known Freenect ;)")
	DEFINES += HAS_FREENECT
	INCLUDEPATH += /usr/local/include/libfreenect
	LIBS += -L/usr/local/lib -lfreenect
	HEADERS +=  $$LEGACYPATH/acquisitions/video/inc/freenectvideoacquisition.h
	SOURCES +=  $$LEGACYPATH/acquisitions/video/src/freenectvideoacquisition.cpp
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



SOURCES += tools/src/PiafFilter.cpp \
	src/main.cpp \
	src/navimagewidget.cpp \
	src/thumbimagewidget.cpp \
	src/searchcriterionwidget.cpp \
	src/qimagedisplay.cpp \
	src/imageinfowidget.cpp \
	src/mainimagewidget.cpp \
	src/emamainwindow.cpp \
	src/thumbimageframe.cpp \
	src/exifdisplayscrollarea.cpp \
	src/metadatawidget.cpp \
	src/emaimagemanager.cpp \
	src/filtermanagerform.cpp \
	src/plugineditorform.cpp \
	../main/tools/src/timehistogramwidget.cpp \
	../main/tools/src/imgutils.cpp \
	src/maindisplaywidget.cpp \
	src/timelinewidget.cpp \
	src/pluginsettingswidget.cpp \
	tools/src/imagewidget.cpp \
	tools/src/collectioneditdialog.cpp \
	tools/src/preferencesdialog.cpp \
	tools/src/batch_progress_widget.cpp \
	src/workflowtypes.cpp \
	src/batchqueuewidget.cpp \
	tools/src/batchthread.cpp \
	tools/src/sequence_select_dialog.cpp


HEADERS += tools/inc/PiafFilter.h \
	inc/navimagewidget.h \
	inc/thumbimagewidget.h \
	inc/searchcriterionwidget.h \
	inc/qimagedisplay.h \
	inc/imageinfowidget.h \
	inc/mainimagewidget.h \
	inc/emamainwindow.h \
	inc/thumbimageframe.h \
	../main/tools/inc/imgutils.h \
	inc/exifdisplayscrollarea.h \
	inc/metadatawidget.h \
	inc/emaimagemanager.h \
	inc/filtermanagerform.h \
	inc/plugineditorform.h \
	../main/tools/inc/timehistogramwidget.h \
	inc/maindisplaywidget.h \
	inc/timelinewidget.h \
	inc/pluginsettingswidget.h \
	tools/inc/imagewidget.h \
	tools/inc/collectioneditdialog.h \
	inc/workflowtypes.h \
	tools/inc/preferencesdialog.h \
	inc/piafworkflow-settings.h \
	tools/inc/batch_progress_widget.h \
	inc/batchqueuewidget.h \
	tools/inc/batchthread.h \
	tools/inc/sequence_select_dialog.h

FORMS += ui/navimagewidget.ui \
	ui/thumbimagewidget.ui \
	ui/searchcriterionwidget.ui \
	ui/imageinfowidget.ui \
	ui/mainimagewidget.ui \
	ui/emamainwindow.ui \
	ui/thumbimageframe.ui \
	ui/exifdisplayscrollarea.ui \
	ui/metadatawidget.ui \
	ui/filtermanagerform.ui \
	ui/plugineditorform.ui \
	../main/tools/ui/timehistogramwidget.ui \
	ui/maindisplaywidget.ui \
	ui/pluginsettingswidget.ui \
	tools/ui/collectioneditdialog.ui \
	tools/ui/preferencesdialog.ui \
	tools/ui/batch_progress_widget.ui \
	ui/batchqueuewidget.ui \
	tools/ui/sequence_select_dialog.ui


unix: {
	MOC_DIR = .moc
	OBJECTS_DIR = .obj
} else {
	MOC_DIR = moc
	OBJECTS_DIR = obj
}

INCLUDEPATH += $$LEGACYPATH/ui $$LEGACYPATH/tools/ui

DEPENDPATH += $$LEGACYPATH/ui

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

FORMS += \
		tools/ui/moviebookmarkform.ui \
		$$LEGACYPATH/ui/plugineditdialog.ui \
		$$LEGACYPATH/tools/ui/imagetoavidialog.ui \
		$$LEGACYPATH/tools/ui/vidacqsettingswindow.ui

HEADERS += $$LEGACYPATH/tools/inc/imagetoavidialog.h
SOURCES += $$LEGACYPATH/tools/src/imagetoavidialog.cpp

HEADERS += $$LEGACYPATH/tools/inc/time_histogram.h
SOURCES += $$LEGACYPATH/tools/src/time_histogram.cpp





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
	$$LEGACYPATH/piaf.qrc



# # INSTALLATION
# INSTALLS += target
# FINAL CONFIGURATION ==================================================
message( "")
message( "")
message( "FINAL CONFIGURATION ================================================== ")
message( "Configuration : ")
message( " config : $$CONFIG ")
message( " defs : $$DEFINES ")
message( " libs : $$LIBS ")
message( "FINAL CONFIGURATION ================================================== ")
message( "")
message( "")
OTHER_FILES += doc/OpenCVToolsForPhoto.txt
























