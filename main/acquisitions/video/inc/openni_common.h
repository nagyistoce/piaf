/***************************************************************************
	 openni_common.h  - Open NI common functions (live+file replay)
									from file for Microsoft Kinect and Asus Xtion Pro

	Tested with OpenNI version is 1.5.3 and 2.1

							 -------------------
	begin                : Thu May 23 2013
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

#ifndef OPENNI_COMMON_H
#define OPENNI_COMMON_H

#include <stdio.h>
#include "sw_types.h"

#ifndef OPENNI_COMMON_CPP
#define OPENNI_VID_EXTERN	extern
#else
#define OPENNI_VID_EXTERN
#endif

// Includes for v1
#ifdef HAS_OPENNI
#include <XnOpenNI.h>
#include <XnLog.h>
#include <XnCppWrapper.h>
#include <XnFPSCalculator.h>

using namespace xn;
#endif // HAS_OPENNI


#ifdef HAS_OPENNI2
#include <OniCAPI.h>
#include <OniCEnums.h>
#include <OniCProperties.h>
#include <OniCTypes.h>
#include <OniEnums.h>
#include <OniPlatform.h>
#include <OniProperties.h>
#include <OniVersion.h>
#include <OpenNI.h>

using namespace openni;

#endif // HAS_OPENNI

// IMAGE MODES
/** @brief Mode for 8bit image of depth with 1 value = 2 cm (1 channel x IPL_DEPTH_8U)
For close distance = 0.6m, the pixels are white (255) and at 5.6 m, they are black.
Unknown distance is black too.
*/
#define OPENNI_MODE_DEPTH_2CM		0

#define OPENNI_DEPTH2CM_RANGE_MIN	0.6
#define OPENNI_DEPTH2CM_RANGE_MAX	7

/** @brief Mode for float (32F) image of depth in meter (1 channel x IPL_DEPTH_32F)
The range is 0.6m to infinite. -1.f means the depth is unknown.
For practical use, do not use depth >7m
*/
#define OPENNI_MODE_DEPTH_M		1

/** \brief Mode for float (32F) image of 3D x,y,z position in meter (3 channels x IPL_DEPTH_32F)
x,y,z is in the camera reference. X on image columns, Y on rows, and Z is depth
x,y : position in 3D in meter
z: depth in meter. The range is 0.6m to infinite. -1.f means the depth is unknown.
For practical use, do not use depth >7m
*/
#define OPENNI_MODE_3D			2

/** \brief Mode for false colors (depth as RGB)
black when unknown
in HSV, H=0 (Red for 0.6m),
Blue for
*/
#define OPENNI_MODE_DEPTH_COLORS	3

/** @brief Max value of raw depth image (11 bits) */
#define OPENNI_RAW_MAX	10000

/** @brief Value for unknown depth in raw depth image (11 bits) */
#define OPENNI_RAW_UNKNOWN 0


#define OPENNI_FRAME_W		640
#define OPENNI_FRAME_H		480
#define OPENNI_FPS			30


OPENNI_VID_EXTERN float g_depth_LUT[OPENNI_RAW_MAX]
	#ifdef OPENNI_COMMON_CPP
	= { -1.f }
  #endif
  ;

OPENNI_VID_EXTERN u8 g_depth_grayLUT[OPENNI_RAW_MAX];
OPENNI_VID_EXTERN float g_depth_8bit2m_LUT[256];

/** \brief Init depth LUT */
void init_OpenNI_depth_LUT();



#include "virtualdeviceacquisition.h"
#include <QMutex>
#include <QWaitCondition>



//---------------------------------------------------------------------------
// Macros
//---------------------------------------------------------------------------
#ifdef HAS_OPENNI

#define CHECK_RC(rc, what)											\
	if (rc != XN_STATUS_OK)											\
	{																\
		fprintf(stderr, "[OpenNI %d] %s:%d : '%s' failed: '%s'\n",	\
				m_idx_device, __func__, __LINE__,					\
				what, xnGetStatusString(rc));						\
		fprintf(stderr, "\n"); fflush(stderr);						\
	}

#define CHECK_XN_STATUS checkOpenNIError(__func__, __LINE__)
#endif // v1

#ifdef HAS_OPENNI2
#define XN_STATUS_OK	STATUS_OK

