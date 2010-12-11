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
#ifndef OPENCVVIDEOACQUISITION_H
#define OPENCVVIDEOACQUISITION_H

#include "sw_types.h"

#include "nolinux_videodev.h"
#include <cv.h>
#include <cvaux.h>
#include <highgui.h>

#include "virtualdeviceacquisition.h"


/** \brief OpenCV based video acquisition device
  Support V4L2, GigEVision
  */
class OpenCVVideoAcquisition : public VirtualDeviceAcquisition
{
public:
	OpenCVVideoAcquisition(int idx_device = 0);
	~OpenCVVideoAcquisition();

	/** \brief Use sequential mode
		If true, grabbing is done when a new image is requested.
		Else a thread is started to grab */
	void setSequentialMode(bool on);

	/** \brief Function called by the doc (parent) thread */
	int grab();

	/** \brief Return true if acquisition device is ready */
	bool isDeviceReady() { return (m_capture != NULL); }

	/** \brief Return true if acquisition is running */
	bool isAcquisitionRunning() { return m_captureIsInitialised; }

	/** \brief Start acquisition */
	int startAcquisition();

	/** \brief Return image size */
	CvSize getImageSize();

	/** \brief Stop acquisition */
	int stopAcquisition();

	/** \brief Grabs one image and convert to RGB32 coding format
		if sequential mode, return last acquired image, else read and return image

		\return NULL if error
		*/
	IplImage * readImageRGB32() ;

	/** \brief Grabs one image and convert to grayscale coding format
		if sequential mode, return last acquired image, else read and return image

		\return NULL if error
		*/
	IplImage * readImageY();

	/** \brief Grabs one image of depth buffer
		if sequential mode, return last acquired image, else read and return image

		\return NULL until OpenCV integrates Kinect driver
		*/
	IplImage * readImageDepth();


	/** @brief Get video properties (not updated) */
	t_video_properties getVideoProperties();

	/** @brief Update and return video properties */
	t_video_properties updateVideoProperties();

	/** @brief Set video properties (not updated) */
	int setVideoProperties(t_video_properties props);

private:

	/// Index of device for CpenCV
	int m_idx_device;

	/// Use OpenCV because it's better for other kinds of devices (IEEE1394, ...)
	CvCapture * m_capture;

	/// Last captured frame in original format (may be color or not)
	IplImage * m_iplImage;

	/// Last captured frame in grayscale
	IplImage * m_grayImage;

	/// Init flag
	int m_captureIsInitialised;

	/// Input image size
	CvSize m_imageSize;

	/// Video properties from OpenCV capture API
	t_video_properties m_video_properties ;


};

#endif // OPENCVVIDEOACQUISITION_H
