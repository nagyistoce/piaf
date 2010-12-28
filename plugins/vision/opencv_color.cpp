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
#define SUBCATEGORY	"Colors"


char *HSVlist[] = {"Hue", "Saturation", "Value"};
swStringListStruct HSVplane = {
	3, // nb elements
	0, // default element
	HSVlist
	};
swFuncParams HSV_params[] = {
	{"plane", swStringList, (void *)&HSVplane}
};
void HSV();


char *HSLlist[] = {"Hue", "Luminance", "Saturation"};
swStringListStruct HSLplane = {
	3, // nb elements
	0, // default element
	HSLlist
	};
swFuncParams HSL_params[] = {
	{"plane", swStringList, (void *)&HSLplane}
};
void HSL();



char *YCrCblist[] = {"Y", "Cr", "Cb"};
swStringListStruct YCrCbplane = {
	3, // nb elements
	0, // default element
	YCrCblist
	};
swFuncParams YCrCb_params[] = {
	{"plane", swStringList, (void *)&YCrCbplane}
};
void YCrCb();


/* swFunctionDescriptor :
	char * : function name
	int : number of parameters
	swFuncParams : function parameters
	swType : input type
	swType : output type
	void * : procedure
	*/
swFunctionDescriptor functions[] = {
	{"HSV", 		1,	HSV_params,  	swImage, swImage, &HSV, NULL},
	{"HSL",			1,	HSL_params,  	swImage, swImage, &HSL, NULL},
	{"YCrCb",		1,	YCrCb_params,	swImage, swImage, &YCrCb, NULL}
};
int nb_functions = 3;


IplImage * cvIm1 = NULL;
IplImage * cvIm2 = NULL;
IplImage * planes1[4];
IplImage * planes2[4];

IplImage * cvImHSV = NULL;
IplImage * cvImRGB = NULL;

IplImage * cvImGray = NULL;





void allocateImages()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	CvSize size;
	size.width = imIn->width;
	size.height = imIn->height;

	if(!cvIm1) {
		cvIm1 = cvCreateImageHeader(size,  IPL_DEPTH_8U, imIn->depth);

		cvImHSV = cvCreateImage(size, IPL_DEPTH_8U, 3);
		cvImRGB = cvCreateImage(size, IPL_DEPTH_8U, 3);

		if(imIn->depth > 1) {
			for(int i=0;i<imIn->depth; i++)
				planes1[i] = cvCreateImage(size,  IPL_DEPTH_8U, 1);

			for(int i=imIn->depth; i<4;i++)
				planes1[i] = NULL;

			cvImGray = cvCreateImage(size,  IPL_DEPTH_8U, 1);
		}
		else {
			for(int i=0; i<4;i++) {	planes1[i] = NULL; }

			cvImGray = cvCreateImageHeader(size,  IPL_DEPTH_8U, 1);
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
			for(int i=0; i<4;i++) {	planes2[i] = NULL; }
		}
	}

	cvIm1->imageData = (char *)imageIn;
	cvIm2->imageData = (char *)imageOut;

	if(cvIm1->nChannels == 4)
	{
		cvCvtColor(cvIm1, cvImRGB, CV_RGBA2RGB);
	}
	else if(cvIm1->nChannels == 3)
	{
		cvCopy(cvIm1, cvImRGB);
	}

}



void finishImages()
{
	if(cvIm2->nChannels>1) {
		if(cvIm2->nChannels == 3)
			cvCvtColor(cvImGray, cvIm2, CV_GRAY2RGB);
		else
			cvCvtColor(cvImGray, cvIm2, CV_GRAY2RGBA);
	}
	else
	{
		cvCopy(cvIm1, cvIm2);
	}
}


void HSV()
{
	fprintf(stderr, "%s:%d\n", __func__, __LINE__); fflush(stderr);
	allocateImages();
	fprintf(stderr, "%s:%d\n", __func__, __LINE__); fflush(stderr);

	if(cvIm1->nChannels >= 3)
	{
		cvCvtColor(cvImRGB, cvImHSV, CV_BGR2HSV);

		// Separate planes
		cvCvtPixToPlane(cvImHSV, planes1[0], planes1[1], planes1[2], NULL);

		// use selected plane
		cvCopy(planes1[HSVplane.curitem], cvImGray);
	}
	fprintf(stderr, "%s:%d\n", __func__, __LINE__); fflush(stderr);

	finishImages();
	fprintf(stderr, "%s:%d\n", __func__, __LINE__); fflush(stderr);

}
void HSL()
{
	allocateImages();

	if(cvIm1->nChannels >= 3)
	{
		cvCvtColor(cvImRGB, cvImHSV, CV_RGB2HLS);

		// Separate planes
		cvCvtPixToPlane(cvImHSV, planes1[0], planes1[1], planes1[2], NULL);

		// use selected plane
		cvCopy(planes1[HSLplane.curitem], cvImGray);
	}

	finishImages();
}
void YCrCb()
{

	allocateImages();

	if(cvIm1->nChannels >= 3)
	{
		cvCvtColor(cvImRGB, cvImHSV, CV_RGB2YCrCb);

		// Separate planes
		cvCvtPixToPlane(cvImHSV, planes1[0], planes1[1], planes1[2], NULL);

		// use selected plane
		cvCopy(planes1[YCrCbplane.curitem], cvImGray);
	}

	finishImages();

}









// DO NOT MODIFY BELOW THIS LINE ------------------------------------------------------------------------------


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


















