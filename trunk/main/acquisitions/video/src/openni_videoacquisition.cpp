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

#define OPENNI_VIDEOACQUISITION_CPP

#include <stdio.h>

#include "piaf-common.h"
#include "openni_videoacquisition.h"


#define OPENNI_PRINTF(...) { \
	fprintf(stderr, "[OpenNI %p %d]::%s:%d : ", this, m_idx_device, __func__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); \
}

OpenNIVideoAcquisition::OpenNIVideoAcquisition(int idx_device)
	: OpenNICommonAcquisition(NULL)
{
	m_idx_device = idx_device;
}

OpenNIVideoAcquisition::OpenNIVideoAcquisition()
	: OpenNICommonAcquisition(NULL)
{
	m_idx_device = 0;
}

OpenNIVideoAcquisition::~OpenNIVideoAcquisition()
{
	fprintf(stderr, "OpenNIVideoAcquisition::%s:%d : stop video acquisition device",
					__func__, __LINE__);

	stopAcquisition();
}


