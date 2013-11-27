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

#define OPENNI_VIDEOACQUISITION_CPP

#include <stdio.h>

#include "piaf-common.h"
#include "openni_videoacquisition.h"

//---------------------------------------------------------------------------
// Macros
//---------------------------------------------------------------------------
#define CHECK_RC(rc, what)											\
	if (rc != XN_STATUS_OK)											\
	{																\
		fprintf(stderr, "[OpenNI %d] %s:%d : '%s' failed: '%s'\n",	\
				m_idx_device, __func__, __LINE__,					\
				what, xnGetStatusString(rc));						\
		fprintf(stderr, "\n"); fflush(stderr);						\
	}

//return rc;

#define OPENNI_PRINTF(...) { \
	fprintf(stderr, "[OpenNI %p %d]::%s:%d : ", this, m_idx_device, __func__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); \
}


using namespace xn;

XnBool fileExists(const char *fn)
{
	XnBool exists;
	xnOSDoesFileExist(fn, &exists);
	return exists;
}


#define SAMPLE_XML_PATH "../../../../Data/SamplesConfig.xml"
#define SAMPLE_XML_PATH_LOCAL "SamplesConfig.xml"


OpenNIVideoAcquisition::OpenNIVideoAcquisition(int idx_device)
{
	m_idx_device = idx_device;

	init_OpenNI_depth_LUT();

	init();
}

OpenNIVideoAcquisition::OpenNIVideoAcquisition()
{
	m_idx_device = 0;

	init();
}

OpenNIVideoAcquisition::~OpenNIVideoAcquisition()
{
	fprintf(stderr, "OpenNIVideoAcquisition::%s:%d : stop video acquisition device",
					__func__, __LINE__);

	stopAcquisition();

	swReleaseImage(&m_depthRawImage16U);
	swReleaseImage(&m_depthImage32F);
	swReleaseImage(&m_cameraIRImage8U);
	swReleaseImage(&m_cameraRGBImage8U);
	swReleaseImage(&m_bgr24Image);
	swReleaseImage(&m_bgr32Image);
	swReleaseImage(&m_grayImage);
}



