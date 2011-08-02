/***************************************************************************
 *  V4L2Device.h - Low level acquisition class for Video4Linux2 API
 *
 *  Thu Mar 6 10:06:23 2002
 *  Copyright  2002  Christophe Seyve - SISELL
 *  christophe.seyve@sisell.com
 ****************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _SW_V4L2DEVICE_H_
#define _SW_V4L2DEVICE_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include "nolinux_videodev.h"
#include <pthread.h>
#include <string.h>


#include "sw_types.h"

//#include "VirtualDevice.h"
#include "virtualdeviceacquisition.h"

#define DEFAULT_DEVICE 						"/dev/video0"
#define MAX_CHANNELS							10
#define NB_DEVICES_USE_SELECT		1
#define NB_PALETTES								16

// From EffecTV.h 
typedef unsigned int RGB32;
#define PIXEL_SIZE (sizeof(RGB32))
#define DEFAULT_VIDEO_WIDTH 640
#define DEFAULT_VIDEO_HEIGHT 480
#define DEFAULT_VIDEO_DEVICE "/dev/video"
#define V4L_DEFAULT_DEPTH 32
#define DEFAULT_PALETTE VIDEO_PALETTE_RGB32
#define DEFAULT_VIDEO_NORM VIDEO_MODE_PAL
// End from


typedef struct _t_v4l2_buffer {
	void *	start;
	size_t	length;
} t_v4l2_buffer;

/** Video4Linux Device Structure
 */
struct _v4l2device
{
	int fd;
	struct video_capability capability;
	struct video_channel channel[10];
	struct video_picture picture;
	struct video_clip clip;
	struct video_window window;
	struct video_capture capture;
	struct video_buffer buffer;
	struct video_mmap mmap;
	struct video_mbuf mbuf;
	struct video_unit unit;
	/// Pixel format (defined from FourCC code)
	int v4l2_pix_fmt;
	int width, height;
	unsigned char *map;
	pthread_mutex_t mutex;
	int frame;
	int frame_period_ms;
	int frame_rate;
	
	int framestat[2];
	
	struct timeval last_acq_tv;
	float measured_period;
	
	int overlay;
};

typedef struct _v4l2device v4l2device;

/** \brief low-level acquisition class for video4linux devices
	Video4Linux device interface class. This object opens, commands, grab 
	images and close a v4l device.
	\author Christophe Seyve - SISELL \mail christophe.seyve@sisell.com
	\version 0.1.0 - \Date
	\see SwVideoAcquisition
 */
class V4L2Device : public VirtualDeviceAcquisition
{
public:
	
	/// Constructor. Does not open any device.
	V4L2Device(int idx_device = 0);
	
	/// Destructor
	~V4L2Device();
	/** \brief Use sequential mode
		If true, grabbing is done when a new image is requested.
		Else a thread is started to grab */
	void setSequentialMode(bool on);

	/** \brief Function called by the doc (parent) thread */
	int grab();

	/** \brief Return true if acquisition device is ready */
	bool isDeviceReady() { return initialised; }

	/** \brief Return true if acquisition is running */
	bool isAcquisitionRunning() { return initialised; }

	/** \brief Start acquisition */
	int startAcquisition();

	/** \brief Return image size */
	CvSize getImageSize();

	/** \brief Stop acquisition */
	int stopAcquisition();

	/** \brief Grabs one image and convert to RGB32 coding format
		if sequential mode, return last acquired image, else read and return image

		\return NULL if error
		*/
	IplImage * readImageRGB32() ;

	/** \brief Grabs one image and convert to grayscale coding format
		if sequential mode, return last acquired image, else read and return image

		\return NULL if error
		*/
	IplImage * readImageY();

	/** \brief Grabs one image of depth buffer
		if sequential mode, return last acquired image, else read and return image

		\return NULL until V4L2 integrates Kinect driver
		*/
	IplImage * readImageDepth() { return NULL; }


