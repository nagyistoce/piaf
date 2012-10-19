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

#include <math.h>
#ifdef _CH_
#pragma package <opencv>
#endif

#ifndef _EiC
#include <stdio.h>
#include <math.h>
#include <string.h>
#endif


// include componenet header
#include "SwPluginCore.h"

/* compile with : ./build_all.sh */


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
#define SUBCATEGORY	"Feature"


// function parameters




double hough_rho = 2.0;
double hough_theta = CV_PI/180;
short  hough_thresh = 80;
short  hough_linelength = 20;
short  hough_linegap = 5;

swFuncParams hough_params[] = {
	{"Rho (radius resolution)", swDouble, (void *)&hough_rho},
	{"Theta (angle resolution)", swDouble, (void *)&hough_theta},
	{"Threshold", swS16, (void *)&hough_thresh},
	{"Minimum line length", swS16, (void *)&hough_linelength}, 
	{"Maximum line gap", swS16, (void *)&hough_linegap}

};
void HoughSHT();

swFuncParams houghP_params[] = {
	{"Rho (radius resolution)", swDouble, (void *)&hough_rho},
	{"Theta (angle resolution)", swDouble, (void *)&hough_theta},
	{"Threshold", swS16, (void *)&hough_thresh},
};
void HoughP();

short findSquare_thresh = 50;
float findSquare_contourmin = 1000.f;

swFuncParams findSquare_params[] = {
	{"Threshold", swS16, (void *)&findSquare_thresh},
	{"Minimum perimeter", swFloat, (void *)&findSquare_contourmin}
};
void findSquare();
void findTriangle();


u8 save_snapshots = 0;
short findFace_width = 30;
char * face_list[] = {
		"frontalface_default",
		"frontalface_alt2",
		"frontalface_alt_tree",
		"frontalface_alt",
		"fullbody",
		"eye_tree_eyeglasses",
		"eye",
		"lefteye_2splits",
		"lowerbody",
		"mcs_eyepair_big",
		"mcs_eyepair_small",
		"mcs_lefteye",
		"mcs_mouth",
		"mcs_nose",
		"mcs_righteye",
		"mcs_upperbody",
		"profileface",
		"righteye_2splits",
		"upperbody"
};

swStringListStruct face_profile = {
	19, // nb elements
	0, // default element
	face_list
	};

swFuncParams findFace_params[] = {
	{"Face size (% of height)", swS16, (void *)&findFace_width},
	{"Profile", swStringList, (void *)&face_profile },
	{"Save snapshots", swU8, (void *)&save_snapshots }
};
void findFace();


/* swFunctionDescriptor : 
	char * : function name
	int : number of parameters
	swFuncParams : function parameters
	swType : input type
	swType : output type
	void * : procedure
	*/
swFunctionDescriptor functions[] = {
	{"HoughSHT", 	5,	hough_params, swImage, swImage, &HoughSHT, NULL},
	{"HoughP", 	3,	houghP_params, swImage, swImage, &HoughP, NULL},
	{"Squares", 	2,	findSquare_params, swImage, swImage, &findSquare, NULL},
	{"Triangles", 	2,	findSquare_params, swImage, swImage, &findTriangle, NULL},
	{"Faces",		3,	findFace_params, swImage, swImage, &findFace, NULL}
};
int nb_functions = 5;

/******************* END OF USER SECTION ********************/




IplImage * cvIm1 = NULL;
IplImage * cvGray = NULL;
IplImage * cvIm2 = NULL;
static IplImage * planes1[4];
static IplImage * planes2[4];
CvMemStorage* storage = 0;

void allocateImages()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);

	CvSize size;
	size.width = imIn->width;
	size.height = imIn->height;

	if(!cvIm1) {
		cvIm1 = cvCreateImageHeader(size,  IPL_DEPTH_8U, imIn->depth);
		if(imIn->depth>1) {
			for(int i=0;i<imIn->depth; i++)
				planes1[i] = cvCreateImage(size,  IPL_DEPTH_8U, 1);
			for(int i=imIn->depth; i<4;i++)
				planes1[i] = NULL;
		}
	}
	if(!cvGray) {
		cvGray = cvCreateImage(size,  IPL_DEPTH_8U, 1);
	}

	if(!cvIm2) {
		cvIm2 = cvCreateImageHeader(size,  IPL_DEPTH_8U, imIn->depth);
		if(imIn->depth>1) {
			for(int i=0;i<imIn->depth; i++)
				planes2[i] = cvCreateImage(size,  IPL_DEPTH_8U, 1);
			for(int i=imIn->depth; i<4;i++)
				planes2[i] = NULL;
		}
	}
}

