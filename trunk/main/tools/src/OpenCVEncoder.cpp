/***************************************************************************
	 OpenCVEncoder.cpp  -  OpenCV/Highgui based MJPEG encoder for Piaf
							 -------------------
	begin                : Thu Sep 9 2010
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

#include <stdint.h>

#include "swvideodetector.h"
#include "OpenCVEncoder.h"

OpenCVEncoder::OpenCVEncoder(int imageWidth, int imageHeight, int frame_rate) {
	writer = NULL;
	imgHeader = imgRgb24 = NULL;
	m_width = imageWidth;
	m_height = imageHeight;
	m_fps = frame_rate;
}

OpenCVEncoder::~OpenCVEncoder() {
	stopEncoder();
}

int OpenCVEncoder::startEncoder(char *output_file) {
#ifdef OPENCV_22
	try {
#endif
		writer = cvCreateVideoWriter(output_file,
								 CV_FOURCC('M','J','P','G'),
								 m_fps,
								 cvSize(m_width, m_height), 3);
#ifdef OPENCV_22
	} catch(cv::Exception e)
	{
		writer = NULL;
	}
#endif
	if(!writer) {
		fprintf(stderr, "OpenCVEnc::%s:%d ERROR: cannot create encoder "
				"with size %dx%d @ %d fps for MJPEG\n",
				__func__, __LINE__,
				m_width, m_height, m_fps);
		return 0;
	}
	imgRgb24 = cvCreateImage(cvSize(m_width, m_height), IPL_DEPTH_8U, 3);

	fprintf(stderr, "OpenCVEnc::%s:%d created encoder with size %dx%d @ %d fps\n",
			__func__, __LINE__,
			m_width, m_height, m_fps);

	return 1;
}

int OpenCVEncoder::setQuality(int qscale)
{
	if(qscale < 1 || qscale > 31)
		return 0;
	fprintf(stderr, "OpenCVEncoder::%s:%d set quality scale = %d\n",
			__func__, __LINE__,
			qscale);

	m_quality = qscale;
	return 1;
}
void OpenCVEncoder::stopEncoder() {
	if(writer) {
		cvReleaseVideoWriter( &writer);
		writer = NULL;
	}

	swReleaseImageHeader( &imgHeader);
	swReleaseImage( &imgRgb24);
}

int convertFromRGB32(uint8_t * src, IplImage * destImage)
{
   //dst = RGB[CurBuffer].bits();
   //dy = Y[CurBuffer].bits();
   //du = U[CurBuffer].bits();
   //dv = V[CurBuffer].bits();
	unsigned char *dy, *du, *dv;
	int i;
	int n = destImage->width*destImage->height;
	uint8_t * dest = (uint8_t *)destImage->imageData;

	// Conversion RGB 32...certainement à compléter !
	switch (destImage->nChannels)
	{
	case 4: //VIDEO_PALETTE_RGB32:
		/* cool... */
		memcpy(dest, src, n * 4);
		break;
	default:
	case 3://VIDEO_PALETTE_RGB24:
		for (i = 0; i < n; i++)
		{
			dest[0] = src[0];
			dest[1] = src[1];
			dest[2] = src[2];
			src += 4;
			dest += 3;
		}
		break;

		case 2://VIDEO_PALETTE_YUV420P:
			// Base format with Philips webcams (at least the PCVC 740K = ToUCam PRO)
			dy = src;
			du = src + n;
			dv = du + (n / 4);

//			ccvt_420p_bgr32(imageSize.width, imageSize.height, dy, du, dv, dest);
			break;

//		case VIDEO_PALETTE_YUV420:
			// Non planar format... Hmm...
//			ccvt_420i_bgr32(imageSize.width, imageSize.height, src, dest);
			break;

//	 case VIDEO_PALETTE_YUYV:
			// Hmm. CPiA cam has this nativly. Anyway, it's just another format. The last one, as far as I'm concerned ;)
			/* we need to work this one out... */
