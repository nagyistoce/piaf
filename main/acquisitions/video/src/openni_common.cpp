/***************************************************************************
	 openni_common.cpp  - Open NI common functions (live+file replay)
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


#define OPENNI_COMMON_CPP
#include "openni_common.h"
#include <math.h>
#include "piaf-common.h"

void init_OpenNI_depth_LUT()
{
	if(g_depth_LUT[0]>=0) { return; }
	/* From OpenKinect :
	   http://openkinect.org/w/index.php?title=Imaging_Information&oldid=613
	distance = 0.1236 * tan(rawDisparity / 2842.5 + 1.1863)
	   */
	// Raw in meter (float)
	for(int raw = 0; raw<OPENNI_RAW_MAX; raw++) {
	//	g_depth_LUT[raw] = (float)( 0.1236 * tan((double)raw/2842.5 + 1.1863) );
		g_depth_LUT[raw] = (float)raw / 1000.f ;// openNI returns depth in mm
	}

	g_depth_LUT[OPENNI_RAW_UNKNOWN] = -1.f;

	/* LUT for 8bit info on depth :
	*/
	int l_depth_8bit2m_LUT_count[256];
	memset(l_depth_8bit2m_LUT_count, 0, sizeof(int)*256);

	for(int raw = 0; raw<OPENNI_RAW_MAX; raw++) {
		int val = (int)round( 255. *
							  (1. - (g_depth_LUT[raw] - OPENNI_DEPTH2CM_RANGE_MIN)
								 / (OPENNI_DEPTH2CM_RANGE_MAX - OPENNI_DEPTH2CM_RANGE_MIN) )
							  );
		if(val > 255) val = 255;
		else if(val < 1) val = 1;// keep 0 for unknown

		//l_depth_8bit2m_LUT_count[val]++;
		g_depth_8bit2m_LUT[val] = g_depth_LUT[raw];

		g_depth_grayLUT[raw] = (u8)val;
	}
	g_depth_LUT[OPENNI_RAW_UNKNOWN] = -1.f;
	g_depth_grayLUT[OPENNI_RAW_UNKNOWN] = 0;

	FILE * fLUT = fopen(TMP_DIRECTORY "OpenNI-LUT_8bit_to_meters.txt", "w");
	if(fLUT)
	{
		fprintf(fLUT, "# Recorded with Piaf with 8bit depth from %g m (gray=255) to %g meters (gray=1), unknonw depth is gray=0\n",
				(float)OPENNI_DEPTH2CM_RANGE_MIN, (float)OPENNI_DEPTH2CM_RANGE_MAX);
		fprintf(fLUT, "#  Project homepage & Source code: http://code.google.com/p/piaf/\n#\n");

		fprintf(fLUT, "# 8bit_value\tDistance in meter\tResolution\n");
	}
	for(int val = 0; val<256; val++)
	{
		if(l_depth_8bit2m_LUT_count[val] > 1)
		{
			g_depth_8bit2m_LUT[val] /= l_depth_8bit2m_LUT_count[val];
		}
		if(fLUT)
		{
			fprintf(fLUT, "%d\t%g\t%g\n",
					val, g_depth_8bit2m_LUT[val],
					val > 0 ?  g_depth_8bit2m_LUT[val-1]- g_depth_8bit2m_LUT[val]: 0);
		}
	}
	if(fLUT)
	{
		fclose(fLUT);
	}

	g_depth_grayLUT[OPENNI_RAW_UNKNOWN] = 0;
}