	/** @brief Get video properties (not updated) */
	t_video_properties getVideoProperties();

	/** @brief Update and return video properties */
	t_video_properties updateVideoProperties();

	/** @brief Set video properties (not updated) */
	int setVideoProperties(t_video_properties props);


private:

	int m_idx_device; ///< index of device
	CvSize m_imageSize; ///< Acquisition size
	IplImage * m_iplImageRGB32; ///< Acquired image in BGRA
	IplImage * m_iplImageY;		///< Acquired image in grayscaled
	int m_nChannels;	///< number of channels
	/// Video properties from OpenCV capture API
	t_video_properties m_video_properties ;

	/** @brief Return device node */
	char * getDeviceName();
	
	/** @brief Opens device with size
		@param device device name (ex.: "/dev/video0")
		@param newSize acquisition size
	*/
	int VDopen(char * device, tBoxSize *newSize);
	
	/** @brief Set image quality on hardware compression : UNUSED for the moment */
	int setQuality(int /*q*/) { return 0; }

	/** @brief Set image scale for accelerating copy on some target with slow RAM : UNUSED */
	void setCopyScale(int scale) { m_copyscale=scale; }

	
	/// Close device
	int VDclose();
	
	// global init : read all informations on device
	int VDInitAll();
    
	/// Print to stout global informations on video device
	void	echo();

	// Query functions
	// global query
	/// Reads video capabilities 
	int getcapability		(video_capability * dest);
	int getframebuffer	(video_buffer * dest);
	int getpicture			(video_picture * dest);
	int getmbuf				(video_mbuf * dest);
	int getchannel			(video_channel * dest, int noChannel);
	int getcapturewindow (video_window * dest);
	int getsubcapture	(video_capture * dest);
	int getcurrentchannel();
	char * getnorm();
	int getErrorStatus() { return 0; };
	// precise query
	int getFD();
	/// Gets number of available channels
	int maxchannel();
	/// Gets palette number (V4L specs)
	int getpalette();
	int getcapturewindowsize (tBoxSize * dest);
	pthread_mutex_t * getMutex();
	bool VDHasFramerate();
	bool VDSelectAvailable();
	bool VDIsInitialised();
	bool VDHasMmap();
	bool VDisPWC() { return m_isPWC;};
	/// Grab one image and returns frame buffer address
	unsigned char * getFrameBuffer();
	
	/** @brief Return compressed buffer */
	int getCompressedBuffer(unsigned char ** rawbuffer, 
		unsigned char ** compbuffer, long * compsize);
	
	// Setting Functions
	// global settings
	int setframebuffer	(void *base, int width, int height, int depth, int bpl);
	int setpicture			(int br, int hue, int col, int cont, int white);
	int setsubcapture	( int x, int y, int width, int height, int decimation, int flags);
	int changeSize(tBoxSize * newSize);

	/** \brief change camera control on V4L2 device using ioctl

	*/
	int setCameraControl(unsigned int controlId, int value);

	/** \brief get camera control on V4L2 device using ioctl

	*/
	int getCameraControl(unsigned int controlId);

	// precise settings
	int setdefaultnorm	(int defaultNorme);
	int setchannel			(int ch);
	int setnorm				(char *n); 
	int setfrequence		(int freq);
	int setpalette			(int pal);
	int setcapturewindowsize (tBoxSize dest);

	// From effectv video.h
	int video_setformat(int palette);
	int video_grab_check(int palette);
	int video_set_grabformat(int palette);
	int video_grabstart();
	int video_grabstop();
	int video_changesize(int width, int height);
	int video_setfreq(int v);
	int video_syncframe();
	int video_grabframe();
	unsigned char *video_getaddress();
	void video_change_brightness(int);
	void video_change_hue(int);
	void video_change_color(int);
	void video_change_contrast(int);
	void video_change_whiteness(int);

#define video_getformat() (vd.mmap.format)

