#ifndef CCVT_H
#define CCVT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Colour ConVerT: going from one colour space to another

   Format descriptions:
   420i = "4:2:0 interlaced"
           YYYY UU YYYY UU   even lines
           YYYY VV YYYY VV   odd lines
           U/V data is subsampled by 2 both in horizontal 
           and vertical directions, and intermixed with the Y values.
   
   420p = "4:2:0 planar"
           YYYYYYYY      N lines
           UUUU          N/2 lines
           VVVV          N/2 lines
           U/V is again subsampled, but all the Ys, Us and Vs are placed 
           together, but in separate buffers. The buffers may be placed in
           one piece of contiguous memory though, with Y buffer first,
           followed by U, folloed by V.

   yuyv = "4:2:2 interlaced"
           YUYV YUYV YUYV ...   N lines
           The U/V data is subsampled by 2 in horizontal direction only.

   bgr24 = 3 bytes per pixel, in the order Blue Green Red (whoever came up
           with that idea...)
   rgb24 = 3 bytes per pixel, in the order Red Green Blue (which is sensible)
   rgb32 = 4 bytes per pixel, in the order Red Green Blue Alpha, with 
           Alpha really being a filler byte (0)
   bgr32 = last but not least, 4 bytes per pixel, in the order Blue Green Red
           Alpha, Alpha again a filler byte (0)
 */

/* Functions in ccvt.S */
/* 4:2:0 YUV interlaced to RGB/BGR */
void ccvt_420i_bgr24(int width, int height, void *src, void *dst);
void ccvt_420i_rgb24(int width, int height, void *src, void *dst);
void ccvt_420i_bgr32(int width, int height, void *src, void *dst);
void ccvt_420i_rgb32(int width, int height, void *src, void *dst);

/* 4:2:0 YUYV interlaced to RGB/BGR */
void ccvt_yuyv_rgb32(int width, int height, void *src, void *dst);
void ccvt_yuyv_bgr32(int width, int height, void *src, void *dst);

/* 4:2:0 YUV planar to RGB/BGR     */
void ccvt_420p_rgb32(int width, int height, void *srcy, void *srcu, void *srcv, void *dst);
void ccvt_420p_bgr32(int width, int height, void *srcy, void *srcu, void *srcv, void *dst);

/* RGB/BGR to 4:2:0 YUV interlaced */

/* RGB/BGR to 4:2:0 YUV planar     */
void ccvt_rgb24_420p(int width, int height, void *src, void *dsty, void *dstu, void *dstv);
void ccvt_bgr24_420p(int width, int height, void *src, void *dsty, void *dstu, void *dstv);

/* Go from 420i to other yuv formats */
void ccvt_420i_420p(int width, int height, void *src, void *dsty, void *dstu, void *dstv);
void ccvt_420i_yuyv(int width, int height, void *src, void *dst);

#ifdef __cplusplus
}
#endif

#endif
