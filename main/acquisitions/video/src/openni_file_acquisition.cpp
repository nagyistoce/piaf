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

#include <QDir>
#include <QApplication>


#define OPENNI_PRINTF(...) { \
	fprintf(stderr, "[OpenNI %p '%s'']::%s:%d : ", \
		this, mVideoFilePath.c_str() , __func__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); \
}

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------

#define EXTENSION_OPENNI	"oni"

std::string OpenNIFileAcquisition::mRegistered =
	FileVideoAcquisitionFactory::RegisterCreatorFunction(
		EXTENSION_OPENNI, OpenNIFileAcquisition::creatorFunction);


FileVideoAcquisition* OpenNIFileAcquisition::creatorFunction(std::string path)
{
	PIAF_MSG(SWLOG_INFO, "Create OpenNIFileAcquisition(path='%s')", path.c_str() );

#ifdef HAS_OPENNI2
	// Go back to apps path
	QDir dir;
	dir.cd( QApplication::applicationDirPath() );
	PIAF_MSG(SWLOG_INFO, "Current path = '%s'",
			 dir.absolutePath().toAscii().data());

	openni::Status status = openni::OpenNI::initialize();
 #endif

	return (FileVideoAcquisition *)new OpenNIFileAcquisition( path.c_str() );
}

#define SAMPLE_XML_PATH "/etc/openni/SamplesConfig.xml"
#define SAMPLE_XML_PATH_LOCAL "SamplesConfig.xml"

