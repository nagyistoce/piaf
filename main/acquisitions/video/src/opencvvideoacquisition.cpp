/***************************************************************************
	 OpenCVVideoAcquisition.h  - OpenCV Capture acquisition class for video capture
							 -------------------
	begin                : Thu Dec 2 2010
	copyright            : (C) 2010 by Christophe Seyve (CSE)
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
#include "opencvvideoacquisition.h"
#include "swvideodetector.h"

OpenCVVideoAcquisition::OpenCVVideoAcquisition(int idx_device)
{
	m_capture = NULL;
	m_iplImage = m_grayImage = NULL;
	m_idx_device = idx_device;
	memset(&m_video_properties, 0, sizeof(t_video_properties));
	m_imageSize = cvSize(0,0);

	fprintf(stderr, "OpenCVAcq::%s:%d : create device # %d\n",
			__func__, __LINE__, idx_device);

	// open any type of device and index of desired device
//#ifdef cv::Exception
	try {
		m_capture = cvCreateCameraCapture(CV_CAP_ANY + m_idx_device );
	} catch (cv::Exception e) {
		fprintf(stderr, "[VidAcq] : VDopen ERROR : CAUGHT EXCEPTION WHILE OPENING DEVICE  [%d]\n",
				idx_device
				); fflush(stderr);

		return;
	}
//#else
//	m_capture = cvCreateCameraCapture(CV_CAP_ANY + m_idx_device );
//#endif
	fprintf(stderr, "OpenCVAcq::%s:%d : create device # %d =W capture=%p\n",
			__func__, __LINE__,
			idx_device, m_capture);

}

OpenCVVideoAcquisition::~OpenCVVideoAcquisition()
{
	stopAcquisition();

	if(m_iplImage) cvReleaseImage(&m_iplImage );
	if(m_grayImage) cvReleaseImage(&m_grayImage);
	m_iplImage = m_grayImage = NULL;
}

/** \brief Use sequential mode
	If true, grabbing is done when a new image is requested.
	Else a thread is started to grab */
void OpenCVVideoAcquisition::setSequentialMode(bool on)
{

}

/** \brief Start acquisition */
int OpenCVVideoAcquisition::startAcquisition()
{
	m_imageSize.width = m_imageSize.height = 0;

	fprintf(stderr, "[VAcq]::%s:%d : capture=m_capture=%p\n", __func__, __LINE__,
			m_capture);
	if(m_capture) {
		fprintf(stderr, "[VidAcq] : VDopen SUCCESS : OPEN DEVICE  [%d]\n",
				m_idx_device
				); fflush(stderr);

	} else {
		fprintf(stderr, "[VidAcq] : VDopen ERROR : CANNOT OPEN DEVICE  [%d]\n",
				m_idx_device
				); fflush(stderr);
		return -1;
	}


	fprintf(stderr, "[VidAcq] : VDopen : capture first frame to see if it's posisble\n"); fflush(stderr);
	int ret = grab();
	if(ret >= 0) {
		fprintf(stderr, "[VidAcq] : VDIsInitialised...\n"); fflush(stderr);
		m_captureIsInitialised = 1;
	} else {
		m_captureIsInitialised = 0;
	}

	// update video properties
	updateVideoProperties();

	return 0;
}

int OpenCVVideoAcquisition::grab() {
	// Grab image
	if( !cvGrabFrame( m_capture ) ) {
		fprintf(stderr, "[VA ] %s:%d : capture=%p FAILED\n", __func__, __LINE__,
				m_capture);

		return -1;
	} else {
		IplImage * frame = cvRetrieveFrame( m_capture );
		if(frame) {
			m_imageSize.width = frame->width;
			m_imageSize.height = frame->height;
		} else {

			fprintf(stderr, "[VA ] %s:%d : capture=%p cvRetrieveFrame FAILED\n",
					__func__, __LINE__,
					m_capture);
		}
	}

	return 0;
}

/* Return image size */
CvSize OpenCVVideoAcquisition::getImageSize()
{
	return m_imageSize;
}

/* Stop acquisition */
int OpenCVVideoAcquisition::stopAcquisition()
{
	if(m_capture) {
		cvReleaseCapture( &m_capture );
		m_capture = NULL;
	}

	m_captureIsInitialised = 0;

	return 0;
}

