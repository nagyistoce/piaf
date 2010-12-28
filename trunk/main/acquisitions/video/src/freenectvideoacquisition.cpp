
#include "freenectvideoacquisition.h"

#include "swvideodetector.h"

FreenectVideoAcquisition::FreenectVideoAcquisition(int idx_device)
{
	//m_current_format = m_requested_format = FREENECT_VIDEO_RGB;

	m_current_format = m_requested_format = FREENECT_VIDEO_IR_8BIT;
	m_freenect_ctx = NULL;
	m_freenect_dev = NULL;
	m_freenect_angle = 0;
	m_freenect_led = 0;
	m_got_rgb = 0;
	m_got_depth = 0;

	m_captureIsInitialised = false;

	m_run = m_isRunning = false;
	m_freenect_dev = NULL;
	m_freenect_ctx = NULL;

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


	m_idx_device = idx_device;
	memset(&m_video_properties, 0, sizeof(t_video_properties));

	m_imageSize = cvSize(0,0);

	// Initialize device
	if (freenect_init(&m_freenect_ctx, 0)) {
		printf("Error: Cannot get context\n");
		return ;
	}

	if (freenect_open_device(m_freenect_ctx, &m_freenect_dev, 0)) {
		printf("Error: Cannot get device\n");

		return ;
	}

}


FreenectVideoAcquisition::~FreenectVideoAcquisition()
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

/* Use sequential mode
	If true, grabbing is done when a new image is requested.
	Else a thread is started to grab */
void FreenectVideoAcquisition::setSequentialMode(bool on)
{

}

QList<FreenectVideoAcquisition *> g_freenectDevices;
// CALLBACKS

void depth_cb(freenect_device *dev, void *depth, uint32_t timestamp) {
	// Find device
	QList<FreenectVideoAcquisition *>::iterator it;
	for(it = g_freenectDevices.begin(); it != g_freenectDevices.end(); ++it)
	{
		FreenectVideoAcquisition * freenect = *it;
		if(freenect->getFreenectDevice() == dev)
		{
			// set the buffer
			freenect->setRawDepthBuffer(depth, timestamp);
			return;
		}
	}

}

void rgb_cb(freenect_device *dev, void *rgb, uint32_t timestamp) {
	fprintf(stderr, "[Freenect]::%s:%d : got IR/RGB image : %p\n",
			__func__, __LINE__,
			rgb);

	// Find device
	QList<FreenectVideoAcquisition *>::iterator it;
	for(it = g_freenectDevices.begin(); it != g_freenectDevices.end(); ++it)
	{
		FreenectVideoAcquisition * freenect = *it;
		if(freenect->getFreenectDevice() == dev)
		{
			// set the buffer
			freenect->setRawImageBuffer(rgb, timestamp);
			return;
		}
	}
}

/* Set the raw depth buffer (11 bit in 16bit short) */
int FreenectVideoAcquisition::setRawDepthBuffer(void * depthbuf, uint32_t timestamp )
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
			memcpy(m_depthRawImage16U->imageData + r*m_depthRawImage16U->widthStep,
				   depthbuf + r * m_imageSize.width,
				   m_imageSize.width * sizeof(u16));
		}
	}

	m_got_depth ++;
	m_depth_timestamp = timestamp;

	fprintf(stderr, "[Freenect]::%s:%d : got depth %dx%d x 1\n",
			__func__, __LINE__,
			m_imageSize.width, m_imageSize.height);

	mGrabWaitCondition.wakeAll();

	return 0;
}