void HoughP()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	allocateImages();
	cvIm1->imageData = (char *)imageIn;
	cvIm2->imageData = (char *)imageOut;

	if(!storage) {
		storage = cvCreateMemStorage(0);
	}

	if(imIn->depth>1) {
		// decompose planes
		cvCvtPixToPlane(cvIm1, planes1[0], planes1[1], planes1[2], planes1[3]);
		for(int p = 0; p<imIn->depth; p++)
		{
  	      	CvSeq* lines = 0;
        
			lines = cvHoughLines2( planes1[p], storage, CV_HOUGH_STANDARD, hough_rho, hough_theta, hough_thresh, 0, 0 );

			memcpy(planes2[p]->imageData, planes1[p]->imageData, imIn->buffer_size / imIn->depth);
			int i;
			for( i = 0; i < lines->total; i++ )
    	    {
				float* line = (float*)cvGetSeqElem(lines,i);
				float rho = line[0];
				float theta = line[1];
				CvPoint pt1, pt2;
				double a = cos(theta), b = sin(theta);
				if( fabs(a) < 0.001 )
				{
					pt1.x = pt2.x = cvRound(rho);
					pt1.y = 0;
					pt2.y = planes2[p]->height;
            	}
            	else if( fabs(b) < 0.001 )
            	{
                	pt1.y = pt2.y = cvRound(rho);
                	pt1.x = 0;
                	pt2.x = planes2[p]->width;
            	}
            	else
            	{
                	pt1.x = 0;
                	pt1.y = cvRound(rho/b);
                	pt2.x = cvRound(rho/a);
                	pt2.y = 0;
            	}
            	cvLine( planes2[p], pt1, pt2, cvScalar(255), 3 );
        	}
		}
		cvCvtPlaneToPix(planes2[0], planes2[1], planes2[2], planes2[3], cvIm2);
	}
	else {
			//CvMemStorage* storage = cvCreateMemStorage(0);
  	      	CvSeq* lines = 0;
        
			lines = cvHoughLines2( cvIm1, storage, CV_HOUGH_STANDARD, hough_rho, hough_theta, hough_thresh, 0, 0 );

			memcpy(cvIm2->imageData, cvIm1->imageData, imIn->buffer_size / imIn->depth);
			int i;
			for( i = 0; i < lines->total; i++ )
    	    {
				float* line = (float*)cvGetSeqElem(lines,i);
				float rho = line[0];
				float theta = line[1];
				CvPoint pt1, pt2;
				double a = cos(theta), b = sin(theta);
				if( fabs(a) < 0.001 )
				{
					pt1.x = pt2.x = cvRound(rho);
					pt1.y = 0;
					pt2.y = cvIm2->height;
            	}
            	else if( fabs(b) < 0.001 )
            	{
                	pt1.y = pt2.y = cvRound(rho);
                	pt1.x = 0;
                	pt2.x = cvIm2->width;
            	}
            	else
            	{
                	pt1.x = 0;
                	pt1.y = cvRound(rho/b);
                	pt2.x = cvRound(rho/a);
                	pt2.y = 0;
            	}
            	cvLine( cvIm2, pt1, pt2, cvScalar(255), 3);
			}
	}
}

