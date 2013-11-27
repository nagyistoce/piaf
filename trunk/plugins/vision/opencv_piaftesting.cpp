/***************************************************************************
	opencv_piaftesting.cpp  -  Testing plugin for Piaf PluginCore : crashes,
								takes long time, display ...
                             -------------------
	begin                : Wed Sept 08 08:18:39 CET 2011
	copyright            : (C) 2011 by Christophe Seyve
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
#define SUBCATEGORY	"PiafTesting"


// function parameters
unsigned short long_time_ms = 1000;

swFuncParams long_time_params[] = {
	{"sleep ms", swU16, (void *)&long_time_ms}
};

void crash();
void cvexception();
void long_time();


/* swFunctionDescriptor :
 char * : function name
 int : number of parameters
 swFuncParams : function parameters
 swType : input type
 swType : output type
 void * : procedure
 */
swFunctionDescriptor functions[] = {
	{"Crash",		0,	NULL, swImage, swImage, &crash, NULL},
	{"Throw cv::exception",	0, 	NULL, swImage, swImage, &cvexception, NULL},
	{"Takes long",	1,	long_time_params, swImage, swImage, &long_time, NULL}
};
int nb_functions = 3;



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


#define IPLLINE_8U(_img,_r)	(u8 *)((_img)->imageData + (_r)*(_img)->widthStep)


void crash()
{
	char * toto = NULL;
	toto+=0x42;
	*toto = '!';
}
void cvexception()
{
	cvSaveImage("/tmp/toto", NULL);
}

void long_time()
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

	usleep(1000 * (int)long_time_ms);
}



/***************** END ON PLUGIN DECLARATIONS **********************/
PLUGIN_CORE_FUNC
