/***************************************************************************
     SwV4LDevice.cpp  - low level acquisition class for video4linux capture boards
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

#include "SwV4LDevice.h"

// additionnal libraries for memory/device control
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/videodev.h>
#include <pthread.h>
#include <errno.h>



#define DEBUG_V4L	0

#ifndef devices_use_select
char * devices_use_select[NB_DEVICES_USE_SELECT] = {"Philips"};

// in preference order for our application
paletteDef palettes_defined[NB_PALETTES]=
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
#endif

static normlist normlists[] =
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


SwV4LDevice::SwV4LDevice(){
	init();
}

SwV4LDevice::~SwV4LDevice(){
	if(videodevice)
		delete [] videodevice;
	if(initialised)
		VDclose();
		
	if(framebuffer)
		delete [] framebuffer;
//	pthread_mutex_destroy(&mutex);
}


void SwV4LDevice::init() 
{
	initialised = false;
	// fd = -1 until is done correctly...
	vd.fd = -1;
	vd.frame = 0;
	
	videodevice = NULL;
	// params defaults
	norm = VIDEO_MODE_PAL;
	channel = 0;
    palette = -1;
	frequency_table = 0;
	TVchannel = 0;
	framebuffer = NULL;

#ifdef USE_VLOOPBACK
	vloopbackfile = NULL;
#endif

	// init du mutex
//	pthread_mutex_init(&mutex, NULL);
}

int SwV4LDevice::changeSize(tBoxSize * newSize) 
{
	int w=0, h=0;
	if(newSize->width == 0 || newSize->height == 0) {
        w = DEFAULT_VIDEO_WIDTH;
        h = DEFAULT_VIDEO_HEIGHT;
    }
    else {
    	w = newSize->width;
     	h = newSize->height;
    }

	if(w > MAXWIDTH || h > MAXHEIGHT) {
        w = MAXWIDTH;
        h = MAXHEIGHT;
        fprintf(stderr, "capturing size is set to %dx%d.\n", w, h);
	} else if(w < MINWIDTH || h < MINHEIGHT) {
        w = MINWIDTH;
        h = MINHEIGHT;
        fprintf(stderr, "capturing size is set to %dx%d.\n", w, h);
	}
	
	video_width = w;
	video_height = h;
	video_area = video_width * video_height;
    // tell caller that the size changed
	newSize->width  = w;
	newSize->height = h;

	unsigned char first = 1;
	if(framebuffer) {
		first = 0;
		if (StopGrab() < 0) {
			 fprintf(stderr, "[V4L]::StopGrab failed\n");
			 return -1;
		}
		if(v4l_munmap()<0) {
			fprintf(stderr, "[V4L]::v4l_munmap failed\n");
			return -1;
		}
		vd.frame = 0;
		delete [] framebuffer;
	}
	
	framebuffer = (RGB32 *)malloc(video_area*sizeof(RGB32));
    if(framebuffer == NULL) {
    	fprintf(stderr, "[V4L]: Memory allocation error.\n");
        return -1;
	}
	
	if(first) {
		if(v4l_grabinit(video_width, video_height)) {
			fprintf(stderr, "[V4L]::v4l_grabinit failed\n");
			return -1;
		}
	}
	else {
		// set size
		if(v4l_grabinit(video_width, video_height)) {
			fprintf(stderr, "[V4L]::v4l_grabinit failed\n");
			return -1;
		}
		
		if(v4l_mmap()<0) {
			fprintf(stderr, "[V4L]::v4l_mmap() failed\n");
			return -1;
		}
		if(StartGrab()<0) {
			fprintf(stderr, "[V4L]::StartGrab() failed\n");
			return -1;
		}
		
	}
	return 0;
}

int SwV4LDevice::setnorm(char *n) 
{
	for(int i=0; normlists[i].name[0] != '\0'; i++)
	{
		if(!strcmp(n, normlists[i].name)) {
			norm = normlists[i].type;
		    v4l_setdefaultnorm(norm); // moved after getcapabilities to get the channel number
			// then ioctl to set the channel
			setchannel(channel);
			return 1;
		}
	}
	return 0;
}

// ref effetv video.c function video_init()
int SwV4LDevice::VDopen(char * device, tBoxSize *newSize)
{
	int maxc;
	fprintf(stderr, "[V4L]::VDopen device '%s'\n", device);
	
	if(device == NULL){
    	videodevice = new char [ strlen(DEFAULT_VIDEO_DEVICE) +1];
		strcpy(videodevice, DEFAULT_VIDEO_DEVICE);
	}
	else {
		videodevice = new char [ strlen(device) +1];
		strcpy(videodevice, device);
	}

    if(v4l_open(videodevice)) {
		
		return -1;
	}
    v4l_setdefaultnorm(norm); // moved after getcapabilities to get the channel number
    v4l_getcapability();
//    v4lsetdefaultnorm(norm);
	// Read channel ...
	
	
    
    if(!(vd.capability.type & VID_TYPE_CAPTURE)) {
    	fprintf(stderr, "[V4L]::VDOpen: This device seems not to support video capturing.\n");
    	return -1;
	}
	hastuner = 0;
	if((vd.capability.type & VID_TYPE_TUNER)) {
		hastuner = 1;
		frequency_table = 0; // CSE : didn't undestand ...  = freq;
		TVchannel = 0;
	}

	// set acquisition size
	changeSize(newSize);

    getcurrentchannel();
	
	maxc = vd.capability.channels;
	if((maxc) && (channel <= maxc)) {
    	if(setchannel(channel))
			 return -1;
	}
	if(channel >= maxc) {
		channel = maxc;
		if(setchannel(channel))
			 return -1;
	}
	
    if(v4l_mmap()) {
    	fprintf(stderr, "[V4L]::VDopen: mmap interface is not supported by this driver.\n");
        return -1;
	}

 	/* quick hack for v4l driver that does not support double buffer capturing */
	if(vd.mbuf.frames < 2) {
    	fprintf(stderr, "[V4L]::VDopen: double buffer capturing with mmap is not supported.\n");
        return -1;
	}
    /* OLD COMMENT detecting a pixel format supported by the v4l driver.
     * video_set_grabformat() overwrites both 'converter' and 'converter_hflip'.
     * If 'converter' is non-NULL, palette converter must be initialized. */
	if(setGrabFormat(palette)) {
		fprintf(stderr, "[V4L]::VDopen: ERROR : Can't find a supported pixel format.\n");
        return -1;
	}

    v4l_getpicture();
    picture_brightness = vd.picture.brightness;
    picture_hue = vd.picture.hue;
	picture_colour = vd.picture.colour;

