/***************************************************************************
	 openni_videoacquisition.cpp  - Open NI
									 acquisition class for video capture
									 For Microsoft Kinect and Asus Xtion Pro
							 -------------------
	begin                : Thu Jan 19 2012
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

#include "piaf-common.h"
#include "openni_file_acquisition.h"

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
bool checkOpenNIStatus(openni::Status status, const char *display_error)
{
	if(status != openni::STATUS_OK)
	{
		DEBUG_MSG("'%s' failed: err=%d='%s'\n",
				  display_error, (int)status,
				  openni::OpenNI::getExtendedError());
		return false;
	}

	return true;
}

#define CHECK_RC(rc, what)	checkOpenNIStatus(rc, what)

#define CHECK_XN_STATUS checkOpenNIError(__func__, __LINE__)
#endif // v2



//return rc;

#define OPENNI_PRINTF(...) { \
	fprintf(stderr, "[OpenNI %p %d]::%s:%d : ", this, m_idx_device, __func__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); \
}

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------
extern void init_OpenNI_depth_LUT(); // from openni_videoacquisition

using namespace xn;


#define SAMPLE_XML_PATH "/etc/openni/SamplesConfig.xml"
#define SAMPLE_XML_PATH_LOCAL "SamplesConfig.xml"

OpenNIFileAcquisition::OpenNIFileAcquisition(int idx_device)
{
	m_idx_device = idx_device;
	mVideoFilePath = "";

	init_OpenNI_depth_LUT();

	init();
}


OpenNIFileAcquisition::OpenNIFileAcquisition()
{
	mVideoFilePath = "";
	m_idx_device = 0;

	init();
}

OpenNIFileAcquisition::OpenNIFileAcquisition(const char * filename)
{
	init();
	PIAF_MSG(SWLOG_INFO, "Opening file '%s'...", filename);
	mVideoFilePath = filename;

	// Open filename
	mOpenNIStatus = mContext.OpenFileRecording(filename, mPlayer);

	if(CHECK_XN_STATUS ==0)
	{
		if(mEnableDepth)
		{
			DEBUG_MSG("OpenNI : creating depth generator...");
			mOpenNIStatus = mContext.FindExistingNode(XN_NODE_TYPE_DEPTH,
													  mDepthGenerator);

			if(checkOpenNIError(__func__, __LINE__) != 0)
			{
				PIAF_MSG(SWLOG_ERROR, "Could not find any Depth Generator in context.");
			}
			else
			{
				/*

				-			  mDepthSize.width/2
				|			|--------------/      --> mDepthSize.width/2 is the distance in pixels.
				|			|             /           By which factor do we need to multiply the pixel size to obtain
				|			|            /			  its size in meters at mMinDistance : mScaleFactorWidth
				|			|           /
				|			|          /
				|			|	      /
				|			|        /
			mMinDistance	|       /
				|			|      /
				|			|     /
				|			|__  /
				|			|  \/
				|			|  /HorizontalFOV / 2
				|			| /
				|			|/
				-

				Pixels are square shaped so :
				mScaleFactorHeight = mScaleFactorWidth

				and :

				tan (HorizontalFOV / 2) = (mDepthSize.width/2) / mMinDistance						 <-- In pixels
				tan (HorizontalFOV / 2) = (mDepthSize.width/2) * mScaleFactorWidth / mMinDistance    <-- In meters
				mScaleFactorWidth = tan (HorizontalFOV / 2) * mMinDistance / (mDepthSize.width /2)	 <-- In meters

				To make things easier, mMinDistance = 1


				*/

				xn::DepthMetaData dmd;
				mDepthGenerator.GetMetaData(dmd);
				mDepthSize.width = (int)dmd.FullXRes();
				mDepthSize.height = (int)dmd.FullYRes();
				mDepthFPS = (int)dmd.FPS();

				XnFieldOfView fov;
				mDepthGenerator.GetFieldOfView(fov);
				mMinDistance = 1.;
				mScaleFactorWidth = 2.f * mMinDistance * tan(fov.fHFOV / 2.)/ (float)mDepthSize.width;
				mScaleFactorHeight = 2.f * mMinDistance * tan(fov.fVFOV / 2.)/ (float)mDepthSize.height;

				PIAF_MSG(SWLOG_INFO, "Opened file '%s': %dx%d @ %g fps, scX,Y=%g,%g",
						 filename,
						 mDepthSize.width, mDepthSize.height,
						 mDepthFPS,
						 mScaleFactorWidth, mScaleFactorHeight);

			}
		}


		if(mEnableCamera)
		{
			DEBUG_MSG("OpenNI : creating RGB generator ...");
			mOpenNIStatus = mContext.FindExistingNode(XN_NODE_TYPE_IMAGE,mImageGenerator);

			if(checkOpenNIError(__func__, __LINE__) != 0)
			{
				PIAF_MSG(SWLOG_ERROR, "Could not find any Image Generator in context.");
			}
		}

		// Repeat option
		PIAF_MSG(SWLOG_INFO, "VideoRepeat=%c", mVideoRepeat ? 'T':'F');
		if(!mVideoRepeat)
		{
			mOpenNIStatus = mPlayer.SetRepeat(FALSE); // So that when the video ends, the source stops generating
		}
		else
		{
			mOpenNIStatus = mPlayer.SetRepeat(TRUE);
		}


		PIAF_MSG(SWLOG_INFO, "File '%s' opened ! Success", filename);
	}
	else
	{
		PIAF_MSG(SWLOG_ERROR,
				 "Could not open recorded file %s or create player",
				 mVideoFilePath.c_str());
	}

}

