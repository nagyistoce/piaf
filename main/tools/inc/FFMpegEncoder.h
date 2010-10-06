/***************************************************************************
	 FFMpegEncoder.h  -  FFMPEG based video encoder for Piaf
							 -------------------
	begin                : Thu Sep 9 2010
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

#ifndef FFMPEGENCODER_H
#define FFMPEGENCODER_H

#define INT64_C	int64_t

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#undef exit

/* 5 seconds stream duration */
#define STREAM_DURATION   5.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_NB_FRAMES  ((int)(STREAM_DURATION * STREAM_FRAME_RATE))
#define STREAM_PIX_FMT PIX_FMT_YUV420P /* default pix_fmt */

static int sws_flags = SWS_BICUBIC;

/** @brief New encoder class using FFMPEG */
class FFMpegEncoder {
public:
	FFMpegEncoder(int imageWidth, int imageHeight, int frame_rate);

	/** @brief Start encoding in file */
	int startEncoder(char * output_file);
	/** @brief Set qualitu in 1..31 */
	int setQuality(int qscale);

	/** Destructor */
	~FFMpegEncoder() {
		stopEncoder();
	};

	/** @brief Stop encoding and close file */
	void stopEncoder();

	/// Encode a new frame (YUV420P format)
	int encodeFrameYUV420P(unsigned char *YUV420frame);
	/// Convert RGB24 frame to YUV420P then encode it
	int encodeFrameRGB24(unsigned char *RGB24frame);
	/// Convert RGB32 frame to YUV420P then encode it
	int encodeFrameRGB32(unsigned char *RGB32frame);

	/**  @brief add an audio output stream
	 */
	AVStream *add_audio_stream(AVFormatContext *oc, enum CodecID codec_id);


	/**  @brief open an audio output stream
	 */
	void open_audio(AVFormatContext *oc, AVStream *st);

	/** @brief prepare a 16 bit dummy audio frame of 'frame_size' samples and
	   'nb_channels' channels */
	void get_audio_frame(int16_t *samples, int frame_size, int nb_channels);


	void write_audio_frame(AVFormatContext *oc, AVStream *st);


	void close_audio(AVFormatContext *oc, AVStream *st);
private:
	/**************************************************************/
	/* audio output */

	float t, tincr, tincr2;
	int16_t *samples;
	uint8_t *audio_outbuf;
	int audio_outbuf_size;
	int audio_input_frame_size;

	/**************************************************************/
	/* video output */
	AVFrame *picture, *tmp_picture;
	uint8_t *video_outbuf;
	int frame_count, video_outbuf_size;

	/** @brief add a video output stream */
	AVStream *add_video_stream(AVFormatContext *oc, enum CodecID codec_id);
	AVFrame *alloc_picture(enum PixelFormat pix_fmt, int width, int height);

	void open_video(AVFormatContext *oc, AVStream *st);

	/* prepare a dummy image */
	void fill_yuv_image(AVFrame *pict, int frame_index, int width, int height);

	void write_video_frame(AVFormatContext *oc, AVStream *st);

	void close_video(AVFormatContext *oc, AVStream *st);


/**************************************************************/
/* media file output */

	char filename[1024];
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVStream *audio_st, *video_st;
	double audio_pts, video_pts;
	int m_quality;// MPEG Quality
	int m_width, m_height, m_fps;
	struct SwsContext *img_convert_ctx;
};

#endif // FFMPEGENCODER_H