//			ccvt_yuyv_bgr32(imageSize.width, imageSize.height, src, dest);
			break;
	}
	return 0;
}
int OpenCVEncoder::encodeFrameY(unsigned char * Yframe) {
	if(!imgHeader) {
		imgHeader = cvCreateImageHeader(cvSize(m_width, m_height), IPL_DEPTH_8U, 1);
	}
	imgHeader->imageData = (char *)Yframe;
#ifdef OPENCV_22
	try
	{
//		fprintf(stderr, "OpenCVEnc::%s:%d convert to RGB24 "
//			 "with size %dx%d @ %d fps\n",
//			 __func__, __LINE__,
//			 m_width, m_height, m_fps);
		cvCvtPlaneToPix(imgHeader, imgHeader, imgHeader, NULL, imgRgb24);
//		fprintf(stderr, "OpenCVEnc::%s:%d encode RGB24 "
//				"with size %dx%d @ %d fps\n",
//				__func__, __LINE__,
//				m_width, m_height, m_fps);
		cvWriteFrame(writer, imgRgb24);
	}
	catch(cv::Exception e)
	{
		fprintf(stderr, "OpenCVEnc::%s:%d caught exception while encoding "
				"with size %dx%d @ %d fps\n",
				__func__, __LINE__,
				m_width, m_height, m_fps);
		return 0;
	}
#else
	cvCvtPlaneToPix(imgHeader, imgHeader, imgHeader, NULL, imgRgb24);
//		fprintf(stderr, "OpenCVEnc::%s:%d encode RGB24 "
//				"with size %dx%d @ %d fps\n",
//				__func__, __LINE__,
//				m_width, m_height, m_fps);
	cvWriteFrame(writer, imgRgb24);
#endif
	return 1;
}

// Convert RGB32 frame to YUV420P then encode it
int OpenCVEncoder::encodeFrameRGB32(unsigned char *RGB32frame) {
	if(!writer) return 0;

	if(!imgHeader) {
		imgHeader = cvCreateImageHeader(cvSize(m_width, m_height), IPL_DEPTH_8U, 1);
	}

	// FIXME : convert to RGB24 and it's fine !!'
	imgHeader->imageData = (char *)RGB32frame;
/*
	fprintf(stderr, "OpenCVEnc::%s:%d create encoder with size %dx%d @ %d fps\n",
			__func__, __LINE__,
			m_width, m_height, m_fps);

	*/
#ifdef OPENCV_22
	try {
		convertFromRGB32(RGB32frame, imgRgb24);

		cvWriteFrame(writer, imgRgb24);
	}
	catch(cv::Exception e)
	{
		fprintf(stderr, "OpenCVEnc::%s:%d caught exception while encoding "
				"with size %dx%d @ %d fps\n",
				__func__, __LINE__,
				m_width, m_height, m_fps);
		return 0;
	}
#else
	convertFromRGB32(RGB32frame, imgRgb24);

	cvWriteFrame(writer, imgRgb24);
#endif
	return 1;
}


// Encode directly RGB24 frame
int OpenCVEncoder::encodeFrameRGB24(unsigned char *RGB24frame) {
	if(!writer) return 0;

	if(!imgHeader) {
		imgHeader = cvCreateImageHeader(cvSize(m_width, m_height), IPL_DEPTH_8U, 3);
	}

	// FIXME : convert to RGB24 and it's fine !!'
	imgHeader->imageData = (char *)RGB24frame;
#ifdef OPENCV_22
	try {
		cvCopy(imgHeader, imgRgb24);

		cvWriteFrame(writer, imgRgb24);
	}
	catch(cv::Exception e)
	{
		fprintf(stderr, "OpenCVEnc::%s:%d caught exception while encoding "
				"with size %dx%d @ %d fps\n",
				__func__, __LINE__,
				m_width, m_height, m_fps);
		return 0;
	}
#else
	cvCopy(imgHeader, imgRgb24);

	cvWriteFrame(writer, imgRgb24);
#endif
	return 1;
}

