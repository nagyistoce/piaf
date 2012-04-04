/***************************************************************************
	swimage_utils.h  -  Utilities for converting from/to SwImageStruct:
						QImage, IplImage ...
							 -------------------
	begin                :
	copyright            : (C) 2002 - 2012 Christophe Seyve (CSE)
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

#ifndef SWIMAGE_UTILS_H
#define SWIMAGE_UTILS_H

#include "swopencv.h"
#include "SwTypes.h"

#include "SwImage.h"

/** @brief Create and allocate buffer from SwImageStruct => .buffer must be deleted !*/
swImageStruct * createSwImageFormImage(swImageStruct * swimIn);
/** @brief Create image from IplImage, then map buffer to swimIn->buffer => .buffer does not need to be deleted !*/
swImageStruct * createSwImageHeaderFormImage(swImageStruct * swimIn);

/** @brief Map swimim->buffer to iplImage->imageData */
void mapIplImageToSwImage(IplImage * iplImage, swImageStruct * swim);

/** @brief Create and allocate buffer from IplImage, then copy buffer => .buffer must be deleted !*/
swImageStruct * createSwImageFromIplImage(IplImage * image);

/** @brief Create image from IplImage, then map buffer to IplImage->imageData => .buffer does not need to be deleted !*/
swImageStruct * createSwImageHeaderFromIplImage(IplImage * image);

/** @brief Free swImageStruct allocated */
void freeSwImage(swImageStruct ** swim);

#endif // SWIMAGE_UTILS_H