/** \brief Grabs one image and convert to RGB32 coding format
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * OpenCVVideoAcquisition::readImageRGB32()
{
	if(!m_captureIsInitialised) return NULL;

	int ret = cvGrabFrame( m_capture );
	if(ret == 0) {
		fprintf(stderr, "[VidAcq]::%s:%d : ERROR : cvGrabFrame returned %d !\n",
				__func__, __LINE__, ret);
		return NULL;
	}
	IplImage * frame = cvRetrieveFrame( m_capture );
	if(!m_iplImage && frame) {
		fprintf(stderr, "[VidAcq]::%s:%d : created m_iplImage %d x %d x %d !\n",
				__func__, __LINE__,
				frame->width, frame->height, frame->nChannels);

		m_iplImage = swCreateImage(cvGetSize(frame), IPL_DEPTH_8U,
								   frame->nChannels );//== 1 ? 1 : 4);

		fprintf(stderr, "[VidAcq]::%s:%d : created m_iplImage %d x %d x %d !\n",
				__func__, __LINE__,
				m_iplImage->width, m_iplImage->height, m_iplImage->nChannels);
// FIXME : fourcc
		if(m_iplImage->nChannels == 4) {
			m_video_properties.fourcc_dble = CV_FOURCC('B', 'G', 'R', 'A'); //VIDEO_PALETTE_RGB32;
		} else if(m_iplImage->nChannels == 3) {
			m_video_properties.fourcc_dble = CV_FOURCC('R', 'G', 'B', '\0'); //VIDEO_PALETTE_RGB24;
		} else {
			m_video_properties.fourcc_dble = CV_FOURCC('G', 'R', 'A', 'Y');//VIDEO_PALETTE_GREY;
		}
	}

	if(frame)
	{
		if(frame->nChannels == m_iplImage->nChannels)
		{
			cvCopy(frame, m_iplImage);
		}
		else
		{
			cvConvert(frame, m_iplImage);
		}
	}

	return m_iplImage;
}

/** \brief Grabs one image and convert to grayscale coding format
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * OpenCVVideoAcquisition::readImageY()
{
	if(!m_captureIsInitialised) return NULL;
	if(!m_grayImage)
	{
		m_grayImage = cvCreateImage(cvGetSize(m_iplImage), IPL_DEPTH_8U, 1);
	}
	cvConvert(m_iplImage, m_grayImage);

	return NULL;
}

/** \brief Grabs one image of depth buffer
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * OpenCVVideoAcquisition::readImageDepth()
{
	return NULL;
}


/** @brief Get video properties (not updated) */
t_video_properties OpenCVVideoAcquisition::getVideoProperties()
{
	return m_video_properties;
}

/** @brief Update and return video properties */
t_video_properties OpenCVVideoAcquisition::updateVideoProperties()
{
	if(!m_capture) {
		memset(&m_video_properties, 0, sizeof(t_video_properties));
		return m_video_properties;
	}
		//	m_video_properties. = cvGetCaptureProperty(m_capture, );
	m_video_properties.pos_msec =		cvGetCaptureProperty(m_capture, CV_CAP_PROP_POS_MSEC );/*Film current position in milliseconds or video capture timestamp */
	m_video_properties.pos_frames =		cvGetCaptureProperty(m_capture, CV_CAP_PROP_POS_FRAMES );/*0-based index of the frame to be decoded/captured next */
	m_video_properties.pos_avi_ratio =	cvGetCaptureProperty(m_capture, CV_CAP_PROP_POS_AVI_RATIO );/*Relative position of the video file (0 - start of the film, 1 - end of the film) */
	m_video_properties.frame_width =	cvGetCaptureProperty(m_capture, CV_CAP_PROP_FRAME_WIDTH );/*Width of the frames in the video stream */
	m_video_properties.frame_height =	cvGetCaptureProperty(m_capture, CV_CAP_PROP_FRAME_HEIGHT );/*Height of the frames in the video stream */
	m_video_properties.fps =			cvGetCaptureProperty(m_capture, CV_CAP_PROP_FPS );/*Frame rate */
	m_video_properties.fourcc_dble =	cvGetCaptureProperty(m_capture, CV_CAP_PROP_FOURCC );/*4-character code of codec */
//		char   fourcc[5] =	cvGetCaptureProperty(m_capture, FOURCC coding CV_CAP_PROP_FOURCC 4-character code of codec */
//		char   norm[8] =		cvGetCaptureProperty(m_capture, Norm: pal, ntsc, secam */
	m_video_properties.frame_count =	cvGetCaptureProperty(m_capture, CV_CAP_PROP_FRAME_COUNT );/*Number of frames in the video file */
	m_video_properties.format =			cvGetCaptureProperty(m_capture, CV_CAP_PROP_FORMAT );/*The format of the Mat objects returned by retrieve() */
	m_video_properties.mode =			cvGetCaptureProperty(m_capture, CV_CAP_PROP_MODE );/*A backend-specific value indicating the current capture mode */
	m_video_properties.brightness =		cvGetCaptureProperty(m_capture, CV_CAP_PROP_BRIGHTNESS );/*Brightness of the image (only for cameras) */
	m_video_properties.contrast =		cvGetCaptureProperty(m_capture, CV_CAP_PROP_CONTRAST );/*Contrast of the image (only for cameras) */
	m_video_properties.saturation =		cvGetCaptureProperty(m_capture, CV_CAP_PROP_SATURATION );/*Saturation of the image (only for cameras) */
	m_video_properties.hue =			cvGetCaptureProperty(m_capture, CV_CAP_PROP_HUE );/*Hue of the image (only for cameras) */
	m_video_properties.gain =			cvGetCaptureProperty(m_capture, CV_CAP_PROP_GAIN );/*Gain of the image (only for cameras) */
	m_video_properties.exposure =		cvGetCaptureProperty(m_capture, CV_CAP_PROP_EXPOSURE );/*Exposure (only for cameras) */
	m_video_properties.convert_rgb =	cvGetCaptureProperty(m_capture, CV_CAP_PROP_CONVERT_RGB );/*Boolean flags indicating whether images should be converted to RGB */
	m_video_properties.white_balance = cvGetCaptureProperty(m_capture, CV_CAP_PROP_WHITE_BALANCE );/*Currently unsupported */

		/*! CV_CAP_PROP_RECTIFICATION TOWRITE (note: only supported by DC1394 v 2.x backend currently)
		– Property identifier. Can be one of the following:*/
	m_video_properties.min_width = 160;
	m_video_properties.min_height = 120;
	m_video_properties.max_width = 800;
	m_video_properties.max_height = 600;

	fprintf(stderr, "OpenCVVideoAcquisition::%s:%d : props=\n", __func__, __LINE__);
	printVideoProperties(&m_video_properties);

	return m_video_properties;
}