void HoughSHT()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	allocateImages();
	cvIm1->imageData = (char *)imageIn;
	cvIm2->imageData = (char *)imageOut;

	// decompose planes
	if(imIn->depth>1) {
		cvCvtPixToPlane(cvIm1, planes1[0], planes1[1], planes1[2], planes1[3]);
	
		for(int p = 0; p<imIn->depth; p++)
		{
			int lines[80];
			int maxlines = 20;
			int nb = cvHoughLinesP(planes1[p], 
				hough_rho, hough_theta, hough_thresh, 
				(int)hough_linelength, (int)hough_linegap, 
				lines, maxlines);
			// draw lines on image
			CvPoint P1, P2;
			P1.y = 0;
			memcpy(planes2[p]->imageData, planes1[p]->imageData, imIn->buffer_size / imIn->depth);
//			memset(planes2[p]->imageData, 0, imIn->buffer_size/imIn->depth);
			for(int l=0;l<nb;l++) {
				P1.x = (int)lines[ 4*l];
				P1.y = (int)lines[ 4*l+1];
				P2.x = (int)lines[ 4*l+2];
				P2.y = (int)lines[ 4*l+3];
				cvLine(planes2[p], P1, P2, cvScalar(255), 2);
			}		
		}
		cvCvtPlaneToPix(planes2[0], planes2[1], planes2[2], planes2[3], cvIm2);
	}
	else
		{
			int lines[80];
			int maxlines = 20;
			int nb = cvHoughLinesP(cvIm1, 
				hough_rho, hough_theta, hough_thresh, 
				(int)hough_linelength, (int)hough_linegap, 
				lines, maxlines);
			// draw lines on image
			CvPoint P1, P2;
			P1.y = 0;
			memcpy(cvIm2->imageData, cvIm1->imageData, imIn->buffer_size / imIn->depth);
//			memset(planes2[p]->imageData, 0, imIn->buffer_size/imIn->depth);
			for(int l=0;l<nb;l++) {
				P1.x = (int)lines[ 4*l];
				P1.y = (int)lines[ 4*l+1];
				P2.x = (int)lines[ 4*l+2];
				P2.y = (int)lines[ 4*l+3];
				cvLine(cvIm2, P1, P2, cvScalar(255), 2);
			}		
		}
	
	memcpy(imageOut, cvIm2->imageData, imIn->buffer_size);	
}

int thresh = 50;
IplImage* img = 0;
IplImage* img0 = 0;
CvPoint points[4];




// helper function:
// finds a cosine of angle between vectors
// from pt0->pt1 and from pt0->pt2 
double angle( CvPoint* pt1, CvPoint* pt2, CvPoint* pt0 )
{
    double dx1 = pt1->x - pt0->x;
    double dy1 = pt1->y - pt0->y;
    double dx2 = pt2->x - pt0->x;
    double dy2 = pt2->y - pt0->y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

// returns sequence of squares detected on the image.
// the sequence is stored in the specified memory storage

IplImage* gray = NULL;
IplImage* tgray = NULL;
    
CvSeq* findSquares4( IplImage* img, CvMemStorage* storage )
{
    swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	CvSeq* contours;
    int i, c, l, N = 5;
    IplImage* timg = cvCloneImage( img );
	CvSize sz = cvSize( img->width & -2, img->height & -2 );
    if(!gray)
    	gray = cvCreateImage( sz, 8, 1 ); 
    CvSeq* result;
    double s, t;
    // create empty sequence that will contain points -
    // 4 points per square (the square's vertices)
    CvSeq* squares = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvPoint), storage );
    
    // select the maximum ROI in the image
    // with the width and height divisible by 2
    cvSetImageROI( img, cvRect( 0, 0, sz.width, sz.height ));
    
    // down-scale and upscale the image to filter out the noise
 //CSE   cvPyrDown( timg, pyr, 7 );
 //CSE    cvPyrUp( pyr, timg, 7 );
    if(!tgray)
		tgray = cvCreateImage( sz, 8, 1 );
    
    // find squares in every color plane of the image
    for( c = 0; c < ( imIn->depth < 4 ? imIn->depth : 3 ); c++ )
    {
        // extract the c-th color plane
        if(imIn->depth>1) {
			cvSetImageCOI( img, c+1 );
	        cvCopy( img, tgray, 0 );
		}
		else
		{
			cvCopy(img, tgray, 0);
		}
        
        // try several threshold levels
        for( l = 0; l < N; l++ )
        {
            // hack: use Canny instead of zero threshold level.
            // Canny helps to catch squares with gradient shading   
            if( l == 0 )
            {
                // apply Canny. Take the upper threshold from slider
                // and set the lower to 0 (which forces edges merging) 
                cvCanny( tgray, gray, 0, thresh, 5 );
                // dilate canny output to remove potential
                // holes between edge segments 
                cvDilate( gray, gray, 0, 1 );
            }
            else
            {
                // apply threshold if l!=0:
                //     tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
                cvThreshold( tgray, gray, (l+1)*255/N, 255, CV_THRESH_BINARY );
            }
            
            // find contours and store them all as a list
            cvFindContours( gray, storage, &contours, sizeof(CvContour),
                CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE );
            
            // test each contour
            while( contours )
            {
                // approximate contour with accuracy proportional
                // to the contour perimeter
                result = cvApproxPoly( contours, sizeof(CvContour), storage,
                    CV_POLY_APPROX_DP, cvContourPerimeter(contours)*0.02, 0 );
                // square contours should have 4 vertices after approximation
                // relatively large area (to filter out noisy contours)
                // and be convex.
                // Note: absolute value of an area is used because
                // area may be positive or negative - in accordance with the
                // contour orientation
                if( result->total == 4 &&
                    fabs(cvContourArea(result,CV_WHOLE_SEQ)) > findSquare_contourmin &&
                    cvCheckContourConvexity(result) )
                {
                    s = 0;
                    
                    for( i = 0; i < 5; i++ )
                    {
                        // find minimum angle between joint
                        // edges (maximum of cosine)
                        if( i >= 2 )
                        {
                            t = fabs(angle(
                            (CvPoint*)cvGetSeqElem( result, i ),
                            (CvPoint*)cvGetSeqElem( result, i-2 ),
                            (CvPoint*)cvGetSeqElem( result, i-1 )));
                            s = s > t ? s : t;
                        }
                    }
                    
                    // if cosines of all angles are small
                    // (all angles are ~90 degree) then write quandrange
                    // vertices to resultant sequence 
                    if( s < 0.3 )
                        for( i = 0; i < 4; i++ )
                            cvSeqPush( squares,
                                (CvPoint*)cvGetSeqElem( result, i ));
                }
                
                // take the next contour
                contours = contours->h_next;
            }
        }
    }
    
    // release all the temporary images
    cvReleaseImage( &timg );
    
    return squares;
}


