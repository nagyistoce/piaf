/***************************************************************************
	   time_histogram.h  -  tracking of processing time
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

#ifndef TIME_HISTOGRAM_H
#define TIME_HISTOGRAM_H

/** @brief Processing time statistics
*/
typedef struct {
	int nb_iter;
	unsigned int * histogram;
	int max_histogram;	///< nb of items in max_histogram array
	float time_scale;	///< time scale : item us to index in array
	int index_max;		///< index of histogram max
	unsigned int value_max;
	int overflow_count; ///< nb of overflow count
	double overflow_cumul_us; ///< Cumul of overflow times
} t_time_histogram;

/** @brief Allocate a time histogram and reset (fill it with 0) */
void allocHistogram(t_time_histogram * pTimeHistogram, float maxproctime_us);

/** @brief Allocate a processing time in histogram */
void appendTimeUS(t_time_histogram * pTimeHistogram, float proctime_us);


#endif // TIME_HISTOGRAM_H
