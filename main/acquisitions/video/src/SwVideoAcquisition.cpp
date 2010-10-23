/***************************************************************************
						  SwVideoAcquisition.cpp  -  description
							 -------------------
	begin                : Tue Mar 12 2002
	copyright            : (C) 2002 by Olivier Viné & Christophe Seyve
	email                : olivier.vine@sisell.com  & cseyve@free.fr
	description 		 : class in charge of video acquisition
							(init, start and stop, get properties, acquire, ...)
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SwVideoAcquisition.h"

#include "swvideodetector.h"

SwVideoAcquisition::SwVideoAcquisition()
{
	// no device is wanted, try default device
	//m_capture = new V4L2Device();
	m_capture = NULL;
	m_iplImage = NULL;
	tBoxSize newSize;
	newSize.width = 640;
	newSize.height = 480;
	openDevice(0, newSize);
}

SwVideoAcquisition::SwVideoAcquisition(int idx_device)
{
	// no device is wanted, try default device
	tBoxSize newSize;
	newSize.width = 640;
	newSize.height = 480;
	m_iplImage = NULL;
	m_capture = NULL;

	openDevice(idx_device, newSize);
}

SwVideoAcquisition::SwVideoAcquisition(int idx_device, tBoxSize newSize)
{
	m_capture = NULL;
	m_iplImage = NULL;
	openDevice( idx_device, newSize );
}

int SwVideoAcquisition::getcurrentchannel() {
	fprintf(stderr, "[VidAcq] %s:%d: FIXME...\n", __func__, __LINE__);
	return -1;
//	return m_capture->getcurrentchannel();
}
char * SwVideoAcquisition::getnorm() {
	static char norm_const[]="pal";
	return norm_const;
	//return m_capture->getnorm();
}

int SwVideoAcquisition::openDevice(int idx_device, tBoxSize newSize)
{
	myAcqIsInitialised = false;

	// trying to open asked device
	fprintf(stderr, "[VidAcq]::%s:%d : new OpenCV capture...(m_capture=%p)\n",
			__func__, __LINE__, m_capture); fflush(stderr);

	if(m_capture) {
		delete m_capture;
		m_capture = NULL;
	}

	swReleaseImage(	&m_iplImage );

	fprintf(stderr, "[VidAcq]::%s:%d : new OpenCV capture...\n",
			__func__, __LINE__); fflush(stderr);

	//m_capture = new V4L2Device();
	// open any type of device and index of desired device
	#ifdef cv::Exception
	try {
		m_capture = cvCreateCameraCapture(CV_CAP_ANY + idx_device );
	} catch (cv::Exception e) {
		fprintf(stderr, "[VidAcq] : VDopen ERROR : CAUGHT EXCEPTION WHILE OPENING DEVICE  [%d]\n",
				idx_device
				); fflush(stderr);
		imageSize.width = imageSize.height = 0;
		return 0;
	}
#else
	m_capture = cvCreateCameraCapture(CV_CAP_ANY + idx_device );
#endif
	fprintf(stderr, "[VAcq]::%s:%d : capture=m_capture=%p\n", __func__, __LINE__, m_capture);
	if(m_capture) {
		fprintf(stderr, "[VidAcq] : VDopen SUCCESS : OPEN DEVICE  [%d]\n",
				idx_device
				); fflush(stderr);

	} else {
		fprintf(stderr, "[VidAcq] : VDopen ERROR : CANNOT OPEN DEVICE  [%d]\n",
				idx_device
				); fflush(stderr);
		imageSize.width = imageSize.height = 0;
		return 0;
	}


	fprintf(stderr, "[VidAcq] : VDopen : capture first frame to see if it's posisble\n"); fflush(stderr);
	// Grab image
	if( !cvGrabFrame( m_capture ) ) {
		fprintf(stderr, "[VA ] %s:%d : capture=%p FAILED\n", __func__, __LINE__, m_capture);
		imageSize.width = imageSize.height = 0;
		return 0;
	} else {
		IplImage * frame = cvRetrieveFrame( m_capture );
		if(frame) {
			imageSize.width = frame->width;
			imageSize.height = frame->height;
			fprintf(stderr, "[VidAcq] : VDIsInitialised...\n"); fflush(stderr);
			m_captureIsInitialised = 1; // m_capture->VDIsInitialised();
		} else {
			m_captureIsInitialised = 0; // m_capture->VDIsInitialised();
		}
	}

	// calculate image size	fprintf(stderr, "[VidAcq] : calcBufferSize...\n"); fflush(stderr);
	calcBufferSize();

	myAcqIsInitialised = true;

	// update video properties
	updateVideoProperties();

	return 1;
}

void SwVideoAcquisition::calcBufferSize()
{
	if(!m_iplImage) {
		fprintf(stderr, "SwVA::%s:%d : input image is null\n", __func__, __LINE__);
		return;
	}

	m_capture_palette = VIDEO_PALETTE_RGB32; //=> Cv FIXME //m_capture->getpalette();
	//=> Cv FIXME //m_capture->getcapturewindowsize(&imageSize);

	imageSize.width = m_iplImage->width;
	imageSize.height = m_iplImage->height;

	int image_buffer_size = 0;
	switch(m_capture_palette)
	{
	case VIDEO_PALETTE_GREY:
		image_buffer_size = imageSize.width*imageSize.height;
		break;
	case VIDEO_PALETTE_YUV420P:
		image_buffer_size = imageSize.width*imageSize.height*3/2;
		break;
	case VIDEO_PALETTE_YUYV:
	case VIDEO_PALETTE_UYVY:
		image_buffer_size = imageSize.width*imageSize.height*2;
		break;
	case VIDEO_PALETTE_RGB32:
		image_buffer_size = imageSize.width*imageSize.height*4;
		break;
	default:
		image_buffer_size = imageSize.width*imageSize.height*4;
		fprintf(stderr, "[VidAcq] : unsupported palette (%d) => FIXME. ABORT. Bye.\n",
			m_capture_palette);
		fflush(stderr);
		//exit(0);
		break;
	}

	receptBufferSize = image_buffer_size;
}


SwVideoAcquisition::SwVideoAcquisition(CvCapture * aVD)
{
	// try to initialise given component
	m_capture = aVD;


	m_captureIsInitialised = 0; //m_capture->VDIsInitialised();

	if(!m_captureIsInitialised)
	{
//		m_capture->VDInitAll();

		// FIXME : Grab first frame

		m_captureIsInitialised = 1; //m_capture->VDIsInitialised();
	}

	myAcqIsInitialised = false;
}

int SwVideoAcquisition::setChannel(int ch)
{
	if(! m_capture)
		return -1;

	return 0;
	//Cv Fixme return m_capture->setchannel(ch);
}
int SwVideoAcquisition::setNorm(char *norm)
{
	if(! m_capture)
		return -1;
	if(!norm)
		return -1;
	return 0; // FIXME //return m_capture->setnorm(norm);
}

int SwVideoAcquisition::changeAcqParams(tBoxSize newSize, int ch)
{
	// stop acquisition
	myAcqIsInitialised = false;


	// change acquisition size
	/* FIXME
	if(m_capture->changeSize(&newSize)<0) {
		fprintf(stderr, "[VidAcq]::changeAcqParams : ERROR : cannot change acquisition size !\n");
		return -1;
	}
	m_capture->setchannel(ch);
	*/

	calcBufferSize();

	myAcqIsInitialised = true;

	return 1;
}