#define SETCAPTUREPROP(_val,_prop) cvSetCaptureProperty(m_capture,(_prop),(_val))

/** @brief Set video properties (not updated) */
int OpenCVVideoAcquisition::setVideoProperties(t_video_properties props)
{
	if(!m_capture) {
		return -1;  }

	fprintf(stderr, "OpenCVVidAcq::%s:%d : change video properties:", __func__, __LINE__);
	printVideoProperties(&props);

	//	m_video_properties. = cvGetCaptureProperty(m_capture, );
	SETCAPTUREPROP(	props.pos_msec , CV_CAP_PROP_POS_MSEC );/*Film current position in milliseconds or video capture timestamp */
	SETCAPTUREPROP(	props.pos_frames , CV_CAP_PROP_POS_FRAMES );/*0-based index of the frame to be decoded/captured next */
	SETCAPTUREPROP(	props.pos_avi_ratio , CV_CAP_PROP_POS_AVI_RATIO );/*Relative position of the video file (0 - start of the film, 1 - end of the film) */
	SETCAPTUREPROP(	props.frame_width , CV_CAP_PROP_FRAME_WIDTH );/*Width of the frames in the video stream */
	SETCAPTUREPROP(	props.frame_height , CV_CAP_PROP_FRAME_HEIGHT );/*Height of the frames in the video stream */
	SETCAPTUREPROP(	props.fps , CV_CAP_PROP_FPS );/*Frame rate */
	SETCAPTUREPROP(	props.fourcc_dble , CV_CAP_PROP_FOURCC );/*4-character code of codec */
	//		char   fourcc[5] =	cvGetCaptureProperty(m_capture, FOURCC coding CV_CAP_PROP_FOURCC 4-character code of codec */
	//		char   norm[8] =		cvGetCaptureProperty(m_capture, Norm: pal, ntsc, secam */
	SETCAPTUREPROP(	props.frame_count , CV_CAP_PROP_FRAME_COUNT );/*Number of frames in the video file */
	SETCAPTUREPROP(	props.format , CV_CAP_PROP_FORMAT );/*The format of the Mat objects returned by retrieve() */
	SETCAPTUREPROP(	props.mode , CV_CAP_PROP_MODE );/*A backend-specific value indicating the current capture mode */
	SETCAPTUREPROP(	props.brightness , CV_CAP_PROP_BRIGHTNESS );/*Brightness of the image (only for cameras) */
	SETCAPTUREPROP(	props.contrast , CV_CAP_PROP_CONTRAST );/*Contrast of the image (only for cameras) */
	SETCAPTUREPROP(	props.saturation , CV_CAP_PROP_SATURATION );/*Saturation of the image (only for cameras) */
	SETCAPTUREPROP(	props.hue , CV_CAP_PROP_HUE );/*Hue of the image (only for cameras) */
	SETCAPTUREPROP(	props.gain , CV_CAP_PROP_GAIN );/*Gain of the image (only for cameras) */
	SETCAPTUREPROP(	props.exposure , CV_CAP_PROP_EXPOSURE );/*Exposure (only for cameras) */
	SETCAPTUREPROP(	props.convert_rgb , CV_CAP_PROP_CONVERT_RGB );/*Boolean flags indicating whether images should be converted to RGB */
	SETCAPTUREPROP(	props.white_balance , CV_CAP_PROP_WHITE_BALANCE );/*Currently unsupported */

	updateVideoProperties();
	return 0;
}