OpenNIFileAcquisition::~OpenNIFileAcquisition()
{
	stopAcquisition();

	swReleaseImage(&m_depthRawImage16U);
	swReleaseImage(&m_depthImage32F);
	swReleaseImage(&m_cameraIRImage8U);
	swReleaseImage(&m_cameraRGBImage8U);
	swReleaseImage(&m_bgr24Image);
	swReleaseImage(&m_bgr32Image);
	swReleaseImage(&m_grayImage);
}

int OpenNIFileAcquisition::checkOpenNIError(const char* function, int line)
{

	if (mOpenNIStatus != XN_STATUS_OK)
	{
		fprintf(stderr, "ERROR: ******************************************\n"
				  "OpenNIFileAcquisition::%s:%d : status='%s'\n"
				  "******************************************\n\n",
				  function,
				  line,
				  xnGetStatusString(mOpenNIStatus));
		return -1;
	}
	return 0;
}



int OpenNIFileAcquisition::init()
{
	mEnableDepth = true; // pure depth by default
	mEnableCamera = false;

	mVideoRepeat = false; // no repeat

	m_OpenNI_angle = 0;
	m_OpenNI_led = 0;
	m_got_rgb = 0;
	m_got_depth = 0;

#ifdef HAS_OPENNI2
	mOpenNIStatus = openni::OpenNI::initialize();
#else
	mDepthGenerator = NULL;
	mContext = NULL;
#endif


	m_captureIsInitialised = false;

	m_run = m_isRunning = false;
	m_image_mode = OPENNI_MODE_DEPTH_2CM;

	// Freenect native images
	m_depthRawImage16U = m_cameraIRImage8U = m_cameraRGBImage8U = NULL;
	m_depthImage32F = NULL;

	// Conversion images =========
	// Last captured frame in BGR24 format
	m_bgr24Image = NULL;
	// Iteration of computation of BGR24 image
	m_bgr24Image_iteration = 0;

	// Last captured frame in BGR32 format
	m_bgr32Image = NULL;
	// Iteration of computation of BGR32 image
	m_bgr32Image_iteration = 0;

	// Last captured frame in grayscale
	m_grayImage = NULL;
	m_grayImage_iteration = 0;

	memset(&m_video_properties, 0, sizeof(t_video_properties));

	m_imageSize = cvSize(0,0);

#ifdef HAS_OPENNI2
	mOpenNIStatus = openni::OpenNI::initialize();

	if(m_idx_device == 0) // take first
	{
		nRetVal = mDepthGenerator.Create(mContext);
	}
	else
	{
		/// \todo: FIXME : unsupported
	}


#else // v1
	ScriptNode scriptNode;
	EnumerationErrors errors;

	nRetVal = XN_STATUS_OK;

	const char *fn = NULL;
	if(fileExists(SAMPLE_XML_PATH))
	{
		fn = SAMPLE_XML_PATH;
	}
	else if (fileExists(SAMPLE_XML_PATH_LOCAL))
	{
		fn = SAMPLE_XML_PATH_LOCAL;
	}
	else
	{
		OPENNI_PRINTF("Could not find '%s' nor '%s'. Aborting." ,
					  SAMPLE_XML_PATH, SAMPLE_XML_PATH_LOCAL);

		//return XN_STATUS_ERROR;
	}

	if(fn)
	{
		OPENNI_PRINTF("Reading config from: '%s'\n", fn);
		nRetVal = mContext.InitFromXmlFile(fn, scriptNode, &errors);
	}
	else
	{
		OPENNI_PRINTF("No config file found");

		nRetVal = mContext.Init();

		if(m_idx_device == 0) // take first
		{
			nRetVal = mDepthGenerator.Create(mContext);
		} else {
	//		CHECK_RC(nRetVal, "could not initialize context");

			// Read from tree
			NodeInfoList list;
			OPENNI_PRINTF("EnumerateProductionTrees...");
			nRetVal = mContext.EnumerateProductionTrees(XN_NODE_TYPE_DEPTH,
													   NULL, list, NULL);

			if(list.IsEmpty())
			{
				OPENNI_PRINTF("No device found in tree");
				return -1;
			}
			else// find in list
			{

				return -1;/// \bug FIXME

				OPENNI_PRINTF("Looking for device # %d", m_idx_device);
				NodeInfoList::Iterator nodeIt = list.Begin();
				for(int devIdx = 0; devIdx < m_idx_device; devIdx++)
				{
					OPENNI_PRINTF("inc node");
					nodeIt++;
				}

				// Open Device
				NodeInfo info = (*nodeIt);
//				OPENNI_PRINTF("CreateProductionTree name='%s' ...",
//							  info.GetDescription().strName);
				nRetVal = mContext.CreateProductionTree(info);

				//CHECK_RC(nRetVal, "CreateProductionTree");

				OPENNI_PRINTF("GetInstance ...");
				nRetVal = info.GetInstance(mDepthGenerator);
				//CHECK_RC(nRetVal, "CreateProductionTree");
			}
		}

		if(mDepthGenerator)
		{
			XnMapOutputMode mode;
			mode.nXRes = OPENNI_FRAME_W;
			mode.nYRes = OPENNI_FRAME_H;
			mode.nFPS = OPENNI_FPS;
			OPENNI_PRINTF("set mode = %dx%d @ %d fps",
						  (int)mode.nXRes, (int)mode.nYRes, (int)mode.nFPS);
			mDepthGenerator.SetMapOutputMode(mode);
			m_imageSize = cvSize(mode.nXRes, mode.nYRes);
		}

	}


	if (nRetVal == XN_STATUS_NO_NODE_PRESENT)
	{
		XnChar strError[1024];
		errors.ToString(strError, 1024);

		OPENNI_PRINTF("No node present : '%s'", strError);
		return (nRetVal);
	}
	else if (nRetVal != XN_STATUS_OK)
	{
		OPENNI_PRINTF("Open failed: %s\n", xnGetStatusString(nRetVal));
		return (nRetVal);
	}

	nRetVal = mContext.FindExistingNode(XN_NODE_TYPE_DEPTH, mDepthGenerator);
	CHECK_RC(nRetVal, "Find depth generator");

	nRetVal = xnFPSInit(&xnFPS, 180);
	CHECK_RC(nRetVal, "FPS Init");
#endif

	return 0;
}