SwVideoAcquisition::~SwVideoAcquisition()
{
	if(m_capture) {
		delete m_capture;
	}
	m_capture = NULL;
}

int SwVideoAcquisition::VAcloseVD()
{
	if(!m_capture) return -1;

	cvReleaseCapture( &m_capture );
	m_capture = NULL;
	return 0;
	//return (m_capture->VDclose());
}

int SwVideoAcquisition::VAInitialise( bool )//startCapture)
{
	if(!m_captureIsInitialised)
		return -1;
	// general initialisation
	contAcquisition = false;
	noAcquisition = 0;

	myAcqIsInitialised = true;

	return 0;
}

int SwVideoAcquisition::readImageRGB32(unsigned char * image,
		long * buffersize,
		bool //contAcq
	)
{
	if(image == NULL) {
		fprintf(stderr, "[VidAcq]::%s:%d : ERROR : no output buffer !\n", __func__, __LINE__);
		return -1;
	}
	unsigned char * imageBrute;

	if(!myAcqIsInitialised){
		fprintf(stderr, "[VidAcq]::%s:%d : ERROR : not initalized !\n", __func__, __LINE__);
		return -1;
	}

	// read raw image
	long raw_size = 0;
	if((imageBrute = readImageBuffer(&raw_size))==NULL) {
		if(buffersize) {
			*buffersize = 0;
		}
		fprintf(stderr, "[VidAcq]::%s:%d : ERROR : readImageBuffer failed !\n", __func__, __LINE__);
		return -1;
	}
	if(buffersize) {
		*buffersize = raw_size;
	}

	// if needed, conversion to RGB32 coding
	if(m_capture_palette == VIDEO_PALETTE_RGB32) {
		memcpy(image,  imageBrute, imageSize.width*imageSize.height*4);
	} else {
//		fprintf(stderr, "%s:%d : convert2RGB32...\n", __func__, __LINE__);
		convert2RGB32(imageBrute, image);
	}

	return 0;
}


