/*|------------------------------------------------------------------------------------------------------------|*/
/*|     FileVideoAcquisition.h  -  description												|*/
/*|		  ---------------------------------														|*/				
/*|	begin					: Wed July 9 2003																					|*/
/*|   	copyright				: (C) 2003 by Christophe Seyve																		|*/
/*|    email					: christophe.seyve@sisell.com																		|*/
/*|	description			: Entete de la classe VideoAcquisition par fichier											|*/
/*|------------------------------------------------------------------------------------------------------------|*/

#ifndef _FILE_VIDEOACQUISITION_
#define _FILE_VIDEOACQUISITION_

#include "nolinux_videodev.h"


#ifndef WIN32
#include <sys/mman.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#else

#include <win32_videodev.h>

#endif

#include "sw_types.h"

#include "ccvt.h"

extern "C" {
#include <avcodec.h>
#include <avformat.h>
}

#define SUPPORTED_VIDEO_CODECS_EXT "*.AVI *.avi *.MPG *.mpg *.MPEG *.mpeg *.vob"


/** Video acquisition high-level class
 * Performs device management and image acquisitions.
 @author Christophe Seyve - Sisell - christophe.seyve@sisell.com
 @version 0.1.0 \Date
 */

class FileVideoAcquisition {
public:
	/// Constructor
	FileVideoAcquisition();
		
	/** Constructor with device entry
		\param movie filename
		*/
	FileVideoAcquisition(const char * device);

	/** return file name */
	char * getDeviceName() { return m_videoFileName; };
	
	/** Constructor with SwV4LDevice pointer entry
		\param aVD SwV4LDevice class entry. Rare usage.
		*/
	/// Destructor
	~FileVideoAcquisition();

	/// Close video device (only on demand)
	int VAcloseVD();
	
	void setCopyScale(int) { };
	
	/// Checks if video device is initialised
	bool VDIsInitialised();	
	/// Checks if video acquisition is initialised
	bool AcqIsInitialised();
	
	int openDevice(const char * aDevice, tBoxSize newSize);
	/// Gets definitive image size. You should do that to verify that your selected image size is supported by device.
	tBoxSize getImageSize();
	/// change acquisition size
	int changeAcqParams(tBoxSize newSize, int channel);
	
	int setQuality(int) { return -1; };
	
	/// Set channel number (for devices which support multiple channels)
	int setChannel(int ch);
	int getcurrentchannel() { return m_videoStream;};

	/// Get video capability (min and max size)
	int getcapability(video_capability * vc);

	/** Returns image buffer
		@return pointer to image buffer
		*/
	unsigned char * readImageBuffer(long * buffersize); // read acquisition buffer adress
	
	
	int readImageRaw(unsigned char ** rawimage, 
		unsigned char ** compbuffer , long * compsize);
	
	/** Reads raw image (with coding from palette specifications, YUV420P for example)
		\param image pointer to allocated buffer
		\param contAcq perform or not continuous acquisition (e.g. launch another acquisition)
		*/
	int readImageRaw(unsigned char * rawimage, unsigned char * compimage, long * compsize, bool contAcq);
	
	/** Return acquisition period, computed from frame rate */
	long getPeriod() { return m_period; };
	
	/** Returns address of last acquisition buffer and launch no new acquisition */
	unsigned char * getImageRawNoAcq();
	
	/** Read one frame in Y mode */
	int readImageY(unsigned char* image, long * buffersize);
	/** Read one frame in YUV mode */
	int readImageYUV(unsigned char* image, long * buffersize);
	/** Returns address of last acquisition buffer for Y field and launch no new acquisition */
	int readImageYNoAcq(unsigned char* image, long * buffersize);
	/** Returns address of last acquisition buffer for RGB32 field and launch no new acquisition */
	int readImageRGB32NoAcq(unsigned char* image, long * buffersize);

	/** Grabs one image and convert to RGB32 coding format
		\param image pointer to allocated buffer
		\param contAcq perform or not continuous acquisition (e.g. launch another acquisition)
		@return 0 if error
		*/
	int readImageRGB32(unsigned char* image, long * buffersize, bool contAcq);
	
	// SlotRewindMovie...
	void slotRewindMovie();
	
	/** Return the index of last read frame (needed for post-processing or permanent encoding) */
	long getFrameIndex(); 
	/** Return the absolute position to be stored in database (needed for post-processing or permanent encoding) */
	unsigned long getAbsolutePosition();
	
	/** set absolute position in file
	 */
	void setAbsolutePosition(unsigned long long newpos);
	/** go to frame position in file
	 */
	void setAbsoluteFrame(int frame);
	
	/** get position of picture n-1
	 */
	unsigned long long getPrevAbsolutePosition() { return m_prevPosition; }

	/** Return file size in bytes */
	unsigned long long getFileSize() { return m_fileSize; }

	/// Not really used, just for Video Device compatibility
	char * getnorm() { static char norm[]="pal"; return norm;};
	int setNorm(char * norm);
	/// Set picture brightness, contrast, saturation, color, 
	int setpicture(int br, int hue, int col, int cont, int white);
	int getpicture(video_picture * pic);
	/** Gets palette id.
		@see linux/videodev.h
		*/
	int getPalette();
	/** @brief Return the index of current frame, from start */
	int getPlayFrame() { return playFrame; };
	
	bool endOfFile();
	
	
	// Compatibility for save_frame() call in different classes
	unsigned char * bufY();
	unsigned char * bufU();
	unsigned char * bufV();
	int get_xsize();
	int get_ysize();
	int get_pitch();
	int get_uW();
	int get_vW();
	enum PixelFormat get_pixfmt();
	/** @brief Go to absolute position in file (in bytes from start) */
	void rewindToPosMovie(unsigned long long position);
	bool GetNextFrame();

private:
	void purge();
	void initPlayer();
	void fill_buffer();

private:
	// Video file name
	char m_videoFileName[128];
    // FFMPEG Format Context
	AVFormatContext *m_pFormatCtx;
    // FFMPEG Codec Context
    AVCodecContext  *m_pCodecCtx;
    AVCodec         *m_pCodec;

	// FFMPEG Frames
    AVFrame         *m_pFrame; 
    AVFrame         *m_pFrameRGB;
    bool m_isJpeg;
	// RAW Data buffer
 	unsigned char	*receptBuffer;
	unsigned char	*m_noPitchBuffer;
	long m_noPitchBufferSize;
	// Original pixel format image buffer
	uint8_t         *m_origbuff;
	// RGB image buffer
	uint8_t         *m_inbuff;
	// flag indicating that we have initialised the avcodec lib²
	bool m_bAvcodecIsInitialized;
	bool myVDIsInitialised;
	bool myAcqIsInitialised;
	// no of current stream
	int m_videoStream;
	AVPacket packet;
	long m_period;
	
	// reference time & step
	struct tm FileVA_Tm;
	struct timeval FileVA_Tv;
	
	time_t timeref_sec;
	time_t timestep_sec;
#ifdef WIN32 
	// The type suseconds_t shall be a signed integer type capable of storing values at least in the range [-1, 1000000].
	long timestep_usec;
	long timeref_usec;
#else
	suseconds_t timestep_usec;
	suseconds_t timeref_usec;
#endif
	long playFrame;
	unsigned long long m_fileSize;

	/// Usage to be confirmed...
	int myVD_palette;

	// Previous picture position
	unsigned long long m_prevPosition;
};

#endif
