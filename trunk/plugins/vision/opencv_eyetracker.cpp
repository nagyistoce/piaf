/***************************************************************************
	opencv_eyetracker.cpp  -  Piaf plugin for tracking eye position drawn in image
							by eye-tracker :
							the eye tracker export videos with a cross inside
							image showing the position of look in the field of
							view of the person
							 -------------------
	begin                : Fri Jan  21 08:22:21 CEST 2011
	copyright            : (C) 2011 by Christophe Seyve
	email                : cseyve@free.fr
 ***************************************************************************/

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <vector>

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
./build_all.sh
# needs libSwPluginCore (qmake piaf-lib.pro && sudo make install)
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
#define SUBCATEGORY	"Eye-tracker"

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

uchar threshold_min = 127;
uchar threshold_max = 255;

swFuncParams threshold_params[] = {
	{">= Thr. min", swU8, (void *)&threshold_min},
	{"<= Thr. min", swU8, (void *)&threshold_max},
};
void threshold();


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
	{"YCrCb",		1,	YCrCb_params,	swImage, swImage, &YCrCb, NULL},
	{"threshold",		2,	threshold_params,	swImage, swImage, &threshold, NULL}
};
int nb_functions = 4;


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
		if(cvIm2->nChannels == 3) {
			cvCvtColor(cvImGray, cvIm2, CV_GRAY2RGB);
		} else {
			cvCvtColor(cvImGray, cvIm2, CV_GRAY2RGBA);
		}
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


IplImage * cvAccOne = NULL;
IplImage * cvAcc32F = NULL; ///< accumulator


void threshold()
{
	allocateImages();

	switch(cvIm1->nChannels)
	{
	case 3:
		cvCvtColor(cvImRGB, planes1[0], CV_RGB2GRAY);
		break;
	case 4:
		cvCvtColor(cvImRGB, planes1[0], CV_RGBA2GRAY);
		break;
	case 1:
		cvCopy(cvImRGB, planes1[0]);
		break;
	}

	// threshold
	if(cvIm1->nChannels >= 3)
	{
		cvCvtColor(cvImRGB, cvImHSV, CV_BGR2HSV);

		// Separate planes
		cvCvtPixToPlane(cvImHSV, planes1[0], planes1[1], planes1[2], NULL);

		// use selected plane
		cvCopy(planes1[HSVplane.curitem], cvImGray);
	}
	cvThreshold(planes1[0], cvImGray, threshold_min, threshold_max, 0);

	// Get the max of x and y
	int histoRow[cvImGray->height];
	int histoCol[cvImGray->width];
	memset(histoRow, 0, sizeof(int)*cvImGray->height);
	memset(histoCol, 0, sizeof(int)*cvImGray->width);
	for(int r = 0; r<cvImGray->height; r++)
	{
		u8 * line = (u8 *)(cvImGray->imageData + r * cvImGray->widthStep);
		for(int c = 0; c<cvImGray->width; c++)
		{
			// store histo
			if(line[c])
			{
				histoRow[r] ++;
				histoCol[c] ++;
			}
		}
	}

	// Get max
	int maxr = cvImGray->width/2, maxc = cvImGray->height/2;
	int cross_r = -1, cross_c = -1;

	for(int r = 0; r<cvImGray->height; r++)
	{
		if(histoRow[r]> maxr) {
			maxr = histoRow[r];
			cross_r = r;
		}
	}
	for(int c = 0; c<cvImGray->width; c++)
	{
		if(histoCol[c]> maxc) {
			maxc = histoCol[c];
			cross_c = c;
		}
	}


	static std::vector<CvPoint> eye_tracking;

	CvPoint cur_point = cvPoint(cross_c, cross_r);
	//
	if(cross_c >= 0 && cross_r >=0)
	{
		//
		cvCircle(cvIm2, cvPoint(cross_c, cross_r),
				 20, CV_RGB(255, 255, 0), 2);

		eye_tracking.push_back(cur_point);
	}
	if(!cvAccOne) {
		cvAccOne = cvCreateImage(cvGetSize(cvIm2), IPL_DEPTH_8U, 1);
	}
	cvZero(cvAccOne);

	if(!cvAcc32F) {
		cvAcc32F = cvCreateImage(cvGetSize(cvIm2), IPL_DEPTH_32F, 1);
		cvZero(cvAcc32F);
	}

	std::vector<CvPoint>::iterator it;
	int iter = 0;
	CvPoint prev_point;

	cvCopy(cvIm1, cvIm2);
	for(it = eye_tracking.begin(); it != eye_tracking.end(); ++it, ++iter)
	{
		prev_point = cur_point;
		CvPoint next_point = *it;
		if(iter > 0) {
			cvLine(cvIm2, cur_point, next_point,
			   CV_RGB(255, 127, 0), 2);
			cvLine(cvAccOne, cur_point, next_point,
			   cvScalarAll(1), 1);
		}
		cur_point = next_point;
	}

	if(cross_c >= 0 && cross_r >=0)
	{
		cvLine(cvAccOne, cur_point, prev_point,
		   cvScalarAll(1), 1);
	}

	static int acc_count = 0;
	acc_count++;
	cvAcc(cvAccOne, cvAcc32F);
	//cvConvertScale(cvAcc32F, cvImGray, 1., 0);
	if(cross_c >= 0 && cross_r >=0)
	{
		cvLine(cvIm2, cur_point, prev_point,
		   CV_RGB(255, 127, 0), 1);
	}

	//finishImages();

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


















