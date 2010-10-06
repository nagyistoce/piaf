/***************************************************************************
                          cversatz.h  -  description
                             -------------------
    begin                : Tue Jan 7 2003
    copyright            : (C) 2003 by Christophe Seyve
    email                : christophe.seyve@sisell.com
 ***************************************************************************/

#ifndef __CVERSATZ__
#define __CVERSATZ__

#ifndef __WITHOUT_OPENCV__
#include <cv.hpp>

#else

#include <stdio.h>
#include <stdlib.h>


/* there are some structured used defined in cv.hpp.
  Though we can't used opencv with PowerPC processors, lets redefine
  the structures used in this file

Missing :
Structures:
	CvSize
	CvRect
	CvConnectedComp
	CvPoint
	CvKalman
	IplImage
Functions : principaly
	cvCreateImage
	cvReleaseImage
	cvRectangle
	cvAcc
	cvSquareAcc
Defines
*/


/************** DEFINES ***************/
             /*
 * The following definitions (until #endif)
 * is an extract from IPL headers.
 * Copyright (c) 1995 Intel Corporation.
 */
// From    OpenCV-0.9.4/cv/include/cvtypes.h
#define IPL_DEPTH_SIGN 0x80000000

#define IPL_DEPTH_1U     1
#define IPL_DEPTH_8U     8
#define IPL_DEPTH_16U   16
#define IPL_DEPTH_32F   32

#define IPL_DEPTH_8S  (IPL_DEPTH_SIGN| 8)
#define IPL_DEPTH_16S (IPL_DEPTH_SIGN|16)
#define IPL_DEPTH_32S (IPL_DEPTH_SIGN|32)


/************* STRUCTURES *****************/
// From OpenCV-0.9.4/cv/include/cvtypes.h
typedef struct CvPoint
{
    int x;
    int y;
}
CvPoint;

typedef struct CvRect
{
    int x;
    int y;
    int width;
    int height;
}
CvRect;

typedef struct
{
    int width;
    int height;
}
CvSize;


// used for returning segmented regions
typedef struct CvConnectedComp
{
    double area;  /* area of the connected component  */
    double value; /* average brightness of the connected component
                     (or packed RGB color) */
    CvRect rect;  /* ROI of the component  */
    struct CvSeq* contour; /* optional component boundary
                     (the contour might have child contours corresponding to the holes)*/
}
CvConnectedComp;

//
typedef struct _IplImage {
    int  nSize;         /* sizeof(IplImage) */
    int  ID;            /* version (=0)*/
    int  nChannels;     /* Most of OpenCV functions support 1,2,3 or 4 channels */
    int  alphaChannel;  /* ignored by OpenCV */
    int  depth;         /* pixel depth in bits: IPL_DEPTH_8U, IPL_DEPTH_8S, IPL_DEPTH_16S,
                           IPL_DEPTH_32S, IPL_DEPTH_32F and IPL_DEPTH_64F are supported */
    char colorModel[4]; /* ignored by OpenCV */
    char channelSeq[4]; /* ditto */
    int  dataOrder;     /* 0 - interleaved color channels, 1 - separate color channels.
                           cvCreateImage can only create interleaved images */
    int  origin;        /* 0 - top-left origin,
                           1 - bottom-left origin (Windows bitmaps style) */
    int  align;         /* Alignment of image rows (4 or 8).
                           OpenCV ignores it and uses widthStep instead */
    int  width;         /* image width in pixels */
    int  height;        /* image height in pixels */
//CSE    struct _IplROI *roi;/* image ROI. if NULL, the whole image is selected */
    struct _IplImage *maskROI; /* must be NULL */
    void  *imageId;     /* ditto */
    struct _IplTileInfo *tileInfo; /* ditto */
    int  imageSize;     /* image data size in bytes
                           (==image->height*image->widthStep
                           in case of interleaved data)*/
    char *imageData;  /* pointer to aligned image data */
    int  widthStep;   /* size of aligned image row in bytes */
    int  BorderMode[4]; /* ignored by OpenCV */
    int  BorderConst[4]; /* ditto */
    char *imageDataOrigin; /* pointer to very origin of image data
                              (not necessarily aligned) -
                              needed for correct deallocation */
}
IplImage;

typedef unsigned char uchar;

typedef struct CvMat
{
    int type;
    int step;

    /* for internal use only */
    int* refcount;

    union
    {
        uchar* ptr;
        short* s;
        int* i;
        float* fl;
        double* db;
    } data;

#ifdef __cplusplus
    union
    {
        int rows;
        int height;
    };

    union
    {
        int cols;
        int width;
    };
#else
    int rows;
    int cols;
#endif

} CvMat;

typedef struct CvKalman
{
    int MP;                     /* number of measurement vector dimensions */
    int DP;                     /* number of state vector dimensions */
    int CP;                     /* number of control vector dimensions */

    /* backward compatibility fields */
#if 1
    float* PosterState;         /* =state_pre->data.fl */
    float* PriorState;          /* =state_post->data.fl */
    float* DynamMatr;           /* =transition_matrix->data.fl */
    float* MeasurementMatr;     /* =measurement_matrix->data.fl */
    float* MNCovariance;        /* =measurement_noise_cov->data.fl */
    float* PNCovariance;        /* =process_noise_cov->data.fl */
    float* KalmGainMatr;        /* =gain->data.fl */
    float* PriorErrorCovariance;/* =error_cov_pre->data.fl */
    float* PosterErrorCovariance;/* =error_cov_post->data.fl */
    float* Temp1;               /* temp1->data.fl */
    float* Temp2;               /* temp2->data.fl */
#endif

    CvMat* state_pre;           /* predicted state (x'(k)):
                                    x(k)=A*x(k-1)+B*u(k) */
    CvMat* state_post;          /* corrected state (x(k)):
                                    x(k)=x'(k)+K(k)*(z(k)-H*x'(k)) */
    CvMat* transition_matrix;   /* state transition matrix (A) */
    CvMat* control_matrix;      /* control matrix (B)
                                   (it is not used if there is no control)*/
    CvMat* measurement_matrix;  /* measurement matrix (H) */
    CvMat* process_noise_cov;   /* process noise covariance matrix (Q) */
    CvMat* measurement_noise_cov; /* measurement noise covariance matrix (R) */
    CvMat* error_cov_pre;       /* priori error estimate covariance matrix (P'(k)):
                                    P'(k)=A*P(k-1)*At + Q)*/
    CvMat* gain;                /* Kalman gain matrix (K(k)):
                                    K(k)=P'(k)*Ht*inv(H*P'(k)*Ht+R)*/
    CvMat* error_cov_post;      /* posteriori error estimate covariance matrix (P(k)):
                                    P(k)=(I-K(k)*H)*P'(k) */
    CvMat* temp1;               /* temporary matrices */
    CvMat* temp2;
    CvMat* temp3;
    CvMat* temp4;
    CvMat* temp5;

}
CvKalman;


/***************** FUNCTIONS ******************/
IplImage * cvCreateImage(CvSize size, int depth, int channels);

void
cvReleaseImage( IplImage** image );
void
cvRectangle( IplImage* img, CvPoint pt1, CvPoint pt2,
             double color, int thickness );

void cvAcc( IplImage* arr, IplImage* sumarr, const void* maskarr );
void cvSquareAcc( IplImage* arr, IplImage* sumarr, const void* maskarr );

int cvRound(float f);


#endif // opencv
#endif