/* Set the raw IR/RGB buffer (RGB or IR 8bit) */
int FreenectVideoAcquisition::setRawImageBuffer(void * imgbuf, uint32_t timestamp)
{
	if(m_current_format == FREENECT_VIDEO_RGB)
	{
		if(!m_cameraRGBImage8U) {
			fprintf(stderr, "[Freenect]::%s:%d : create RGB image %dx%d x 3\n",
					__func__, __LINE__,
					m_imageSize.width, m_imageSize.height);
			m_cameraRGBImage8U = swCreateImage(m_imageSize, IPL_DEPTH_8U, 3);
		}

		if(m_cameraRGBImage8U->widthStep == 3*m_cameraRGBImage8U->width) { // copy directly
			memcpy(m_cameraRGBImage8U->imageData,
				   imgbuf,
				   m_cameraRGBImage8U->widthStep * m_cameraRGBImage8U->height);
		} else {
			u8 * img8U = (u8 *)imgbuf;
			for(int r = 0; r<m_imageSize.height; ++r)
			{
				memcpy(m_cameraRGBImage8U->imageData + r*m_cameraRGBImage8U->widthStep,
					   img8U + r * 3 * m_imageSize.width,
					   3 * m_imageSize.width * sizeof(u8));
			}

		}
	}
	else if(m_current_format == FREENECT_VIDEO_IR_8BIT)
	{
		if(!m_cameraIRImage8U) {
			fprintf(stderr, "[Freenect]::%s:%d : create IR image %dx%d x 1\n",
					__func__, __LINE__,
					m_imageSize.width, m_imageSize.height);
			m_cameraIRImage8U = swCreateImage(m_imageSize, IPL_DEPTH_8U, 1);
		}

		if(m_cameraIRImage8U->widthStep == m_cameraIRImage8U->width) { // copy directly
			memcpy(m_cameraIRImage8U->imageData,
				   imgbuf,
				   m_cameraIRImage8U->widthStep * m_cameraIRImage8U->height);
		} else {
			u8 * img8U = (u8 *)imgbuf;
			for(int r = 0; r<m_imageSize.height; ++r)
			{
				memcpy(m_cameraIRImage8U->imageData + r*m_cameraIRImage8U->widthStep,
					   img8U + r * m_imageSize.width,
					   m_imageSize.width * sizeof(u8));
			}
		}
	}

	m_got_rgb++;
	m_rgb_timestamp = timestamp;
	fprintf(stderr, "[Freenect]::%s:%d : got image %dx%d x 1\n",
			__func__, __LINE__,
			m_imageSize.width, m_imageSize.height);

	mGrabWaitCondition.wakeAll();
	return 0;
}

void * freenect_threadfunc(void *arg)
{
	FreenectVideoAcquisition * pfreenect = (FreenectVideoAcquisition *)arg;
	fprintf(stderr, "[Freenect]::%s:%d : thread started for dev=%p\n", __func__, __LINE__,
			pfreenect->getFreenectDevice());

	while(pfreenect->isRunning()) {
		pfreenect->processEvents();
	}

	fprintf(stderr, "[Freenect]::%s:%d : thread ended for dev=%p\n", __func__, __LINE__,
			pfreenect->getFreenectDevice());

	return NULL;
}

/** \brief Start acquisition */
int FreenectVideoAcquisition::startAcquisition()
{
	if(!m_freenect_dev) return -1;

	freenect_set_depth_format(m_freenect_dev, FREENECT_DEPTH_11BIT);
	freenect_start_depth(m_freenect_dev);

	freenect_set_video_format(m_freenect_dev, m_requested_format);
	freenect_start_video(m_freenect_dev);

	m_imageSize.width = FREENECT_FRAME_W;
	m_imageSize.height = FREENECT_FRAME_H;

	freenect_set_depth_callback(m_freenect_dev, depth_cb);
	freenect_set_video_callback(m_freenect_dev, rgb_cb);

/*
	int res = pthread_create(&m_freenect_thread, NULL, freenect_threadfunc, this);
	if (res) {
			printf("pthread_create failed\n");
			return 1;
	}
*/
	// append to managed list
	g_freenectDevices.append(this);


	m_rgb_timestamp = m_depth_timestamp = 0;

	fprintf(stderr, "[Freenect]::%s:%d : capture=%p\n", __func__, __LINE__,
			m_freenect_dev);


	m_run = true;
	// start event thread
	start();

	// update video properties
	updateVideoProperties();

	m_captureIsInitialised = true;
	fprintf(stderr, "[Freenect]::%s:%d : init OK !\n", __func__, __LINE__);

	return 0;
}

int FreenectVideoAcquisition::processEvents()
{
	if(!m_freenect_ctx) {
		return -1;
	}

	if(m_requested_format != m_current_format) {

		// Restart acq
		if (m_requested_format != m_current_format) {
			freenect_stop_video(m_freenect_dev);
			freenect_set_video_format(m_freenect_dev, m_requested_format);
			freenect_start_video(m_freenect_dev);
			m_current_format = m_requested_format;
		}

	}

	// process freenect events
	return freenect_process_events(m_freenect_ctx);
}

/* Function called by the doc (parent) thread */
int FreenectVideoAcquisition::grab()
{
	if(!m_freenect_ctx) {
		return -1;
	}

	// Wait for acq
	if(!mGrabWaitCondition.wait(&mGrabMutex, 200))
	{
		return -1;
	}

	fprintf(stderr, "Freenect::%s:%d : unlocked condition\n", __func__, __LINE__);
	return 0;
}

void FreenectVideoAcquisition::run()
{
	m_isRunning = true;
	m_run = true;
	//
	while(m_run && processEvents() >= 0)
	{
		//fprintf(stderr, "Freenect::%s:%d : process loop\n", __func__, __LINE__);
		;
	}

	m_isRunning = false;
}

