/***************************************************************************
	opencv_color.cpp  -  Color spaces conversion for Piaf
							 -------------------
	begin                : Tue Dec  11 15:52:21 CEST 2010
	copyright            : (C) 2010 by Christophe Seyve
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

// OpenCV
#include "swopencv.h"


// include component header
#include "SwPluginCore.h"
#include "swimage_utils.h"


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
uchar HSVthreshold = 127;

swFuncParams HSV_params[] = {
	{"plane", swStringList, (void *)&HSVplane},
	{"threshold", swU8, (void *)&HSVthreshold}
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


/// Swap R and B planes
void swapR_B();

char *RGBlist[] = {"Red", "Green", "Blue", "Alpha"};
swStringListStruct RGBplane = {
	4, // nb elements
	0, // default element
	RGBlist
	};
swFuncParams RGB_params[] = {
	{"plane", swStringList, (void *)&RGBplane}
};
void RGB();

float scale = 1.f/16.f;
swFuncParams scale_params[] = {
	{"factor", swFloat, (void *)&scale}
};
void convertScale();

/* swFunctionDescriptor :
	char * : function name
	int : number of parameters
	swFuncParams : function parameters
	swType : input type
	swType : output type
	void * : procedure
	*/
swFunctionDescriptor functions[] = {
	{"RGB->n", 		1,	RGB_params,  	swImage, swImage, &RGB, NULL},
	{"HSV->n", 		1,	HSV_params,  	swImage, swImage, &HSV, NULL},
	{"HSL->n",		1,	HSL_params,  	swImage, swImage, &HSL, NULL},
	{"R<->B",		0,	YCrCb_params,	swImage, swImage, &swapR_B, NULL},
	{"YCrCb",		1,	YCrCb_params,	swImage, swImage, &YCrCb, NULL},
	{"Scale to 8U",	1,	scale_params,	swImage, swImage, &convertScale, NULL}
};
int nb_functions = 6;


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
			for(int i=0;i<imIn->depth; i++) {
				planes1[i] = cvCreateImage(size,  IPL_DEPTH_8U, 1);
			}
			for(int i=imIn->depth; i<4;i++) {
				planes1[i] = NULL;
			}
			for(int i=0; i<4;i++) {
				planes2[i] = NULL;
			}

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
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	mapIplImageToSwImage(cvImGray, imOut);
	return;

	if(cvIm2->nChannels>1) {
		if(cvIm2->nChannels == 3)
			cvCvtColor(cvImGray, cvIm2, CV_GRAY2RGB);
		else
			cvCvtColor(cvImGray, cvIm2, CV_GRAY2RGBA);

		// Try to change outpue nchannels
		fprintf(stderr, "[%s] %s:%d : try to change output nChannels\n",
				__FILE__, __func__, __LINE__);
		mapIplImageToSwImage(cvImGray, imOut);
	}
	else
	{
		cvCopy(cvIm1, cvIm2);
	}
}

IplImage * cv8U = NULL;
IplImage * cvRawIn = NULL;

void convertScale()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	fprintf(stderr, "convert to IplImage %dx%dx%dx%d\n",
			imIn->width, imIn->height, imIn->depth, imIn->bytedepth);
	convertSwImageToIplImage(imIn, &cvRawIn);
	if(!cv8U)
	{
		cv8U = cvCreateImage(cvSize(imIn->width, imIn->height),
							 IPL_DEPTH_8U, imIn->depth);
	}
	cvConvertScale(cvRawIn, cv8U, scale);

	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	mapIplImageToSwImage(cv8U, imOut);
}

void RGB()
{
	//fprintf(stderr, "%s:%d\n", __func__, __LINE__); fflush(stderr);
	allocateImages();

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__); fflush(stderr);
	if(cvIm1->nChannels >= 3)
	{
		IplImage * to_planes[4] = { planes1[0], planes1[1], planes1[2], planes1[3] };
		switch(cvIm1->nChannels)
		{
		default:

			break;
		case 1:
			cvCopy(cvIm1, cvImGray);
			break;
		case 3: // copy directly the plane because input image is R,G,B
			// change destination plane
			if(RGBplane.curitem>=0 && RGBplane.curitem<3)
			{
				to_planes[RGBplane.curitem] = cvImGray;
			}
			break;
		case 4: // map the plane because input image is B,G,R,A and order of curitem is R,G,B,A
			// change destination plane
			switch(RGBplane.curitem)
			{
			default:
				break;
			case 0: // user wants red
				to_planes[2] = cvImGray;
				break;
			case 1: // user wants green
				to_planes[1] = cvImGray;
				break;
			case 2: // user wants blue
				to_planes[0] = cvImGray;
				break;
			case 3: // user wants green
				to_planes[3] = cvImGray;
				break;
			}
			break;
		}

		// Separate planes
		cvCvtPixToPlane(cvIm1, to_planes[0], to_planes[1], to_planes[2], to_planes[3]);
	}

	finishImages();
}

void swapR_B()
{
	allocateImages();

	switch(cvIm1->nChannels)
	{
	default:
		cvCopy(cvIm1, cvIm2);
		break;
	case 3:
		cvCvtColor(cvIm1, cvIm2, CV_RGB2BGR);
		break;
	case 4:
		cvCvtColor(cvIm1, cvIm2, CV_RGBA2BGRA);
		break;
	}
}

void HSV()
{
	//fprintf(stderr, "%s:%d\n", __func__, __LINE__); fflush(stderr);
	allocateImages();

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__); fflush(stderr);
	if(cvIm1->nChannels >= 3)
	{
		cvCvtColor(cvImRGB, cvImHSV, CV_BGR2HSV);

		// Separate planes
		cvCvtPixToPlane(cvImHSV, planes1[0], planes1[1], planes1[2], NULL);

		// use selected plane
		cvCopy(planes1[HSVplane.curitem], cvImGray);
	}

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__); fflush(stderr);

	finishImages();

	if(HSVplane.curitem == 0 /* Hue */)
	{
		//

	}
	//fprintf(stderr, "%s:%d\n", __func__, __LINE__); fflush(stderr);


	static int iteration = 0;
	iteration++;
	if(iteration == 5)
	{
		//fprintf(stderr, "registerFunctions...\n");
		plugin.registerFunctions(functions, nb_functions );
	}
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
PLUGIN_CORE_FUNC
