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
short size = 3;

swFuncParams generic_params[] = {
	{"iteration", swS16, (void *)&iterations},
	{"shape", swStringList, (void *)&shape},
	{"size", swS16, (void *)&size}
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
	{"Close", 		3,	generic_params,  		swImage, swImage, &close,	NULL},
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

void allocateImages()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);

	CvSize cvsize;
	cvsize.width = imIn->width;
	cvsize.height = imIn->height;

	if(!cvIm1) {
		cvIm1 = cvCreateImage(cvsize,  IPL_DEPTH_8U, imIn->depth);
	}
	if(!cvIm2) {
		cvIm2 = cvCreateImage(cvsize,  IPL_DEPTH_8U, imIn->depth);
	}
}

// function invert
void erode()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;
	
	allocateImages();
	memcpy( cvIm1->imageData, imageIn, imIn->buffer_size);
    	
	cvErode(cvIm1, cvIm2, NULL, erode_iterations);
	
	memcpy( imageOut, cvIm2->imageData, imIn->buffer_size);
}


void dilate()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	allocateImages();
	memcpy( cvIm1->imageData, imageIn, imIn->buffer_size);
    	
	cvDilate(cvIm1, cvIm2, NULL, dilate_iterations);
	
	memcpy( imageOut, cvIm2->imageData, imIn->buffer_size);

}


void initImages()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	CvSize size;
	size.width = imIn->width;
	size.height = imIn->height;
		
	if(!cvIm1) {
		cvIm1 = cvCreateImage(size,  IPL_DEPTH_8U, imIn->depth);
	}
	if(!cvIm2) {
		cvIm2 = cvCreateImage(size,  IPL_DEPTH_8U, imIn->depth);
	}
	if(!cvTmp) {
		cvTmp = cvCreateImage(size,  IPL_DEPTH_8U, imIn->depth);
	}
	
	memcpy( cvIm1->imageData, imageIn, imIn->buffer_size);
}

IplConvKernel *elt = NULL;
void createStructElt()
{
	CvElementShape shapes[3] = { CV_SHAPE_RECT, //a rectangular element;
							CV_SHAPE_CROSS, //a cross-shaped element;
							CV_SHAPE_ELLIPSE};
							
	elt = cvCreateStructuringElementEx (
		(int)size, (int)size,
		(int)(size/2), (int)(size/2), 
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
	
	memcpy( imageOut, cvIm2->imageData, imIn->buffer_size);
	cvReleaseStructuringElement (&elt);
}

// CLOSE OPERATION
void close()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageOut = (unsigned char *)imOut->buffer;
    	
	initImages();
	createStructElt();
	// perform open
	cvMorphologyEx (cvIm1, cvIm2, cvTmp, elt,
		CV_MOP_CLOSE,
		iterations);
	
	memcpy( imageOut, cvIm2->imageData, imIn->buffer_size);
	cvReleaseStructuringElement (&elt);
}

// TOP HAT OPERATION
void tophat()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageOut = (unsigned char *)imOut->buffer;
    	
	initImages();
	createStructElt();
	// perform open
	cvMorphologyEx (cvIm1, cvIm2, cvTmp, elt,
		CV_MOP_TOPHAT,
		iterations);
	
	memcpy( imageOut, cvIm2->imageData, imIn->buffer_size);
	cvReleaseStructuringElement(&elt);
}

// BLACK HAT OPERATION
void blackhat()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageOut = (unsigned char *)imOut->buffer;
    	
	initImages();
	createStructElt();
	// perform open
	cvMorphologyEx (cvIm1, cvIm2, cvTmp, elt,
		CV_MOP_BLACKHAT,
		iterations);
	
	memcpy( imageOut, cvIm2->imageData, imIn->buffer_size);
	cvReleaseStructuringElement(&elt);
}

// gradient OPERATION
void gradient()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageOut = (unsigned char *)imOut->buffer;
    	
	initImages();
	createStructElt();
	// perform open
	cvMorphologyEx (cvIm1, cvIm2, cvTmp, elt,
		CV_MOP_GRADIENT,
		iterations);
	
	memcpy( imageOut, cvIm2->imageData, imIn->buffer_size);
	cvReleaseStructuringElement(&elt);
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