/*** END COPY ****/

	if(StartGrab()) {
		fprintf(stderr, "[V4L]::VDopen: Can't start grab.\n");
  		//normal at beginning or when capture has been stopped
	}

	initialised = true;
//    echo();

	return 0;
}

int SwV4LDevice::VDclose(){
	printf("[V4L] : Closing device...\n");
    if(!initialised) 
		return -1;
	initialised = false;
	v4l_munmap();
	return v4l_close();
}

// starts video acquisition (public function)
int SwV4LDevice::VDInitAll()
{
    if(StartGrab())
    	return -1;
    
	return 0;
}

// display video parameters
void SwV4LDevice::echo()
{
	int i=0;

	if(vd.fd==-1)
		printf("No video device opened !\n");
	else if(!initialised)
		printf("Video device not initialised ! \n");
	else {
		printf("Video device name : %s\n", vd.capability.name);
		printf("Main capabilities : \n");
		printf("\tchannels: %d\n", vd.capability.channels);
		printf("\tmax size: %dx%d\n",vd.capability.maxwidth, vd.capability.maxheight);
		printf("\tmin size: %dx%d\n",vd.capability.minwidth, vd.capability.minheight);
		printf("\tDevice type : \n");
		if(vd.capability.type & VID_TYPE_CAPTURE) printf("\t\tVID_TYPE_CAPTURE,");
		if(vd.capability.type & VID_TYPE_OVERLAY) printf("\t\tVID_TYPE_OVERLAY,");
		if(vd.capability.type & VID_TYPE_CLIPPING) printf("\t\tVID_TYPE_CLIPPING,");
		if(vd.capability.type & VID_TYPE_FRAMERAM) printf("\t\tVID_TYPE_FRAMERAM,");
		if(vd.capability.type & VID_TYPE_SCALES) printf("\t\tVID_TYPE_SCALES,");
		if(vd.capability.type & VID_TYPE_MONOCHROME) printf("\t\tVID_TYPE_MONOCHROME,");
		if(vd.capability.type & VID_TYPE_SUBCAPTURE) printf("\t\tVID_TYPE_SUBCAPTURE,");
		printf("\nCurrent status;\n");
		printf("\tpicture.depth: %d\n",vd.picture.depth);
		printf("\tmbuf.size: %08x\n",vd.mbuf.size);
		printf("\tmbuf.frames: %d\n",vd.mbuf.frames);
		printf("\tmbuf.offsets[0]: %08x\n",vd.mbuf.offsets[0]);
		printf("\tmbuf.offsets[1]: %08x\n",vd.mbuf.offsets[1]);
		printf("\nVideo Window : \n");
		printf("\tx = %d y = %d \n", vd.window.x, vd.window.y);
		printf("\twidth = %d height = %d \n", vd.window.width, vd.window.height);
        printf("Misc : \n");
		printf("Palette : ");
		for(i=0;i<NB_PALETTES;i++)
		{
			if(palettes_defined[i].value==vd.picture.palette)
			{
				printf("%s\n", palettes_defined[i].designation);
				break;
			}
		}
		if(hasFramerate)
			printf("Video device HAS framerate\n");
		else
			printf("Video device DOES NOT support framerate\n");
		if(useSelectAvailable)
			printf("Select() AVAILABLE\n");
		else
			printf("Select() NOT AVAILABLE\n");
	}
}

