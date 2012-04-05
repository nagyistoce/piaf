/***************************************************************************
	swimage_utils.cpp  -  Utilities for converting from/to SwImageStruct:
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

#include "swopencv.h"
#include "swimage_utils.h"

/* Create and allocate buffer from SwImageStruct => .buffer must be deleted !*/
swImageStruct * createSwImageFromImage(swImageStruct * swimIn)
{
	if(!swimIn) { return NULL; }
	// Create
	swImageStruct * swim = new swImageStruct;
	memcpy(swim, swimIn, sizeof(swImageStruct));
	if(swim->bytedepth == 0)
	{
		fprintf(stderr, "[%s] %s:%d : error in bytedepth from "
				"SwImage=%dx%d x depth=%d x bytedepth=%d\n",
				__FILE__, __func__, __LINE__,
				swimIn->width, swimIn->height, swimIn->depth, swimIn->bytedepth
				);
		swim->bytedepth = 1;
	}
	swim->allocated = 1;
	swim->buffer = new u8 [ swim->buffer_size ];
	memcpy(swim->buffer, swimIn->buffer, swim->buffer_size);

	return swim;
}

/* Create image from IplImage, then map buffer to swimIn->buffer => .buffer does not need to be deleted !*/
swImageStruct * createSwImageHeaderFromImage(swImageStruct * swimIn)
{
	if(!swimIn) { return NULL; }
	// Create
	swImageStruct * swim = new swImageStruct;
	memcpy(swim, swimIn, sizeof(swImageStruct));

	if(swim->bytedepth == 0)
	{
		fprintf(stderr, "[%s] %s:%d : error in bytedepth from "
				"SwImage=%dx%d x depth=%d x bytedepth=%d\n",
				__FILE__, __func__, __LINE__,
				swimIn->width, swimIn->height, swimIn->depth, swimIn->bytedepth
				);
		swim->bytedepth = 1;
	}
	swim->allocated = 0;

	return swim;
}



/* Create and allocate buffer from IplImage, then copy buffer => .buffer must be deleted !*/
swImageStruct * createSwImageFromIplImage(IplImage * iplImage)
{
	if(!iplImage) { return NULL; }
	// Create
	swImageStruct * swim = new swImageStruct;
	memset(swim, 0, sizeof(swImageStruct));
	swim->width = iplImage->width;
	swim->height = iplImage->height;
	swim->depth = iplImage->nChannels;
	swim->bytedepth = iplImage->depth/8;
	swim->pitch = iplImage->widthStep;
	if(swim->bytedepth == 0)
	{
		fprintf(stderr, "[%s] %s:%d : error in bytedepth from IplImage=%dx%d x nChannels=%d x depth=%d\n",
				__FILE__, __func__, __LINE__,
				iplImage->width, iplImage->height, iplImage->nChannels, iplImage->depth
				);
		swim->bytedepth = 1;
	}
	mapIplImageToSwImage(iplImage, swim);

	swim->allocated = 1;
	swim->buffer_size = iplImage->widthStep * iplImage->height;
	swim->buffer = new u8 [ swim->buffer_size ];
	memcpy(swim->buffer, iplImage->imageData, swim->buffer_size);

	return swim;
}

/** @brief Create image from IplImage, then map buffer to IplImage->imageData => .buffer does not need to be deleted !*/
swImageStruct * createSwImageHeaderFromIplImage(IplImage * iplImage)
{
	if(!iplImage) { return NULL; }
	// Create
	swImageStruct * swim = new swImageStruct;
	memset(swim, 0, sizeof(swImageStruct));

	mapIplImageToSwImage(iplImage, swim);
	return swim;
}