int SwVideoAcquisition::getPalette()
{
//	m_capture_palette = m_capture->getpalette();
//	return m_capture_palette;
	if(!m_iplImage) return 0;

	if(m_iplImage->nChannels == 4) return VIDEO_PALETTE_RGB32;
	if(m_iplImage->nChannels == 3) return VIDEO_PALETTE_RGB24;
	return VIDEO_PALETTE_GREY;

}

int SwVideoAcquisition::readImageRaw(unsigned char * rawimage,
	unsigned char * ,//compimage,
	long * ,//compsize,
	bool //contAcq
)
{
	if(!myAcqIsInitialised)
		return -1;

	unsigned char * imageBrute = NULL;

	if(!myAcqIsInitialised) {
		fprintf(stderr, "SwVideoAcquisition::readImageRaw: Device not initalised\n");
		return -1;
	}

	// Reading raw image
	long uncompsize = 0;
	if((imageBrute = readImageBuffer(&uncompsize))==NULL)
	{
		fprintf(stderr, "SwVideoAcquisition::readImageRaw: readImageBuffer returned NULL\n");
		return -1;
	}
	if(rawimage == NULL)
		return 0;

	// no conversion, return raw image
	memcpy(rawimage,  imageBrute, receptBufferSize);

	return 0;
}

unsigned char * SwVideoAcquisition::readImageBuffer(long * buffersize)
{
	if(!myAcqIsInitialised) {
		fprintf(stderr, "[VidAcq]::%s:%d : ERROR : not intialized !\n", __func__, __LINE__);
		return NULL;
	}

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

		if(m_iplImage->nChannels == 4)
			m_capture_palette = VIDEO_PALETTE_RGB32;
		else if(m_iplImage->nChannels == 3)
			m_capture_palette = VIDEO_PALETTE_RGB24;
		else
			m_capture_palette = VIDEO_PALETTE_GREY;
	}

	if(frame) {
		if(frame->nChannels == m_iplImage->nChannels) {
			cvCopy(frame, m_iplImage);
		} else {
			cvConvert(frame, m_iplImage);
		}
/*
		FILE * ftmp = fopen("/dev/shm/tmp.pgm", "wb");
		if(ftmp) {
			fprintf(ftmp, "P5\n%d %d\n255\n", m_iplImage->width*m_iplImage->nChannels, m_iplImage->height);
			fwrite(m_iplImage->imageData, m_iplImage->width*m_iplImage->nChannels, m_iplImage->height, ftmp);
			fclose(ftmp);
		}

		fprintf(stderr, "[VidAcq]::%s:%d : update m_iplImage %dx%dx%d !\n",
				__func__, __LINE__,
				m_iplImage->width, m_iplImage->height,
				m_iplImage->nChannels
				);
	*/
	}

	if(buffersize) {
		*buffersize = m_iplImage->widthStep
					  * m_iplImage->height * m_iplImage->nChannels; // UNUSED ??
	}

	return (u8 *)m_iplImage->imageData; //m_capture->getFrameBuffer();
}

unsigned char * SwVideoAcquisition::getImageRawNoAcq()
{
	if(!m_iplImage) return NULL;
	return (u8 *)m_iplImage->imageData;
	//return m_capture->video_getaddress();
}
/* Get video properties (not updated) */
t_video_properties SwVideoAcquisition::getVideoProperties() {
	return m_video_properties;
}