// the function draws all the squares in the image
void drawSquares( IplImage* img, CvSeq* squares )
{
    CvSeqReader reader;
    int i;
    
    // initialize reader of the sequence
    cvStartReadSeq( squares, &reader, 0 );
    
    // read 4 sequence elements at a time (all vertices of a square)
    for( i = 0; i < squares->total; i += 4 )
    {
        CvPoint* rect = points;
        int count = 4;
        
        // read 4 vertices
        memcpy( points, reader.ptr, squares->elem_size );
        CV_NEXT_SEQ_ELEM( squares->elem_size, reader );
        memcpy( points + 1, reader.ptr, squares->elem_size );
        CV_NEXT_SEQ_ELEM( squares->elem_size, reader );
        memcpy( points + 2, reader.ptr, squares->elem_size );
        CV_NEXT_SEQ_ELEM( squares->elem_size, reader );
        memcpy( points + 3, reader.ptr, squares->elem_size );
        CV_NEXT_SEQ_ELEM( squares->elem_size, reader );
        
        // draw the square as a closed polyline 
	if(img->depth > 8)
        	cvPolyLine( img, &rect, &count, 1, 1, CV_RGB(0,255,0), 3, 8 );
	else
		cvPolyLine( img, &rect, &count, 1, 1, cvScalar(255), 3, 8 );
    }
    
    // show the resultant image
//    cvShowImage("image",cpy);
}



void findSquare()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	allocateImages();
	if(!storage)
		storage = cvCreateMemStorage(0);
    
	
	memcpy( cvIm1->imageData, imageIn, imIn->buffer_size);
	memcpy( cvIm2->imageData, imageIn, imIn->buffer_size);
	
	drawSquares( cvIm2, findSquares4( cvIm1, storage ) );
	
	memcpy(imageOut, cvIm2->imageData, imIn->buffer_size);	
	
}




