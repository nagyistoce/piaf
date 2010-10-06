/***************************************************************************
     SwV4LDevice.h  - low level acquisition class for video4linux capture boards
	 	and webcams
                             -------------------
    begin                : Wed Mar 6 2002
    copyright            : (C) 2002 by Olivier Viné (OLV) & Christophe Seyve (CSE)
    email                : olivier.vine@sisell.com & christophe.seyve@sisell.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _SW_V4LDEVICE_H_
#define _SW_V4LDEVICE_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/videodev.h>
#include <pthread.h>
#include <string.h>

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
//#define DEFAULT_DEPTH 32
#define DEFAULT_PALETTE VIDEO_PALETTE_RGB32
#define DEFAULT_VIDEO_NORM VIDEO_MODE_PAL
// End from

#include "sw_types.h"


/// Video4Linux Device Structure
struct _v4ldevice
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
	unsigned char *map;
	pthread_mutex_t mutex;
	int frame;
	int framestat[2];
	int overlay;
};

typedef struct _v4ldevice v4ldevice;

/// palette description and id
struct paletteDef {
	unsigned short value;
	char * designation;
};


/** 
 * Video4Linux device interface class. This object opens, commands, grab 
	images and close a v4l device.
	\brief Low-level video4linux acquisition class
	\author Christophe Seyve - SISELL \mail christophe.seyve@sisell.com
	\version 0.1.0 - \Date
 */
class SwV4LDevice 
{

public:
	
	/// Constructor. Does not open any device.
	SwV4LDevice();
	
	/// Destructor
	~SwV4LDevice();

	/** Opens device with size
		\param device device name (ex.: "/dev/video0")
		\param newSize acquisition size
	*/
	int VDopen(char * device, tBoxSize *newSize);
	/// device name
	char * videodevice;
	
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
	/// Reads frame buffer
	int getframebuffer	(video_buffer * dest);
	/// Reads picture settings (brightness, contrast, ...)
	int getpicture			(video_picture * dest);
	/// Reads memory buffer
	int getmbuf				(video_mbuf * dest);
	/// Reads channel name
	int getchannel			(video_channel * dest, int noChannel);
	/// Reads capture window
	int getcapturewindow (video_window * dest);
	/// Reads sub-capture settings
	int getsubcapture	(video_capture * dest);
	/// Reads current channel
	int getcurrentchannel();
	/// Reads norm description
	char * getnorm();

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

	/// Returns frame buffer adress
	unsigned char * getFrameBuffer();

	// Setting Functions
	// global settings
	int setframebuffer	(void *base, int width, int height, int depth, int bpl);
	int setpicture			(int br, int hue, int col, int cont, int white);
	int setsubcapture	( int x, int y, int width, int height, int decimation, int flags);
	int changeSize(tBoxSize * newSize);

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
	v4ldevice vd;
    /* which contains : DO NOT UNCOMMENT
	int fd;
	struct video_capability capability;
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
	 \param vd device handling struct
	 \param channel channel number
	 \param norm input norm PAL/SECAM/NTSC/OTHER (source: videodev.h)
	 */
	int v4l_setchannelnorm(int ch, int norm);

	int v4l_mmap();
	int v4l_munmap();
	int v4l_setdefaultnorm(int norm);
	int v4l_getcapability();
	/** v4l_setpicture - set the picture properties 
	 @param vd device handling struct
	 \param br picture brightness
	 \param hue picture hue
	 \param col picture color
	 \param cont picture contrast
	 \param white picture whiteness
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
	 \param vd device handling struct
	 \param base base PHYSICAL address of the frame buffer
	 \param width width of the frame buffer
	 \param height height of the frame buffer
	 \param depth color depth of the frame buffer
	 \param bpl number of bytes of memory between the start of two adjacent lines
	 */
	int v4l_setframebuffer(void *base, 
		int width, int height, int depth, int bytesperline);

private:
	void init();
	
	
	// ref effetv main.c	
	bool hasFramerate;
	bool useSelectAvailable;
	bool initialised;
	int fps;
	int palette;
	int norm;
	int hastuner;
	
#ifdef USE_VLOOPBACK
    char *vloopbackfile;
#endif
    // end ref main.c
    
	// ref effectv video.c
	int frequency_table;
	int TVchannel;
	int channel; // = input for example 0 for tuner, 1 for SVHS...
	RGB32 *framebuffer;
	int video_width;
	int video_height;
    int video_area;
#define MAXWIDTH (vd.capability.maxwidth)
#define MAXHEIGHT (vd.capability.maxheight)
#define MINWIDTH (vd.capability.minwidth)
#define MINHEIGHT (vd.capability.minheight)

	// picture params
	int picture_brightness;
	int picture_hue;
	int picture_colour;
	int picture_contrast;


	// end ref video.c
};

#endif
