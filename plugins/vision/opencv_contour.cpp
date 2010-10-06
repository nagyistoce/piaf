/***************************************************************************
    opencv_morphology.cpp  -  description
                             -------------------
    begin                : Tue Dec  11 15:52:21 CEST 2002
    copyright            : (C) 2002 by Christophe Seyve
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// OpenCV
#include <cv.h>
#include <cv.hpp>


// include componenet header
#include "SwPluginCore.h"

/* compile with :
gcc -Wall -I/usr/local/sisell/include SwPluginCore.cpp -c
gcc -Wall -I/usr/local/sisell/include opencv_morphology.cpp -c
gcc opencv_morphology.o SwPluginCore.o -o opencv_morphology -lopencv
# needs SwPluginCore.o
*/


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
#define SUBCATEGORY	"Contour"


// function parameters

short sobel_dx = 1;
short sobel_dy = 0;
short sobel_kernel = 3;
swFuncParams sobel_params[] = {
	{"derivate order on x", swS16, (void *)&sobel_dx},
	{"derivate order on y", swS16, (void *)&sobel_dy},
	{"kernel size", swS16, (void *)&sobel_kernel}
};
void sobel();

double canny_low = 1500.0;
double canny_high = 2000.0;
short canny_kernel = 5;
swFuncParams canny_params[] = {
	{"low tresh", swDouble, (void *)&canny_low},
	{"high tresh", swDouble, (void *)&canny_high},
	{"kernel size", swS16, (void *)&canny_kernel}
};
void canny();


/*
 void cvAdaptiveThreshold( const CvArr* src, CvArr* dst, double max_value,
                          int adaptive_method=CV_ADAPTIVE_THRESH_MEAN_C,
                          int threshold_type=CV_THRESH_BINARY,
                          int block_size=3, double param1=5 );
 */

double adapt_max_value = 500.;
short adapt_block_size = 3;
double adapt_param1 = 5.;


swFuncParams adaptative_params[] = {
	{"max_value", swDouble, (void *)&adapt_max_value},
	{"block_size", swS16, (void *)&adapt_block_size},
	{"param1", swDouble, (void *)&adapt_param1}
};
void adaptative_threshold();

short laplace_kernel = 3;
swFuncParams laplace_params[] = {
	{"kernel size", swS16, (void *)&laplace_kernel}
};
void laplace();




/* swFunctionDescriptor : 
	char * : function name
	int : number of parameters
	swFuncParams : function parameters
	swType : input type
	swType : output type
	void * : procedure
	*/
swFunctionDescriptor functions[] = {
	{"Canny", 		3,	canny_params,  	swImage, swImage, &canny, NULL},
	{"Sobel", 		3,	sobel_params,  	swImage, swImage, &sobel, NULL},
	{"Laplace", 	1,	laplace_params, swImage, swImage, &laplace, NULL},
	{"Adaptative", 	3,	adaptative_params,  	swImage, swImage, &adaptative_threshold, NULL}
};
int nb_functions = 3;

/******************* END OF USER SECTION ********************/




IplImage * cvIm1 = NULL;
IplImage * cvIm2 = NULL;
IplImage * planes1[4];
IplImage * planes2[4];


void allocateImages()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);

	CvSize size;
	size.width = imIn->width;
	size.height = imIn->height;

	if(!cvIm1) {
		cvIm1 = cvCreateImage(size,  IPL_DEPTH_8U, imIn->depth);
		if(imIn->depth > 1) {
			for(int i=0;i<imIn->depth; i++)
				planes1[i] = cvCreateImage(size,  IPL_DEPTH_8U, 1);
			for(int i=imIn->depth; i<4;i++)
				planes1[i] = NULL;
		}
	}
}


void sobel()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;
	CvSize size;
	size.width = imIn->width;
	size.height = imIn->height;
	
	allocateImages();

	memcpy( cvIm1->imageData, imageIn, imIn->buffer_size);
	
	if(!cvIm2) {
		CvSize size;
		size.width = imIn->width;
		size.height = imIn->height;

		cvIm2 = cvCreateImage(size,  IPL_DEPTH_16S, imIn->depth);
		if(imIn->depth > 1) {
			for(int i=0;i<imIn->depth; i++)
				planes2[i] = cvCreateImage(size,  IPL_DEPTH_16S, 1);
			for(int i=imIn->depth; i<4;i++)
				planes2[i] = NULL;
		}
	}
	
	// decompose planes
	if(imIn->depth > 1) {

		cvCvtPixToPlane(cvIm1, planes1[0], planes1[1], planes1[2], planes1[3]);
		for(int p = 0; p<imIn->depth; p++)
		{
			cvSobel(planes1[p], planes2[p], sobel_dx, sobel_dy, sobel_kernel);
		}
		cvCvtPlaneToPix(planes2[0], planes2[1], planes2[2], planes2[3], cvIm2);
	}
	else
		cvSobel(cvIm1, cvIm2, sobel_dx, sobel_dy, sobel_kernel);
	
	for(unsigned long pix=0; pix < imIn->buffer_size; pix++) {
		short res = *((short *)cvIm2->imageData + pix);
		
		imageOut[pix] = (unsigned char)(res>0 ? res/4 : -res/4);
	}
}