/************** FIND TRIANGLES *******************/
CvSeq* findTriangles4( IplImage* img, CvMemStorage* storage )
{
    CvSeq* contours;
    int i, c, l, N = 5;
    IplImage* timg = cvCloneImage( img );
	CvSize sz = cvSize( img->width & -2, img->height & -2 );
    swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	
	if(!gray)
    	gray = cvCreateImage( sz, 8, 1 ); 
    CvSeq* result;
    double s, t;
    // create empty sequence that will contain points -
    // 4 points per square (the square's vertices)
    CvSeq* triangles = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvPoint), storage );
    
    // select the maximum ROI in the image
    // with the width and height divisible by 2
    cvSetImageROI( img, cvRect( 0, 0, sz.width, sz.height ));
    
    // down-scale and upscale the image to filter out the noise
 //CSE   cvPyrDown( timg, pyr, 7 );
 //CSE    cvPyrUp( pyr, timg, 7 );
    if(!tgray)
		tgray = cvCreateImage( sz, 8, 1 );
    
    // find squares in every color plane of the image
    for( c = 0; c < ( imIn->depth < 4 ? imIn->depth : 3 ); c++ )
    {
        // extract the c-th color plane
        if(imIn->depth>1) {
			cvSetImageCOI( img, c+1 );
	        cvCopy( img, tgray, 0 );
		}
		else
		{
			cvCopy(img, tgray, 0);
		}
        
        // try several threshold levels
        for( l = 0; l < N; l++ )
        {
            // hack: use Canny instead of zero threshold level.
            // Canny helps to catch squares with gradient shading   
            if( l == 0 )
            {
                // apply Canny. Take the upper threshold from slider
                // and set the lower to 0 (which forces edges merging) 
                cvCanny( tgray, gray, 0, thresh, 5 );
                // dilate canny output to remove potential
                // holes between edge segments 
                cvDilate( gray, gray, 0, 1 );
            }
            else
            {
                // apply threshold if l!=0:
                //     tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
                cvThreshold( tgray, gray, (l+1)*255/N, 255, CV_THRESH_BINARY );
            }
            
            // find contours and store them all as a list
            cvFindContours( gray, storage, &contours, sizeof(CvContour),
                CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE );
            
            // test each contour
            while( contours )
            {
                // approximate contour with accuracy proportional
                // to the contour perimeter
                result = cvApproxPoly( contours, sizeof(CvContour), storage,
                    CV_POLY_APPROX_DP, cvContourPerimeter(contours)*0.02, 0 );
                // square contours should have 4 vertices after approximation
                // relatively large area (to filter out noisy contours)
                // and be convex.
                // Note: absolute value of an area is used because
                // area may be positive or negative - in accordance with the
                // contour orientation
                if( result->total == 3 &&
                    fabs(cvContourArea(result,CV_WHOLE_SEQ)) > findSquare_contourmin &&
                    cvCheckContourConvexity(result) )
                {
                    s = 0;
                    
                    for( i = 0; i < 5; i++ )
                    {
                        // find minimum angle between joint
                        // edges (maximum of cosine)
                        if( i >= 2 )
                        {
                            t = fabs(fabs(angle(
                            (CvPoint*)cvGetSeqElem( result, i ),
                            (CvPoint*)cvGetSeqElem( result, i-2 ),
                            (CvPoint*)cvGetSeqElem( result, i-1 )))
								- 0.5);
                            s = s > t ? s : t;
                        }
                    }
                    
                    // if cosines of all angles are small
                    // (all angles are ~90 degree) then write quandrange
                    // vertices to resultant sequence 
                    if( s < 0.3 )
                        for( i = 0; i < 4; i++ )
                            cvSeqPush( triangles,
                                (CvPoint*)cvGetSeqElem( result, i ));
                }
                
                // take the next contour
                contours = contours->h_next;
            }
        }
    }
    
    // release all the temporary images
    cvReleaseImage( &timg );
    
    return triangles;
}