/** \brief Check OpenNI status */
bool checkOpenNIStatus(openni::Status status,
					   const char *display_error, int line);

#define CHECK_RC(rc, what)	checkOpenNIStatus(rc, what)

//#define CHECK_XN_STATUS checkOpenNIError(__func__, __LINE__)
#define CHECK_XN_STATUS (checkOpenNIStatus(mOpenNIStatus, __func__, __LINE__)?0:1)
#endif // v2


/** \brief Common class for live+file OpenNI objects


	This class supports:
	- PrimeSense reference device: NOT TESTED (since we cannot buy one)
	- Asus Xtion Pro and Xtion Pro Live
	- Microsoft Kinect
*/
class OpenNICommonAcquisition : public VirtualDeviceAcquisition
{
public:

	/** @brief Default constructor with path of OpenNI file, NULL for live

	*/
	OpenNICommonAcquisition(const char * filenameONI = NULL);

	~OpenNICommonAcquisition();

	/** @brief Return true if acquisition device is ready */
	bool isDeviceReady();

	/** @brief Return true if acquisition is running */
	bool isAcquisitionRunning() { return m_captureIsInitialised; }

	/** @brief Function called by the doc (parent) thread */
	int grab();

	/** @brief Use sequential mode: UNSUPPORTED
		If true, grabbing is done when a new image is requested.
		Else a thread is started to grab */
	void setSequentialMode(bool seq )
	{
		mFreeRun = !seq;
		PIAF_MSG(SWLOG_INFO, "mFreeRun=%c", mFreeRun ? 'T':'F');
	}

	/** @brief Start acquisition */
	int startAcquisition();

	/** @brief Return image size */
	CvSize getImageSize();

	/** @brief Stop acquisition */
	int stopAcquisition();

	/** \brief Get the list of output format */
	QList<t_video_output_format> getOutputFormats();

	/** \brief Set the output format */
	int setOutputFormat(int id);

	/** @brief Read image as data of selected format */
	IplImage * readImage();

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

	/** Return the absolute position to be stored in database (needed for post-processing or permanent encoding) */
	unsigned long long getAbsolutePosition();
	void rewindMovie();
	bool endOfFile();

	/** @brief Go to absolute position in file (in bytes from start) */
	void rewindToPosMovie(unsigned long long position);
	/** @brief Read next frame */
	bool GetNextFrame();
	/** set absolute position in file
	 */
	void setAbsolutePosition(unsigned long long newpos);
	/** go to frame position in file
	 */
	void setAbsoluteFrame(int frame);


	/** @brief Get video properties (not updated) */
	t_video_properties getVideoProperties();

	/** @brief Update and return video properties */
	t_video_properties updateVideoProperties();

	/** @brief Set video properties (not updated) */
	int setVideoProperties(t_video_properties props);

	/// Thread loop
	void run();

protected:
	/// value used for registering in factory
	//static std::string mRegistered;
	bool m_isLive;	///< true if live device
	/// Free-run mode : if set the thread run() does the acquisition in real time
	bool mFreeRun;

	QList<t_video_output_format> mOutputFormats;
	int mOutputFormatIndex;
//private:
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


	int m_OPENNI_dev;

	// OPENNI STRUCTURES
#ifdef HAS_OPENNI2
	Status mOpenNIStatus;
	Device mDevice;
	VideoStream mDepthGenerator, mImageGenerator;
	VideoStream**		m_streams;
	openni::VideoFrameRef		m_depthFrame;
	openni::VideoFrameRef		m_colorFrame;

#else // this is v1
	XnStatus mOpenNIStatus;
	/// OpenNI context
	Context mContext;
	Player mPlayer;

	DepthGenerator mDepthGenerator;
	ImageGenerator mImageGenerator;

	XnFPSData xnFPS;
	DepthMetaData depthMD;
	XnStatus nRetVal;
	ScriptNode scriptNode;
#endif // v1
	/** \brief Enable depth stream */
	bool mEnableDepth;
	/** \brief Enable camera stream */
	bool mEnableCamera;


	/** @brief Acquire one frame now */
	int acquireFrameNow();

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

	// -------- IMAGE INFORMATION -------
public:
	/** @brief Read image information */
	t_image_info_struct readImageInfo() { return mImageInfo; }

protected:
	t_image_info_struct mImageInfo;
};

#endif // OPENNI_COMMON_H



