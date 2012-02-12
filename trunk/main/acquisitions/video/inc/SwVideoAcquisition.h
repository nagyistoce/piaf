/***************************************************************************
	 SwVideoAcquisition.h  - High level acquisition class for video capture boards
							 -------------------
	begin                : Wed Mar 6 2002
	copyright            : (C) 2002 by Olivier Viné (OLV) & Christophe Seyve (CSE)
	email                : olivier.vine@sisell.com & cseyve@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _SW_VIDEOACQUISITION_
#define _SW_VIDEOACQUISITION_

#include "virtualdeviceacquisition.h"

//use highgui instead of V4L2 : #include "V4L2Device.h"

#include "ccvt.h"
#include "sw_types.h"

#include "nolinux_videodev.h"
#include "virtualdeviceacquisition.h"

// TODO : port this class with virtual device inheritance


/** Performs device management and image acquisitions with simple API.
	\brief High-level video acquisition class.
 \author Christophe Seyve - cseyve@free.fr
 \version 0.1.0 \Date
 */

class SwVideoAcquisition {
public:
	/// Constructor
	SwVideoAcquisition();

	/** Constructor with device entry
		@param device video device (ex.: "/dev/video0")
		*/
	SwVideoAcquisition(int idx_device);

	/** Constructor with device entry and acquisition size
		@param device video device (ex.: "/dev/video0")
		\param newsize acquisition size
		*/
	SwVideoAcquisition(int idx_device, tBoxSize newsize);

	/** Constructor with V4L2Device pointer entry
	\param aVD V4L2Device class entry. Rare usage.
	*/
//	SwVideoAcquisition(V4L2Device *aVD);
	SwVideoAcquisition(CvCapture *aVD);

	/// Destructor
	~SwVideoAcquisition();

	unsigned char *video_getaddress();

	/// Initializes video acquisition.
	int VAInitialise(bool startCapture);

	/// Set channel number (for devices which support multiple channels)
	int setChannel(int ch);
	int getcurrentchannel();
	char * getnorm();

	/// Prints to stdout acquisition properties
	int VAEcho();
	/// Close video device (only on demand)
	int VAcloseVD();

	/** @brief Get video properties (not updated) */
	t_video_properties getVideoProperties();
	/** @brief Update and return video properties */
	t_video_properties updateVideoProperties();
	/** @brief Set video properties (not updated) */
	int setVideoProperties(t_video_properties props);

	/** Returns image buffer
		@return pointer to image buffer
		*/
	unsigned char * readImageBuffer(long * buffersize); // read acquisition buffer adress

	/** Reads raw image (with coding from palette specifications, YUV420P for example)
		\param image pointer to allocated buffer
		\param contAcq perform or not continuous acquisition (e.g. launch another acquisition)
		*/
	int readImageRaw(unsigned char * rawimage, unsigned char * compimage, long * compsize, bool contAcq);
	// Returns address of last acquisition buffer and launch no new acquisition
	unsigned char * getImageRawNoAcq();

	/** Grabs one image and convert to RGB32 coding format
		\param image pointer to allocated buffer
		\param contAcq perform or not continuous acquisition (e.g. launch another acquisition)
		\return 0 if error
		*/
	int readImageRGB32(unsigned char* image, long * buffersize, bool contAcq);

	/** Gets palette id.
		@see linux/videodev.h
		*/
	int getPalette();
	/// Checks if video device is initialised
	bool VDIsInitialised();

	/// Checks if video acquisition is initialised
	bool AcqIsInitialised();

	/// Gets definitive image size. You should do that to verify that your selected image size is supported by device.
	tBoxSize getImageSize();
	/// change acquisition size
	int changeAcqParams(tBoxSize newSize, int channel);

	/// Set picture brightness, contrast, saturation, color,
	int setpicture(int br, int hue, int col, int cont, int white);
	int getpicture(video_picture * pic);
	/// Get video capability (min and max size)
	int getcapability(video_capability * vc);

	/// Set channel number (for devices which support multiple channels)
	int setNorm(char * norm);

	int openDevice(int idx_device, tBoxSize newSize);
private:
	int convert2RGB32(unsigned char * src, unsigned char * dest);
	int convert2RGB24(unsigned char * src, unsigned char * dest);
	int convert2YUV	(unsigned char * src, unsigned char * dest);
	int convert2GREY	(unsigned char * src, unsigned char * dest);
	int convert2YUV420P(unsigned char * src, unsigned char * dest); //pour l'encodage mpeg avec ffmpeg !

private:
	void calcBufferSize();

	/// low-level V4L2Device objet for driving device
//	V4L2Device		*m_capture;
	/// Use OpenCV because it's better for other kinds of devices (IEEE1394, ...)
	CvCapture * m_capture;
	/// Last captured frame
	IplImage * m_iplImage;

	/// Video properties from OpenCV capture API
	t_video_properties m_video_properties ;

	bool			m_captureIsInitialised;
	bool			myAcqIsInitialised;
	video_mbuf 		m_capture_mbuf;
	int				m_capture_fd;
	int				m_capture_palette;
	unsigned char	*receptBuffer;
	unsigned long 	receptBufferSize;
	bool			withFramerate;
	bool			useMmap;
	bool			contAcquisition;
	tBoxSize		imageSize;
	unsigned long 	noAcquisition;
};

#endif
