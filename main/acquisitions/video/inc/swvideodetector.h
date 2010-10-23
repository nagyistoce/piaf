/***************************************************************************
        swvideodetector.h  -  video processing utilities
                             -------------------
    begin                : Fri Jun 7 2002
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
#ifndef _SW_VIDEO_DEFS_

#define _SW_VIDEO_DEFS_



#include <sys/time.h>

#include "video_asm.h"

#ifndef __WITHOUT_OPENCV__
#include "cv.h"
#include "cv.hpp"
#else
#include "cversatz.h"
#endif
#include "sw_types.h"


// Définitions utilisées pour la gestion des modes
// Modes de fonctionnement
#define MODE_STOP									0
#define MODE_INITIALISATION					1
#define MODE_CALIBRATION						2
#define MODE_DETECTION							3
#define MODE_RECORD									4
#define MODE_FAILURE									255
// Etats
#define ARRET_OK											0
#define INIT_IN_PROGRESS								1
#define INIT_SUCCEED										2
#define INIT_FAILED										3
#define CALIBRATION_INIT							4
#define CALIBRATION_IN_PROGRESS			5
#define CALIBRATION_FIN_OK					6
#define CALIBRATION_FAILED					7
#define DETECTION_IN_PROGRESS					8
#define DYNAMIC_CALIBRATION		9
// Alarms
#define NO_ALARM										0
#define CMD_INVALID						1
#define IMAGE_SIZE_INVALID	2
#define LOW_LUMINOSITY						3

#define DEFAULT_LEVEL_CALIB					40  // à confirmer
#define DEFAULT_CALIBRATION_TIME 			2   // seconds
#define DEFAULT_LEVEL_MIN					20

// Failures
#define NO_FAILURE							0
#define MEMORY_ALLOCATION				1


/// alarm struct for processing status
struct t_cre
{
	int mode;
	int etat;
	int alarme;
	int panne;
};

/// For image storage
typedef struct _Img {
	unsigned char 	*pixmap;
	unsigned short 	width;
	unsigned short 	height;
	unsigned long 	size;
	unsigned char 	depth;
} Img;

#ifndef max
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#endif

/// Gets size from PNM file (PPM/PGM)
tBoxSize LoadPPMHeader(char *name);
/// Loads a PNM image from file to memory (must be allocated !). Please use LoadPPMHeader before.
int LoadPPMFile(char *name, unsigned char * Image, int maxsize);
/// Saves a PNM file from a memory buffer (Grayscaled or RGB24 coded)
void SavePPMFile(char *filename, bool colored, tBoxSize size, unsigned char *buffer);


#define MARKER_COLOR 255

/** @brief Print image properties */
void swPrintProperties(IplImage * img);

/** @brief Return image layer depth in bytes (e.g. 8bit jpeg is 1 byte, 16bit:2bytes...) */
int swByteDepth(IplImage * iplImage);

/** @brief Create an IplImage width OpenCV's cvCreateImage and clear buffer */
IplImage * swCreateImage(CvSize size, int depth, int channels);
/** @brief Create an IplImage header width OpenCV's cvCreateImageHeader */
IplImage * swCreateImageHeader(CvSize size, int depth, int channels);

/** @brief Release an image and clear pointer */
void swReleaseImage(IplImage ** img);
/** @brief Release an image header and clear pointer */
void swReleaseImageHeader(IplImage ** img);

#endif