// public function : returns already read capability
int SwV4LDevice::getcapability(video_capability * dest)
{
	if(!initialised)
		return -1;

	memcpy(dest, &(vd.capability), sizeof(video_capability));
	return 0;
}

int SwV4LDevice::getpicture(video_picture * dest)
{
	if(!initialised)
		return -1;

	memcpy(dest, &(vd.picture), sizeof(video_picture));
	return 0;
}

int SwV4LDevice::getmbuf(video_mbuf * dest)
{
	if(!initialised)
		return -1;

	memcpy(dest, &(vd.mbuf), sizeof(video_mbuf));
	return 0;
}

int SwV4LDevice::getFD()
{
	return vd.fd;
}

int SwV4LDevice::maxchannel()
{
	if(!initialised)
		return -1;

	return vd.capability.channels;
}

int SwV4LDevice::getchannel(video_channel * dest, int noChannel)
{
	if(!initialised)
		return -1;

	if(noChannel>(vd.capability.channels-1))
		return -1;

	memcpy(dest, &(vd.channel[noChannel]), sizeof(video_channel));
	return 0;
}

int SwV4LDevice::getpalette()
{
	if(!initialised)
		return -1;
/*	fprintf(stderr, "[V4L]::getpalette : current palette : %d = ", 
				vd.picture.palette);
*/
	for(int i=0;i<NB_PALETTES;i++)
	{
		if(palettes_defined[i].value==vd.picture.palette)
		{
			//??? Why this shit ? return palettes_defined[i].value;
/*			fprintf(stderr, "[V4L]::getpalette : current palette : '%s'", 
				palettes_defined[i].designation);
	*/	}
	}
	//fprintf(stderr, "\n");
	
	return vd.picture.palette;
}

