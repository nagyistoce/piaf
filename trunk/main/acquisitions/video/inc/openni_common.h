/***************************************************************************
	 openni_common.h  - Open NI common functions (live+file replay)
									from file for Microsoft Kinect and Asus Xtion Pro

	Tested with OpenNI version is 1.5.3 and 2.1

							 -------------------
	begin                : Thu May 23 2013
	copyright            : (C) 2013 by Christophe Seyve (CSE)
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

#ifndef OPENNI_COMMON_H
#define OPENNI_COMMON_H

#include <stdio.h>
#include "sw_types.h"

#ifndef OPENNI_COMMON_CPP
#define OPENNI_VID_EXTERN	extern
#else
#define OPENNI_VID_EXTERN
#endif

// Includes for v1
#ifdef HAS_OPENNI
#include <XnOpenNI.h>
#include <XnLog.h>
#include <XnCppWrapper.h>
#include <XnFPSCalculator.h>

using namespace xn;
#endif // HAS_OPENNI




#ifdef HAS_OPENNI2
#include <OniCAPI.h>
#include <OniCEnums.h>
#include <OniCProperties.h>
#include <OniCTypes.h>
#include <OniEnums.h>
#include <OniPlatform.h>
#include <OniProperties.h>
#include <OniVersion.h>
#include <OpenNI.h>

using namespace openni;

#endif // HAS_OPENNI

// IMAGE MODES
/** @brief Mode for 8bit image of depth with 1 value = 2 cm (1 channel x IPL_DEPTH_8U)
For close distance = 0.6m, the pixels are white (255) and at 5.6 m, they are black.
Unknown distance is black too.
*/
#define OPENNI_MODE_DEPTH_2CM		0

#define OPENNI_DEPTH2CM_RANGE_MIN	0.6
#define OPENNI_DEPTH2CM_RANGE_MAX	7

/** @brief Mode for float (32F) image of depth in meter (1 channel x IPL_DEPTH_32F)
The range is 0.6m to infinite. -1.f means the depth is unknown.
For practical use, do not use depth >7m
*/
#define OPENNI_MODE_DEPTH_M		1

/** \brief Mode for float (32F) image of 3D x,y,z position in meter (3 channels x IPL_DEPTH_32F)
x,y,z is in the camera reference. X on image columns, Y on rows, and Z is depth
x,y : position in 3D in meter
z: depth in meter. The range is 0.6m to infinite. -1.f means the depth is unknown.
For practical use, do not use depth >7m
*/
#define OPENNI_MODE_3D			2

/** \brief Mode for false colors (depth as RGB)
black when unknown
in HSV, H=0 (Red for 0.6m),
Blue for
*/
#define OPENNI_MODE_DEPTH_COLORS	3

/** @brief Max value of raw depth image (11 bits) */
#define OPENNI_RAW_MAX	10000

/** @brief Value for unknown depth in raw depth image (11 bits) */
#define OPENNI_RAW_UNKNOWN 0


#define OPENNI_FRAME_W		640
#define OPENNI_FRAME_H		480
#define OPENNI_FPS			30


OPENNI_VID_EXTERN float g_depth_LUT[OPENNI_RAW_MAX]
	#ifdef OPENNI_COMMON_CPP
	= { -1.f }
  #endif
  ;

OPENNI_VID_EXTERN u8 g_depth_grayLUT[OPENNI_RAW_MAX];
OPENNI_VID_EXTERN float g_depth_8bit2m_LUT[256];

void init_OpenNI_depth_LUT();

#endif // OPENNI_COMMON_H