void printVideoProperties(t_video_properties * prop) {
	if(!prop) { return ; }

	fprintf(stderr, "%s:%d : video_properties={"
			"  CV_CAP_PROP_POS_MSEC =%g,"
			"  CV_CAP_PROP_POS_FRAMES =%g,"
			"  CV_CAP_PROP_POS_AVI_RATIO =%g,"
			"  CV_CAP_PROP_FRAME_WIDTH  =%g,"
			"  CV_CAP_PROP_FRAME_HEIGHT =%g,"
			"  CV_CAP_PROP_FPS =%g,"
			"  CV_CAP_PROP_FOURCC =%g='%s'"
			"  CV_CAP_PROP_FRAME_COUNT =%g,"
			"  CV_CAP_PROP_FORMAT =%g,"
			"  CV_CAP_PROP_MODE =%g,"
			"  CV_CAP_PROP_BRIGHTNESS =%g,"
			"  CV_CAP_PROP_CONTRAST =%g,"
			"  CV_CAP_PROP_SATURATION =%g,"
			"  CV_CAP_PROP_HUE =%g,"
			"  CV_CAP_PROP_GAIN =%g,"
			"  CV_CAP_PROP_CONVERT_RGB =%g }\n\n",
			__func__, __LINE__,
			prop->pos_msec,
			prop->pos_frames,
			prop->pos_avi_ratio,
			prop->frame_width,
			prop->frame_height,
			prop->fps,
			prop->fourcc_dble, prop->fourcc,
			prop->frame_count,
			prop->format,
			prop->mode,
			prop->brightness,
			prop->contrast,
			prop->saturation,
			prop->hue,
			prop->gain,
			prop->convert_rgb
			);
}

/* Update and return video properties */
t_video_properties SwVideoAcquisition::updateVideoProperties() {
	memset(&m_video_properties, 0, sizeof(t_video_properties));
	fprintf(stderr, "[VidAcq]::%s:%d : updating video properties.........\n",
			__func__, __LINE__);

	// Read
	m_video_properties.pos_msec = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_POS_MSEC	);
	m_video_properties.pos_frames = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_POS_FRAMES );
	m_video_properties.pos_avi_ratio = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_POS_AVI_RATIO );
	m_video_properties.frame_width = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_FRAME_WIDTH  );
	m_video_properties.frame_height = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_FRAME_HEIGHT );
	m_video_properties.fps = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_FPS );
	m_video_properties.fourcc_dble = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_FOURCC );
	memcpy(m_video_properties.fourcc, &m_video_properties.fourcc_dble, 4);
	m_video_properties.fourcc[4] = '\0';

	m_video_properties.frame_count = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_FRAME_COUNT );
	m_video_properties.format = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_FORMAT );
	m_video_properties.mode = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_MODE );
	m_video_properties.brightness = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_BRIGHTNESS);
	m_video_properties.contrast = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_CONTRAST);
	m_video_properties.saturation = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_SATURATION);
	m_video_properties.hue = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_HUE);
	m_video_properties.gain = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_GAIN );
	m_video_properties.convert_rgb = cvGetCaptureProperty(m_capture,  CV_CAP_PROP_CONVERT_RGB );

	printVideoProperties(&m_video_properties);

	return m_video_properties;
}

/** @brief Set video properties (not updated) */
int SwVideoAcquisition::setVideoProperties(t_video_properties props) {
	// FIXME
	m_video_properties = props;

	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_POS_MSEC		, m_video_properties.pos_msec );
	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_POS_FRAMES     , m_video_properties.pos_frames );
	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_POS_AVI_RATIO  , m_video_properties.pos_avi_ratio );
	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_FRAME_WIDTH    , m_video_properties.frame_width );
	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_FRAME_HEIGHT   , m_video_properties.frame_height );
	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_FPS            , m_video_properties.fps );
	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_FOURCC         , m_video_properties.fourcc_dble );
	memcpy(m_video_properties.fourcc, &m_video_properties.fourcc_dble, 4);
	m_video_properties.fourcc[4] = '\0';

	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_FRAME_COUNT    , m_video_properties.frame_count );
	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_FORMAT         , m_video_properties.format );
	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_MODE           , m_video_properties.mode );
	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_BRIGHTNESS		, m_video_properties.brightness );
	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_CONTRAST		, m_video_properties.contrast );
	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_SATURATION		, m_video_properties.saturation );
	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_HUE			, m_video_properties.hue );
	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_GAIN          , m_video_properties.gain );
	cvSetCaptureProperty(m_capture,  CV_CAP_PROP_CONVERT_RGB   , m_video_properties.convert_rgb );


	return 1;
}

int SwVideoAcquisition::setpicture(int brightness, int hue, int colour, int contrast, int white)
{
	if(!m_capture) return -1;
	double coef = 1./65535.;

	cvSetCaptureProperty(m_capture, CV_CAP_PROP_BRIGHTNESS, coef * brightness);
	cvSetCaptureProperty(m_capture, CV_CAP_PROP_CONTRAST, coef * contrast);
	cvSetCaptureProperty(m_capture, CV_CAP_PROP_SATURATION, coef * colour);
	cvSetCaptureProperty(m_capture, CV_CAP_PROP_HUE, coef * hue);
	cvSetCaptureProperty(m_capture, CV_CAP_PROP_GAIN, coef * white);

	// check by re-reading it
	updateVideoProperties();

	return 0;
}

