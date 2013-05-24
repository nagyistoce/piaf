/***************************************************************************
	 opencv_file_acquisition.cpp - OpenCV acquisition class for video replay
									 using highgui
							 -------------------
	begin                : Fri May 17 2013
	copyright            : (C) 2013 by Christophe Seyve (CSE)
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

#include "piaf-common.h"
#include "opencv_file_acquisition.h"

#include "file_video_acquisition_factory.h"

using namespace std;

//---------------------------------------------------------------------------
// Macros
//---------------------------------------------------------------------------


//return rc;

#define OPENCV_PRINTF(...) { \
	fprintf(stderr, "[OpenCV %p '%s']::%s:%d : ", this, mVideoFilePath.c_str(), __func__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); \
}

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------

#define EXTENSION_OPENCV	"mpeg"

std::string OpenCVFileVideoAcquisition::mRegistered =
	FileVideoAcquisitionFactory::RegisterCreatorFunction(
		(EXTENSION_OPENCV), OpenCVFileVideoAcquisition::creatorFunction);


FileVideoAcquisition* OpenCVFileVideoAcquisition::creatorFunction(std::string path)
{
	PIAF_MSG(SWLOG_INFO, "Create OpenCVFileVideoAcquisition(path='%s')", path.c_str() );
	return (FileVideoAcquisition *)new OpenCVFileVideoAcquisition( path.c_str() );
}


OpenCVFileVideoAcquisition::OpenCVFileVideoAcquisition(const char * filename)
{
	init();
	PIAF_MSG(SWLOG_INFO, "Opening file '%s'...", filename);
	mVideoFilePath = std::string( filename );

	// Open filename
	try {
		mCapture = cvCaptureFromAVI( mVideoFilePath.c_str() );
	}
	catch(cv::Exception e)
	{
		PIAF_MSG(SWLOG_ERROR, "Cannot open file '%s'", filename);
	}

	int retgrab = grab();
	OPENCV_PRINTF("=> mCapture = %p  / grab() returned %d => image = %dx%dx%dbx%d",
				  mCapture,
				  retgrab,
				  (m_imageRaw ? m_imageRaw->width : -1),
				  (m_imageRaw ? m_imageRaw->height : -1),
				  (m_imageRaw ? m_imageRaw->depth : -1),
				  (m_imageRaw ? m_imageRaw->nChannels : -1)
				  );
}

OpenCVFileVideoAcquisition::~OpenCVFileVideoAcquisition()
{
	stopAcquisition();

	purge();

	//purgeVirtualDevice();
}

void OpenCVFileVideoAcquisition::purge()
{
	swReleaseImage(&m_imageRGB32);
	swReleaseImage(&m_imageRaw);
	swReleaseImage(&m_imageY);

}


void OpenCVFileVideoAcquisition::init()
{
//	initVirtualDevice();

	mCapture = NULL;
	m_imageRGB32 = m_imageRaw =
		m_imageY = NULL;

	memset(&m_video_properties, 0, sizeof(t_video_properties));

	mImageSize = cvSize(0,0);
}

/** \brief Start acquisition */
int OpenCVFileVideoAcquisition::startAcquisition()
{
	OPENCV_PRINTF("Starting acquisition ...");

	if (!mCapture )
	{
		OPENCV_PRINTF("Could not start acquisition ...");
		return -1;
	}

	// update video properties
	updateVideoProperties();

	OPENCV_PRINTF("init OK !\n");

	return 0;
}


/* Function called by the doc (parent) thread */
int OpenCVFileVideoAcquisition::grab()
{
	if(!mCapture) {
		OPENCV_PRINTF("Capture is not initialized");
		return -1;
	}

	// Wait for acq
	IplImage* grabbed = cvQueryFrame( mCapture );
	if(!grabbed)
	{
		OPENCV_PRINTF("grab failed => EOF");
		swReleaseImage(&m_imageRaw);
		return -1;
	}
	OPENCV_PRINTF("grab succeed => %dx%dx%dbx%d",
				  grabbed->width, grabbed->height,
				  grabbed->depth, grabbed->nChannels
				  );

	if(m_imageRaw &&
			(grabbed->width != m_imageRaw->width
			 || grabbed->height != m_imageRaw->height )
			) {
		purge();
	}

	if(!m_imageRaw)
	{
		mImageSize = cvGetSize(grabbed);
		OPENCV_PRINTF("Cloning grabbed to m_imageRaw=%dx%dx%dbx%d",
					  grabbed->width, grabbed->height, grabbed->depth, grabbed->nChannels);
		m_imageRaw = tmCloneImage(grabbed);
		if(m_imageRaw->nChannels == 3)
		{
			// force color model to be BGR
	//		strcpy(m_imageRaw->colorModel, "BGR");
		}
	}

	cvCopy(grabbed, m_imageRaw);

	/// FIXME: debug only
	cvSaveImage(SHM_DIRECTORY "opencv_file.png", m_imageRaw);

	return 0;
}