int OpenNIVideoAcquisition::init()
{
	clearImageInfoStruct(&mImageInfo);

	m_OpenNI_angle = 0;
	m_OpenNI_led = 0;
	m_got_rgb = 0;
	m_got_depth = 0;

	mDepthGenerator = NULL;
	mContext = NULL;

	m_captureIsInitialised = false;

	m_run = m_isRunning = false;
	m_image_mode = OPENNI_MODE_DEPTH_2CM;
	init_OpenNI_depth_LUT();

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
	else {
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
		OPENNI_PRINTF("No config file found => enumerate ");

		nRetVal = mContext.Init();

		if(m_idx_device == -1) // take first
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

//				return -1;/// \bug FIXME

				OPENNI_PRINTF("Looking for device # %d", m_idx_device);
				NodeInfoList::Iterator nodeIt = list.Begin();
				NodeInfo selinfo = (*nodeIt); // selected node info
				bool found = false;
				for(int devIdx = 0;
					nodeIt != list.End()
					//devIdx < m_idx_device
					; devIdx++)
				{
					NodeInfo info = (*nodeIt);
					OPENNI_PRINTF("\t node [%d]: info={\n"
								  "\t\tCreationInfo='%s' InstanceName='%s' \n"
								  "\t\tVersion={%d.%d.%d (maintenance).%d (build)}\n"
								  "\t\tadditional data=%p\n"
								  "\t}",
								  devIdx,
								   info.GetCreationInfo(),
								  info.GetInstanceName(),
								  (int)info.GetDescription().Version.nMajor,
								  (int)info.GetDescription().Version.nMinor,
								  (int)info.GetDescription().Version.nMaintenance,
								  (int)info.GetDescription().Version.nBuild,
								  info.GetAdditionalData()
								  );
					if(devIdx == m_idx_device)
					{
						OPENNI_PRINTF("\tWE FOUND THE WANTED NODE !!");
						selinfo = (*nodeIt);
						found = true;
					}
					nodeIt++;
				}

				if(!found)
				{
					OPENNI_PRINTF("\tWE COULD NOT FIND THE WANTED NODE !!");

				}
				else
				{
					// Open Device
	//				OPENNI_PRINTF("CreateProductionTree name='%s' ...",
	//							  info.GetDescription().strName);
					nRetVal = mContext.CreateProductionTree(selinfo);

					//CHECK_RC(nRetVal, "CreateProductionTree");

					OPENNI_PRINTF("GetInstance to create depth...");
					nRetVal = selinfo.GetInstance(mDepthGenerator);
					//CHECK_RC(nRetVal, "CreateProductionTree");
				}
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

	return 0;
}


/*  Return true if acquisition device is ready */
bool OpenNIVideoAcquisition::isDeviceReady()
{
#ifdef HAS_OPENNI2
	return (nRetVal == STATUS_OK);
#else
	return (nRetVal == XN_STATUS_OK);
#endif
}
/** \brief Get the list of output format */
QList<t_video_output_format> OpenNIVideoAcquisition::getOutputFormats()
{
	t_video_output_format RGB32format;
	RGB32format.id = 0;
	strcpy(RGB32format.description, "BGR32");
	RGB32format.ipl_depth = IPL_DEPTH_8U;
	RGB32format.ipl_nchannels = 4;
	QList<t_video_output_format> out;
	out.append(RGB32format);
	DEBUG_MSG("NOT IMPLEMENTED => only RGB32");

	return out;
}

/** \brief Set the output format */
int OpenNIVideoAcquisition::setOutputFormat(int id)
{
	DEBUG_MSG("NOT IMPLEMENTED => id=%d", id);

	return id;
}

/** @brief Read image as data of selected format */
IplImage * OpenNIVideoAcquisition::readImage()
{
	DEBUG_MSG("NOT IMPLEMENTED => return readimageRGB32()");
	return readImageRGB32();
}


/** \brief Start acquisition */
int OpenNIVideoAcquisition::startAcquisition()
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
	OPENNI_PRINTF("Starting thread ...");

	start();

	OPENNI_PRINTF("Set capture to 1 ...");
	m_captureIsInitialised = 1;

	// update video properties
	updateVideoProperties();


	OPENNI_PRINTF("init OK !\n");

	return 0;
}

/** @brief Set tilt angle in degrees */
int OpenNIVideoAcquisition::setAngle(int degree)
{
	m_OpenNI_angle = degree;
	OPENNI_PRINTF("set tilt angle = %d deg", degree);
	return m_OpenNI_angle;
}

int OpenNIVideoAcquisition::setLEDMode(int led_mode)
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
int OpenNIVideoAcquisition::grab()
{
	//OPENNI_PRINTF("grab...");
	if(!m_captureIsInitialised) {
		OPENNI_PRINTF("Capture is not initialized");
		sleep(1);
		return -1;
	}

	// Wait for acq
	if(!mGrabWaitCondition.wait(&mGrabMutex, 200))
	{
		OPENNI_PRINTF("wait failed");
		return -1;
	}
//	OPENNI_PRINTF("unlocked by notify");

	return 0;
}