int SwVideoAcquisition::getpicture(video_picture * pic)
{
	if(!m_capture) return -1;
	if(!pic) return -1;


	/* retrieve or set capture properties


#define CV_CAP_PROP_POS_MSEC       0
#define CV_CAP_PROP_POS_FRAMES     1
#define CV_CAP_PROP_POS_AVI_RATIO  2
#define CV_CAP_PROP_FRAME_WIDTH    3
#define CV_CAP_PROP_FRAME_HEIGHT   4
#define CV_CAP_PROP_FPS            5
#define CV_CAP_PROP_FOURCC         6
#define CV_CAP_PROP_FRAME_COUNT    7
#define CV_CAP_PROP_FORMAT         8
#define CV_CAP_PROP_MODE           9
#define CV_CAP_PROP_BRIGHTNESS    10  ok
#define CV_CAP_PROP_CONTRAST      11 ok
#define CV_CAP_PROP_SATURATION    12 ok
#define CV_CAP_PROP_HUE           13 ok
#define CV_CAP_PROP_GAIN          14
#define CV_CAP_PROP_CONVERT_RGB   15

		CVAPI(double) cvGetCaptureProperty( CvCapture* capture, int property_id );
		CVAPI(int)    cvSetCaptureProperty( CvCapture* capture, int property_id, double value );
	*/
	double coef = 65535.;

	pic->brightness = (int)(coef * cvGetCaptureProperty(m_capture, CV_CAP_PROP_BRIGHTNESS));
	pic->contrast = (int)(coef * cvGetCaptureProperty(m_capture, CV_CAP_PROP_CONTRAST));
	pic->colour = (int)(coef * cvGetCaptureProperty(m_capture, CV_CAP_PROP_SATURATION));
	pic->hue = (int)(coef * cvGetCaptureProperty(m_capture, CV_CAP_PROP_HUE));
	pic->whiteness = (int)(coef * cvGetCaptureProperty(m_capture, CV_CAP_PROP_GAIN)); // FIXME
	fprintf(stderr, "SwVA::%s:%d : picture={br=%d, cont=%d, colour=%d, hue=%d}",
			__func__, __LINE__,
			pic->brightness, pic->contrast, pic->colour, pic->hue);
	return -1;
	//FIXMEreturn m_capture->getpicture(pic);
}

int SwVideoAcquisition::getcapability(video_capability * vc)
{
	if(!m_capture) return -1;
	double coef = 65535.;

	return 0;
//	return m_capture->getcapability(vc);
}

int SwVideoAcquisition::convert2RGB32(unsigned char * src, unsigned char * dest)
{
   //dst = RGB[CurBuffer].bits();
   //dy = Y[CurBuffer].bits();
   //du = U[CurBuffer].bits();
   //dv = V[CurBuffer].bits();
	unsigned char *dy, *du, *dv;
	int i;
	int n = imageSize.width*imageSize.height;

	// Conversion RGB 32...certainement à compléter !
	switch (m_capture_palette)
	{
		case VIDEO_PALETTE_RGB32:
			/* cool... */
			memcpy(dest, src, n * 4);
			break;

		case VIDEO_PALETTE_RGB24:
			for (i = 0; i < n; i++)
			{
				dest[0] = src[0];
				dest[1] = src[1];
				dest[2] = src[2];
				src += 3;
				dest += 4;
			}
			break;

		case VIDEO_PALETTE_YUV420P:
			// Base format with Philips webcams (at least the PCVC 740K = ToUCam PRO)
			dy = src;
			du = src + n;
			dv = du + (n / 4);

			ccvt_420p_bgr32(imageSize.width, imageSize.height, dy, du, dv, dest);
			break;

		case VIDEO_PALETTE_YUV420:
			// Non planar format... Hmm...
			ccvt_420i_bgr32(imageSize.width, imageSize.height, src, dest);
			break;

	 case VIDEO_PALETTE_YUYV:
			// Hmm. CPiA cam has this nativly. Anyway, it's just another format. The last one, as far as I'm concerned ;)
			/* we need to work this one out... */
			ccvt_yuyv_bgr32(imageSize.width, imageSize.height, src, dest);
			break;
	}
	return 0;
}

bool SwVideoAcquisition::VDIsInitialised()
{
	return m_captureIsInitialised;
}

bool SwVideoAcquisition::AcqIsInitialised()
{
	return myAcqIsInitialised;
}

tBoxSize SwVideoAcquisition::getImageSize()
{
	return imageSize;
}
