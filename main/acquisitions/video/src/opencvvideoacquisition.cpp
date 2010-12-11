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
	// open any type of device and index of desired device
#ifdef cv::Exception
	try {
		m_capture = cvCreateCameraCapture(CV_CAP_ANY + m_idx_device );
	} catch (cv::Exception e) {
		fprintf(stderr, "[VidAcq] : VDopen ERROR : CAUGHT EXCEPTION WHILE OPENING DEVICE  [%d]\n",
				idx_device
				); fflush(stderr);
		imageSize.width = imageSize.height = 0;
		return 0;
	}
#else
	m_capture = cvCreateCameraCapture(CV_CAP_ANY + m_idx_device );
#endif
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
	return m_video_properties;
}


/** @brief Set video properties (not updated) */
int OpenCVVideoAcquisition::setVideoProperties(t_video_properties props)
{
	return 0;
}