OpenNIFileAcquisition::OpenNIFileAcquisition(const char * filename)
{
	init();
	OPENNI_PRINTF("Opening file '%s'...", filename);

	mVideoFilePath = filename;


#ifdef HAS_OPENNI2
	// Go back to apps path
	QDir dir;
	dir.cd( QApplication::applicationDirPath() );
	PIAF_MSG(SWLOG_INFO, "Current path = '%s'",
			 dir.absolutePath().toAscii().data());

	mOpenNIStatus = OpenNI::initialize();
	if(CHECK_XN_STATUS != 0)
	{
		PIAF_MSG(SWLOG_ERROR, "Could not init OpenNI API");
		return;
	}


	// Open the file
	mOpenNIStatus = mDevice.open(mVideoFilePath.c_str());
	if(CHECK_XN_STATUS != 0)
	{
		PIAF_MSG(SWLOG_ERROR,
				 "Could not open recorded file '%s' or create player",
				 mVideoFilePath.c_str());
		return;
	}
#else
	// Open filename
	mOpenNIStatus = mContext.OpenFileRecording(filename, mPlayer);
#endif
	if(CHECK_XN_STATUS != 0)
	{
		PIAF_MSG(SWLOG_ERROR,
				 "Could not open recorded file '%s'' or create player",
				 mVideoFilePath.c_str());
	}
	else
	{
		if(mEnableDepth)
		{
			DEBUG_MSG("OpenNI : creating depth generator...");
#ifdef HAS_OPENNI2
			// Create depth generator
			mOpenNIStatus = mDepthGenerator.create(mDevice, SENSOR_DEPTH);
#else

			mOpenNIStatus = mContext.FindExistingNode(XN_NODE_TYPE_DEPTH,
													  mDepthGenerator);
#endif
			if(checkOpenNIError(__func__, __LINE__) != 0)
			{
				PIAF_MSG(SWLOG_ERROR, "Could not find any Depth Generator in context.");
			}
			else
			{
//				mDepthGenerator.start();
				if(checkOpenNIError(__func__, __LINE__) != 0)
				{
					PIAF_MSG(SWLOG_ERROR, "Could not start Depth Generator.");
#ifdef HAS_OPENNI2
					mDepthGenerator.destroy();
#endif
				}

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
#ifdef HAS_OPENNI2

				/// \todo use fov of sensor
				float fHFOV = 1.f; //fov.fHFOV;
				float fVFOV = 1.f; //fov.fVFOV;

				if (!mDepthGenerator.isValid() )
				{
					mDepthSize = cvSize(0,0);
					mDepthFPS = 0.f;
					mMinDistance = 1.f;
				} else {
					VideoMode depthVideoMode = mDepthGenerator.getVideoMode();

					mDepthSize.width = depthVideoMode.getResolutionX();
					mDepthSize.height = depthVideoMode.getResolutionY();
					mDepthFPS = depthVideoMode.getFps();
					mMinDistance = 1.f; ///< \todo Weird
				}
#else
				xn::DepthMetaData dmd;
				mDepthGenerator.GetMetaData(dmd);
				mDepthSize.width = (int)dmd.FullXRes();
				mDepthSize.height = (int)dmd.FullYRes();
				mDepthFPS = (int)dmd.FPS();

				XnFieldOfView fov;
				mDepthGenerator.GetFieldOfView(fov);
				float fHFOV = fov.fHFOV;
				float fVFOV = fov.fVFOV;
#endif
				mMinDistance = 1.;
				mScaleFactorWidth = 2.f * mMinDistance * tan(fHFOV / 2.)/ (float)mDepthSize.width;
				mScaleFactorHeight = 2.f * mMinDistance * tan(fVFOV / 2.)/ (float)mDepthSize.height;

				OPENNI_PRINTF("Opened file '%s': %dx%d @ %g fps, scX,Y=%g,%g",
						 filename,
						 mDepthSize.width, mDepthSize.height,
						 mDepthFPS,
						 mScaleFactorWidth, mScaleFactorHeight);
			}
		}


		if(mEnableCamera)
		{
			DEBUG_MSG("OpenNI : creating RGB generator ...");
#ifdef HAS_OPENNI2
			// Create depth generator
			mOpenNIStatus = mImageGenerator.create(mDevice, SENSOR_COLOR);
#else
			mOpenNIStatus = mContext.FindExistingNode(XN_NODE_TYPE_IMAGE,mImageGenerator);
#endif
			if(checkOpenNIError(__func__, __LINE__) != 0)
			{
				PIAF_MSG(SWLOG_ERROR, "Could not find any Image Generator in context.");
			}
			else
			{
#ifdef HAS_OPENNI2
				// Create RGB generator
//				mImageGenerator.start();
				if(checkOpenNIError(__func__, __LINE__) != 0)
				{
					PIAF_MSG(SWLOG_ERROR, "Could not start Image Generator in context.");
					mImageGenerator.destroy();
				}
#endif

			}
		}

		// Repeat option
		PIAF_MSG(SWLOG_INFO, "VideoRepeat=%c", mVideoRepeat ? 'T':'F');
#ifdef HAS_OPENNI
		mOpenNIStatus = mPlayer.SetRepeat(mVideoRepeat);
		// So that when the video ends, the source stops generating
#else
		mOpenNIStatus = mDevice.getPlaybackControl()->setRepeatEnabled(mVideoRepeat);
#endif

		PIAF_MSG(SWLOG_INFO, "File '%s' opened ! Success", filename);
	}
	m_captureIsInitialised = 1;


	m_video_properties.frame_width = mImageInfo.width = mDepthSize.width;
	m_video_properties.frame_height =mImageInfo.height = mDepthSize.height;
	mImageInfo.filepath = QString::fromStdString( mVideoFilePath );
	m_video_properties.fps = mImageInfo.fps = mDepthFPS;
	/// \todo continue with other values

	OPENNI_PRINTF("Calling startAcquisition");
	startAcquisition();
}

OpenNIFileAcquisition::~OpenNIFileAcquisition()
{
	stopAcquisition();
#ifdef HAS_OPENNI2
	if (m_streams != NULL)
	{
		delete [] m_streams;
	}
#endif
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
#ifdef HAS_OPENNI2
	if (mOpenNIStatus != STATUS_OK)
	{
		fprintf(stderr, "ERROR: ******************************************\n"
				  "OpenNIFileAcquisition::%s:%d : status='%s'\n"
				  "******************************************\n\n",
				  function,
				  line,
				  openni::OpenNI::getExtendedError());
		return -1;
	}
#else
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
#endif
	return 0;
}