/** \brief Start acquisition */
int OpenNIFileAcquisition::startAcquisition()
{
	OPENNI_PRINTF("Starting acquisition ...");

	m_rgb_timestamp = m_depth_timestamp = 0;
	if (nRetVal != XN_STATUS_OK)											\
	{
		OPENNI_PRINTF("Could not start acquisition ...");
		return -1;
	}
	if(!mDepthGenerator)
	{
		OPENNI_PRINTF("Could not start depth ...");
		return -1;
	}
	m_run = true;

	mDepthGenerator.StartGenerating();

	// start event thread
	start();

	m_captureIsInitialised = true;

	// update video properties
	updateVideoProperties();


	OPENNI_PRINTF("init OK !\n");

	return 0;
}

/** @brief Set tilt angle in degrees */
int OpenNIFileAcquisition::setAngle(int degree)
{
	m_OpenNI_angle = degree;
	OPENNI_PRINTF("set tilt angle = %d deg", degree);
	return m_OpenNI_angle;
}

int OpenNIFileAcquisition::setLEDMode(int led_mode)
{
	m_OpenNI_led = led_mode;
	OPENNI_PRINTF("set led_mode = %d", led_mode);
	if(m_captureIsInitialised) {
		//FIXME OPENNI_set_led(m_OPENNI_dev, led_mode);
		return 0;
	}

	return -1;
}

