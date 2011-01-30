/***************************************************************************
        swvideodetector.h  -  video processing utilities
                             -------------------
    begin                : Fri Jun 7 2002
    copyright            : (C) 2002 by Olivier Viné (OLV) & Christophe Seyve (CSE)
    email                : olivier.vine@sisell.com & cseyve@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _SW_VIDEO_DEFS_

#define _SW_VIDEO_DEFS_



#include <sys/time.h>

#include "video_asm.h"

#ifndef __WITHOUT_OPENCV__

#ifndef OPENCV_22
#include <cv.h>
#include <cvaux.h>
#include <highgui.h>
#else
#include <opencv.hpp>
#include <legacy/compat.hpp>
#endif

#else
#include "cversatz.h"
#endif
#include "sw_types.h"

/// For image storage
typedef struct _Img {
	unsigned char 	*pixmap;
	unsigned short 	width;
	unsigned short 	height;
	unsigned long 	size;
	unsigned char 	depth;
} Img;

#ifndef max
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#endif

/// Gets size from PNM file (PPM/PGM)
CvSize LoadPPMHeader(char *name);
/// Loads a PNM image from file to memory (must be allocated !). Please use LoadPPMHeader before.
int LoadPPMFile(char *name, unsigned char * Image, int maxsize);
/// Saves a PNM file from a memory buffer (Grayscaled or RGB24 coded)
void SavePPMFile(char *filename, bool colored, CvSize size, unsigned char *buffer);


#define MARKER_COLOR 255

#define IPLLINE_8U(_img, _r)	(u8 *)((_img)->imageData + (_r)*(_img)->widthStep)
#define IPLLINE_16U(_img, _r)	(u16 *)((_img)->imageData + (_r)*(_img)->widthStep)
#define IPLLINE_16S(_img, _r)	(i16 *)((_img)->imageData + (_r)*(_img)->widthStep)
#define IPLLINE_32U(_img, _r)	(u32 *)((_img)->imageData + (_r)*(_img)->widthStep)
#define IPLLINE_32S(_img, _r)	(i32 *)((_img)->imageData + (_r)*(_img)->widthStep)
#define IPLLINE_32F(_img, _r)	(float *)((_img)->imageData + (_r)*(_img)->widthStep)


/** @brief Print image properties */
void swPrintProperties(IplImage * img);

/** @brief Return image layer depth in bytes (e.g. 8bit jpeg is 1 byte, 16bit:2bytes...) */
int swByteDepth(IplImage * iplImage);

/** @brief Create an IplImage width OpenCV's cvCreateImage and clear buffer */
IplImage * swCreateImage(CvSize size, int depth, int channels);
/** @brief Create an IplImage header width OpenCV's cvCreateImageHeader */
IplImage * swCreateImageHeader(CvSize size, int depth, int channels);

/** @brief Release an image and clear pointer */
void swReleaseImage(IplImage ** img);
/** @brief Release an image header and clear pointer */
void swReleaseImageHeader(IplImage ** img);

/** @brief Convert an image into another */
int swConvert(IplImage *imageIn, IplImage * imageOut);

#endif