/* Return image size */
CvSize OpenCVFileVideoAcquisition::getImageSize()
{
	return mImageSize;
}

/* Stop acquisition */
int OpenCVFileVideoAcquisition::stopAcquisition()
{
	if(mCapture)
	{
		cvReleaseCapture(&mCapture);
		mCapture = NULL;
	}
	return 0;
}





IplImage * OpenCVFileVideoAcquisition::readImageRaw()
{
	fprintf(stderr, "%s %s:%d : NOT IMPLEMENTED\n", __FILE__, __func__, __LINE__);
	return NULL; /// \todo FIXME : return raw value
}
/** \brief Grabs one image and convert to RGB32 coding format
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * OpenCVFileVideoAcquisition::readImageRGB32()
{
	if(!m_imageRaw)
	{
		OPENCV_PRINTF("could not grab because m_imageRaw=NULL");
		return NULL;
	}

	if(m_imageRGB32)
	{
		CvSize imageSize = cvGetSize(m_imageRGB32);
		if(imageSize.width != mImageSize.width
				|| imageSize.width != mImageSize.width )
		{
			OPENCV_PRINTF("Size changed => purge");
			purge();
		}
	}


	if(!m_imageRGB32)
	{
		OPENCV_PRINTF("create BGR32 image : %dx%dx8bx4",
					  mImageSize.width, mImageSize.height);
		m_imageRGB32 = swCreateImage(mImageSize, IPL_DEPTH_8U, 4);

		// set the alpha channel
		memset(m_imageRGB32->imageData, 255,
			   sizeof(char) * (m_imageRGB32->widthStep * m_imageRGB32->height));
	}

	tmConvert(m_imageRaw, m_imageRGB32);

	return m_imageRGB32;
}


/** \brief Grabs one image and convert to grayscale coding format
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * OpenCVFileVideoAcquisition::readImageY()
{
	if(!m_imageRaw)
	{
		OPENCV_PRINTF("could not grab because m_imageRaw=NULL");
		return NULL;
	}



	if(m_imageY)
	{
		CvSize imageSize = cvGetSize(m_imageY);
		if(imageSize.width != mImageSize.width
				|| imageSize.width != mImageSize.width )
		{
			OPENCV_PRINTF("Size changed => purge");
			purge();
		}
	}

	if(!m_imageY)
	{
		OPENCV_PRINTF("Create grayscale image %d x %d x 1",
					  mImageSize.width, mImageSize.height);
		m_imageY = swCreateImage(mImageSize, IPL_DEPTH_8U, 1);
	}

	tmConvert(m_imageRaw, m_imageY);

	return m_imageY;
}


/* Update and return video properties */
t_video_properties OpenCVFileVideoAcquisition::updateVideoProperties()
{
	if(!mCapture) {
		OPENCV_PRINTF("Capture is not initialized");
		memset(&m_video_properties, 0, sizeof(t_video_properties));
		return m_video_properties;
	}

//	m_video_properties. = cvGetCaptureProperty(mCapture, );
	m_video_properties.pos_msec = cvGetCaptureProperty(mCapture, CV_CAP_PROP_POS_MSEC);
	m_video_properties.pos_frames =	cvGetCaptureProperty(mCapture, CV_CAP_PROP_POS_FRAMES);
	m_video_properties.pos_avi_ratio =	-1.;
	m_video_properties.frame_width =	mImageSize.width;
	m_video_properties.frame_height =	mImageSize.height;
	m_video_properties.fps =	cvGetCaptureProperty(mCapture, CV_CAP_PROP_FPS);
	m_video_properties.fourcc_dble =	cvGetCaptureProperty(mCapture, CV_CAP_PROP_FOURCC );/*4-character code of codec */
//		char   fourcc[5] =	cvGetCaptureProperty(mCapture, FOURCC coding CV_CAP_PROP_FOURCC 4-character code of codec */
//		char   norm[8] =		cvGetCaptureProperty(mCapture, Norm: pal, ntsc, secam */
	m_video_properties.frame_count =	cvGetCaptureProperty(mCapture, CV_CAP_PROP_FRAME_COUNT);
	//m_video_properties.format =			cvGetCaptureProperty(mCapture, CV_CAP_PROP_FORMAT );/*The format of the Mat objects returned by retrieve() */
	m_video_properties.mode =	cvGetCaptureProperty(mCapture, CV_CAP_PROP_MODE);; /*A backend-specific value indicating the current capture mode */
//	m_video_properties.brightness =		cvGetCaptureProperty(mCapture, CV_CAP_PROP_BRIGHTNESS );/*Brightness of the image (only for cameras) */
//	m_video_properties.contrast =		cvGetCaptureProperty(mCapture, CV_CAP_PROP_CONTRAST );/*Contrast of the image (only for cameras) */
//	m_video_properties.saturation =		cvGetCaptureProperty(mCapture, CV_CAP_PROP_SATURATION );/*Saturation of the image (only for cameras) */
//	m_video_properties.hue =			cvGetCaptureProperty(mCapture, CV_CAP_PROP_HUE );/*Hue of the image (only for cameras) */
//	m_video_properties.gain =			cvGetCaptureProperty(mCapture, CV_CAP_PROP_GAIN );/*Gain of the image (only for cameras) */
//	m_video_properties.exposure =		cvGetCaptureProperty(mCapture, CV_CAP_PROP_EXPOSURE );/*Exposure (only for cameras) */
//	m_video_properties.convert_rgb =	cvGetCaptureProperty(mCapture, CV_CAP_PROP_CONVERT_RGB );/*Boolean flags indicating whether images should be converted to RGB */
//	m_video_properties.white_balance = cvGetCaptureProperty(mCapture, CV_CAP_PROP_WHITE_BALANCE );/*Currently unsupported */

	/*! CV_CAP_PROP_RECTIFICATION TOWRITE (note: only supported by DC1394 v 2.x backend currently)
	– Property identifier. Can be one of the following:*/
	m_video_properties.min_width = mImageSize.width;
	m_video_properties.min_height = mImageSize.height;
	m_video_properties.max_width = mImageSize.width;
	m_video_properties.max_height = mImageSize.height;

	return m_video_properties;
}


