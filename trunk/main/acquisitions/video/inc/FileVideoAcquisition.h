/*|-------------------------------------------------------------------------------------------------------------|*/
/*|     FileVideoAcquisition.h  -  Interface class for video acquisition										|*/
/*|		  ---------------------------------																		|*/
/*|	begin					: Wed July 9 2003																	|*/
/*|   	copyright				: (C) 2003 by Christophe Seyve													|*/
/*|    email					: cseyve@free.fr																|*/
/*|	description			: Entete de la classe VideoAcquisition par fichier										|*/
/*|-------------------------------------------------------------------------------------------------------------|*/

#ifndef FILE_VIDEOACQUISITION_H
#define FILE_VIDEOACQUISITION_H


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

#include "nolinux_videodev.h"
#include "virtualdeviceacquisition.h"
#include "piaf-common.h"

//#include "file_video_acquisition_factory.h"

#ifdef PIAF_LEGACY
#include "workshopmovie.h"
#else
#include "workflowtypes.h"
#endif 

#include "sw_types.h"


#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN	1024
#endif


/** Video acquisition high-level class
 * Performs device management and image acquisitions.
 @author Christophe Seyve - Sisell - cseyve@free.fr
 @version 0.1.0 \Date
 */
class FileVideoAcquisition : public VirtualDeviceAcquisition {
public:
		
//	/** Constructor with device entry
//		\param movie filename
//		*/
//	virtual FileVideoAcquisition(const char * device) = 0;

	/** return file name */
	char * getDeviceName() { return m_videoFileName; }
	
	/** Constructor with SwV4LDevice pointer entry
		\param aVD SwV4LDevice class entry. Rare usage.
		*/
	/// Destructor
	//~FileVideoAcquisition();

	/** @brief Return true if this is the end of file */
	virtual bool endOfFile() = 0;
	
	/** @brief Return the index of current frame, from start */
	int getPlayFrame() { return mPlayFrame; }

	// VIDEO PROPERTIES
	/** @brief Return acquisition frame rate in frame per second */
	float getFrameRate() { return m_fps; }
	/** Return acquisition period in milliseconds,

		computed from frame rate */
	long getPeriod() { return m_period_ms; }


	/** @brief Go to absolute position in file (in bytes from start) */
	virtual void rewindToPosMovie(unsigned long long position) = 0;

	/** @brief Rewind to start */
	virtual void rewindMovie() = 0;

	/** @brief Read next frame */
	virtual bool GetNextFrame() = 0;

	/** @brief Get movie position */
	t_movie_pos getMoviePosition() { return m_movie_pos; }

	/** Return the index of last read frame (needed for post-processing or permanent encoding) */
	long getFrameIndex();

	/** Return the absolute position to be stored in database (needed for post-processing or permanent encoding) */
	virtual unsigned long long getAbsolutePosition() = 0;


	/** get position of picture n-1
	 */
	unsigned long long getPrevAbsolutePosition() { return m_prevPosition; }

	/** set absolute position in file
	 */
	virtual void setAbsolutePosition(unsigned long long newpos) = 0;
	/** go to frame position in file
	 */
	virtual void setAbsoluteFrame(int frame) = 0;

	/** @brief Read image information */
	virtual t_image_info_struct readImageInfo() = 0;

	/** Return file size in bytes */
	unsigned long long getFileSize() { return m_fileSize; }

protected:
	/// Main video properties
	t_video_properties m_video_properties;

	/// Video file name
	char m_videoFileName[MAX_PATH_LEN];
	/// File size in bytes
	unsigned long long m_fileSize;

	/// Real image size (because codec may change to fit block size)
	CvSize mImageSize;

	float m_fps; ///< Framerate in frame per second
	long m_period_ms;	///< File period in ms

	/// Index of current stream
	int m_videoStream;

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

	// index of played frame
	long mPlayFrame;
	// Previous picture position
	unsigned long long m_prevPosition;

	/** @brief Last movie position, defined by the position of last key frame and nb of frame since this key frame */
	t_movie_pos m_movie_pos;


	// Variables for VirtualDeviceAcquisition API
	void initVirtualDevice();///< init of VirtualDeviceAcqiosition API variables
	void purgeVirtualDevice();///< init of VirtualDeviceAcqiosition API variables

};



#endif