void mapIplImageToSwImage(IplImage * iplImage, swImageStruct * swim)
{	
	swim->width = iplImage->width;
	swim->height = iplImage->height;
	swim->depth = iplImage->nChannels;
	swim->bytedepth = iplImage->depth/8;
	swim->pitch = iplImage->widthStep;

	switch(iplImage->depth)
	{
	default:
		swim->pixelType = 0;
		break;
	case IPL_DEPTH_8U:
		swim->pixelType = swU8;
		break;
	case IPL_DEPTH_8S:
		swim->pixelType = swS8;
		break;
	case IPL_DEPTH_16U:
		swim->pixelType = swU16;
		break;
	case IPL_DEPTH_16S:
		swim->pixelType = swS16;
		break;
	case IPL_DEPTH_32S:
		swim->pixelType = swS32;
		break;

	case IPL_DEPTH_32F:
		swim->pixelType = swFloat;
		break;

	}

	if(swim->bytedepth == 0)
	{
		fprintf(stderr, "[%s] %s:%d : error in bytedepth from IplImage=%dx%d x nChannels=%d x depth=%d\n",
				__FILE__, __func__, __LINE__,
				iplImage->width, iplImage->height, iplImage->nChannels, iplImage->depth
				);
		swim->bytedepth = 1;
	}
	swim->allocated = 0;
	swim->buffer_size = iplImage->widthStep * iplImage->height;
	swim->buffer = (u8 *)iplImage->imageData;
}

/** @brief Free swImageStruct allocated */
void freeSwImage(swImageStruct ** pswim)
{
	if(!pswim) { return; }
	swImageStruct  * swim = *pswim;
	if(!swim) { return; }

	if(swim->allocated)
	{
		delete [] swim->buffer;
		swim->buffer = 0;
	}
	// Clear structure
	memset(swim, 0, sizeof(swImageStruct));
	*pswim = NULL;
}


/** @brief Convert an image struct to IplIMage, resize/alloc iplImage if needed */
IplImage * convertSwImageToIplImage(swImageStruct * swim, IplImage ** pimg)
{
	if(!pimg)
	{
		fprintf(stderr, "[%s] %s:%d : error in IplImage pointer\n",
				__FILE__, __func__, __LINE__
				);
		return NULL;
	}

	IplImage * img = *pimg;
	if(img && (img->widthStep != swim->pitch || img->height != swim->height))
	{
		fprintf(stderr, "[%s] %s:%d : size does not match => "
				"release IplImage=%dx%d x nChannels=%d x depth=%d\n",
				__FILE__, __func__, __LINE__,
				img->width, img->height, img->nChannels, img->depth
				);
		cvReleaseImage(pimg);
		*pimg = img = NULL;
	}

	if(!img)
	{
		fprintf(stderr, "\n\n[%s] %s:%d : create IplImage=%dx%d x nChannels=%d x depth=%d\n",
				__FILE__, __func__, __LINE__,
				swim->width, swim->height, swim->depth, swim->bytedepth*8
				); fflush(stderr);

		img = cvCreateImage(cvSize(swim->width, swim->height),
									swim->bytedepth * 8, swim->depth);
		*pimg = img;
	}

	// Copy buffer
	if(swim->pitch == img->widthStep)
	{
		fprintf(stderr, "\n\n[%s] %s:%d : memcpy IplImage=%dx%d x nChannels=%d x depth=%d\n",
				__FILE__, __func__, __LINE__,
				swim->width, swim->height, swim->depth, swim->bytedepth*8
				); fflush(stderr);
		memcpy(img->imageData, swim->buffer, swim->buffer_size);
	}
	else // do it line by line
	{
		fprintf(stderr, "\n\n[%s] %s:%d : memcpy line by line IplImage=%dx%d x nChannels=%d x depth=%d\n",
				__FILE__, __func__, __LINE__,
				swim->width, swim->height, swim->depth, swim->bytedepth*8
				); fflush(stderr);
		for(int r = 0; r<swim->height; r++)
		{
			memcpy(img->imageData + r*img->widthStep,
				   swim->buffer + r * swim->pitch,
				   swim->width * swim->bytedepth * swim->depth);
		}
	}

	return img;
}

