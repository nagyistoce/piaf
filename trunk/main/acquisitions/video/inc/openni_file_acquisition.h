/***************************************************************************
	 openni_file_acquisition.h  - Open NI acquisition class for capture
									from file for Microsoft Kinect and Asus Xtion Pro

	Tested with OpenNI version is 1.3.3.6

							 -------------------
	begin                : Mon Apr 02 2012
	copyright            : (C) 2012 by Christophe Seyve (CSE)
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

#ifndef OPENNI_FILE_ACQUISITION_H
#define OPENNI_FILE_ACQUISITION_H


#include "virtualdeviceacquisition.h"

// for LUT
#include "openni_videoacquisition.h"

#include <XnOpenNI.h>
#include <XnLog.h>
#include <XnCppWrapper.h>
#include <XnFPSCalculator.h>

using namespace xn;

#include <QMutex>
#include <QWaitCondition>

/** @brief OpenNI based video acquisition device for PrimeSense based 3D cameras

  This class supports:
  - PrimeSense reference device: NOT TESTED (since we cannot buy one)
  - Asus Xtion Pro and Xtion Pro Live
  - Microsoft Kinect
  */
class OpenNIFileAcquisition : public VirtualDeviceAcquisition
{
public:
	/** @brief Default constructor with index of OpenNI device */
	OpenNIFileAcquisition(int idx_device = 0);
	OpenNIFileAcquisition();
	OpenNIFileAcquisition(const char * filename);
	~OpenNIFileAcquisition();

	/** @brief Return true if acquisition device is ready */
	bool isDeviceReady() { return (nRetVal== XN_STATUS_OK); }

	/** @brief Return true if acquisition is running */
	bool isAcquisitionRunning() { return m_captureIsInitialised; }

	/** @brief Function called by the doc (parent) thread */
	int grab();

	/** @brief Use sequential mode: UNSUPPORTED
		If true, grabbing is done when a new image is requested.
		Else a thread is started to grab */
	void setSequentialMode(bool ) {}

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
	int init();

	/// Index of device for CpenCV
	int m_idx_device;

	/// File path
	std::string mVideoFilePath;

	/// Option to play movie in loop mode
	bool mVideoRepeat;

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

	/// Input RGB image size
	CvSize m_imageSize;
	/// Input depth image size
	CvSize mDepthSize;

	/// Input depth image framerate
	float mDepthFPS;
	/// Minimal distance for depth, in meters
	float mMinDistance;

	/// Pixel to meter scale factor, for width/columns
	float mScaleFactorWidth;
	/// Pixel to meter scale factor, for height/rows
	float mScaleFactorHeight;


	/// Video properties from OpenCV capture API
	t_video_properties m_video_properties ;

	/** @brief Print the error nature if there is an error

	  @return 0 if ok, -1 if error
	  */
	int checkOpenNIError(const char* function, int line);

	// OPENNI STRUCTURES
	XnStatus mOpenNIStatus;
	/// OpenNI context
	Context mContext;
	Player mPlayer;
	int m_OPENNI_dev;

	DepthGenerator mDepthGenerator;
	ImageGenerator mImageGenerator;

	XnFPSData xnFPS;
	DepthMetaData depthMD;
	XnStatus nRetVal;
	ScriptNode scriptNode;

	/** \brief Enable depth stream */
	bool mEnableDepth;
	/** \brief Enable camera stream */
	bool mEnableCamera;


	// THREAD VARIABLES
	/// Run command for thread
	bool m_run;
	/// Run status for thread
	bool m_isRunning ;
	/** @brief Tilt angle in degrees */
	int m_OpenNI_angle;
	/** @brief Front LED mode */
	int m_OpenNI_led;
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
	bool isRunning() { return m_run; }

	/** @brief Process driver events

	  @return <0 if error */
	int processEvents();

	/** @brief Set tilt angle in degrees */
	int setAngle(int degree);

	/** @brief Set front LED mode */
	int setLEDMode(int led_mode);

	/** @brief Use RGB mode (private) */
	void setModeImageRGB(bool rgb_on);

	/** @brief Set the raw depth buffer (11 bit in 16bit short) */
	int setRawDepthBuffer(void * depthbuf, uint32_t timestamp = 0);

	/** @brief Set the raw IR/RGB buffer (RGB or IR 8bit) */
	int setRawImageBuffer(void * imgbuf, uint32_t timestamp = 0);
};

#endif // OPENNI_FILE_ACQUISITION_H