	int videox_getnorm(const char *name);
	int videox_getfreq(const char *name);
    // end from effectv video.h
	
	int sync();
	// ----------------- V4L API ---------------------
	v4l2device vd;
    /* which contains : DO NOT UNCOMMENT
	int fd;
	struct v4l2_capability capability;
	struct video_channel channel[MAX_CHANNELS];
	struct video_picture picture;
	struct video_clip clip;
	struct video_window window;
	struct video_capture capture;
	struct video_buffer buffer;
	struct video_mbuf mbuf;
	struct video_unit unit;
	pthread_mutex_t mutex;
	unsigned char * map;
	int frame;
	int framestat[2];
	int overlay;  */
    // end ref.
	int StartGrab();
	int StopGrab();
	int GrabCheck(int pal);
	int setGrabFormat(int pal);
	
	int v4l_open(char *name);
	int v4l_close();

	int v4l_grabstart(int frame);
	int v4l_grabinit(int width, int height);
	
	int v4l_getpicture();
	int v4l_setpalette(int pal);
	/** v4l_setchannelnorm - set the norm of channel
	 @param vd device handling struct
	 @param channel channel number
	 @param norm input norm PAL/SECAM/NTSC/OTHER (source: videodev.h)
	 */
	int v4l_setchannelnorm(int ch, int norm);

	int v4l_mmap();
	int v4l_munmap();
	int v4l_setdefaultnorm(int norm);
	int v4l_getcapability();
	/** v4l_setpicture - set the picture properties 
	 @param vd device handling struct
	 @param br picture brightness
	 @param hue picture hue
	 @param col picture color
	 @param cont picture contrast
	 @param white picture whiteness
	 */
	int v4l_setpicture(int br, int hue, int col, int cont, int white);
	unsigned char * v4l_getaddress();
	int v4l_grabf();
	int v4l_syncf();
	int v4l_sync(int frame);
	/// v4l_getmbuf - get the size of the buffer to mmap
	int v4l_getmbuf();
	/// v4l_getframebuffer - get frame buffer address
	int v4l_getframebuffer();
	/** v4l_setframebuffer - set parameters of frame buffer
	 @param vd device handling struct
	 @param base base PHYSICAL address of the frame buffer
	 @param width width of the frame buffer
	 @param height height of the frame buffer
	 @param depth color depth of the frame buffer
	 @param bpl number of bytes of memory between the start of two adjacent lines
	 */
	int v4l_setframebuffer(void *base, 
		int width, int height, int depth, int bytesperline);

private:
	void init();
	char * videodevice;
	bool m_isPWC;
	// ref effetv main.c	
	bool hasFramerate;
	bool useSelectAvailable;
	bool initialised;
	int fps;
	int m_capture_palette; ///< Palette as defined in V4L : VIDEO_PALETTE_RGB32...
	int norm;
	int hastuner;
	int m_copyscale;
#ifdef USE_VLOOPBACK
    char *vloopbackfile;
#endif
    // end ref main.c
    
	// ref effectv video.c
	int frequency_table;
	int TVchannel;
	int channel; // = input for example 0 for tuner, 1 for SVHS...
	u8 *rawframebuffer;///< Address of lastly acquired buffer

	int convert2RGB32(unsigned char * src, unsigned char * dest);
	int convert2Y(unsigned char * src, unsigned char * dest);


	int video_width;
	int video_height;
    int video_area;


	int max_width, max_height;
	int min_width, min_height;
		
		
	// picture params
	int picture_brightness;
	int picture_hue;
	int picture_colour;
	int picture_contrast;


	// end ref video.c
	
		
	// V4L2 SPECIFIC
	int init_device(int w, int h);
	int init_mmap();
	t_v4l2_buffer * buffers;
	unsigned int n_buffers;
	unsigned char * read_frame(void);
	int start_capturing();
	int stop_capturing();
};
#endif
