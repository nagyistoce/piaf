/***************************************************************************
 *  VirtualDevice.h - Virtual class for video acquisition : it will be inherited
 *		
 *
 *  Thu Mar  3 09:29:00 2006
 *  Copyright  2006  Christophe Seyve - SISELL
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
#ifndef _VIRTUALDEVICE_H_
#define _VIRTUALDEVICE_H_

#ifndef WIN32
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#endif
#include "nolinux_videodev.h"
#include "sw_types.h"

#define NB_DEVICES_USE_SELECT                1
#define NB_PALETTES                         16

#ifdef RGB32
typedef unsigned long RGB32;
#define PIXEL_SIZE (sizeof(RGB32))
#endif // RGB32


struct paletteDef {
	unsigned short value;
	const char * designation;
};

// in preference order for our application
const paletteDef palettes_defined[NB_PALETTES]=
{
	{VIDEO_PALETTE_YUV420P, 	"YUV 4:2:0 Planar"},
	{VIDEO_PALETTE_RGB32, 		"32 bits RGB"},
	{VIDEO_PALETTE_RGB24, 		"24 bits RGB"},
	{VIDEO_PALETTE_GREY,		"Greyscale"},
	{VIDEO_PALETTE_YUV410P, 	"YUV 4:1:0 Planar"},
	{VIDEO_PALETTE_RGB565, 		"565 16 bits RGB"},
	{VIDEO_PALETTE_RGB555, 		"555 15 bits RGB"},
	{VIDEO_PALETTE_YUV422, 		"YUV422"},
	{VIDEO_PALETTE_YUYV, 		"YUYV"},
	{VIDEO_PALETTE_UYVY, 		"UYVY"},
	{VIDEO_PALETTE_YUV420, 		"YUV420"},
	{VIDEO_PALETTE_YUV411, 		"YUV411"},
	{VIDEO_PALETTE_RAW, 		"Raw capture (BT848)"},
	{VIDEO_PALETTE_YUV422P, 	"YUV 4:2:2 Planar"},
	{VIDEO_PALETTE_YUV411P, 	"YUV 4:1:1 Planar"},
	{VIDEO_PALETTE_HI240, 		"High 240 cube (BT848)"}
};


typedef struct _normlist
{
        char name[10];
        int type;
} normlist;



const normlist normlists[] =
{
        {"ntsc"   , VIDEO_MODE_NTSC},
        {"pal"    , VIDEO_MODE_PAL},
        {"secam"  , VIDEO_MODE_SECAM},
        {"auto"   , VIDEO_MODE_AUTO},
/* following values are supported by bttv driver. */
        {"pal-nc" , 3},
        {"pal-m"  , 4},
        {"pal-n"  , 5},
        {"ntsc-jp", 6},
        {"", -1}
};




/** \brief low-level virtual acquisition class for video4linux devices
	Video4Linux device interface class. This object opens, commands, grab 
	images and close a v4l device.
	\author Christophe Seyve - SISELL \mail christophe.seyve@sisell.com
	\version 0.1.0 - \Date
	\see SwV4LDevice SwV4L2Device...
	\see SwVideoAcquisition
 */
class VirtualDevice 
{
public:
	/// Constructor. Does not open any device.
	VirtualDevice();
	
	/// Destructor
	virtual ~VirtualDevice();
	
	/** @brief return device node */
	virtual char * getDeviceName() = 0;

	/** Opens device with size
		@param device device name/node (ex.: "/dev/video0")
		@param newSize acquisition size
	*/
	virtual int VDopen(char * device, tBoxSize *newSize) = 0;
	
	/// Close device
	virtual int VDclose() = 0;
	virtual int setQuality(int q) = 0;
	/** Set image scale for accelerating copy on some target with slow RAM */
	virtual void setCopyScale(int scale) = 0;

	// global init : read all informations on device
	virtual int VDInitAll() = 0;
    virtual bool VDIsInitialised() = 0;
	
	// Query functions
	// global query
	/// Reads video capabilities 
	virtual int getcapability		(video_capability * dest) = 0;
	virtual int getpicture			(video_picture * dest) = 0;
	virtual int getchannel			(video_channel * dest, int noChannel) = 0;
	virtual int getcurrentchannel() = 0;
	
	virtual int getcapturewindow (video_window * dest) = 0;
	//NOT in SwV4D : virtual int getsubcapture	(video_capture * dest);
	virtual char * getnorm() = 0;
	virtual int getErrorStatus() = 0;
	/// Gets number of available channels
	virtual int maxchannel() = 0;
	/// Gets palette number (V4L specs)
	virtual int getpalette() = 0;
	virtual int getcapturewindowsize (tBoxSize * dest) = 0;
	
	virtual unsigned char * video_getaddress() = 0;
	
	//?bool VDHasMmap();
	//?bool VDisPWC() { return m_isPWC;};
	/// Returns frame buffer adress
	virtual unsigned char * getFrameBuffer() = 0;
	virtual int getCompressedBuffer(unsigned char ** rawbuffer, 
		unsigned char ** compbuffer, long * compsize) = 0;

	// Setting Functions
	// global settings
	virtual int setframebuffer	(void *base, int width, int height, int depth, int bpl) = 0;
	virtual int setpicture			(int br, int hue, int col, int cont, int white) = 0;
	//NOT in SwV4D : virtual int setsubcapture	( int x, int y, int width, int height, int decimation, int flags);
	virtual int changeSize(tBoxSize * newSize) = 0;

	// precise settings
	//NOT in SwV4D : virtual int setdefaultnorm	(int defaultNorme);
	virtual int setchannel			(int ch) = 0;
	virtual int setnorm				(char *n) = 0; 
	virtual int setfrequence		(int freq) = 0;
	virtual int setpalette			(int pal) = 0;
	//NOT in SwV4D : virtual int setcapturewindowsize (tBoxSize dest);
};
#endif
