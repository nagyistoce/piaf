/***************************************************************************
	 openni_videoacquisition.cpp  - Open NI
									 acquisition class for video capture
									 For Microsoft Kinect and Asus Xtion Pro
							 -------------------
	begin                : Thu Jan 19 2012
	copyright            : (C) 2012 by Christophe Seyve (CSE)
	email                : cseyve@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "piaf-common.h"
#include "openni_file_acquisition.h"

#include <QDir>
#include <QApplication>


#define OPENNI_PRINTF(...) { \
	fprintf(stderr, "[OpenNIFile %p '%s'']::%s:%d : ", \
		this, mVideoFilePath.c_str() , __func__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); \
}

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------

#define EXTENSION_OPENNI	"oni"

std::string OpenNIFileAcquisition::mRegistered =
	FileVideoAcquisitionFactory::RegisterCreatorFunction(
		EXTENSION_OPENNI, OpenNIFileAcquisition::creatorFunction);


FileVideoAcquisition* OpenNIFileAcquisition::creatorFunction(std::string path)
{
	PIAF_MSG(SWLOG_INFO, "Create OpenNIFileAcquisition(path='%s')", path.c_str() );
	return (FileVideoAcquisition *)new OpenNIFileAcquisition( path.c_str() );
}

#define SAMPLE_XML_PATH "/etc/openni/SamplesConfig.xml"
#define SAMPLE_XML_PATH_LOCAL "SamplesConfig.xml"

OpenNIFileAcquisition::OpenNIFileAcquisition(const char * filename)
	: OpenNICommonAcquisition(filename)
{
	OPENNI_PRINTF("Opening ONI file '%s'...",
				  filename);

}

OpenNIFileAcquisition::~OpenNIFileAcquisition()
{
	stopAcquisition();
}






