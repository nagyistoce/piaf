/***************************************************************************
 *            ccvt_c.c Pure C version of CCVT.s
 *
 *  Tue Dec 19 12:55:42 2006
 *  Copyright  2006  Christophe Seyve - SISELL
 *  christophe.seyve@sisell.com
 ****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>




#include "ccvt.h"

#define PUSH_RGB24	1
#define PUSH_BGR24	2
#define PUSH_RGB32	3
#define PUSH_BGR32	4


/* This is a simplistic approach. */
static void ccvt_420p(int width, int height, const unsigned char *src, unsigned char *dst, int push)
{
	int line, col, linewidth;
	int y, u, v, yy, vr, ug, vg, ub;
	int r, g, b;
	const unsigned char *py, *pu, *pv;

	linewidth = width >> 1;
	py = src;
	pu = py + (width * height);
	pv = pu + (width * height) / 4;

	y = *py++;
	yy = y << 8;
	u = *pu - 128;
	ug =   88 * u;
	ub =  454 * u;
	v = *pv - 128;
	vg =  183 * v;
	vr =  359 * v;

	for (line = 0; line < height; line++) {
		for (col = 0; col < width; col++) {
			r = (yy +      vr) >> 8;
			g = (yy - ug - vg) >> 8;
			b = (yy + ub     ) >> 8;

			if (r < 0)   r = 0;
			if (r > 255) r = 255;
			if (g < 0)   g = 0;
			if (g > 255) g = 255;
			if (b < 0)   b = 0;
			if (b > 255) b = 255;

			switch(push) {
			case PUSH_RGB24:
				*dst++ = r;
				*dst++ = g;
				*dst++ = b;
				break;

			case PUSH_BGR24:
				*dst++ = b;
				*dst++ = g;
				*dst++ = r;
				break;
			
			case PUSH_RGB32:
				*dst++ = r;
				*dst++ = g;
				*dst++ = b;
				*dst++ = 0;
				break;

			case PUSH_BGR32:
				*dst++ = b;
				*dst++ = g;
				*dst++ = r;
				*dst++ = 0;
				break;
			}
			
			y = *py++;
			yy = y << 8;
			if (col & 1) {
				pu++;
				pv++;

				u = *pu - 128;
				ug =   88 * u;
				ub =  454 * u;
				v = *pv - 128;
				vg =  183 * v;
				vr =  359 * v;
			}
		} /* ..for col */
		if ((line & 1) == 0) { // even line: rewind
			pu -= linewidth;
			pv -= linewidth;
		}
	} /* ..for line */
}
void ccvt_420p_rgb24(int width, int height, const void *src, void *dst)
{
	ccvt_420p(width, height, (const unsigned char *)src, (unsigned char *)dst, PUSH_RGB24);
}

void ccvt_420p_bgr24(int width, int height, const void *src, void *dst)
{
	ccvt_420p(width, height, (const unsigned char *)src, (unsigned char *)dst, PUSH_BGR24);
}

#define NOT_IMPLEMENTED { \
	fprintf(stderr, "[%s] %s:%d : NOT IMPLEMENTED\n", __FILE__, __func__, __LINE__); \
}

/* 4:2:0 YUV interlaced to RGB/BGR */
void ccvt_420i_bgr24(int width, int height, void *src, void *dst) NOT_IMPLEMENTED
void ccvt_420i_rgb24(int width, int height, void *src, void *dst) NOT_IMPLEMENTED
void ccvt_420i_bgr32(int width, int height, void *src, void *dst) NOT_IMPLEMENTED
void ccvt_420i_rgb32(int width, int height, void *src, void *dst) NOT_IMPLEMENTED

/* 4:2:0 YUYV interlaced to RGB/BGR */
void ccvt_yuyv_rgb32(int width, int height, void *src, void *dst) {
//NOT_IMPLEMENTED
	// FAKE : use only Y
	int pos;
	unsigned char * src_8u = (unsigned char *) src;
	unsigned char * dst_8u = (unsigned char *) dst;
	for(pos = 0; pos < width*height; pos++) {
		memset(dst_8u+4*pos, src_8u[2*pos], 4);
	}
} 
void ccvt_yuyv_bgr32(int width, int height, void *src, void *dst) {
//NOT_IMPLEMENTED
	// FAKE : use only Y
	int pos;
	unsigned char * src_8u = (unsigned char *) src;
	unsigned char * dst_8u = (unsigned char *) dst;
	for(pos = 0; pos < width*height; pos++) {
		memset(dst_8u+4*pos, src_8u[2*pos], 4);
	}
} 

/* 4:2:0 YUV planar to RGB/BGR     */
void ccvt_420p_rgb32(int width, int height, void *srcy, void *srcu, void *srcv, void *dst) NOT_IMPLEMENTED
void ccvt_420p_bgr32(int width, int height, void *srcy, void *srcu, void *srcv, void *dst) {
	int i;
	for(i = 0; i<width*height; i++) {
		memset((unsigned char *)dst + 4*i, *((unsigned char *)srcy +i), 4);
	}
}

/* RGB/BGR to 4:2:0 YUV interlaced */

/* RGB/BGR to 4:2:0 YUV planar     */
void ccvt_rgb24_420p(int width, int height, void *src, void *dsty, void *dstu, void *dstv) NOT_IMPLEMENTED
void ccvt_bgr24_420p(int width, int height, void *src, void *dsty, void *dstu, void *dstv) NOT_IMPLEMENTED

/* Go from 420i to other yuv formats */
void ccvt_420i_420p(int width, int height, void *src, void *dsty, void *dstu, void *dstv) NOT_IMPLEMENTED
void ccvt_420i_yuyv(int width, int height, void *src, void *dst) NOT_IMPLEMENTED


/*

void ccvt_420p_rgb32(int width, int height, const void *src, void *dst)
{
	ccvt_420p(width, height, (const unsigned char *)src, (unsigned char *)dst, PUSH_RGB32);
}

void ccvt_420p_bgr32(int width, int height, const void *src, void *dst)
{
	ccvt_420p(width, height, (const unsigned char *)src, (unsigned char *)dst, PUSH_BGR32);
}



*/
