/***************************************************************************
	 VirtualDeviceAcquisition.h  - Virtual acquisition class for video capture
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

#ifndef VIRTUALDEVICEACQUISITION_H
#define VIRTUALDEVICEACQUISITION_H

#include <sys/mman.h>

#include "ccvt.h"
#include "sw_types.h"

#include "nolinux_videodev.h"
#include <cv.h>
#include <cvaux.h>
#include <highgui.h>

#include <QThread>


/** @brief Video input properties

INHERITED FROM OPENCV's CAPTURE API
   http://opencv.willowgarage.com/documentation/python/reading_and_writing_images_and_video.html#getcaptureproperty

*/
typedef struct {
	double pos_msec;		/*! CV_CAP_PROP_POS_MSEC Film current position in milliseconds or video capture timestamp */
	double pos_frames;		/*! CV_CAP_PROP_POS_FRAMES 0-based index of the frame to be decoded/captured next */
	double pos_avi_ratio;	/*! CV_CAP_PROP_POS_AVI_RATIO Relative position of the video file (0 - start of the film, 1 - end of the film) */
	double frame_width;		/*! Frame width CV_CAP_PROP_FRAME_WIDTH Width of the frames in the video stream */
	double frame_height;	/*! Frame height CV_CAP_PROP_FRAME_HEIGHT Height of the frames in the video stream */
	double fps;				/*! Frame rate CV_CAP_PROP_FPS Frame rate */
	double fourcc_dble;		/*! FourCC coding as double CV_CAP_PROP_FOURCC 4-character code of codec */
	char   fourcc[5];		/*! FOURCC coding CV_CAP_PROP_FOURCC 4-character code of codec */
	char   norm[8];			/*! Norm: pal, ntsc, secam */
	double frame_count;		/*! Frame count CV_CAP_PROP_FRAME_COUNT Number of frames in the video file */
	double format;			/*! CV_CAP_PROP_FORMAT The format of the Mat objects returned by retrieve() */
	double mode;			/*! CV_CAP_PROP_MODE A backend-specific value indicating the current capture mode */
	double brightness;		/*! Brightness CV_CAP_PROP_BRIGHTNESS Brightness of the image (only for cameras) */
	double contrast;		/*! Contrast CV_CAP_PROP_CONTRAST Contrast of the image (only for cameras) */
	double saturation;		/*! Saturation CV_CAP_PROP_SATURATION Saturation of the image (only for cameras) */
	double hue;			/*! Hue CV_CAP_PROP_HUE Hue of the image (only for cameras) */
	double gain;			/*! Gain CV_CAP_PROP_GAIN Gain of the image (only for cameras) */
	double exposure;		/*! CV_CAP_PROP_EXPOSURE Exposure (only for cameras) */
	double convert_rgb;		/*! CV_CAP_PROP_CONVERT_RGB Boolean flags indicating whether images should be converted to RGB */
	double white_balance;	/*! CV_CAP_PROP_WHITE_BALANCE Currently unsupported */

	/*! CV_CAP_PROP_RECTIFICATION TOWRITE (note: only supported by DC1394 v 2.x backend currently)
	– Property identifier. Can be one of the following:*/
} t_video_properties;


typedef struct {
	int id;
	char description[64];
} t_lexique;

/** @brief Video input capabilities

*/
typedef struct {
	int nb_channels;	/*! Number of channels */

	int min_width;		/*! Minimum width */
	int min_height;		/*! Minimum height */
	int max_width;		/*! Maximum width */
	int max_height;		/*! Maximum height */

	int nb_modes;		/*! Number of modes */
	t_lexique * modes;	/*!	List of modes */

} t_video_capabilities;


/** Inherited class for video device management and image acquisitions with simple API.
	\brief Low-level video acquisition base class.
 \author Christophe Seyve - cseyve@free.fr
 */
class VirtualDeviceAcquisition : public QThread
{
public:
	/// Default constructor
	VirtualDeviceAcquisition () {}

	/// Default destructor
	~VirtualDeviceAcquisition () {}

	/** \brief Use sequential mode
		If true, grabbing is done when a new image is requested.
		Else a thread is started to grab */
	virtual void setSequentialMode(bool on) = 0;

	/** \brief Function called by the doc (parent) thread */
	virtual int grab() = 0;

	/** \brief Return true if acquisition device is ready to start */
	virtual bool isDeviceReady() = 0;

	/** \brief Return true if acquisition is running */
	virtual bool isAcquisitionRunning() = 0;

	/** \brief Start acquisition */
	virtual int startAcquisition() = 0;

	/** \brief Return image size */
	virtual CvSize getImageSize() = 0;

	/** \brief Stop acquisition */
	virtual int stopAcquisition() = 0;

	/** \brief Grabs one image and convert to RGB32 coding format
		if sequential mode, return last acquired image, else read and return image

		\return NULL if error
		*/
	virtual IplImage * readImageRGB32() = 0;

	/** \brief Grabs one image and convert to grayscale coding format
		if sequential mode, return last acquired image, else read and return image

		\return NULL if error
		*/
	virtual IplImage * readImageY() = 0;

	/** \brief Grabs one image of depth buffer, in meter
		if sequential mode, return last acquired image, else read and return image

		\return NULL if error
		*/
	virtual IplImage * readImageDepth() = 0;


	/** @brief Get video properties (not updated) */
	virtual t_video_properties getVideoProperties() = 0;

	/** @brief Update and return video properties */
	virtual t_video_properties updateVideoProperties() = 0;

	/** @brief Set video properties (not updated) */
	virtual int setVideoProperties(t_video_properties props) = 0;

};

#endif // VIRTUALDEVICEACQUISITION_H