int SwV4LDevice::getcapturewindow (video_window * dest)
{
	if(!initialised)
		return -1;

	memcpy(dest, &(vd.window), sizeof(video_window));
	return 0;
}

int SwV4LDevice::getcapturewindowsize (tBoxSize * dest)
{
	if(!initialised)
		return -1;

	dest->x = vd.window.x;
	dest->y = vd.window.y;
	if(VDHasMmap()) {
		dest->width = vd.mmap.width;
		dest->height = vd.mmap.height;
	} else {
		dest->width = vd.window.width;
		dest->height = vd.window.height;
	}
	return 0;
}

/* UNUSED XXX
// only after a SetSubCapture
// this value isn't read at init()
int SwV4LDevice::getsubcapture(video_capture * dest)
{
	if(vd.fd==-1)
		return -1;
	
	v4l_getsubcapture(dest);
	return 0;
}
*/

pthread_mutex_t * SwV4LDevice::getMutex()
{
	return &(vd.mutex);
}

bool SwV4LDevice::VDHasFramerate()
{
	return hasFramerate;
}
bool SwV4LDevice::VDSelectAvailable()
{
	return useSelectAvailable;
}

bool SwV4LDevice::VDIsInitialised()
{
	return initialised;
}

bool SwV4LDevice::VDHasMmap()
{
	return(vd.mbuf.size>0);
}

int SwV4LDevice::setframebuffer(void *base, int width, int height, int depth, int bpl)
{
	if(vd.fd==-1)
		return -1;
	return v4l_setframebuffer(base, width, height, depth, bpl);
}

int SwV4LDevice::setpicture(int br, int hue, int col, int cont, int white)
{
	if(vd.fd ==-1)
		return -1;

	video_picture tmpPicture;
	if(br>=0)
		tmpPicture.brightness = br;
	if(hue>=0)
		tmpPicture.hue = hue;
	if(col>=0)
		tmpPicture.colour = col;
	if(cont>=0)
		tmpPicture.contrast = cont;
	if(white>=0)
		tmpPicture.whiteness = white;
	v4l_setpicture(br, hue, col, cont, white);

	return 0;
}



/* XXX UNUSED ??????
// Beware : always do VIDIOSYNC before calling this function
int SwV4LDevice::setcapturewindowsize (tBoxSize size)
{
	if(vd.fd==-1)
		return -1;

   	struct video_window vwin;
	tBoxSize tmpBox;

	//TEST
//	return video_changesize(size.width, size.height);

	
	if(((int)size.width > vd.capability.maxwidth) || ((int)size.height > vd.capability.maxheight) )
		return -1;

	if(ioctl(vd.fd, VIDIOCGWIN, &vwin) < 0)
		return -1;

	// sauvegarde pour restauration éventuelle...
	tmpBox.x = vwin.x;
	tmpBox.y = vwin.y;
	tmpBox.width = vwin.width;
	tmpBox.height = vwin.height;

	// on tente de configurer les nouveaux paramètres
	vwin.x = size.x;
	vwin.y = size.y;
	vwin.width = size.width;
	vwin.height = size.height;
	vwin.clipcount = 0;
	if(ioctl(vd.fd, VIDIOCSWIN, &vwin) < 0)
		return -1;

	// then read values
	if(ioctl(vd.fd, VIDIOCGWIN, &vwin) < 0)
		return -1;

	if(vwin.width == 0 && vwin.height == 0) 
	{
		// keep old params
		vwin.x = tmpBox.x;
		vwin.y = tmpBox.y;
		vwin.width = tmpBox.width;
		vwin.height = tmpBox.height;

		ioctl(vd.fd, VIDIOCSWIN, &vwin); 
		return -1;
	}
	vd.window.width = vwin.width;
	vd.window.height = vwin.height;

	return 0;
}
*/

