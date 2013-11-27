/*|------------------------------------------------------------------------------------------------------------|*/
/*|     FFmpegFileVideoAcquisition.h  -  description												|*/
/*|		  ---------------------------------														|*/				
/*|	begin					: Wed July 9 2003																					|*/
/*|   	copyright				: (C) 2003 by Christophe Seyve																		|*/
/*|    email					: cseyve@free.fr																		|*/
/*|	description			: Entete de la classe VideoAcquisition par fichier											|*/
/*|------------------------------------------------------------------------------------------------------------|*/

#ifndef FFMPEG_FILE_VIDEOACQUISITION_H
#define FFMPEG_FILE_VIDEOACQUISITION_H


#ifndef WIN32
#include <sys/mman.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>

#else

//#include <win32_videodev.h>

#endif

// Problem with ffmpeg
#ifndef UINT64_C
# if __WORDSIZE == 64
#  define UINT64_C(c)   c ## UL
# else
#  define UINT64_C(c)   c ## ULL
# endif
#endif

// For later versions of ffmpeg
#ifndef PIX_FMT_RGBA32
#define PIX_FMT_RGBA PIX_FMT_RGBA32
#endif

#include "imageinfo.h"

#include "nolinux_videodev.h"
#include "virtualdeviceacquisition.h"
#include "FileVideoAcquisition.h"

#ifdef PIAF_LEGACY
#include "workshopmovie.h"
#else
#include "workflowtypes.h"
#endif 



#include "ccvt.h"

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN	1024
#endif

extern "C" {
#include <stdint.h>
#include <avcodec.h>
#include <avformat.h>
}




/** \brief Video acquisition high-level class

	using FFMPEG for decoding, performs file device management
	and image acquisitions and conversion to IplImage.

	@author Christophe Seyve - Sisell - cseyve@free.fr
	@version 0.1.0 \Date
 */
class FFmpegFileVideoAcquisition : public FileVideoAcquisition
{
public:
	/// creator function registered in factory
	static FileVideoAcquisition* creatorFunction(std::string path);

	/// Constructor
	FFmpegFileVideoAcquisition();
		
	/** Constructor with device entry
		\param movie filename
		*/
	FFmpegFileVideoAcquisition(const char * device);
	
	/** Constructor with SwV4LDevice pointer entry
		\param aVD SwV4LDevice class entry. Rare usage.
		*/
	/// Destructor
	~FFmpegFileVideoAcquisition();

	/// Close video device (only on demand)
	int VAcloseVD();
	
	void setCopyScale(int) { };
	
	/// Checks if video device is initialised
	bool VDIsInitialised();	
	/// Checks if video acquisition is initialised
	bool AcqIsInitialised();
	
	/** @brief Open device and read first image */
	int openDevice(const char * aDevice, tBoxSize newSize);

	// FOR VirtualDeviceAcquisition PURE VIRTUAL API
	/** \brief Use sequential mode
		If true, grabbing is done when a new image is requested.
		Else a thread is started to grab */
	void setSequentialMode(bool on);

	/** \brief Function called by the doc (parent) thread */
	int grab();

	/** \brief Return true if acquisition device is ready to start */
	bool isDeviceReady();

	/** \brief Return true if acquisition is running */
	bool isAcquisitionRunning();

	/** \brief Start acquisition */
	int startAcquisition();

	/** \brief Return image size */
	CvSize getImageSize();

	/** \brief Stop acquisition */
	int stopAcquisition();


	/** \brief Get the list of output format */
	QList<t_video_output_format> getOutputFormats();

	/** \brief Set the output format */
	int setOutputFormat(int id);

	/** @brief Read image as data of selected format */
	IplImage * readImage();


	/** \brief Grabs one image and convert to RGB32 coding format
		if sequential mode, return last acquired image, else read and return image

		\return NULL if error
		*/
	IplImage * readImageRGB32();

	/** \brief Grabs one image and convert to grayscale coding format
		if sequential mode, return last acquired image, else read and return image

		\return NULL if error
		*/
	IplImage * readImageY();

	/** \brief Grabs one image of depth buffer, in meter
		if sequential mode, return last acquired image, else read and return image

		\return NULL if error
		*/
	IplImage * readImageDepth() { return 0; }
	/** @brief Read image as raw data */
	IplImage * readImageRaw();

	/** @brief Get video properties (not updated) */
	t_video_properties getVideoProperties() { updateVideoProperties(); return m_video_properties; }

	/** @brief Update and return video properties */
	t_video_properties updateVideoProperties();

	/** @brief Set video properties (not updated) */
	int setVideoProperties(t_video_properties props);


	// end of VirtualDeviceAcquisition API

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
	int readImageRGB32(unsigned char* image, long * buffersize, bool contAcq = false);
	/** @brief Read image as raw data */


	/**************** FROM FileVideoAcquisition (BEGIN) ************************/
	/** Return the index of last read frame (needed for post-processing or permanent encoding) */
	long getFrameIndex();
	/** Return the absolute position to be stored in database (needed for post-processing or permanent encoding) */
	unsigned long long getAbsolutePosition();
	void rewindMovie();

	/**************** FROM FileVideoAcquisition (END) ************************/

	/** get position of picture n-1
	 */
	unsigned long long getPrevAbsolutePosition() { return m_prevPosition; }

	/** set absolute position in file
	 */
	void setAbsolutePosition(unsigned long long newpos);
	/** go to frame position in file
	 */
	void setAbsoluteFrame(int frame);


	/** @brief Get last movie position */
	t_movie_pos getMoviePosition() { return m_movie_pos; }


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
	
	bool endOfFile();
	
	/** @brief Go to absolute position in file (in bytes from start) */
	void rewindToPosMovie(unsigned long long position);
	/** @brief Read next frame */
	bool GetNextFrame();

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


private:
	void purge();
	void initPlayer();
	void fill_buffer();

private:
	/// Main video properties
	t_video_properties m_video_properties;
	/// Video file name
	char m_videoFileName[MAX_PATH_LEN];

	/// FFMPEG Format Context
	AVFormatContext *m_pFormatCtx;

	/// FFMPEG Codec Context
    AVCodecContext  *m_pCodecCtx;
    AVCodec         *m_pCodec;

	/// FFMPEG Frames
    AVFrame         *m_pFrame; 

	/// FFMPEG Frames decoded in RGB
	AVFrame         *m_pFrameRGB;

	bool m_isJpeg;

	/// Real image size (because pCodecCtx may change to fit block size)
	CvSize mImageSize;

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


	/// Usage to be confirmed...
	int myVD_palette;

	// Previous picture position
	unsigned long long m_prevPosition;


	// Variables for VirtualDeviceAcquisition API
	void initVirtualDevice();///< init of VirtualDeviceAcqiosition API variables
	void purgeVirtualDevice();///< init of VirtualDeviceAcqiosition API variables

	bool mSequentialMode; ///< Sequential mode vs threaded (default= true)
	IplImage * m_imageRGB32;
	IplImage * m_imageBGR32;
	IplImage * m_imageY;

protected:
	/// value used for registering in factory
	static std::string mRegistered;

	// -------- IMAGE INFORMATION -------
public:
	/** @brief Read image information */
	t_image_info_struct readImageInfo() { return mImageInfo; }

protected:
	t_image_info_struct mImageInfo;

};

#endif
