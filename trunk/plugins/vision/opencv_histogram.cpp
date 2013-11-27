/***************************************************************************
                          opencv_histogram.cpp  -  description
                             -------------------
    begin                : Wed Dec 18 13:18:39 CET 2002
    copyright            : (C) 2002 by Christophe Seyve - SISELL
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


#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// include component header
#include "SwPluginCore.h"

// OpenCV
#ifndef OPENCV_22
#include <cv.h>
#include <cvaux.h>
#include <highgui.h>
#else
#include <opencv.hpp>
#include <legacy/compat.hpp>
#endif

#include <math.h>

/********************** GLOBAL SECTION ************************
DO NOT MODIFY THIS SECTION
plugin.data_in  : input data. You should cast this pointer to read the
   data with the correct type
plugin.data_out : output data. You should cast this pointer to write the
   data with the correct type
***************************************************************/
SwPluginCore plugin;

/******************* END OF GLOBAL SECTION ********************/

/********************** USER SECTION ************************
YOU SHOULD MODIFY THIS SECTION
Declare your variables, function parameters and function list


***************************************************************/
#define CATEGORY	"Vision"
#define SUBCATEGORY	"Histogram"


// function parameters
unsigned char histo_min = 127;
unsigned char histo_max = 255;

swFuncParams histo_params[] = {
	{"min", swU8, (void *)&histo_min},
	{"max", swU8, (void *)&histo_max},
};

float height_scale = 0.4f;
swFuncParams view_histo_params[] = {
	{"Height scale", swFloat, (void *)&height_scale}
};


void stretch_histo();
void logstretch_histo();
void view_histo();
void view_histo_log();


/* swFunctionDescriptor :
 char * : function name
 int : number of parameters
 swFuncParams : function parameters
 swType : input type
 swType : output type
 void * : procedure
 */
swFunctionDescriptor functions[] = {
	{"View hist",		1,	view_histo_params, swImage, swImage, &view_histo, NULL},
	{"View log hist",	1, 	view_histo_params, swImage, swImage, &view_histo_log, NULL},
	{"Stretch hist",	0,	NULL, swImage, swImage, &stretch_histo, NULL},
	{"Log Stretch hist", 	0,	NULL, swImage, swImage, &logstretch_histo, NULL}
};
int nb_functions = 4;



/*************** PLUGIN FUNCTIONS IMPLEMENTATION *****************/
IplImage * cvIm1 = NULL;
IplImage * cvIm2 = NULL;
IplImage * planes1[4] = {NULL, NULL, NULL, NULL};
IplImage * planes2[4] = {NULL, NULL, NULL, NULL};



void allocateImages()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);

	CvSize size;
	size.width = imIn->width;
	size.height = imIn->height;

	if(!cvIm1) {
		cvIm1 = cvCreateImageHeader(size,  IPL_DEPTH_8U, imIn->depth);
		if(imIn->depth > 1) {
			for(int i=0;i<imIn->depth; i++)
				planes1[i] = cvCreateImage(size,  IPL_DEPTH_8U, 1);
			for(int i=imIn->depth; i<4;i++)
				planes1[i] = NULL;
		} else {
			planes1[0] = cvIm1;
		}
	}
	if(!cvIm2) {
		cvIm2 = cvCreateImageHeader(size,  IPL_DEPTH_8U, imIn->depth);
		if(imIn->depth > 1) {
			for(int i=0;i<imIn->depth; i++)
				planes2[i] = cvCreateImage(size,  IPL_DEPTH_8U, 1);
			for(int i=imIn->depth; i<4;i++)
				planes2[i] = NULL;
		}
		else {
			planes2[0] = cvIm2;
		}
	}

	cvIm1->imageData = (char *)imIn->buffer;
	cvIm2->imageData = (char *)imOut->buffer;
}