int SwV4LDevice::setsubcapture ( int x, int y, int width, int height, int decimation, int flags)
{
	if(vd.fd==-1)
		return -1;

	video_capture tmpCapture;
	tmpCapture.x = x;
	tmpCapture.y = y;
	tmpCapture.width = width;
	tmpCapture.height = height;
	tmpCapture.decimation = decimation;
	tmpCapture.flags = flags;

	if(ioctl(vd.fd, VIDIOCSCAPTURE, &tmpCapture) < 0)
		return -1;

	// MAJ de capture inutile : valeur relue à chaque appel de getsubcapture
	return 0;
}

/** unused
int SwV4LDevice::setdefaultnorm	(int defaultNorme)
{
	if(vd.fd==-1)
		return -1;

	int i;

	for(i=0;i<vd.capability.channels;i++) {
		vd.channel[i].norm = defaultNorme;
	}

	return 0;
}
*/
int SwV4LDevice::getcurrentchannel()
{
	if(vd.fd==-1) {
		perror("[V4L]::getcurrentchannel: vd.fd==-1");
		return -1;
	}

	return channel;
}
char * SwV4LDevice::getnorm()
{
	if(ioctl(vd.fd, VIDIOCGCHAN, &(vd.channel[channel])) < 0)
	{
		fprintf(stderr, "[V4L]::getnorm:VIDIOCGCHAN (channel = %d)", channel);
		return NULL;
	}
	for(int i=0; normlists[i].name[0] != '\0'; i++)
		if(normlists[i].type == vd.channel[channel].norm)
			return normlists[i].name;
	return NULL;
}

int SwV4LDevice::setchannel(int ch)
{
	if(vd.fd==-1)
		return -1;

	if(ioctl(vd.fd, VIDIOCSCHAN, &(vd.channel[ch])) < 0)
	{
		perror("[V4L]::setchannel:VIDIOCSCHAN");
		return -1;
	}
	channel = ch;
	return 0;
}

int SwV4LDevice::setfrequence(int freq)
{
	if(vd.fd==-1)
		return -1;

	unsigned long longfreq=(freq*16)/1000;
	if(ioctl(vd.fd, VIDIOCSFREQ, &longfreq) < 0)
		return -1;

	return 0;
}

int SwV4LDevice::setpalette(int pal)
{
	if(vd.fd==-1)
		return -1;

	vd.picture.palette = pal;
	if(ioctl(vd.fd, VIDIOCSPICT, &(vd.picture)) < 0)
		return -1;

	palette = pal;
	return 0;
}

/** v4lsyncf - flip-flop sync
 */
int SwV4LDevice::v4l_syncf()
{
	return v4l_sync(vd.frame);
}


unsigned char * SwV4LDevice::getFrameBuffer()
{
	if(v4l_syncf()) {
		fprintf(stderr, "[V4L]::getFrameBuffer: error in v4l_syncf()\n");
		vd.frame = (vd.frame+1)%2;
		if(v4l_grabstart(vd.frame) < 0)
			return NULL;
	}
	unsigned char * buf = v4l_getaddress();
	
    if(v4l_grabf()) {
		fprintf(stderr, "[V4L]::getFrameBuffer: error in v4l_grabf()\n");
		return NULL;
	}

	return buf;
}



/***********************************************************************
 *                MODIFIED / VALIDATED VERSION                         *
 ***********************************************************************/
/* Start the continuous grabbing */
int SwV4LDevice::StartGrab()
{
	vd.frame = 0;
	if(v4l_grabstart(0) < 0)
		return -1;
	if(v4l_grabstart(1) < 0)
		return -1;
	return 0;
}

/* Stop the continuous grabbing */
int SwV4LDevice::StopGrab()
{
	// wait for acquisition to stop then stop
	if(vd.framestat[vd.frame]) {
		if(v4l_sync(vd.frame) < 0)
			return -1;
	}
	if(vd.framestat[vd.frame ^ 1]) {
		if(v4l_sync(vd.frame ^ 1) < 0)
			return -1;
	}
	return 0;
}