/** @brief Set video properties (not updated) */
int OpenCVFileVideoAcquisition::setVideoProperties(t_video_properties props)
{
	OPENCV_PRINTF("not implemented for props=");
	printVideoProperties(&props);
	return 0;
}


bool OpenCVFileVideoAcquisition::endOfFile()
{
	// if no read yet, try again, else it is the end
	if(!m_imageRaw) { return !grab(); }

	OPENCV_PRINTF("not implemented");
	return (m_imageRaw == NULL);
}

/** @brief Go to absolute position in file (in bytes from start) */
void OpenCVFileVideoAcquisition::rewindToPosMovie(unsigned long long position)
{
	OPENCV_PRINTF("not implemented for pos=%llu",
				  position);

}

/** @brief Read next frame */
bool OpenCVFileVideoAcquisition::GetNextFrame()
{
	return (grab() >= 0);
}

/* go to frame position in file */
void OpenCVFileVideoAcquisition::setAbsoluteFrame(int frame)
{
	int ret = cvSetCaptureProperty(mCapture, CV_CAP_PROP_POS_FRAMES, frame);
	OPENCV_PRINTF("cvSetCaptureProperty(mCapture, CV_CAP_PROP_POS_FRAMES, frame=%d); => ret=%d",
				  frame,
				  ret);
	GetNextFrame();
}

void OpenCVFileVideoAcquisition::rewindMovie()
{
	setAbsoluteFrame(0);
}

/* Use sequential mode
	If true, grabbing is done when a new image is requested.
	Else a thread is started to grab */
void OpenCVFileVideoAcquisition::setSequentialMode(bool on)
{
	mSequentialMode = on;
	OPENCV_PRINTF("Set sequential mode to %c => UNUSED",
				  mSequentialMode ? 'T' : 'F');
}

/** Return the absolute position to be stored in database (needed for post-processing or permanent encoding) */
unsigned long long OpenCVFileVideoAcquisition::getAbsolutePosition()
{
	// update just this value
	m_video_properties.frame_count = cvGetCaptureProperty(mCapture, CV_CAP_PROP_FRAME_COUNT);
	return m_video_properties.frame_count;
}

/** go to frame position in file
 */
void OpenCVFileVideoAcquisition::setAbsolutePosition(unsigned long long newpos)
{
	// Set the position in file equal to the number of frame in the case of Highgui
	// because we can't get more info through the API
	setAbsoluteFrame(newpos);
}



