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
#ifndef OPENCV_22
#include <cv.h>
#include <cvaux.h>
#include <highgui.h>
#else
#include <opencv.hpp>
#include <legacy/compat.hpp>
#endif


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
#define SUBCATEGORY	"Morphology"


// function parameters
unsigned char threshold_threshold = 127;

swFuncParams threshold_params[] = {
	{"threshold", swU8, (void *)&threshold_threshold},
};

swFuncParams invert_params[] = {
};


short erode_iterations = 1;
swFuncParams erode_params[] = {
	{"iteration", swS16, (void *)&erode_iterations}
};
void erode();
void sobel();

short dilate_iterations = 1;
swFuncParams dilate_params[] = {
	{"iteration", swS16, (void *)&dilate_iterations}
};
void dilate();

short iterations = 1;
char *Shapelist[] = {"Rectangle", "Cross", "Ellipse"};
swStringListStruct shape = { 
	3, // nb elements
	0, // default element
	Shapelist
	};
short kernel_size = 3;

swFuncParams generic_params[] = {
	{"iteration", swS16, (void *)&iterations},
	{"shape", swStringList, (void *)&shape},
	{"size", swS16, (void *)&kernel_size}
};


void open();
void close();
void gradient();
void tophat();
void blackhat();


/* swFunctionDescriptor : 
	char * : function name
	int : number of parameters
	swFuncParams : function parameters
	swType : input type
	swType : output type
	void * : procedure
	*/
swFunctionDescriptor functions[] = {
	{"Erode", 		1,	erode_params,  		swImage, swImage, &erode,	NULL},
	{"Dilate", 		1,	dilate_params,  	swImage, swImage, &dilate,	NULL},
	{"Close", 		3,	generic_params,  	swImage, swImage, &close,	NULL},
	{"Open", 		3,	generic_params,  	swImage, swImage, &open,	NULL},
	{"Gradient", 	3,	generic_params,  	swImage, swImage, &gradient,	NULL},
	{"Top hat", 	3,	generic_params,  	swImage, swImage, &tophat,	NULL},
	{"Black Hat", 	3,	generic_params,  	swImage, swImage, &blackhat,	NULL}
};
int nb_functions = 7;

/******************* END OF USER SECTION ********************/




IplImage * cvIm1 = NULL;
IplImage * cvIm2 = NULL;
IplImage * cvTmp = NULL;

/*
 * Allocate images and make cvIm1=image input and cvIm2=image output
 */
void initImages()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	CvSize cvsize;
	cvsize.width = imIn->width;
	cvsize.height = imIn->height;

	if(!cvIm1) {
		cvIm1 = cvCreateImageHeader(cvsize,  IPL_DEPTH_8U, imIn->depth);
	}
	cvIm1->imageData = (char *)imageIn;

	if(!cvIm2) {
		cvIm2 = cvCreateImageHeader(cvsize,  IPL_DEPTH_8U, imIn->depth);
	}
	cvIm2->imageData = (char *)imageOut;

	if(!cvTmp) {
		cvTmp = cvCreateImage(cvsize,  IPL_DEPTH_8U, imIn->depth);
	}
}

// function invert
void erode()
{
	initImages(); // cvIm1 is image input and cvIm2 is output
    	
	// perform erode
	cvErode(cvIm1, cvIm2, NULL, erode_iterations);
}


void dilate()
{
	initImages(); // cvIm1 is image input and cvIm2 is output

	cvDilate(cvIm1, cvIm2, NULL, dilate_iterations);
}

IplConvKernel *elt = NULL;

void createStructElt()
{
	CvElementShape shapes[3] = { CV_SHAPE_RECT, //a rectangular element;
								 CV_SHAPE_CROSS, //a cross-shaped element;
								 CV_SHAPE_ELLIPSE};

	elt = cvCreateStructuringElementEx (
		(int)kernel_size, (int)kernel_size,
		(int)(kernel_size/2), (int)(kernel_size/2),
		shapes[shape.curitem], NULL);
}


// OPEN OPERATION
void open()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageOut = (unsigned char *)imOut->buffer;
    	
	initImages();
	createStructElt();

	// perform open
	cvMorphologyEx (cvIm1, cvIm2, cvTmp, elt,
		CV_MOP_OPEN,
		iterations);
	
	cvReleaseStructuringElement (&elt);
}

// CLOSE OPERATION
void close()
{
	initImages();

	createStructElt();

	// perform close
	cvMorphologyEx (cvIm1, cvIm2, cvTmp, elt,
		CV_MOP_CLOSE,
		iterations);
	
	cvReleaseStructuringElement (&elt);
}

// TOP HAT OPERATION
void tophat()
{
	initImages();

	createStructElt();

	// perform top hat
	cvMorphologyEx (cvIm1, cvIm2, cvTmp, elt,
		CV_MOP_TOPHAT,
		iterations);
	
	cvReleaseStructuringElement(&elt);
}

// BLACK HAT OPERATION
void blackhat()
{
	initImages();

	createStructElt();

	// perform black hat
	cvMorphologyEx (cvIm1, cvIm2, cvTmp, elt,
		CV_MOP_BLACKHAT,
		iterations);
	
	cvReleaseStructuringElement(&elt);
}

// gradient OPERATION
void gradient()
{
	initImages();

	createStructElt();

	// perform gradient
	cvMorphologyEx (cvIm1, cvIm2, cvTmp, elt,
		CV_MOP_GRADIENT,
		iterations);
	
	cvReleaseStructuringElement(&elt);
}






/********************** DO NOT MODIFY BELOW **********************/

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
	
	fprintf(stderr, "[%s] registerCategory...\n", __FILE__);
	plugin.registerCategory(CATEGORY, SUBCATEGORY);
	
	// register functions 
	fprintf(stderr, "[%s] registerFunctions : %d functions...\n", __FILE__, nb_functions);
	plugin.registerFunctions(functions, nb_functions );

	// process loop
	fprintf(stderr, "[%s] starting loop...\n", __FILE__);
	plugin.loop();
	
  	return EXIT_SUCCESS;
}