/** v4l_open - open the v4l device.
 @param name device name (ex: "/dev/video1")
 @param vd V'ldevice struct for output parameters
 */
int SwV4LDevice::v4l_open(char *name)
{
	int i;
	char device[32] = DEFAULT_DEVICE;
	if(name)
		strcpy(device, name);

	if(DEBUG_V4L) 
		fprintf(stderr, "[V4L]::v4l_open:open...\n");
	
	if((vd.fd = open(name,O_RDWR)) < 0) {
		perror("[V4L]::v4l_open:open");
		return -1;
	}
	if(v4l_getcapability()) {
		perror("[V4L]::v4l_open:v4l_getcapability");
		return -1;
	}
	if(DEBUG_V4L) 
		fprintf(stderr, "[V4L]::v4l_open:VIDIOCGCHAN...\n");
	
	for(i=0;i<vd.capability.channels;i++) {
		vd.channel[i].channel = i;
		if(ioctl(vd.fd, VIDIOCGCHAN, &(vd.channel[i])) < 0) {
			perror("[V4L]::v4l_open:VIDIOCGCHAN");
			return -1;
		}
	}
	v4l_getpicture();
	pthread_mutex_init(&vd.mutex, NULL);
	if(DEBUG_V4L) 
		fprintf(stderr, "[V4L]::v4l_open:quit\n");
	return 0;
}

/** v4l_close - close device
 */
int SwV4LDevice::v4l_close()
{
	if(DEBUG_V4L) 
		fprintf(stderr, "[V4L]::v4lclose:close...\n");
	close(vd.fd);
	if(DEBUG_V4L) 
		fprintf(stderr, "[V4L]::v4lclose:quit\n");
	return 0;
}

/** v4l_getcapability - get the capability into vd.capability
 */
int SwV4LDevice::v4l_getcapability()
{
	if(DEBUG_V4L) fprintf(stderr, "[V4L]::v4l_getcapability:VIDIOCGCAP...\n");
	if(ioctl(vd.fd, VIDIOCGCAP, &(vd.capability)) < 0) {
		perror("[V4L]::v4l_open:VIDIOCGCAP");
		return -1;
	}
	if(DEBUG_V4L) 
		fprintf(stderr, "\tv4l_getcapability:done\n");
	return 0;
}

/** v4l_setdefaultnorm - set default norm and reset parameters
 */
int SwV4LDevice::v4l_setdefaultnorm(int n)
{
	int i;

	for(i=0;i<vd.capability.channels;i++) {
		v4l_setchannelnorm(i, n);
	}
	if(v4l_getcapability())
		return -1;
	if(v4l_getpicture())
		return -1;
	norm = n;
	return 0;
}

/** v4l_setchannelnorm - set the norm of channel
 @param vd device handling struct
 @param channel channel number
 @param n input norm PAL/SECAM/NTSC/OTHER (source: videodev.h)
 */
int SwV4LDevice::v4l_setchannelnorm(int ch, int n)
{
	vd.channel[ch].norm = n;
	return 0;
}



/** v4l_getpicture - get current properties of the picture
 */
int SwV4LDevice::v4l_getpicture()
{
	if(ioctl(vd.fd, VIDIOCGPICT, &(vd.picture)) < 0) {
		perror("[V4L]::v4l_getpicture:VIDIOCGPICT");
		return -1;
	}
	return 0;
}

/// returns video buffer address
unsigned char * SwV4LDevice::video_getaddress()
{
	return (vd.map + vd.mbuf.offsets[vd.frame]);
}

/// v4lgetmbuf - get the size of the buffer to mmap
int SwV4LDevice::v4l_getmbuf()
{
	if(ioctl(vd.fd, VIDIOCGMBUF, &(vd.mbuf))<0) {
		perror("[V4L]::v4l_getmbuf:VIDIOCGMBUF");
		return -1;
	}
	return 0;
}