// the function draws all the squares in the image
void drawTriangles( IplImage* img, CvSeq* triangles )
{
    CvSeqReader reader;
    int i;
    
    // initialize reader of the sequence
    cvStartReadSeq( triangles, &reader, 0 );
    
    // read 4 sequence elements at a time (all vertices of a square)
    for( i = 0; i < triangles->total; i += 3 )
    {
        CvPoint* rect = points;
        int count = 3;
        
        // read 3 vertices
        memcpy( points, reader.ptr, triangles->elem_size );
        CV_NEXT_SEQ_ELEM( triangles->elem_size, reader );
        memcpy( points + 1, reader.ptr, triangles->elem_size );
        CV_NEXT_SEQ_ELEM( triangles->elem_size, reader );
        memcpy( points + 2, reader.ptr, triangles->elem_size );
        CV_NEXT_SEQ_ELEM( triangles->elem_size, reader );
        
        // draw the square as a closed polyline 
	if(img->depth > 8)
        	cvPolyLine( img, &rect, &count, 1, 1, CV_RGB(0,255,0), 3, 8 );
	else
		cvPolyLine( img, &rect, &count, 1, 1, cvScalar(255), 3, 8 );
    }
    
    // show the resultant image
//    cvShowImage("image",cpy);
}



void findTriangle()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	allocateImages();
	if(!storage) {
		storage = cvCreateMemStorage(0);
	}
	
	memcpy( cvIm1->imageData, imageIn, imIn->buffer_size);
	memcpy( cvIm2->imageData, imageIn, imIn->buffer_size);
	
	drawTriangles( cvIm2, findTriangles4( cvIm1, storage ) );
	
	memcpy(imageOut, cvIm2->imageData, imIn->buffer_size);		
}
CvHaarClassifierCascade* cascade = NULL;


void findFace() {
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	static int face_profile_last = -1;

	allocateImages();
	cvIm1->imageData = (char *)imageIn;
	cvIm2->imageData = (char *)imageOut;
#define CASCADE_FORMAT	"/usr/local/piaf/haarcascade_%s.xml"

	if(face_profile.curitem != face_profile_last) {
		//
		face_profile_last = face_profile.curitem;
		cvRelease((void **)&cascade);
		cascade = NULL;
	}

	if(!cascade) {
		char cascade_name[512];
		sprintf(cascade_name, CASCADE_FORMAT, face_list[ face_profile.curitem ]);
		fprintf(stderr, "%s %s:%d : loading file '%s'\n",
				__FILE__, __func__, __LINE__, cascade_name);

		cascade = (CvHaarClassifierCascade*) cvLoad (cascade_name, 0, 0, 0);
		if(!cascade) {
			fprintf(stderr, "%s:%d : Cannot load '%s'\n", __func__, __LINE__,
					cascade_name);
		}
	}

	if(!storage) {
		storage = cvCreateMemStorage(0);
	}

	if(imIn->depth > 1) {
		cvCvtColor(cvIm1, cvGray, CV_BGR2GRAY);
	} else {
		memcpy(cvGray->imageData, imageIn, imIn->buffer_size);
	}

	// Detect faces in gray image
	cvCopy(cvIm1, cvIm2);

	float scale = 1.f;
	int face_width = findFace_width * cvGray->height/100;
	CvSeq* faces = cvHaarDetectObjects (cvGray, cascade, storage,
												1.1, 2, CV_HAAR_DO_CANNY_PRUNING,
												cvSize (face_width, face_width));

	// draw faces
	for (int i = 0; i < (faces ? faces->total : 0); i++)
	{
		CvRect* r = (CvRect*) cvGetSeqElem (faces, i);
		if(r) {
			CvPoint center;
			int radius;
			center.x = cvRound( //(cvGray->width - r->width*0.5 - r->x) *scale
					(r->width*0.5 + r->x)*scale
					);
			center.y = cvRound((r->y + r->height*0.5)*scale);
			radius = cvRound((r->width + r->height)*0.25*scale);

			cvCircle (cvIm2, center, radius, CV_RGB(0,255,0), 3, 8, 0 );
		}
	}

}







/*********************************************************************************/

void signalhandler(int sig)
{
	fprintf(stderr, "[%s] ================== RECEIVED SIGNAL %d = '%s' From process %d ==============\n",
			__FILE__,
			sig, sys_siglist[sig], getpid());
	signal(sig, signalhandler);
	
	if(sig != SIGUSR1) {
		fprintf(stderr, "[%s] RECEIVED SIGNAL %d = '%s' process %d => exit(0)\n",
				__FILE__,
				sig, sys_siglist[sig], getpid());
		exit(0);
	}
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
