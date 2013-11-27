/***************************************************************************
	 openni_file_acquisition.h  - Open NI acquisition class for capture
									from file for Microsoft Kinect and Asus Xtion Pro

	Tested with OpenNI version is 1.3.3.6

							 -------------------
	begin                : Mon Apr 02 2012
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

#ifndef OPENNI_FILE_ACQUISITION_H
#define OPENNI_FILE_ACQUISITION_H

#include "openni_common.h"

#include "virtualdeviceacquisition.h"
#include "file_video_acquisition_factory.h"

// for inheritance
#include "openni_common.h"

#include <QMutex>
#include <QWaitCondition>

/** @brief OpenNI based video acquisition device for PrimeSense based 3D cameras

  This class supports:
  - PrimeSense reference device: NOT TESTED (since we cannot buy one)
  - Asus Xtion Pro and Xtion Pro Live
  - Microsoft Kinect
  */
class OpenNIFileAcquisition : public OpenNICommonAcquisition
{
public:
	/// creator function registered in factory
	static FileVideoAcquisition* creatorFunction(std::string path);

	/** @brief Default constructor with path of OpenNI file */
	OpenNIFileAcquisition(const char * filenameONI);
	~OpenNIFileAcquisition();


protected:
	/// value used for registering in factory
	static std::string mRegistered;
	// EVERYTHING ELSE IS INHERITED!!
};

#endif // OPENNI_FILE_ACQUISITION_H