/** v4lmmap - initialize mmap interface
 */
int SwV4LDevice::v4l_mmap()
{
	if(v4l_getmbuf()<0)
		return -1;
	if((vd.map = (unsigned char *)mmap(0, vd.mbuf.size, PROT_READ|PROT_WRITE, MAP_SHARED, vd.fd, 0)) < 0) {
		perror("[V4L]::v4l_mmap:mmap");
		return -1;
	}
	return 0;
}

/** v4l_munmap - free memory area for mmap interface
 */
int SwV4LDevice::v4l_munmap()
{
	if(munmap(vd.map, vd.mbuf.size) < 0) {
		perror("[V4L]::v4l_munmap:munmap");
		return -1;
	}
	return 0;
}

/** v4l_grabinit - set parameters for mmap interface
 @param vd device =handling struct
 @param width width of the buffer
 @param height height of the buffer
 */
int SwV4LDevice::v4l_grabinit(int width, int height)
{
	vd.mmap.width = width;
	vd.mmap.height = height;
	vd.mmap.format = vd.picture.palette;
	vd.frame = 0;
	vd.framestat[0] = 0;
	vd.framestat[1] = 0;
	return 0;
}

/** v4l_grabstart - activate mmap capturing
 @param vd: v4l device object
 @param frame: frame number for storing captured image
 */
int SwV4LDevice::v4l_grabstart(int frame)
{
	if(vd.framestat[frame]) {
		fprintf(stderr, "[V4L]::v4l_grabstart: frame %d is already used to grab.\n\t>", frame);
		
		frame = frame ^ 1; // XXX TEST : marche a priori (07/05/2003)
		//return -1;
	}
	vd.mmap.frame = frame;
	if(ioctl(vd.fd, VIDIOCMCAPTURE, &(vd.mmap)) < 0) {
		perror("[V4L]::v4l_grabstart:VIDIOCMCAPTURE");
		return -1;
	}
	vd.framestat[frame] = 1;
	return 0;
}

/** v4l_sync - wait until mmap capturing of the frame is finished
 */
int SwV4LDevice::v4l_sync(int frame)
{
	if(DEBUG_V4L) 
		fprintf(stderr, "[V4L]::v4l_sync: sync frame %d.\n",frame);
	if(vd.framestat[frame] == 0) {
		fprintf(stderr, "[V4L]::v4l_sync: grabbing to frame %d is not started.\n", frame);
	}
	if(ioctl(vd.fd, VIDIOCSYNC, &frame) < 0) {
		perror("[V4L]::v4l_sync:VIDIOCSYNC");
		return -1;
	}
	vd.framestat[frame] = 0;
	return 0;
}

/* check supported pixel format */
int SwV4LDevice::GrabCheck(int pal)
{
	int ret;

	fprintf(stderr, "\tSwV4LDevice::GrabCheck(%d) : v4l_setpalette(%d)\n", pal, pal);
 	// problem with Philips webcam when we suppress converter
	if(v4l_setpalette(pal)) {
		ret = -1;
		return ret;
	}
	
	fprintf(stderr, "\tSwV4LDevice::GrabCheck(%d) : v4l_grabstart(0)\n", pal);
	if(v4l_grabstart(0) <0) {
		ret = -1;
		return ret;
	}
	fprintf(stderr, "\tSwV4LDevice::GrabCheck(%d) : v4l_sync(0)\n", pal);
	ret = v4l_sync(0);
	
	fprintf(stderr, "\tSwV4LDevice::GrabCheck(%d) : ok with pal = %d\n", pal, pal);
	// read vd
	v4l_getpicture();
    fprintf(stderr, "\tSwV4LDevice::GrabCheck(%d) : v4l_getpicture returned pal = %d\n", 
		pal, vd.picture.palette);
	
	palette = vd.picture.palette;
	return 0;
}

