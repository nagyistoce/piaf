/***************************************************************************
				piaf-common.h  - Piaf common header
	common macros and definitions for Piaf GUI
							 -------------------
	begin                : Thu July 5 2010
	copyright            : (C) 2010 Christophe Seyve (CSE)
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

#ifndef PIAFCOMMON_H
#define PIAFCOMMON_H

#ifndef BASE_DIRECTORY
#define BASE_DIRECTORY "/usr/local/piaf/"
#endif
#ifndef TMP_DIRECTORY
#define TMP_DIRECTORY	"/tmp/"
#endif
#ifndef SHM_DIRECTORY
#define SHM_DIRECTORY	"/dev/shm/"
#endif
#define EXIT_ON_ERROR fprintf(stderr,"%s (%s:%d) : EXIT - FATAL ERROR !!!!!!!!!!!!!\n",__func__,__FILE__,__LINE__);fflush(stderr);exit(0);
#define DEBUG_ALL(s) fprintf(stderr, "DEBUG_MSG %s (%s l.%d) '%s'\n",__func__,__FILE__,__LINE__, (s));
#define PRINT_FIXME fprintf(stderr, "%s : %s:%d FIXME\n", __FILE__, __func__, __LINE__);
#ifndef DEBUG_MSG
#include <sys/time.h>

#define DEBUG_MSG(...) { \
							struct timeval l_nowtv; gettimeofday(&l_nowtv, NULL); \
							time_t l_raw = l_nowtv.tv_sec; \
							struct tm l_timeinfo; \
							if(localtime(&l_raw)) { \
								memcpy(&l_timeinfo, localtime(&l_raw), sizeof(struct tm)); \
								fprintf(stderr, "%04d-%02d-%02d %02d:%02d:%02d.%02d\t[%s] %s:%d DEBUG: \t", \
									(int)(l_timeinfo.tm_year + 1900), (int)(l_timeinfo.tm_mon + 1), (int)(l_timeinfo.tm_mday), \
									(int)(l_timeinfo.tm_hour), (int)(l_timeinfo.tm_min), (int)(l_timeinfo.tm_sec), \
									(int)(l_nowtv.tv_usec/10000),  \
									__FILE__, __func__, __LINE__); \
							} else { \
								fprintf(stderr, "%04d.%02ds\t[%s] %s:%d DEBUG: \t", \
										(int)(l_nowtv.tv_sec%1000),  \
										(int)(l_nowtv.tv_usec/10000),  \
										__FILE__, __func__, __LINE__); \
							} \
							fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); \
						}
#endif
// MEMORY ALLOCATION MACROS
#define CPP_DELETE(buf) {if((buf)) delete (buf); (buf)=NULL;}
#define CPP_DELETE_ARRAY(buf) {if((buf)) delete [] (buf); (buf)=NULL;}
#define CPP_DELETE_DOUBLE_ARRAY(buf,nb) {for(int i=0;i<(nb);i++) if(buf[i]) delete [] buf[i]; \
		if((buf)) delete [] (buf); (buf)=NULL;}

#ifdef WORKSHOP_CPP
#define PIAF_EXTERN
#define PIAF_ZERO		=0
#else
#define PIAF_EXTERN		extern
#define PIAF_ZERO
#endif

//
#define SWLOG_TRACE		-2
#define SWLOG_DEBUG		-1
#define SWLOG_INFO		0
#define SWLOG_WARNING	1
#define SWLOG_ERROR		2
#define SWLOG_CRITICAL	3

// DEBUG MESSAGE

// NTFS/FAT is not protected against multiple write, so we use a mutex to lock printf
PIAF_EXTERN int g_debug_piaf	PIAF_ZERO;

const char swlog_msgtab[6][12] = {
	"TRACE  ",
	"DEBUG  ",
	"INFO   ",
	"WARNING",
	"ERROR  ",
	"none   "
};

#define SWLOG_MSG(a) ((a)>=SWLOG_TRACE&&(a)<=SWLOG_CRITICAL?swlog_msgtab[(a)+2]:"UNKNOWN")

#define PIAF_MSG(a,...)       { \
			if( (a)>=g_debug_piaf ) { \
					struct timeval l_nowtv; gettimeofday (&l_nowtv, NULL); \
					fprintf(stderr,"%03d.%03d %s [Piaf] [%s] %s:%d : ", \
							(int)(l_nowtv.tv_sec%1000), (int)(l_nowtv.tv_usec/1000), \
							SWLOG_MSG((a)), __FILE__,__func__,__LINE__); \
					fprintf(stderr,__VA_ARGS__); \
					fprintf(stderr,"\n"); \
			} \
		}

//#ifndef max
//#define max(a,b) ((a)>(b)?(a):(b))
//#endif
//#ifndef min
//#define min(a,b) ((a)<(b)?(a):(b))
//#endif


#endif // PIAFCOMMON_H
