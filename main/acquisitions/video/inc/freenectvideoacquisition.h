/***************************************************************************
	 FreenectVideoAcquisition.h  - Microsoft Kinect
									 acquisition class for video capture
							 -------------------
	begin                : Thu Dec 9 2010
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
#ifndef FREENECTVIDEOACQUISITION_H
#define FREENECTVIDEOACQUISITION_H
#include "libfreenect.h"

#include "virtualdeviceacquisition.h"


// DETECT IF THE API IS A NEWER ONE OR ONE OF THE FIRST RELEASES OF FREENECT
#ifndef FREENECT_FRAME_W
#define FREENECT_FRAME_W	640
#define FREENECT_FRAME_H	480

#define NEWER_FREENECT_API
#endif


// IMAGE MODES
/** @brief Mode for 8bit image of depth with 1 value = 2 cm (1 channel x IPL_DEPTH_8U)
For close distance = 0.6m, the pixels are white (255) and at 5.6 m, they are black.
Unknown distance is black too.
*/
#define FREENECT_MODE_DEPTH_2CM		0

#define FREENECT_DEPTH2CM_RANGE_MIN	0.6
#define FREENECT_DEPTH2CM_RANGE_MAX	7

/** @brief Mode for float (32F) image of depth in meter (1 channel x IPL_DEPTH_32F)
The range is 0.6m to infinite. -1.f means the depth is unknown.
For practical use, do not use depth >7m
*/
#define FREENECT_MODE_DEPTH_M		1

/** \brief Mode for float (32F) image of 3D x,y,z position in meter (3 channels x IPL_DEPTH_32F)
x,y,z is in the camera reference. X on image columns, Y on rows, and Z is depth
x,y : position in 3D in meter
z: depth in meter. The range is 0.6m to infinite. -1.f means the depth is unknown.
For practical use, do not use depth >7m
*/
#define FREENECT_MODE_3D			2

/** \brief Mode for false colors (depth as RGB)
black when unknown
in HSV, H=0 (Red for 0.6m),
Blue for
*/
#define FREENECT_MODE_DEPTH_COLORS	3

/** @brief Max value of raw depth image (11 bits) */
#define FREENECT_RAW_MAX	2048

/** @brief Value for unknown depth in raw depth image (11 bits) */
#define FREENECT_RAW_UNKNOWN 2047



#include <QThread>
#include <QMutex>
#include <QWaitCondition>

/** @brief Freenect based video acquisition device for microsoft Kinect 3D camera
  */