/* Return image size */
CvSize FreenectVideoAcquisition::getImageSize()
{
	return m_imageSize;
}

/* Stop acquisition */
int FreenectVideoAcquisition::stopAcquisition()
{
	g_freenectDevices.remove(this);

	m_run = false;
	while(m_isRunning)
	{
		usleep(1000);
	}

	if(m_freenect_dev) {
		freenect_stop_depth(m_freenect_dev);
		freenect_stop_video(m_freenect_dev);
		freenect_close_device(m_freenect_dev);

		// remove for handled list
		m_freenect_dev = NULL;
	}

	if(m_freenect_dev) {
		freenect_shutdown(m_freenect_ctx);
		m_freenect_ctx = NULL;
	}

	m_captureIsInitialised = 0;

	return 0;
}
static float g_depth_LUT[2047] = {-1.f };

void init_depth_LUT() {
	if(g_depth_LUT[0]>=0) return;
	/* From OpenKinect :
	   http://openkinect.org/w/index.php?title=Imaging_Information&oldid=613
	distance = 0.1236 * tan(rawDisparity / 2842.5 + 1.1863)
	   */
	for(int raw = 0; raw<2047; raw++) {
		g_depth_LUT[raw] = (float)( 0.1236 * tan((double)raw/2842.5 + 1.1863) );
	}

	g_depth_LUT[2047]=-1.f;
}

/** \brief Grabs one image and convert to RGB32 coding format
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * FreenectVideoAcquisition::readImageRGB32()
{
	if(!m_captureIsInitialised) return NULL;

	if(!m_bgr32Image)
	{
		fprintf(stderr, "[Freenect]::%s:%d : create BGR32 image\n", __func__, __LINE__);
		m_bgr32Image = swCreateImage(m_imageSize, IPL_DEPTH_8U, 4);
	}

	int mode = (int)round(m_video_properties.mode);
#define FREENECT_MODE_DEPTH_2CM	0
	switch(mode)
	{
	default:
		if(m_requested_format == FREENECT_VIDEO_RGB && m_cameraRGBImage8U) {
			cvCvtColor(m_cameraRGBImage8U, m_bgr32Image, CV_RGB2BGRA);
		} else if(m_cameraRGBImage8U) {
			cvCvtColor(m_cameraIRImage8U, m_bgr32Image, CV_GRAY2BGRA);
		}
		break;
	case FREENECT_MODE_DEPTH_2CM: // 2CM
		init_depth_LUT();

		if(!m_grayImage)
		{
			m_grayImage = swCreateImage(m_imageSize, IPL_DEPTH_8U, 1);
		}

		// convert to gray
		if(getDepthImage32F()) {

			for(int r=0; r<m_depthImage32F->height; r++)
			{
				float * linedepth = (float *)(m_depthImage32F->imageData + r*m_depthImage32F->widthStep);
				u8 * line8u = (u8 *)(m_grayImage->imageData + r*m_grayImage->widthStep);
				for(int c = 0; c<m_grayImage->width; c++)
				{
					line8u[c] = (u8)roundf(linedepth[c]*100.f/2.f);
				}
			}
			cvCvtColor(m_grayImage, m_bgr32Image, CV_GRAY2BGRA);
		}
		break;
	}

	return m_bgr32Image;
}

IplImage * FreenectVideoAcquisition::getDepthImage32F()
{
	if(!m_depthRawImage16U) return NULL;
	if(!m_depthImage32F) {
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
IplImage * FreenectVideoAcquisition::readImageY()
{
	if(!m_captureIsInitialised) return NULL;
	if(!m_grayImage)
	{
		m_grayImage = swCreateImage(m_imageSize, IPL_DEPTH_8U, 1);
	}

	if(m_requested_format == FREENECT_VIDEO_RGB)
		cvConvert(m_cameraRGBImage8U, m_grayImage);
	else if(m_cameraIRImage8U)
		cvCopy(m_cameraIRImage8U, m_grayImage);

	return m_grayImage;
}

/** \brief Grabs one image of depth buffer
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * FreenectVideoAcquisition::readImageDepth()
{
	return m_depthRawImage16U;
}


/** @brief Get video properties (not updated) */
t_video_properties FreenectVideoAcquisition::getVideoProperties()
{
	return m_video_properties;
}

/** @brief Update and return video properties */
t_video_properties FreenectVideoAcquisition::updateVideoProperties()
{
	return m_video_properties;
}


/** @brief Set video properties (not updated) */
int FreenectVideoAcquisition::setVideoProperties(t_video_properties props)
{

	return 0;
}