void draw_histo(bool mode_log) {

	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	allocateImages();


	// decompose planes
	if(imIn->depth > 1) {
		cvCvtPixToPlane(cvIm1, planes1[0], planes1[1], planes1[2], planes1[3]);
		cvCvtPixToPlane(cvIm1, planes2[0], planes2[1], planes2[2], planes2[3]);
	}
	cvCopy(cvIm1, cvIm2);

	CvHistogram * cvHistTab[4];
	float max_value = 0;
	for(int p = 0; p<imIn->depth; p++)
	{
		// calculate histogram
		int dims[] = { 256 };

		CvHistogram * cvHist = cvHistTab[p] = cvCreateHist(1, dims, CV_HIST_ARRAY, NULL, 1 );
		if(imIn->depth > 1) 
			cvCalcHist( &planes1[p], cvHist, 0, NULL );
		else
			cvCalcHist( &cvIm1, cvHist, 0, NULL );

		float plane_max = 0.f;
		cvGetMinMaxHistValue( cvHist, 0, &plane_max, 0, 0 );
		if(plane_max > max_value) {
			max_value = plane_max;
		}
	}


	//	float factor = (float)imIn->height/log(max_value);
#define HISTO_HEIGHT	200
	int histoheight = (imIn->height * height_scale);
	//int histoheight = (imIn->height>HISTO_HEIGHT ? HISTO_HEIGHT : imIn->height);
	int roffset = imIn->height - histoheight;
	float factor = 0;
	if(max_value > 0) {
		if(mode_log) {
			factor = (float)histoheight / log(max_value);
		} else {
			factor = (float)histoheight / (max_value);
		}
	}
#define IPLLINE_8U(_img,_r)	(u8 *)((_img)->imageData + (_r)*(_img)->widthStep)

	double cscale = 1.;
	int coffset = imIn->width-1 - 256;
	if(coffset < 0) {
		coffset = 0;
		cscale = (double)(imIn->width-1) / 256.;
	}
	
	for(int p = 0; p<imIn->depth; p++)
	{
		int pix;
		CvHistogram * cvHist = cvHistTab[p];
		
		cvLine(planes1[p], 
			   cvPoint(coffset+128.*cscale, imIn->height - histoheight),
			   cvPoint(coffset+128.*cscale, imIn->height-1),
			   cvScalarAll(255), 1);
		cvLine(planes1[p], 
			   cvPoint(coffset+64.*cscale, imIn->height - histoheight),
			   cvPoint(coffset+64.*cscale, imIn->height-1),
			   cvScalarAll(192), 1);
		cvLine(planes1[p], 
			   cvPoint(coffset+192.*cscale, imIn->height - histoheight),
			   cvPoint(coffset+192.*cscale, imIn->height-1),
			   cvScalarAll(192), 1);

		// memset(planes2[p]->imageData, 0, imIn->width * imIn->height);
		for( pix = 0; pix < 256; pix++ )
		{
			float bin_val = cvQueryHistValue_1D( cvHist, pix );
			

			int c;
			if(imIn->width >= 256) {
				c = coffset + pix;
			} else {
				c = coffset + (int)roundf(cscale * (double)pix);
			}
			
			if(bin_val > 0) {
				// compute c
				int rhist = ( mode_log ?
								 imIn->height-1 - (int)cvRound( log(bin_val) * factor) :
								 imIn->height-1 - (int)cvRound( bin_val * factor )
								 );

				u8 * plane1line, * plane2line;
				
				// Darken pixels above
				int r;
				if(rhist >= 0 && rhist<imIn->height) {
					for(r =roffset ; r< rhist; r++) {
						plane1line = IPLLINE_8U(planes1[p], r);
						plane2line = IPLLINE_8U(planes2[p], r);
						plane2line[c] = (((plane1line[c] & 0xFE)>>1));
					}

					plane2line = IPLLINE_8U(planes2[p], rhist);
					plane2line[c] = 255;
					
					for(r = rhist+1 ; r< imIn->height; r++) {
						plane1line = IPLLINE_8U(planes1[p], r);
						plane2line = IPLLINE_8U(planes2[p], r);
						plane2line[c] = (0x80 | ((plane1line[c] & 0xFE)>>1));
					}
				}
			} else {
				for(int r = roffset ; r<imIn->height; r++) {
					u8 * plane1line = IPLLINE_8U(planes1[p], r);
					u8 * plane2line = IPLLINE_8U(planes2[p], r);
					plane2line[c] = (((plane1line[c] & 0xFE)>>1));
				}
			}
		}
		
		// Mark borders
		
		cvRectangle(planes2[p], cvPoint(coffset, imIn->height - histoheight),
					cvPoint(coffset+256.*cscale, imIn->height-1),
					cvScalarAll(32), 1);

		cvReleaseHist(&cvHistTab[p]);
	}

	if(imIn->depth > 1) {
		cvCvtPlaneToPix(planes2[0], planes2[1], planes2[2], planes2[3], cvIm2);
	}
	memcpy(imageOut, cvIm2->imageData, imIn->buffer_size);
}

void view_histo_log()
{
	draw_histo(true);
}

void view_histo()
{
	draw_histo(false);
}

