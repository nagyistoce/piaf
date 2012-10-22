/***************************************************************************
	 virtualdeviceacquisition.cpp  - Virtual acquisition class for video capture
							 -------------------
	begin                : Thu Dec 2 2010
	copyright            : (C) 2010 by Christophe Seyve (CSE)
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

#include <stdio.h>

#include "virtualdeviceacquisition.h"
#include "piaf-common.h"

void printVideoProperties(t_video_properties * props)
{
	fprintf(stderr, "Props = { ");
	fprintf(stderr, "pos_msec=%g, ", props->pos_msec);
	fprintf(stderr, "pos_frames=%g, ", props->pos_frames);
	fprintf(stderr, "pos_avi_ratio=%g, ", props->pos_avi_ratio);
	fprintf(stderr, "frame_width=%d x ", props->frame_width);
	fprintf(stderr, "frame_height=%d, ", props->frame_height);
	fprintf(stderr, "fps=%g, ", props->fps);
	fprintf(stderr, "fourcc_dble=%g, ", props->fourcc_dble);
	fprintf(stderr, "fourcc='%s', ", props->fourcc);
	fprintf(stderr, "norm='%s', ", props->norm);
	fprintf(stderr, "frame_count=%g, ", props->frame_count);
	fprintf(stderr, "format=%g, ", props->format);
	fprintf(stderr, "mode=%g, ", props->mode);
	fprintf(stderr, "brightness=%d, ", props->brightness);
	fprintf(stderr, "contrast=%d, ", props->contrast);
	fprintf(stderr, "saturation=%d, ", props->saturation);
	fprintf(stderr, "hue=%d, ", props->hue);
	fprintf(stderr, "gain=%d, ", props->gain);
	fprintf(stderr, "exposure=%g, ", props->exposure);
	fprintf(stderr, "convert_rgb=%g, ", props->convert_rgb);
	fprintf(stderr, "white_balance=%d, ", props->white_balance);

	fprintf(stderr, " }\n");
}









static u32 * grayToBGR32 = NULL;
static void init_grayToBGR32()
{
	if(grayToBGR32) {
		return;
	}

	grayToBGR32 = new u32 [256];
	for(int c = 0; c<256; c++) {
		int Y = c;
		u32 B = Y;// FIXME
		u32 G = Y;
		u32 R = Y;
		grayToBGR32[c] = (R << 16) | (G<<8) | (B<<0);
	}

}



QImage iplImageToQImage(IplImage * iplImage, bool swap_RB)
{
	if(!iplImage) {
		PIAF_MSG(SWLOG_ERROR, "IplImage is null");
		return QImage();
	}

	int depth = iplImage->nChannels;

	bool rgb24_to_bgr32 = false;
	if(depth == 3  ) {// RGB24 is obsolete on Qt => use 32bit instead
		depth = 4;
		rgb24_to_bgr32 = true;
	}

	u32 * grayToBGR32palette = grayToBGR32;
	bool gray_to_bgr32 = false;


	if(depth == 1) {// GRAY is obsolete on Qt => use 32bit instead
		depth = 4;
		gray_to_bgr32 = true;

		init_grayToBGR32();

		grayToBGR32palette = grayToBGR32;
	}

	int orig_width = iplImage->width;
	int orig_height = iplImage->height;

	QImage qImage(orig_width, orig_height,
				   depth > 1 ? QImage::Format_RGB32 : QImage::Format_Indexed8);
	memset(qImage.bits(), 127, orig_width*orig_height*depth);

	switch(iplImage->depth) {
	default:
		fprintf(stderr, "imageinfowidget %s:%d : Unsupported depth = %d\n", __func__, __LINE__, iplImage->depth);
		break;

	case IPL_DEPTH_8U: {
		if(!rgb24_to_bgr32 && !gray_to_bgr32) {
			if(iplImage->nChannels != 4) {
				//
				if(!swap_RB)
				{
					for(int r=0; r<iplImage->height; r++) {
						// NO need to swap R<->B
						memcpy(qImage.bits() + r*orig_width*depth,
							iplImage->imageData + r*iplImage->widthStep,
							orig_width*depth);
					}
				} else {
					for(int r=0; r<iplImage->height; r++) {
						// need to swap R<->B
						u8 * buf_out = (u8 *)(qImage.bits()) + r*orig_width*depth;
						u8 * buf_in = (u8 *)(iplImage->imageData) + r*iplImage->widthStep;
						memcpy(qImage.bits() + r*orig_width*depth,
							iplImage->imageData + r*iplImage->widthStep,
							orig_width*depth);

						for(int pos3 = 0 ; pos3<orig_width*depth; pos3+=depth,
							buf_out+=3, buf_in+=depth
							 ) {
							buf_out[2] = buf_in[0];
							buf_out[0] = buf_in[2];
						}
					}
				}
			} else {

				for(int r=0; r<iplImage->height; r++) {
					// need to swap R<->B
					u8 * buf_out = (u8 *)(qImage.bits()) + r*orig_width*depth;
					u8 * buf_in = (u8 *)(iplImage->imageData) + r*iplImage->widthStep;
					memcpy(qImage.bits() + r*orig_width*depth,
						iplImage->imageData + r*iplImage->widthStep,
						orig_width*depth);

					if(swap_RB) {
						for(int pos4 = 0 ; pos4<orig_width*depth; pos4+=depth,
							buf_out+=4, buf_in+=depth
							 ) {
							buf_out[2] = buf_in[0];
							buf_out[0] = buf_in[2];
						}
					}
				}
			}
		}
		else if(rgb24_to_bgr32) {
			// RGB24 to BGR32
			u8 * buffer3 = (u8 *)iplImage->imageData;
			u8 * buffer4 = (u8 *)qImage.bits();
			int orig_width4 = 4 * orig_width;

			for(int r=0; r<iplImage->height; r++)
			{
				int pos3 = r * iplImage->widthStep;
				int pos4 = r * orig_width4;
				if(!swap_RB) {

					for(int c=0; c<orig_width; c++, pos3+=3, pos4+=4)
					{
						buffer4[pos4   ] = buffer3[pos3];
						buffer4[pos4 + 1] = buffer3[pos3+1];
						buffer4[pos4 + 2] = buffer3[pos3+2];
					}
				} else { // SWAP R<->B
					for(int c=0; c<orig_width; c++, pos3+=3, pos4+=4)
					{
						buffer4[pos4 + 2] = buffer3[pos3];
						buffer4[pos4 + 1] = buffer3[pos3+1];
						buffer4[pos4    ] = buffer3[pos3+2];
					}
				}
			}
		} else if(gray_to_bgr32) {
			for(int r=0; r<iplImage->height; r++)
			{
				u32 * buffer4 = (u32 *)qImage.bits() + r*qImage.width();
				u8 * bufferY = (u8 *)(iplImage->imageData + r*iplImage->widthStep);
				for(int c=0; c<orig_width; c++) {
					buffer4[c] = grayToBGR32palette[ (int)bufferY[c] ];
				}
			}
		}
		}break;
	case IPL_DEPTH_16S: {
		if(!rgb24_to_bgr32) {

			u8 * buffer4 = (u8 *)qImage.bits();
			short valmax = 0;

			for(int r=0; r<iplImage->height; r++)
			{
				short * buffershort = (short *)(iplImage->imageData + r*iplImage->widthStep);
				for(int c=0; c<iplImage->width; c++)
					if(buffershort[c]>valmax)
						valmax = buffershort[c];
			}

			if(valmax>0)
				for(int r=0; r<iplImage->height; r++)
				{
					short * buffer3 = (short *)(iplImage->imageData
									+ r * iplImage->widthStep);
					int pos3 = 0;
					int pos4 = r * orig_width;
					for(int c=0; c<orig_width; c++, pos3++, pos4++)
					{
						int val = abs((int)buffer3[pos3]) * 255 / valmax;
						if(val > 255) val = 255;
						buffer4[pos4] = (u8)val;
					}
				}
		}
		else {
			u8 * buffer4 = (u8 *)qImage.bits();
			if(depth == 3) {

				for(int r=0; r<iplImage->height; r++)
				{
					short * buffer3 = (short *)(iplImage->imageData + r * iplImage->widthStep);
					int pos3 = 0;
					int pos4 = r * orig_width*4;
					for(int c=0; c<orig_width; c++, pos3+=3, pos4+=4)
					{
						buffer4[pos4   ] = buffer3[pos3];
						buffer4[pos4 + 1] = buffer3[pos3+1];
						buffer4[pos4 + 2] = buffer3[pos3+2];
					}
				}
			} else if(depth == 1) {
				short valmax = 0;
				short * buffershort = (short *)(iplImage->imageData);
				for(int pos=0; pos< iplImage->widthStep*iplImage->height; pos++)
					if(buffershort[pos]>valmax)
						valmax = buffershort[pos];

				if(valmax>0) {
					for(int r=0; r<iplImage->height; r++)
					{
						short * buffer3 = (short *)(iplImage->imageData
											+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = (u8)val;
						}
					}
				}
			}
		}
		}break;
	case IPL_DEPTH_16U: {

		if(!rgb24_to_bgr32) {

			unsigned short valmax = 0;

			for(int r=0; r<iplImage->height; r++)
			{
				unsigned short * buffershort = (unsigned short *)(iplImage->imageData + r*iplImage->widthStep);
				for(int c=0; c<iplImage->width; c++) {
					if(buffershort[c]>valmax) {
						valmax = buffershort[c];
					}
				}
			}

			if(valmax>0) {
				if(!gray_to_bgr32) {
					u8 * buffer4 = (u8 *)qImage.bits();
					for(int r=0; r<iplImage->height; r++)
					{
						unsigned short * buffer3 = (unsigned short *)(iplImage->imageData
										+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = (u8)val;
						}
					}
				} else {
					u32 * buffer4 = (u32 *)qImage.bits();
					for(int r=0; r<iplImage->height; r++)
					{
						unsigned short * buffer3 = (unsigned short *)(iplImage->imageData
										+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = grayToBGR32palette[ val ];
						}
					}
				}
			}
		}
		else {
			fprintf(stderr, "imageinfowidget %s:%d : U16  depth = %d -> BGR32\n", __func__, __LINE__, iplImage->depth);
			u8 * buffer4 = (u8 *)qImage.bits();
			if(depth == 3) {

				for(int r=0; r<iplImage->height; r++)
				{
					short * buffer3 = (short *)(iplImage->imageData + r * iplImage->widthStep);
					int pos3 = 0;
					int pos4 = r * orig_width*4;
					for(int c=0; c<orig_width; c++, pos3+=3, pos4+=4)
					{
						buffer4[pos4   ] = buffer3[pos3]/256;
						buffer4[pos4 + 1] = buffer3[pos3+1]/256;
						buffer4[pos4 + 2] = buffer3[pos3+2]/256;
					}
				}
			} else if(depth == 1) {
				short valmax = 0;
				short * buffershort = (short *)(iplImage->imageData);
				for(int pos=0; pos< iplImage->widthStep*iplImage->height; pos++)
					if(buffershort[pos]>valmax)
						valmax = buffershort[pos];

				if(valmax>0)
					for(int r=0; r<iplImage->height; r++)
					{
						short * buffer3 = (short *)(iplImage->imageData
											+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = (u8)val;
						}
					}
			}
		}
		}break;
	case IPL_DEPTH_32F: {
		// create temp image
		IplImage * image8bit = swCreateImage(cvGetSize(iplImage),
											 IPL_DEPTH_8U, iplImage->nChannels);
		double minVal=0. , maxVal = 255.;
		CvPoint maxPt;
		#ifdef OPENCV_22
		try
		#endif
		{
			cvMinMaxLoc(iplImage, &minVal, &maxVal, &maxPt);
		}
		#ifdef OPENCV_22
		catch(cv::Exception e)
		{
			maxVal = 255.;
		}
		#endif
		// Get dynamic of image
		int limit = 1;
		while (limit < maxVal)
		{
			limit = limit << 1;
		}
		// convert scale
		cvConvertScale(iplImage, image8bit, 255./(double)limit, 0.);

		// convert
		qImage = iplImageToQImage(image8bit).copy();
		swReleaseImage(&image8bit);

		}break;
	}

	if(qImage.depth() == 8) {
		qImage.setNumColors(256);

		for(int c=0; c<256; c++) {
			qImage.setColor(c, qRgb(c,c,c));
		}
	}

	return qImage;
}
