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
#define SUBCATEGORY	"Blob-tracker"

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

uchar threshold_min = 200;
uchar threshold_max = 255;

swFuncParams threshold_params[] = {
	{">= Thr. min", swU8, (void *)&threshold_min},
	{"<= Thr. max", swU8, (void *)&threshold_max},
};
void threshold();

void floodfill();


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
	{"threshold",	2,	threshold_params,	swImage, swImage, &threshold, NULL},
	{"blob track",	2,	threshold_params,	swImage, swImage, &floodfill, NULL}
};
int nb_functions = 5;


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

/*
IplConvKernel* cvCreateStructuringElementEx( int nCols, int nRows, int anchorX, int anchorY,
											 CvElementShape shape, int* values );
nCols
Number of columns in the structuring element.
nRows
Number of rows in the structuring element.
anchorX
Relative horizontal offset of the anchor point.
anchorY
Relative vertical offset of the anchor point.
shape
Shape of the structuring element; may have the following values:
CV_SHAPE_RECT , a rectangular element;
CV_SHAPE_CROSS , a cross-shaped element;
CV_SHAPE_ELLIPSE , an elliptic element;
CV_SHAPE_CUSTOM , a user-defined element. In this case the parameter values specifies the mask, that is, which neighbors of the pixel must be considered.
*/

IplConvKernel* erodeElement = NULL;

typedef struct {
	int trackID;			///< Track ID
	CvPoint curPoint;		///< current point
	std::vector<CvPoint>	trajectory;	///< History of movement in image
	std::vector<CvPoint3D32f>	trajectory3D;	///< History of movement in 3D
	int lost;				///< lost status iteration counter
} t_head_track;

std::vector<t_head_track *> tracks;


/*

  */
CvPoint3D32f convertTo3D(CvPoint pt, int depth8bit)
{
/*
8bit image has been constucting using this formula :

 int val = (int)round( 255. *
						  (1. - (g_depth_LUT[raw] - OPENNI_DEPTH2CM_RANGE_MIN)
							 / (OPENNI_DEPTH2CM_RANGE_MAX - OPENNI_DEPTH2CM_RANGE_MIN) )
						  );

with :
 g_depth_LUT[raw] = (float)raw / 1000.f = depth in meter ;

=> depth Z = (255-val8bit) / 255. * (OPENNI_DEPTH2CM_RANGE_MAX - OPENNI_DEPTH2CM_RANGE_MIN)
			+ OPENNI_DEPTH2CM_RANGE_MIN

*/
#define OPENNI_DEPTH2CM_RANGE_MIN	0.6
#define OPENNI_DEPTH2CM_RANGE_MAX	7
#define OPENNI_FOV_H_DEG	58.
#define OPENNI_FRAME_W	640
#define OPENNI_FRAME_H	480

	CvPoint3D32f pt3D;

	pt3D.z = (double)(255-depth8bit) * (double)(OPENNI_DEPTH2CM_RANGE_MAX - OPENNI_DEPTH2CM_RANGE_MIN)/255.f
			+ OPENNI_DEPTH2CM_RANGE_MIN;

	double pixel_size = tan(OPENNI_FOV_H_DEG/2.) / (OPENNI_FRAME_W/2.);
	pt3D.x = (pt.x - (OPENNI_FRAME_W/2.)) * pixel_size * pt3D.z;
	pt3D.y = (pt.y - (OPENNI_FRAME_H/2.)) * pixel_size * pt3D.z;

	fprintf(stderr, "%s:%d : %d,%d %d => %g,%g,%g (pix size=%g)",
			__func__, __LINE__,
			pt.x, pt.y, depth8bit,
			pt3D.x, pt3D.y, pt3D.z,
			pixel_size
			);
	// Compute z from camera height= 2.9
	pt3D.z = 2.9f - pt3D.z;

	return pt3D;
}

int curID = 0;