/* Function called by the doc (parent) thread */
int OpenNIFileAcquisition::grab()
{
	if(!m_captureIsInitialised) {
		OPENNI_PRINTF("Capture is not initialized");
		return -1;
	}
	// Wait for acq
	if(!mGrabWaitCondition.wait(&mGrabMutex, 200))
	{
		OPENNI_PRINTF("wait failed");
		return -1;
	}

	return 0;
}
/* Set the raw depth buffer (11 bit in 16bit short) */
int OpenNIFileAcquisition::setRawDepthBuffer(void * depthbuf, uint32_t timestamp)
{
	if(!m_depthRawImage16U)
	{
		m_depthRawImage16U = swCreateImage(m_imageSize, IPL_DEPTH_16U, 1);
	}

	if(m_depthRawImage16U->widthStep == (int)sizeof(uint16_t) * m_imageSize.width)
	{
		// copy directly
		memcpy(m_depthRawImage16U->imageData, depthbuf, m_depthRawImage16U->widthStep * m_depthRawImage16U->height);
	}
	else
	{
		// copy each line
		u16 * depth16U = (u16 *)depthbuf;
		for(int r = 0; r<m_imageSize.height; ++r)
		{
			memcpy(m_depthRawImage16U->imageData + r * m_depthRawImage16U->widthStep,
				   depth16U + r * m_imageSize.width,
				   m_imageSize.width * sizeof(u16));
		}
	}

	m_got_depth ++;
	m_depth_timestamp = timestamp;

//	OPENNI_PRINTF("got depth %dx%d x 1",
//				  m_imageSize.width, m_imageSize.height);

	mGrabWaitCondition.wakeAll();

	return 0;
}

/* Return image size */
CvSize OpenNIFileAcquisition::getImageSize()
{
	return m_imageSize;
}

/* Stop acquisition */
int OpenNIFileAcquisition::stopAcquisition()
{
	if(mDepthGenerator) {
		mDepthGenerator.StopGenerating();
	}

	m_run = false;
	while(m_isRunning)
	{
		usleep(1000);
	}


	mDepthGenerator.Release();
	scriptNode.Release();
	mContext.Release();

	m_captureIsInitialised = 0;

	return 0;
}



