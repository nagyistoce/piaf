/***************************************************************************
	 openni_videoacquisition.h  - Open NI acquisition class for video capture
									 For Microsoft Kinect and Asus Xtion Pro

	Tested with OpenNI version is 1.3.3.6

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

#ifndef OPENNI_VIDEOACQUISITION_H
#define OPENNI_VIDEOACQUISITION_H
#include "virtualdeviceacquisition.h"

#include "openni_common.h"

#include <QMutex>
#include <QWaitCondition>

#ifdef HAS_OPENNI2
bool fileExists(const char *fn);
#else
XnBool fileExists(const char *fn);
#endif

/** @brief OpenNI based video acquisition device for PrimeSense based 3D cameras

  This class supports:
  - PrimeSense reference device: NOT TESTED (since we cannot buy one)
  - Asus Xtion Pro and Xtion Pro Live
  - Microsoft Kinect
  */
class OpenNIVideoAcquisition : public OpenNICommonAcquisition
{
public:
	/** @brief Default constructor with index of OpenNI device */
	OpenNIVideoAcquisition(int idx_device = 0);
	OpenNIVideoAcquisition();
	~OpenNIVideoAcquisition();
// EVERYTHING INHERITED, just constructor with null

};

#endif // OPENNI_VIDEOACQUISITION_H
