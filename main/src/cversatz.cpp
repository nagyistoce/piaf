/***************************************************************************
                          cversatz.cpp  -  description
                             -------------------
    begin                : Tue Jan 7 2003
    copyright            : (C) 2003 by Christophe Seyve
    email                : christophe.seyve@sisell.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "cversatz.h"
#include <string.h>
#include <math.h>

IplImage * cvCreateImage(CvSize size, int depth, int channels)
{

	IplImage *img = 0;

    img = new IplImage;
    if(!img)
		return NULL;
	
    img->width = size.width;
    img->height = size.height;
    img->imageSize = img->width * img->height;
    img->depth = depth;

    switch(depth)
    {
	case IPL_DEPTH_8U:
        img->imageData = (char *)new unsigned char [img->imageSize];
		break;
	case IPL_DEPTH_16S:
        img->imageData = (char *)new short [img->imageSize];
		break;
	case IPL_DEPTH_32F:
        img->imageData = (char *)new float [img->imageSize];
		break;
	default:
		fprintf(stderr, "cvCreateImage: NON IMPLEMENTED IPL_DEPTH !!!\n");
		exit(0);
		break;
	}
    
	return img;
}

void cvReleaseImage( IplImage** image )
{
    if( !image ) {
		fprintf(stderr, "cvReleaseImage error : null argument.\n");
		return;
	}
    if( *image )
    {
        IplImage* img = *image;

        // delete data
		if(img->imageData)
			switch(img->depth) {
			case IPL_DEPTH_8U:
#ifdef __SWMOTIONDETECTOR_DEBUG__
				fprintf(stderr, "\tcversatz.cpp : cvReleaseImage 8U\n");
#endif
				delete [] img->imageData;
				break;
			case IPL_DEPTH_16S:
#ifdef __SWMOTIONDETECTOR_DEBUG__
				fprintf(stderr, "\tcversatz.cpp : cvReleaseImage 16S\n");
#endif
				delete [] img->imageData;
				break;
			case IPL_DEPTH_32F:
#ifdef __SWMOTIONDETECTOR_DEBUG__
				fprintf(stderr, "\tcversatz.cpp : cvReleaseImage 32F\n");
#endif
				delete [] img->imageData;
				break;
			default:
				fprintf(stderr, "cvReleaseImage: NON IMPLEMENTED IPL_DEPTH !!!\n");
				exit(0);
				break;
			}
		// delete image structure
		delete img;
        *image = 0;
    }
}
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#endif

void
cvRectangle( IplImage* img, CvPoint P1, CvPoint P2,
             double color, int thickness )
{
//	return;
	
	// draw a rectangle
	// check parameters
	int xmin = min(P1.x, P2.x);
	if(xmin<0) xmin=0;
	int xmax = max(P1.x, P2.x);
	if(xmax>=img->width) xmax=img->width-1;
	
	int ymin = min(P1.y, P2.y);
	if(ymin<0) ymin=0;
	int ymax = max(P1.y, P2.y);
	if(ymax>=img->height) ymax=img->height-1;

	// draw top and bottom borders
	
    switch(img->depth) {
	case IPL_DEPTH_8U: {
        unsigned char c = (unsigned char)color;
        memset( (unsigned char *) img->imageData + xmin + img->width * ymin,
			c, xmax-xmin+1);
        memset( (unsigned char *) img->imageData + xmin + img->width * ymax,
			c, xmax-xmin+1);
		for(int r=ymin+1; r<ymax; r++)
		{
			*((unsigned char *) img->imageData + xmin + img->width * r) = c;
			*((unsigned char *) img->imageData + xmax + img->width * r) = c;
		}
		}
		break;
	case IPL_DEPTH_16S: {
        short s = (short)color;
        for(int c=xmin; c<=xmax; c++) {
			*((short *) img->imageData + c + img->width * ymin) = s;
			*((short *) img->imageData + c + img->width * ymax) = s;
		}
		for(int r=ymin+1; r<ymax; r++)
		{
			*((short *) img->imageData + xmin + img->width * r) = s;
			*((short *) img->imageData + xmax + img->width * r) = s;
		}

		}
		break;
	default:
		fprintf(stderr, "cvCreateImage: NON IMPLEMENTED IPL_DEPTH !!!\n");
		exit(0);
		break;
	}
}


// needed bu SwMotionDetector
void cvAcc( IplImage* arr, IplImage* sumarr, const void* maskarr )
{
	for(int i=0; i<sumarr->imageSize; i++)
	{
		*((float *)sumarr->imageData + i) += (float)*((unsigned char *)arr->imageData + i);
	}
}

void cvSquareAcc( IplImage* arr, IplImage* sumarr, const void* maskarr )
{
	for(int i=0; i<sumarr->imageSize; i++)
	{
		*((float *)sumarr->imageData + i) += sqrt((float)*((unsigned char *)arr->imageData + i));
	}
}

int cvRound(float f)
{
	int ret = (int)f;
	if((f-(float)ret) > 0.5f) ret++;
	return ret;
}