void OpenNIFileAcquisition::run()
{
	m_isRunning = true;
	m_run = true;

	//
	while(m_run )//&& processEvents() >= 0)
	{
		nRetVal = mContext.WaitOneUpdateAll(mDepthGenerator);
		if (nRetVal != XN_STATUS_OK)
		{
			OPENNI_PRINTF("UpdateData failed: '%s'", xnGetStatusString(nRetVal));
			usleep(100000);
		}
		else
		{
			xnFPSMarkFrame(&xnFPS);

			mDepthGenerator.GetMetaData(depthMD);
			const XnDepthPixel* pDepthMap = depthMD.Data();

//			OPENNI_PRINTF("Frame %d Middle point is: %u. FPS: %f\n",
//						  depthMD.FrameID(), depthMD(depthMD.XRes() / 2, depthMD.YRes() / 2), xnFPSCalc(&xnFPS));
			setRawDepthBuffer((void *)pDepthMap, depthMD.Timestamp());
		}
	}

	m_isRunning = false;
}
IplImage * OpenNIFileAcquisition::readImageRaw()
{
	fprintf(stderr, "%s %s:%d : NOT IMPLEMENTED\n", __FILE__, __func__, __LINE__);
	return NULL; /// \todo FIXME : return raw value
}
/** \brief Grabs one image and convert to RGB32 coding format
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * OpenNIFileAcquisition::readImageRGB32()
{
	if(!m_captureIsInitialised) {
		OPENNI_PRINTF("m_captureIsInitialised = %d", m_captureIsInitialised);

		return NULL; }

	if(!m_bgr32Image)
	{
		OPENNI_PRINTF("create BGR32 image\n");
		m_bgr32Image = swCreateImage(m_imageSize, IPL_DEPTH_8U, 4);
	}

	int mode = (int)round(m_video_properties.mode);
	OPENNI_PRINTF("mode = %d", mode);
	switch(mode)
	{
	default:
		OPENNI_PRINTF("FIXME");
//		if(m_requested_format == FREENECT_VIDEO_RGB && m_cameraRGBImage8U) {
//			cvCvtColor(m_cameraRGBImage8U, m_bgr32Image, CV_RGB2BGRA);
//		} else if(m_cameraRGBImage8U) {
//			cvCvtColor(m_cameraIRImage8U, m_bgr32Image, CV_GRAY2BGRA);
//		}
		break;
	case OPENNI_MODE_DEPTH_2CM: // 2CM

		if(!m_grayImage)
		{
			OPENNI_PRINTF("Create grayscale image %d x %d x 1",
						  m_imageSize.width, m_imageSize.height);
			m_grayImage = swCreateImage(m_imageSize, IPL_DEPTH_8U, 1);
		}

		// convert to gray
		if(m_depthRawImage16U) {

			for(int r=0; r<m_depthRawImage16U->height; r++)
			{
				u16 * linedepth = IPLLINE_16U(m_depthRawImage16U, r);
				u8 * line8u = IPLLINE_8U(m_grayImage, r);
				for(int c = 0; c<m_grayImage->width; c++)
				{
					/// FIXME : just divide
					line8u[c] = g_depth_grayLUT[ linedepth[c] ];
				}
			}

			//OPENNI_PRINTF("mode = %d => 2Cm => gray => BGR32\n", mode);

			cvCvtColor(m_grayImage, m_bgr32Image, CV_GRAY2BGRA);
		}
		else
		{
			OPENNI_PRINTF("ERROR: no 16bit image");
		}

		break;
	}

	return m_bgr32Image;
}

IplImage * OpenNIFileAcquisition::getDepthImage32F()
{
	if(!m_depthRawImage16U) return NULL;
	if(!m_depthImage32F)
	{
		CvSize imgSize = cvGetSize(m_depthRawImage16U);
		OPENNI_PRINTF("Create 32f image : %dx%d", imgSize.width, imgSize.height);
		m_depthImage32F = swCreateImage(cvGetSize(m_depthRawImage16U), IPL_DEPTH_32F, 1);
	}

	for(int r=0; r<m_depthImage32F->height; r++)
	{
		float * linedepth = (float *)(m_depthImage32F->imageData + r*m_depthImage32F->widthStep);
		u16 * lineraw = (u16 *)(m_depthRawImage16U->imageData + r*m_depthRawImage16U->widthStep);

		for(int c = 0; c<m_depthRawImage16U->width; c++)
		{
			linedepth[c] = g_depth_LUT[ lineraw[c] ];
		}
	}

	return m_depthImage32F;
}

/** \brief Grabs one image and convert to grayscale coding format
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * OpenNIFileAcquisition::readImageY()
{
	if(!m_captureIsInitialised) return NULL;
	if(!m_grayImage)
	{
		m_grayImage = swCreateImage(m_imageSize, IPL_DEPTH_8U, 1);
	}

	OPENNI_PRINTF("FIXME : converto to grayImage");
//	if(m_requested_format == FREENECT_VIDEO_RGB)
//		cvConvert(m_cameraRGBImage8U, m_grayImage);
//	else if(m_cameraIRImage8U)
//		cvCopy(m_cameraIRImage8U, m_grayImage);

	return m_grayImage;
}

/** \brief Grabs one image of depth buffer
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * OpenNIFileAcquisition::readImageDepth()
{
	return m_depthRawImage16U;
}


/** @brief Get video properties (not updated) */
t_video_properties OpenNIFileAcquisition::getVideoProperties()
{
	return m_video_properties;
}

