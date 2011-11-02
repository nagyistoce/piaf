/***************************************************************************
	   time_histogram.cpp  -  tracking of processing time
							 -------------------
	begin                : Thu Oct 27 2011
	copyright            : (C) 2002-2011 by Christophe Seyve
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

#include "time_histogram.h"
#include <math.h>
#include <stdio.h>
#include <string.h>


void allocHistogram(t_time_histogram * pTimeHistogram, float maxproctime_us)
{
	pTimeHistogram->max_histogram = 256;
	pTimeHistogram->histogram = new unsigned int [ pTimeHistogram->max_histogram ];
	memset(pTimeHistogram->histogram, 0, sizeof(unsigned int)*pTimeHistogram->max_histogram);

	// Scale : max = 2*first proc time
	pTimeHistogram->time_scale = pTimeHistogram->max_histogram / maxproctime_us;
	fprintf(stderr, "[time histo] %s:%d: max proc time at init : %g us "
			"=> max=%d => scale=%g item/us\n",
			__func__, __LINE__, maxproctime_us,
			pTimeHistogram->max_histogram,
			pTimeHistogram->time_scale
			);
}
/* delete a time histogram */
void deleteHistogram(t_time_histogram * pTimeHistogram)
{
	if(pTimeHistogram->histogram)
	{
		delete [] pTimeHistogram->histogram;
		pTimeHistogram->histogram = NULL;
	}
	memset(pTimeHistogram, 0, sizeof(t_time_histogram) );
}

void appendTimeUS(t_time_histogram * pTimeHistogram, float proctime_us)
{
	// don't count first iteration
	pTimeHistogram->nb_iter++;
	if(pTimeHistogram->nb_iter<=0) { return; }

	if(!pTimeHistogram->histogram) {
		fprintf(stderr, "[time histo] %s:%d: realloc with twice the max (%g us), eg %g us",
				__func__, __LINE__, proctime_us, 2.f * proctime_us);
		// use twice the proctime as max
		allocHistogram(pTimeHistogram, 2.f * proctime_us);
	}

	int index = (int)roundf(pTimeHistogram->time_scale * proctime_us);

	if(index > pTimeHistogram->max_histogram) {
		pTimeHistogram->overflow_count ++;
		pTimeHistogram->overflow_cumul_us += proctime_us;
	} else {
		pTimeHistogram->histogram[index]++;
	}

	if(pTimeHistogram->overflow_count > 1
	   && pTimeHistogram->overflow_count > pTimeHistogram->nb_iter / 5)
	{	// too many overflows, recompute
		float overflow_mean = pTimeHistogram->overflow_cumul_us / (double)pTimeHistogram->overflow_count;

		unsigned int * oldhistogram = pTimeHistogram->histogram;
		if(oldhistogram) {
			float old_scale = pTimeHistogram->time_scale;

			// realloc histogram, then convert old stats
			allocHistogram(pTimeHistogram, 2.f * overflow_mean);
			for(int h = 0; h< pTimeHistogram->max_histogram; h++)
			{
				// convert to new scale
				float old_us = (float)h / old_scale;
				int newh = old_us * pTimeHistogram->time_scale;
				pTimeHistogram->histogram[newh] = oldhistogram[h];
			}

			// delete old
			delete [] oldhistogram;
		}
		pTimeHistogram->overflow_count = 0;
		pTimeHistogram->overflow_cumul_us = 0.;
	}

}