int SwV4LDevice::setGrabFormat(int pal)
{
	if(pal == 0) {
		if(GrabCheck(DEFAULT_PALETTE) == 0) {
			return 0;
		}
	} else {
	}

    fprintf(stderr, "[V4L]::setGrabFormat : default palette not supported. Looking for supported palette...\n");
    
	v4l_getpicture();
	
	fprintf(stderr, "\tGet current palette : %d = ", 
		vd.picture.palette);
	int i;
	for(i=0; i<NB_PALETTES; i++)
    	if(palettes_defined[i].value == vd.picture.palette)
			fprintf(stderr, "%s\n", palettes_defined[i].designation);

	for(i=0; i<NB_PALETTES; i++)
       	if(GrabCheck( palettes_defined[i].value ) == 0) {
        	fprintf(stderr, "[V4L]::setGrabFormat : Palette %s is supported.\n",
         		palettes_defined[i].designation);
			palette = palettes_defined[i].value;
			return 0;
		}

    fprintf(stderr, "SwVideoAcquisition : no supported palette found.\n");

	return 1;
}
/** v4l_getframebuffer - get frame buffer address
 */
int SwV4LDevice::v4l_getframebuffer()
{
	if(ioctl(vd.fd, VIDIOCGFBUF, &(vd.buffer)) < 0) {
		perror("[V4L]::v4l_getframebuffer:VIDIOCGFBUF");
		return -1;
	}
	return 0;
}

/** v4l_setframebuffer - set parameters of frame buffer
 @param vd device handling struct
 @param base base PHYSICAL address of the frame buffer
 @param width width of the frame buffer
 @param height height of the frame buffer
 @param depth color depth of the frame buffer
 @param bpl number of bytes of memory between the start of two adjacent lines
 */
int SwV4LDevice::v4l_setframebuffer(void *base, 
	int width, int height, int depth, int bytesperline)
{
	vd.buffer.base = base;
	vd.buffer.width = width;
	vd.buffer.height = height;
	vd.buffer.depth = depth;
	vd.buffer.bytesperline = bytesperline;
	
	if(ioctl(vd.fd, VIDIOCSFBUF, &(vd.buffer)) < 0) {
		perror("[V4L]::v4l_setframebuffer:VIDIOCSFBUF");
		return -1;
	}
	return 0;
}


/** v4l_setpalette - set the palette for the images
 */
int SwV4LDevice::v4l_setpalette(int pal)
{
	vd.picture.palette = pal;
	vd.mmap.format = pal;
	if(ioctl(vd.fd, VIDIOCSPICT, &(vd.picture)) < 0) {
		perror("[V4L]::v4l_setpalette:VIDIOCSPICT");
		return -1;
	}
	palette = pal;
	return 0;
}

/** v4l_setpicture - set the picture properties 
 @param vd device handling struct
 @param br picture brightness
 @param hue picture hue
 @param col picture color
 @param cont picture contrast
 @param white picture whiteness
 */
int SwV4LDevice::v4l_setpicture(int br, int hue, int col, int cont, int white)
{
	if(br>=0)
		vd.picture.brightness = br;
	if(hue>=0)
		vd.picture.hue = hue;
	if(col>=0)
		vd.picture.colour = col;
	if(cont>=0)
		vd.picture.contrast = cont;
	if(white>=0)
		vd.picture.whiteness = white;
	if(ioctl(vd.fd, VIDIOCSPICT, &(vd.picture)) < 0) {
		perror("[V4L]::v4l_setpicture:VIDIOCSPICT");
		return -1;
	}
	return 0;
}

/** v4lgrabf - flip-flop grabbing
 */
int SwV4LDevice::v4l_grabf()
{
	int f;

	f = vd.frame;
	vd.frame = vd.frame ^ 1;
	return v4l_grabstart(f);
}

/** v4l_getaddress - returns a offset addres of buffer for mmap capturing
 */
unsigned char * SwV4LDevice::v4l_getaddress()
{
	return (vd.map + vd.mbuf.offsets[vd.frame]);
}