/* Update and return video properties */
t_video_properties OpenNIFileAcquisition::updateVideoProperties()
{
	if(m_captureIsInitialised<=0) {
		OPENNI_PRINTF("Capture is not initialized");
		memset(&m_video_properties, 0, sizeof(t_video_properties));
		return m_video_properties;
	}

//	m_video_properties. = cvGetCaptureProperty(m_capture, );
	m_video_properties.pos_msec = m_depth_timestamp;
	m_video_properties.pos_frames =	m_got_depth;
	m_video_properties.pos_avi_ratio =	-1.;
	m_video_properties.frame_width =	m_imageSize.width;
	m_video_properties.frame_height =	m_imageSize.height;
	m_video_properties.fps =			30.;
//	m_video_properties.fourcc_dble =	cvGetCaptureProperty(m_capture, CV_CAP_PROP_FOURCC );/*4-character code of codec */
//		char   fourcc[5] =	cvGetCaptureProperty(m_capture, FOURCC coding CV_CAP_PROP_FOURCC 4-character code of codec */
//		char   norm[8] =		cvGetCaptureProperty(m_capture, Norm: pal, ntsc, secam */
	m_video_properties.frame_count =	m_got_depth;
	//m_video_properties.format =			cvGetCaptureProperty(m_capture, CV_CAP_PROP_FORMAT );/*The format of the Mat objects returned by retrieve() */
	m_video_properties.mode =	m_image_mode; /*A backend-specific value indicating the current capture mode */
//	m_video_properties.brightness =		cvGetCaptureProperty(m_capture, CV_CAP_PROP_BRIGHTNESS );/*Brightness of the image (only for cameras) */
//	m_video_properties.contrast =		cvGetCaptureProperty(m_capture, CV_CAP_PROP_CONTRAST );/*Contrast of the image (only for cameras) */
//	m_video_properties.saturation =		cvGetCaptureProperty(m_capture, CV_CAP_PROP_SATURATION );/*Saturation of the image (only for cameras) */
//	m_video_properties.hue =			cvGetCaptureProperty(m_capture, CV_CAP_PROP_HUE );/*Hue of the image (only for cameras) */
//	m_video_properties.gain =			cvGetCaptureProperty(m_capture, CV_CAP_PROP_GAIN );/*Gain of the image (only for cameras) */
//	m_video_properties.exposure =		cvGetCaptureProperty(m_capture, CV_CAP_PROP_EXPOSURE );/*Exposure (only for cameras) */
//	m_video_properties.convert_rgb =	cvGetCaptureProperty(m_capture, CV_CAP_PROP_CONVERT_RGB );/*Boolean flags indicating whether images should be converted to RGB */
//	m_video_properties.white_balance = cvGetCaptureProperty(m_capture, CV_CAP_PROP_WHITE_BALANCE );/*Currently unsupported */

	/*! CV_CAP_PROP_RECTIFICATION TOWRITE (note: only supported by DC1394 v 2.x backend currently)
	– Property identifier. Can be one of the following:*/
	m_video_properties.min_width = m_imageSize.width;
	m_video_properties.min_height = m_imageSize.height;
	m_video_properties.max_width = m_imageSize.width;
	m_video_properties.max_height = m_imageSize.height;

	return m_video_properties;
}


/** @brief Set video properties (not updated) */
int OpenNIFileAcquisition::setVideoProperties(t_video_properties props)
{
	// Read mode
	int mode = (int)round(props.mode);
	if(m_image_mode != mode)
	{
		switch(mode)
		{
		default:
			break;
		case OPENNI_MODE_DEPTH_2CM:
			break;
		case OPENNI_MODE_DEPTH_M:
			break;
		case OPENNI_MODE_3D:
			break;

		}
		m_image_mode = mode;
	}

	return 0;
}






