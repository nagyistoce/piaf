/***************************************************************************
	 OpenCVEncoder.h  -  OpenCV/Highgui based video encoder for Piaf
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

#ifndef OPENCVENCODER_H
#define OPENCVENCODER_H

#include "swvideodetector.h"


/** @brief New encoder class using OpenCV HighGUI */
class OpenCVEncoder {
public:
	OpenCVEncoder(int imageWidth, int imageHeight, int frame_rate);

	/** @brief Start encoding in file */
	int startEncoder(char * output_file);
	/** @brief Set qualitu in 1..31 */
	int setQuality(int qscale);

	/** @brief Destructor */
	~OpenCVEncoder();

	/** @brief Stop encoding and close file */
	void stopEncoder();


	/** @brief Encode an IplImage */
	int encodeImage(IplImage * image);

	/** @brief Encode a new frame (YUV420P format) */
	int encodeFrameYUV420P(unsigned char *YUV420frame);

	/** @brief Convert RGB24 frame to YUV420P then encode it */
	int encodeFrameRGB24(unsigned char *RGB24frame);

	/** @brief Convert grayscaled frame to RGB32 then encode it */
	int encodeFrameY(unsigned char * Yframe);
	/** @brief Convert RGB32 frame to YUV420P then encode it */
	int encodeFrameRGB32(unsigned char *RGB32frame);

private:
	/// Video movie encoder using OpenCV/Highgui
	CvVideoWriter* writer;
	IplImage * imgHeader, *imgRgb24;

	int m_quality;// MPEG Quality
	int m_width, m_height, m_fps;

};
#endif // OPENCVENCODER_H