void floodfill()
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
	cvThreshold(planes1[0], cvImGray, threshold_min, threshold_max, CV_THRESH_TOZERO);

	// Erode to remove noise from JPEG compression
	if(!erodeElement)
	{
		erodeElement = cvCreateStructuringElementEx(5,5, 2,2, CV_SHAPE_ELLIPSE);
	}

	//cvErode( const CvArr* A, CvArr* C, IplConvKernel* B=0, int iterations=1 );)
	cvErode( cvImGray, planes1[0], erodeElement, 1);
	cvCopy(planes1[0], cvImGray);

	cvThreshold(cvImGray, planes1[0], threshold_min, threshold_max, 0);


	//cvCopy(cvIm1, cvIm2);
	if(cvIm2->nChannels > 1) {
		cvCvtPlaneToPix(cvImGray, cvImGray, cvImGray, 0, cvIm2);
	} else {
		cvCopy(cvImGray, cvIm2);
	}
	cvCopy(cvIm1, cvIm2);

	// Clear update flag for later checking
	std::vector<t_head_track *>::iterator it;
	for(it = tracks.begin(); it !=  tracks.end(); it++)
	{
		(*it)->lost++;
	}

	// Use cvFloodFill to get the blobs
	for(int r = 0; r<cvImGray->height; ++r)
	{
		u8 * gray = (u8 *)(planes1[0]->imageData + r * planes1[0]->widthStep);
		u8 * fill = (u8 *)(cvImGray->imageData + r * cvImGray->widthStep);
		for(int c = 0; c<cvImGray->width; ++c)
		{
			if(gray[c] > 32)
			{
				CvConnectedComp conn;
				//
//				void cvFloodFill( CvArr* img, CvPoint seed, double newVal,
//				                  double lo=0, double up=0, CvConnectedComp* comp=0,
//				                  int flags=4, CvArr* mask=0 );
				cvFloodFill(planes1[0], cvPoint(c, r),
							cvScalarAll(32),
							cvScalarAll(threshold_min-64), cvScalarAll(threshold_max),
							&conn
							);
//				fprintf(stderr, "%s:%d Conn: %d,%d+%dx%d\n",
//						__func__, __LINE__,
//						conn.rect.x, conn.rect.y, conn.rect.width, conn.rect.height
//						);

				// Don't track too small areas
				if(conn.area > 50)
				{
					cvRectangle(cvIm2,
								cvPoint(conn.rect.x, conn.rect.y),
								cvPoint(conn.rect.x+conn.rect.width, conn.rect.y+conn.rect.height),
								CV_RGB(0,0,255), 1);
					double minVal, maxVal;
					CvPoint maxPoint, minPoint;

					//void cvMinMaxLoc( const CvArr* A, double* minVal, double* maxVal,
					//								CvPoint* minLoc, CvPoint* maxLoc, const CvArr* mask=0 );
					cvSetImageROI(cvImGray, conn.rect);
					cvMinMaxLoc(cvImGray, &minVal, &maxVal,
							 &minPoint, &maxPoint);
					cvResetImageROI(cvImGray);

					maxPoint.x += conn.rect.x;
					maxPoint.y += conn.rect.y;

					// Find the track
					t_head_track * best = NULL;
					float maxdist = cvImGray->height/8;
					for(it = tracks.begin(); it !=  tracks.end(); it++)
					{
						t_head_track * phead = (*it);
						int dx = phead->curPoint.x - maxPoint.x;
						int dy = phead->curPoint.y - maxPoint.y;
						float dist = sqrtf(dx*dx+dy*dy);
						if( dist < maxdist )
						{
							maxdist = dist;
							best = phead;
						}
					}

					if(!best)
					{
						curID++;
						best = new t_head_track;
						best->trackID = curID;
						fprintf(stderr, "Create track %d\n", best->trackID);
						tracks.push_back(best);
						cvLine(cvIm2,
							   cvPoint(maxPoint.x - 10, maxPoint.y),
							   cvPoint(maxPoint.x + 10, maxPoint.y),
							   CV_RGB(255,0,0),
							   3
							   );
						cvLine(cvIm2,
							   cvPoint(maxPoint.x, maxPoint.y - 10),
							   cvPoint(maxPoint.x, maxPoint.y + 10),
							   CV_RGB(255,0,0),
							   3
							   );
					}

					if(best)
					{
						// append point to trajectory
						best->curPoint = maxPoint;
						best->trajectory.push_back(maxPoint);
						best->trajectory3D.push_back( convertTo3D(maxPoint, maxVal) );
						best->lost = 0;
					}

					fprintf(stderr, "%s:%d Thresh: %d -> %d, Max: %d,%d: %g\n",
							__func__, __LINE__,
							(int)threshold_min, (int)threshold_max,
							maxPoint.x, maxPoint.y, maxVal);
				}
			}
		}
	}


	// Draw all traj
	for(it = tracks.begin(); it !=  tracks.end();)
	{
		t_head_track * phead = (*it);
		if(phead->lost > 5)
		{
			fprintf(stderr, "Remove track %d\n", phead->trackID);
			//cvCircle(cvIm2, phead->curPoint, 10, CV_RGB(255,0,0), 4);
			cvLine(cvIm2,
				   cvPoint(phead->curPoint.x -10, phead->curPoint.y-10),
				   cvPoint(phead->curPoint.x +10, phead->curPoint.y+10),
				   CV_RGB(255,0,0), 4);
			cvLine(cvIm2,
				   cvPoint(phead->curPoint.x +10, phead->curPoint.y-10),
				   cvPoint(phead->curPoint.x -10, phead->curPoint.y+10),
				   CV_RGB(255,0,0), 4);

			it = tracks.erase(it);
		}
		else
		{
			CvScalar color=CV_RGB( (17+phead->trackID * 37) % 255,
								   (57+phead->trackID * 17) % 255,
								   (170+phead->trackID * 23) % 255);

			for(int idx=1; idx<phead->trajectory.size(); ++idx)
			{
				CvPoint cur = phead->trajectory.at(idx);
				CvPoint prev = phead->trajectory.at(idx-1);
				cvLine(cvIm2, prev, cur, color, 2);
			}
			cvCircle(cvIm2, phead->curPoint, 5, color,  2);

			if(phead->trajectory3D.size()>0) {
				CvPoint3D32f lastPt3D = phead->trajectory3D.at(phead->trajectory3D.size() - 1);
				CvFont font;
				cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
				char ztxt[64];
				sprintf(ztxt, "X,Y=%g,%g Z=%g",
						(float)((int)(lastPt3D.x*100.f)/100.f),
						(float)((int)(lastPt3D.y*100.f)/100.f),
						(float)((int)(lastPt3D.z*100.f)/100.f)
						);
				CvPoint dispPt = cvPoint(std::max(10, phead->curPoint.x+10),
										 std::max(10, phead->curPoint.y+20));
				if(dispPt.x > cvIm2->width-200) {
					dispPt.x = cvIm2->width-200;
				}
				if(dispPt.y > cvIm2->height-20) {
					dispPt.y = cvIm2->height-20;
				}

				cvPutText(cvIm2, ztxt, cvPoint(dispPt.x+1, dispPt.y+1),
						  &font, CV_RGB(0,0,0)
						  );
				cvPutText(cvIm2, ztxt, dispPt,
						  &font, color
						  );
			}
			++it;
		}

	}

	//
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


