class FreenectVideoAcquisition : //public QThread,
	public VirtualDeviceAcquisition
{

public:
	/** @brief Default constructor with index of Kinect */
	FreenectVideoAcquisition(int idx_device = 0);
	~FreenectVideoAcquisition();

	/** @brief Return true if acquisition device is ready */
	bool isDeviceReady() { return (m_freenect_dev != NULL); }

	/** @brief Return true if acquisition is running */
	bool isAcquisitionRunning() { return m_captureIsInitialised; }

	/** @brief Function called by the doc (parent) thread */
	int grab();

	/** @brief Use sequential mode
		If true, grabbing is done when a new image is requested.
		Else a thread is started to grab */
	void setSequentialMode(bool on);

	/** @brief Start acquisition */
	int startAcquisition();

	/** @brief Return image size */
	CvSize getImageSize();

	/** @brief Stop acquisition */
	int stopAcquisition();

	/** @brief Read image as raw data */
	IplImage * readImageRaw();

	/** @brief Grabs one image and convert to RGB32 coding format
		if sequential mode, return last acquired image, else read and return image

		\return NULL if error
		*/
	IplImage * readImageRGB32() ;

	/** @brief Grabs one image and convert to grayscale coding format
		if sequential mode, return last acquired image, else read and return image

		\return NULL if error
		*/
	IplImage * readImageY();

	/** @brief Grabs one image of depth buffer
		if sequential mode, return last acquired image, else read and return image

		\return NULL if error
		*/
	IplImage * readImageDepth();


	/** @brief Get video properties (not updated) */
	t_video_properties getVideoProperties();

	/** @brief Update and return video properties */
	t_video_properties updateVideoProperties();

	/** @brief Set video properties (not updated) */
	int setVideoProperties(t_video_properties props);

	/// Thread loop
	void run();

private:

	/// Index of device for CpenCV
	int m_idx_device;

	/// Last captured frame in BGR24 format
	IplImage * m_bgr24Image;
	/// Iteration of computation of BGR24 image
	int m_bgr24Image_iteration;

	/// Last captured frame in BGR32 format
	IplImage * m_bgr32Image;
	/// Iteration of computation of BGR32 image
	int m_bgr32Image_iteration;

	/// Last captured frame in grayscale
	IplImage * m_grayImage;
	int m_grayImage_iteration;

	/// Init flag
	int m_captureIsInitialised;

	/// Input image size
	CvSize m_imageSize;

	/// Video properties from OpenCV capture API
	t_video_properties m_video_properties ;


	// FREENECT STRUCTURES

	/** @brief Requested video format when change is requested
	  either FREENECT_VIDEO_RGB or FREENECT_VIDEO_IR_8BIT
	  with the setProperties, the mode is used for changing this channel
	  */
	freenect_video_format m_requested_format;
	freenect_video_format m_current_format;
	pthread_t m_freenect_thread;


	freenect_device *m_freenect_dev;
	/// Run command for thread
	bool m_run;
	/// Run status for thread
	bool m_isRunning ;
	/** @brief Tilt angle in degrees */
	int m_freenect_angle;
	/** @brief Front LED mode */
	int m_freenect_led;
	int m_got_rgb;
	int m_got_depth;

	/** \brief Output image mode */
	int m_image_mode;

	uint32_t m_rgb_timestamp;
	uint32_t m_depth_timestamp;

	/// Raw image of depth (11 bit stored in 16bit)
	IplImage * m_depthRawImage16U;

	// Depth image in float and in meter
	IplImage * m_depthImage32F;
	/** \brief update and return depth image in meter (float) */
	IplImage * getDepthImage32F();

	/// Raw image of IR camera (1 channel of IPL_DEPTH_8U)
	IplImage * m_cameraIRImage8U;
	/// Raw image of RGB camera (3 channels of IPL_DEPTH_8U)
	IplImage * m_cameraRGBImage8U;

	QWaitCondition mGrabWaitCondition;
	QMutex mGrabMutex;
public:
	/// return pointer to Kinect device, in order to implement callbacks
	freenect_device * getFreenectDevice() { return m_freenect_dev; }

	bool isRunning() { return m_run; }

	/** @brief Process driver events

	  @return <0 if error */
	int processEvents();

	/** @brief Set tilt angle in degrees */
	int setAngle(int degree)
	{
		m_freenect_angle = degree;
		if(m_freenect_dev) {
			return 	freenect_set_tilt_degs(m_freenect_dev, m_freenect_angle);
		}
		return 0;
	}
	/** @brief Set front LED mode */
	int setLEDMode(int led_mode) {
		m_freenect_led = led_mode;
		if(m_freenect_dev) {
			//FIXME freenect_set_led(m_freenect_dev, led_mode);
			return 0;
		}
		return -1;
	}

	/** @brief Use RGB mode (private) */
	void setModeImageRGB(bool rgb_on) {
		if(rgb_on)
			m_requested_format = FREENECT_VIDEO_RGB;
		else
			m_requested_format = FREENECT_VIDEO_IR_8BIT;
	}

	/** @brief Set the raw depth buffer (11 bit in 16bit short) */
	int setRawDepthBuffer(void * depthbuf, uint32_t timestamp = 0);
	/** @brief Set the raw IR/RGB buffer (RGB or IR 8bit) */
	int setRawImageBuffer(void * imgbuf, uint32_t timestamp = 0);
	// -------- IMAGE INFORMATION -------
public:
	/** @brief Read image information */
	t_image_info_struct readImageInfo() { return mImageInfo; }

protected:
	t_image_info_struct mImageInfo;

};

#endif // FREENECTVIDEOACQUISITION_H