int OpenNIFileAcquisition::init()
{
	init_OpenNI_depth_LUT();

	mFreeRun = false; // frame by frame

	mEnableDepth = true; // pure depth by default
	mEnableCamera = false;

	mVideoRepeat = false; // no repeat

	m_OpenNI_angle = 0;
	m_OpenNI_led = 0;
	m_got_rgb = 0;
	m_got_depth = 0;

#ifdef HAS_OPENNI2
	// Go back to apps path
	QDir dir;
	dir.cd( QApplication::applicationDirPath() );

	PIAF_MSG(SWLOG_INFO, "OpenNI::initialize() from current path = '%s'",
			 dir.absolutePath().toAscii().data());

	mOpenNIStatus = openni::OpenNI::initialize();

	m_streams = new openni::VideoStream*[2];
	m_streams[0] = &mDepthGenerator;
	m_streams[1] = &mImageGenerator;
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

	if (mOpenNIStatus != XN_STATUS_OK)
	{
		OPENNI_PRINTF("Could not start acquisition ...");
		return -1;
	}
#ifdef HAS_OPENNI2
	if(!mDepthGenerator.isValid())
#else
	if(!mDepthGenerator)
#endif
	{
		OPENNI_PRINTF("Could not start depth ...");
		return -1;
	}
	m_run = true;

#ifdef HAS_OPENNI2
	mDepthGenerator.start();
	mImageGenerator.start();
#else
	mDepthGenerator.StartGenerating();
#endif

	// start event thread
	start();

	m_captureIsInitialised = true;

	// Grab first image to get the size
	grab();

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

int OpenNIFileAcquisition::acquireFrameNow()
{
#ifdef HAS_OPENNI2
	int changedIndex;
	openni::Status rc = openni::OpenNI::waitForAnyStream(m_streams, 2, &changedIndex);
	if (rc != openni::STATUS_OK)
	{
		OPENNI_PRINTF("Wait failed with code=%d = %s",
					  (int)rc, openni::OpenNI::getExtendedError());
		return -1;
	}

	switch (changedIndex)
	{
	case 0:
		rc = mDepthGenerator.readFrame(&m_depthFrame);
		break;
	case 1:
		rc = mImageGenerator.readFrame(&m_colorFrame);
		break;
	default:
		OPENNI_PRINTF("Error in wait\n");
	}

	OPENNI_PRINTF("readFrame returned %d", (int)rc);
	if (rc != STATUS_OK)
	{
		OPENNI_PRINTF("readFrame failed for changedIndex=%d", changedIndex);
		return -1;
	}

	// Get the current frame
	uint32_t timestamp = 0;
	void * pDepthBufferVoid = NULL;
	switch (changedIndex)
	{
	default:
		break;
	case 0: {
		const openni::DepthPixel* pDepthMap = (const openni::DepthPixel*)m_depthFrame.getData();
		timestamp = m_depthFrame.getTimestamp() ; /// \todo check timecode
		pDepthBufferVoid = (void *)pDepthMap;

		//int rowSize = m_depthFrame.getStrideInBytes() / sizeof(openni::DepthPixel);
		//OPENNI_PRINTF("Depth rowStride=%d timestamp=%llu",
		//			  rowSize, (unsigned long long)timestamp);
		}break;
	case 1: {
		OPENNI_PRINTF("TO BE IMPLEMENTED"); /// \todo
		}break;
	}


	m_video_properties.frame_width =
			mImageInfo.width =
			mDepthSize.width =
			m_imageSize.width =
			m_depthFrame.getWidth();
	m_video_properties.frame_height =
			mImageInfo.height =
			mDepthSize.height =
			m_imageSize.height =
			m_depthFrame.getHeight();

	setRawDepthBuffer( pDepthBufferVoid, timestamp );
#endif
	return 0;
}

/* Function called by the doc (parent) thread */
int OpenNIFileAcquisition::grab()
{
	if(!m_captureIsInitialised) {
		OPENNI_PRINTF("Capture is not initialized");
		return -1;
	}

#if defined(HAS_OPENNI2)
	//&& defined(DIRECT_GRAB)
	if(!mFreeRun)
	{
		return acquireFrameNow();
	} // else we wait for the grab
#endif

	// Wait for acq
	if(!mGrabWaitCondition.wait(&mGrabMutex, 200))
	{
		OPENNI_PRINTF("wait failed");
		return -1;
	}

	OPENNI_PRINTF("grab SUCCESS !");
	printVideoProperties(&m_video_properties);
	return 0;
}


/* Set the raw depth buffer (11 bit in 16bit short) */
int OpenNIFileAcquisition::setRawDepthBuffer(void * depthbuf, uint32_t timestamp)
{
	if(!m_depthRawImage16U)
	{
		OPENNI_PRINTF("Create m_depthRawImage16U: %d x %d x 16U",
					  mDepthSize.width, mDepthSize.height);
		m_depthRawImage16U = swCreateImage(mDepthSize, IPL_DEPTH_16U, 1);
	}

	if(m_depthRawImage16U->widthStep == (int)sizeof(uint16_t) * mDepthSize.width)
	{
		// copy directly
		memcpy(m_depthRawImage16U->imageData, depthbuf, m_depthRawImage16U->widthStep * m_depthRawImage16U->height);
	}
	else
	{
		OPENNI_PRINTF("pitch is not the same : m_depthRawImage16U->widthStep=%d != (int)sizeof(uint16_t) * m_imageSize.width=%d",
					  m_depthRawImage16U->widthStep,
					  mDepthSize.height );
		// copy each line
		u16 * depth16U = (u16 *)depthbuf;
		for(int r = 0; r<mDepthSize.height; ++r)
		{
			memcpy(m_depthRawImage16U->imageData + r * m_depthRawImage16U->widthStep,
				   depth16U + r * mDepthSize.width,
				   m_imageSize.width * sizeof(u16));
		}
	}

	m_got_depth ++;
	m_depth_timestamp = timestamp;

//	OPENNI_PRINTF("got depth %dx%d x 16b",
//				  m_imageSize.width,
//				  m_imageSize.height);

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
	m_run = false;
	int retry_ms = 0;
	while(m_isRunning && retry_ms < 10000)
	{
		m_run = false;
		retry_ms += 10;
		usleep(10000);
	}

#ifdef HAS_OPENNI2
	if(mDepthGenerator.isValid())
	{
		mDepthGenerator.stop();
	}
	if(mImageGenerator.isValid())
	{
		mImageGenerator.stop();
	}
#else
	if(mDepthGenerator)
	{
		mDepthGenerator.StopGenerating();
	}
	if(mImageGenerator)
	{
		mImageGenerator.StopGenerating();
	}
#endif



#ifdef HAS_OPENNI2
	mDepthGenerator.destroy();
	mImageGenerator.destroy();
#else
	mDepthGenerator.Release();
	scriptNode.Release();
	mContext.Release();
#endif

	m_captureIsInitialised = 0;

	return 0;
}



void OpenNIFileAcquisition::run()
{
	m_isRunning = true;
	m_run = true;

	OPENNI_PRINTF("OpenNI acquisition thread started");

	//
	while( m_run )//&& processEvents() >= 0)
	{
#ifdef HAS_OPENNI2
		int changedIndex=0;
		if(mFreeRun)
		{
			mOpenNIStatus = OpenNI::waitForAnyStream(m_streams, 2, &changedIndex);
			//		OPENNI_PRINTF("waitForAnyStream returned with changed = %d !!",
			//					  changedIndex);
			if (mOpenNIStatus != STATUS_OK)
			{
				OPENNI_PRINTF("UpdateData failed wait 100ms");
				usleep(100000);
			}
		}
#else
		mOpenNIStatus = mContext.WaitOneUpdateAll(mDepthGenerator);
		if (mOpenNIStatus != XN_STATUS_OK)
		{
			OPENNI_PRINTF("UpdateData failed: '%s'", xnGetStatusString(mOpenNIStatus));
			usleep(100000);
		}
#endif
		else
		{
			uint32_t timestamp=0;
			void * pDepthBufferVoid = NULL;
#ifdef HAS_OPENNI2
			openni::Status rc ;
			switch (changedIndex)
			{
			case 0:
				rc = mDepthGenerator.readFrame(&m_depthFrame); break;
			case 1:
				rc = mImageGenerator.readFrame(&m_colorFrame); break;
			default:
				PIAF_MSG(SWLOG_ERROR, "Error in wait\n");
			}

			//OPENNI_PRINTF("readFrame returned %d", (int)rc);
			if (rc != STATUS_OK)
			{
				OPENNI_PRINTF("readFrame failed");
			}
			switch (changedIndex)
			{
			default:
				break;
			case 0: {
				const openni::DepthPixel* pDepthMap = (const openni::DepthPixel*)m_depthFrame.getData();
				int rowSize = m_depthFrame.getStrideInBytes() / sizeof(openni::DepthPixel);
				timestamp = m_depthFrame.getTimestamp() ; /// \todo check timecode
				pDepthBufferVoid = (void *)pDepthMap;

				m_video_properties.frame_width =
						mImageInfo.width =
						mDepthSize.width =
						m_imageSize.width =
						m_depthFrame.getWidth();
				m_video_properties.frame_height =
						mImageInfo.height =
						mDepthSize.height =
						m_imageSize.height =
						m_depthFrame.getHeight();


//				OPENNI_PRINTF("Depth rowStride=%d timestamp=%llu",
//						 rowSize, (unsigned long long)timestamp);
				}break;
			case 1: {
				OPENNI_PRINTF("TO BE IMPLEMENTED"); /// \todo
				}break;
			}
#else
			xnFPSMarkFrame(&xnFPS);
			m_video_properties.frame_width =
					mImageInfo.width =
					m_imageSize.width =
					mDepthSize.width ;
			m_video_properties.frame_height =
					mImageInfo.height =
					m_imageSize.height =
					mDepthSize.height ;

			mDepthGenerator.GetMetaData(depthMD);
			const XnDepthPixel* pDepthMap = depthMD.Data();
			pDepthBufferVoid = (void *)pDepthMap;
			timestamp = depthMD.Timestamp();
			//			OPENNI_PRINTF("Frame %d Middle point is: %u. FPS: %f\n",
			//						  depthMD.FrameID(), depthMD(depthMD.XRes() / 2, depthMD.YRes() / 2), xnFPSCalc(&xnFPS));
#endif
			setRawDepthBuffer( pDepthBufferVoid, timestamp );
		}
	}

	OPENNI_PRINTF("Thread has stopped");
	m_isRunning = false;
}

/** \brief Get the list of output format */
QList<t_video_output_format> OpenNIFileAcquisition::getOutputFormats()
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
int OpenNIFileAcquisition::setOutputFormat(int id)
{
	DEBUG_MSG("NOT IMPLEMENTED => id=%d", id);

	return id;
}

/** @brief Read image as data of selected format */
IplImage * OpenNIFileAcquisition::readImage()
{
	DEBUG_MSG("NOT IMPLEMENTED => return readimageRGB32()");
	return readImageRGB32();
}


/** \brief Grabs one image and convert to RGB32 coding format
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * OpenNIFileAcquisition::readImageRGB32()
{
	if(!m_captureIsInitialised) {
		OPENNI_PRINTF("m_captureIsInitialised = %d", m_captureIsInitialised);

		return NULL;
	}

	if(m_depthRawImage16U) {
		// Return this image
		return m_depthRawImage16U;
	}

	if(!m_bgr32Image)
	{
		m_bgr32Image = swCreateImage(m_imageSize, IPL_DEPTH_8U, 4);
		OPENNI_PRINTF("create BGR32 image : %dx%dx%dbx%d",
					  m_bgr32Image->width, m_bgr32Image->height, m_bgr32Image->depth, m_bgr32Image->nChannels
					  );
		// set the alpha channel
		memset(m_bgr32Image->imageData, 255,
			   m_bgr32Image->widthStep*m_bgr32Image->height);
	}

	int mode = (int)round(m_video_properties.mode);

	OPENNI_PRINTF("mode = %d", mode);
	switch(mode)
	{
	default:
		OPENNI_PRINTF("FIXME: unsupported mode %d", mode);
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

//			OPENNI_PRINTF("mode = %d => 2Cm => gray => BGR32 %dx%dx%dbx%d\n",
//						  mode, m_bgr32Image->width, m_bgr32Image->height, m_bgr32Image->depth, m_bgr32Image->nChannels);

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
	if(!m_depthRawImage16U) {
		OPENNI_PRINTF("No depth image m_depthRawImage16U");
		return NULL; }
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


bool OpenNIFileAcquisition::isDeviceReady()
{
#ifdef HAS_OPENNI2
	return mDepthGenerator.isValid();
#else
	return (mDepthGenerator);
#endif
}


/** \brief Grabs one image and convert to grayscale coding format
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * OpenNIFileAcquisition::readImageY()
{
	if(!m_captureIsInitialised) { return NULL; }
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
#ifdef HAS_OPENNI2
//	m_video_properties.frame_count = mDep;
#else
	m_video_properties.pos_frames =	m_got_depth;
#endif
	m_video_properties.pos_avi_ratio =	-1.;
	m_video_properties.frame_width =	m_imageSize.width;
	m_video_properties.frame_height =	m_imageSize.height;
	m_video_properties.fps =			30.;
//	m_video_properties.fourcc_dble =	cvGetCaptureProperty(m_capture, CV_CAP_PROP_FOURCC );/*4-character code of codec */
//		char   fourcc[5] =	cvGetCaptureProperty(m_capture, FOURCC coding CV_CAP_PROP_FOURCC 4-character code of codec */
//		char   norm[8] =		cvGetCaptureProperty(m_capture, Norm: pal, ntsc, secam */
#ifdef HAS_OPENNI2
	m_video_properties.frame_count =	mDevice.getPlaybackControl()->getNumberOfFrames(mDepthGenerator);
#else
	OPENNI_PRINTF("not implemented for frame_count");
#endif


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


/** Return the absolute position to be stored in database (needed for post-processing or permanent encoding) */
unsigned long long OpenNIFileAcquisition::getAbsolutePosition()
{
#ifdef HAS_OPENNI2
	return m_video_properties.pos_frames;
#else
	OPENNI_PRINTF("not implemented");
	return 0;
#endif
}

void OpenNIFileAcquisition::rewindMovie()
{
#ifdef HAS_OPENNI2
	OPENNI_PRINTF("not implemented/COMMENTED");
	return ;
	if(mDevice.getPlaybackControl() && mDepthGenerator.isValid())
	{
		mDevice.getPlaybackControl()->seek(mDepthGenerator, 0);
	}
	if(mDevice.getPlaybackControl() && mImageGenerator.isValid())
	{
		mDevice.getPlaybackControl()->seek(mImageGenerator, 0);
	}
#else
	OPENNI_PRINTF("not implemented");
#endif
}

bool OpenNIFileAcquisition::endOfFile()
{
	/// \todo completer
	return false;
}

/** @brief Go to absolute position in file (in bytes from start) */
void OpenNIFileAcquisition::rewindToPosMovie(unsigned long long position)
{
#ifdef HAS_OPENNI2
	mDevice.getPlaybackControl()->seek(mDepthGenerator, position);
	mDevice.getPlaybackControl()->seek(mImageGenerator, position);
#else
	OPENNI_PRINTF("not implemented");
#endif
}

/** @brief Read next frame */
bool OpenNIFileAcquisition::GetNextFrame()
{
	return (grab() >= 0);
}

/** set absolute position in file
 */
void OpenNIFileAcquisition::setAbsolutePosition(unsigned long long newpos)
{
#ifdef HAS_OPENNI2
	mDevice.getPlaybackControl()->seek(mDepthGenerator, newpos);
	mDevice.getPlaybackControl()->seek(mImageGenerator, newpos);
#else
	OPENNI_PRINTF("not implemented");
#endif
}

/** go to frame position in file
 */
void OpenNIFileAcquisition::setAbsoluteFrame(int frame)
{
#ifdef HAS_OPENNI2
	mDevice.getPlaybackControl()->seek(mDepthGenerator, frame);
	mDevice.getPlaybackControl()->seek(mImageGenerator, frame);
#else
	OPENNI_PRINTF("not implemented");
#endif
}