/* Set the raw depth buffer (11 bit in 16bit short) */
int OpenNIVideoAcquisition::setRawDepthBuffer(void * depthbuf, uint32_t timestamp)
{
	if(!m_depthRawImage16U)
	{
		OPENNI_PRINTF("Create m_depthRawImage16U (m_imageSize=%dx%d, 16U, 1",
					  m_imageSize.width, m_imageSize.height
					  );
		if(m_imageSize.width == 0 || m_imageSize.height == 0)
		{

		}
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

//	OPENNI_PRINTF("got depth %d x %d x %d x %d",
//				  m_depthRawImage16U->width, m_depthRawImage16U->height,
//				  m_depthRawImage16U->depth, m_depthRawImage16U->nChannels);

	mGrabWaitCondition.wakeAll();

	return 0;
}

/* Return image size */
CvSize OpenNIVideoAcquisition::getImageSize()
{
	return m_imageSize;
}

/* Stop acquisition */
int OpenNIVideoAcquisition::stopAcquisition()
{


	m_run = false;
	int retry = 0;
	while(m_isRunning && retry<3000)
	{
		usleep(100000);

		m_run = false;
		retry += 100;
	}

	if(mDepthGenerator) {
		OPENNI_PRINTF("Stop depth generation ...");
		mDepthGenerator.StopGenerating();
//		depth.Release();
	}


//	scriptNode.Release();
//	context.Release();

	PIAF_MSG(SWLOG_INFO, "Stopped OpenNI");
	m_captureIsInitialised = 0;

	return 0;
}




void OpenNIVideoAcquisition::run()
{
	OPENNI_PRINTF("Acquisition loop started ...");
	m_isRunning = true;
	m_run = true;

	//
	while( m_run )//&& processEvents() >= 0)
	{
//		OPENNI_PRINTF("Acquisition loop : wait for img...");
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
	OPENNI_PRINTF("Acquisition loop stopped ...");

	m_isRunning = false;
}

/** \brief Grabs one image and convert to RGB32 coding format
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * OpenNIVideoAcquisition::readImageRGB32()
{
	if(!m_captureIsInitialised) { return NULL; }

	if(!m_bgr32Image)
	{
		fprintf(stderr, "[Freenect]::%s:%d : create BGR32 image\n", __func__, __LINE__);
		m_bgr32Image = swCreateImage(m_imageSize, IPL_DEPTH_8U, 4);
	}

	int mode = (int)round(m_video_properties.mode);

	switch(mode)
	{
	default:
		OPENNI_PRINTF("FIXME: Only 2cm mode is supported");

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

			OPENNI_PRINTF("mode = %d => 2Cm => gray => BGR32\n", mode);
			try {
				cvCvtColor(m_grayImage, m_bgr32Image, CV_GRAY2BGRA);
			}
			catch(cv::Exception e)
			{
				OPENNI_PRINTF("Unable to use cvCvtColor(m_grayImage=%dx%dx%dx%d, m_bgr32Image=%dx%dx%dx%d, CV_GRAY2BGRA)",
							  m_grayImage->width, m_grayImage->height, m_grayImage->depth, m_grayImage->nChannels,
							  m_bgr32Image->width, m_bgr32Image->height, m_bgr32Image->depth, m_bgr32Image->nChannels);
			}

		}


		break;
	}

	return m_bgr32Image;
}

IplImage * OpenNIVideoAcquisition::getDepthImage32F()
{
	if(!m_depthRawImage16U)
	{
		OPENNI_PRINTF("16b is null");
		return NULL;
	}

	if(!m_depthImage32F)
	{
		CvSize imgSize = cvGetSize(m_depthRawImage16U);

		OPENNI_PRINTF("Create 32F image : %dx%d", imgSize.width, imgSize.height);

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

IplImage * OpenNIVideoAcquisition::readImageRaw()
{
//	OPENNI_PRINTF("Return m_depthRawImage16U=%p", m_depthRawImage16U);
//	if(m_depthRawImage16U)
//	{
//		OPENNI_PRINTF("return m_depthRawImage16U: %d x %d x %d x %d",
//					  m_depthRawImage16U->width, m_depthRawImage16U->height,
//					  m_depthRawImage16U->depth, m_depthRawImage16U->nChannels);
//	}



	return m_depthRawImage16U;
}

/** \brief Grabs one image and convert to grayscale coding format
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * OpenNIVideoAcquisition::readImageY()
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
IplImage * OpenNIVideoAcquisition::readImageDepth()
{
	return m_depthRawImage16U;
}


/** @brief Get video properties (not updated) */
t_video_properties OpenNIVideoAcquisition::getVideoProperties()
{
	return m_video_properties;
}

/* Update and return video properties */
t_video_properties OpenNIVideoAcquisition::updateVideoProperties()
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
int OpenNIVideoAcquisition::setVideoProperties(t_video_properties props)
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