void laplace()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	allocateImages();
	memcpy( cvIm1->imageData, imageIn, imIn->buffer_size);
	
	if(!cvIm2) {
		CvSize size;
		size.width = imIn->width;
		size.height = imIn->height;

		cvIm2 = cvCreateImage(size,  IPL_DEPTH_16S, imIn->depth);
		if(imIn->depth > 1) {
			for(int i=0;i<imIn->depth; i++)
				planes2[i] = cvCreateImage(size,  IPL_DEPTH_16S, 1);
			for(int i=imIn->depth; i<4;i++)
				planes2[i] = NULL;
		}
	}

	if(imIn->depth>1) {
		// decompose planes
		cvCvtPixToPlane(cvIm1, planes1[0], planes1[1], planes1[2], planes1[3]);
		for(int p = 0; p<4; p++)
		{
			cvLaplace(planes1[p], planes2[p], laplace_kernel);
		}
		cvCvtPlaneToPix(planes2[0], planes2[1], planes2[2], planes2[3], cvIm2);
	}
	else
		cvLaplace(cvIm1, cvIm2, laplace_kernel);
	
	for(unsigned long pix=0; pix < imIn->buffer_size; pix++) {
		short res = *((short *)cvIm2->imageData + pix);
		
		imageOut[pix] = (unsigned char)(res>0 ? res : -res);
	}
}

void canny()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	allocateImages();	
	memcpy( cvIm1->imageData, imageIn, imIn->buffer_size);
	
	if(!cvIm2) {
		CvSize size;
		size.width = imIn->width;
		size.height = imIn->height;
		cvIm2 = cvCreateImage(size,  IPL_DEPTH_8U, imIn->depth);
		if(imIn->depth>1) {
			for(int i=0;i<imIn->depth; i++)
				planes2[i] = cvCreateImage(size,  IPL_DEPTH_8U, 1);
			for(int i=imIn->depth; i<4;i++)
				planes2[i] = NULL;
		}
	}
	if(imIn->depth>1) {
		// decompose planes
		cvCvtPixToPlane(cvIm1, planes1[0], planes1[1], planes1[2], planes1[3]);
		for(int p = 0; p<4; p++)
		{
			cvCanny(planes1[p], planes2[p], canny_low, canny_high, (int)canny_kernel);
		}
		cvCvtPlaneToPix(planes2[0], planes2[1], planes2[2], planes2[3], cvIm2);
	} else
		cvCanny(cvIm1, cvIm2, canny_low, canny_high, (int)canny_kernel);
	
	memcpy(imageOut, cvIm2->imageData, imIn->buffer_size);
}



void adaptative_threshold() {
	/*
 void cvAdaptiveThreshold( const CvArr* src, CvArr* dst, double max_value,
                          int adaptive_method=CV_ADAPTIVE_THRESH_MEAN_C,
                          int threshold_type=CV_THRESH_BINARY,
                          int block_size=3, double param1=5 );
 */
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	allocateImages();	
	memcpy( cvIm1->imageData, imageIn, imIn->buffer_size);
	
	if(!cvIm2) {
		CvSize size;
		size.width = imIn->width;
		size.height = imIn->height;
		cvIm2 = cvCreateImage(size,  IPL_DEPTH_8U, imIn->depth);
		if(imIn->depth>1) {
			for(int i=0;i<imIn->depth; i++)
				planes2[i] = cvCreateImage(size,  IPL_DEPTH_8U, 1);
			for(int i=imIn->depth; i<4;i++)
				planes2[i] = NULL;
		}
	}
	
	
	if(imIn->depth>1) {
		// decompose planes
		cvCvtPixToPlane(cvIm1, planes1[0], planes1[1], planes1[2], planes1[3]);
		for(int p = 0; p<4; p++)
		{
			
			cvAdaptiveThreshold(planes1[p], planes2[p], adapt_max_value,
					CV_ADAPTIVE_THRESH_MEAN_C, 
					CV_THRESH_BINARY, adapt_block_size,
					adapt_param1);
		}
		cvCvtPlaneToPix(planes2[0], planes2[1], planes2[2], planes2[3], cvIm2);
	} else
		cvCanny(cvIm1, cvIm2, canny_low, canny_high, (int)canny_kernel);
	
	memcpy(imageOut, cvIm2->imageData, imIn->buffer_size);

}


void signalhandler(int sig)
{
	fprintf(stderr, "================== RECEIVED SIGNAL %d = '%s' From process %d ==============\n", sig, sys_siglist[sig], getpid());
	signal(sig, signalhandler);
	if(sig != SIGUSR1)
		exit(0);
}
int main(int argc, char *argv[])
{
	// SwPluginCore load
	for(int i=0; i<NSIG; i++)
		signal(i, signalhandler);
	
	fprintf(stderr, "registerCategory...\n");
	plugin.registerCategory(CATEGORY, SUBCATEGORY);
	
	// register functions 
	fprintf(stderr, "registerFunctions...\n");
	plugin.registerFunctions(functions, nb_functions );

	// process loop
	fprintf(stderr, "loop...\n");
	plugin.loop();
	
  	return EXIT_SUCCESS;
}