void stretch_histo()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;
	CvSize size;
	size.width = imIn->width;
	size.height = imIn->height;

	allocateImages();
	cvCopy(cvIm1, cvIm2);

	// decompose planes
	if(imIn->depth > 1) {
		cvCvtPixToPlane(cvIm1, planes1[0], planes1[1], planes1[2], planes1[3]);
		cvCvtPixToPlane(cvIm1, planes2[0], planes2[1], planes2[2], planes2[3]);
	}

	for(int p = 0; p<imIn->depth; p++)
	{
		// calculate histogram
        int dims[] = { 256 };

		CvHistogram * cvHist = cvCreateHist(1, dims, CV_HIST_ARRAY, NULL, 1 );
		if(imIn->depth > 1) 
			cvCalcHist( &planes1[p], cvHist,
					   0, NULL );
		else
			cvCalcHist( &cvIm1, cvHist,
					   0, NULL );
		//		cvGetHistValue_1D( cvHist, idx0 );


		float max_value = 0;
		cvGetMinMaxHistValue( cvHist, 0, &max_value, 0, 0 );
		int pix;
		//memset(planes2[p]->imageData, 0, imIn->width * imIn->height);
		pix = 0;
		int minfound = 0;
        while(pix < 256 && !minfound)
		{
			if(cvQueryHistValue_1D( cvHist, pix ) > 0.f)
				minfound = pix;
			pix++;
		}
		pix = 255;
		int maxfound = 0;
        while(pix >= 0 && !maxfound)
		{
			if(cvQueryHistValue_1D( cvHist, pix ) > 0.f)
				maxfound = pix;
			pix--;
		}

		// calculate stretch factor
		if(maxfound != minfound) {
			float factor = 255.f/(float)(maxfound - minfound);
			unsigned char conv[256];
			memset(conv, 0, 256);
			for( pix = minfound; pix <= maxfound; pix++ )
			{
				conv[pix] = (unsigned char)(cvRound( (float)(pix - minfound)*factor));
			}
			for(int i=0; i<imIn->width*imIn->height; i++)
				if(imIn->depth > 1) 
					*((unsigned char *)planes2[p]->imageData + i) = 
						conv[(int)*((unsigned char *)planes1[p]->imageData + i)];
				else
					*((unsigned char *)cvIm2->imageData + i) = 
						conv[(int)*((unsigned char *)cvIm1->imageData + i)];
		}
		cvReleaseHist(&cvHist);
	}
	if(imIn->depth > 1)
		cvCvtPlaneToPix(planes2[0], planes2[1], planes2[2], planes2[3], cvIm2);
	
	memcpy(imageOut, cvIm2->imageData, imIn->buffer_size);


}


void logstretch_histo()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;
	CvSize size;
	size.width = imIn->width;
	size.height = imIn->height;

	allocateImages();
	cvCopy(cvIm1, cvIm2);

	// decompose planes
	if(imIn->depth > 1) {
		cvCvtPixToPlane(cvIm1, planes1[0], planes1[1], planes1[2], planes1[3]);
		cvCvtPixToPlane(cvIm1, planes2[0], planes2[1], planes2[2], planes2[3]);
	}

	for(int p = 0; p<imIn->depth; p++)
	{
		// calculate histogram
        int dims[] = { 256 };

		CvHistogram * cvHist = cvCreateHist(1, dims, CV_HIST_ARRAY, NULL, 1 );
		if(imIn->depth > 1)
			cvCalcHist( &planes1[p], cvHist,
					   0, NULL );
		else
			cvCalcHist( &cvIm1, cvHist,
					   0, NULL );
		//		cvGetHistValue_1D( cvHist, idx0 );


		float max_value = 0;
		cvGetMinMaxHistValue( cvHist, 0, &max_value, 0, 0 );
		int pix;
		//memset(planes2[p]->imageData, 0, imIn->width * imIn->height);
		pix = 0;
		int minfound = 0;
        while(pix < 256 && !minfound)
		{
			if(cvQueryHistValue_1D( cvHist, pix ) > 0.f)
				minfound = pix;
			pix++;
		}
		pix = 255;
		int maxfound = 0;
        while(pix >= 0 && !maxfound)
		{
			if(cvQueryHistValue_1D( cvHist, pix ) > 0.f)
				maxfound = pix;
			pix--;
		}

		// calculate stretch factor
		if(maxfound != minfound) {
			float factor = 1.f/(float)(maxfound - minfound);
			float l255 = 255.f / log(factor);
			unsigned char conv[256];
			memset(conv, 0, 256);
			for( pix = minfound+1; pix <= maxfound; pix++ )
			{
				conv[pix] = 255 - (unsigned char)(cvRound( log( (float)(pix - minfound)*factor) * l255));
			}
			for(int i=0; i<imIn->width*imIn->height; i++)
				if(imIn->depth > 1)
					*((unsigned char *)planes2[p]->imageData + i) = conv[(int)*((unsigned char *)planes1[p]->imageData + i)];
				else
					*((unsigned char *)cvIm2->imageData + i) = conv[(int)*((unsigned char *)cvIm1->imageData + i)];
		}
		cvReleaseHist(&cvHist);
	}
	if(imIn->depth > 1)
		cvCvtPlaneToPix(planes2[0], planes2[1], planes2[2], planes2[3], cvIm2);
	memcpy(imageOut, cvIm2->imageData, imIn->buffer_size);


}


/***************** END ON PLUGIN DECLARATIONS **********************/
PLUGIN_CORE_FUNC
