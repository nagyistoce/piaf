/***************************************************************************
						  FileVideoAcquisition.cpp  -  description
							 -------------------
	begin                : Tue Mar 12 2002
	copyright            : (C) 2002 by Olivier Viné & Christophe Seyve
	email                : olivier.vine@sisell.com  & cseyve@free.fr
	description 		 : class in charge of video acquisition
							(init, start and stop, get properties, acquire, ...)
 ***************************************************************************/
#include <math.h>

#include "FileVideoAcquisition.h"
#include "swvideodetector.h"
#include "piaf-common.h"

extern "C" {
#include <swscale.h>
}

#include <errno.h>

#if LIBAVCODEC_VERSION_INT >=  ((51<<16)+(50<<8)+0)
#define URLPB(pb) (pb)
#else
#define URLPB(pb) &(pb)
#endif



FileVideoAcquisition::FileVideoAcquisition()
{
	m_bAvcodecIsInitialized = FALSE;
	// no device is wanted, try default device
	tBoxSize newSize;newSize.x=newSize.y=0;
	newSize.width = 640;
	newSize.height = 480;
	initPlayer();
	openDevice(NULL, newSize);
}

FileVideoAcquisition::FileVideoAcquisition(const char *device)
{
	m_bAvcodecIsInitialized = FALSE;
	// no device is wanted, try default device
	tBoxSize newSize;
	newSize.x = newSize.y = 0;
	newSize.width = 640; // Fake size, will be overwritten by movie file image size
	newSize.height = 480;

	initPlayer();
	openDevice(device, newSize);
}

FileVideoAcquisition::~FileVideoAcquisition() {
	purge();
}

long FileVideoAcquisition::getFrameIndex() {
	return (long)playFrame;
}

unsigned long long FileVideoAcquisition::getAbsolutePosition() {
	if(!m_pFormatCtx)
		return 0;

	return url_ftell(URLPB(m_pFormatCtx->pb));
}

void FileVideoAcquisition::purge() {
	//fprintf(stderr, "FileVideoAcquisition::%s:%d this=%p...\n", __func__, __LINE__, this); fflush(stderr);
	VAcloseVD();
	//fprintf(stderr, "FileVideoAcquisition::%s:%d this=%p...\n", __func__, __LINE__, this); fflush(stderr);

	// Free the RGB image
	av_free(m_pFrameRGB);
	//fprintf(stderr, "FileVideoAcquisition::%s:%d this=%p...\n", __func__, __LINE__, this); fflush(stderr);
	//fprintf(stderr, "FileVideoAcquisition::%s:%d this=%p...\n", __func__, __LINE__, this); fflush(stderr);

	// Free the YUV frame
	//fprintf(stderr, "FileVideoAcquisition::%s:%d this=%p...\n", __func__, __LINE__, this); fflush(stderr);
	av_free(m_pFrame);

	//fprintf(stderr, "FileVideoAcquisition::%s:%d this=%p...\n", __func__, __LINE__, this); fflush(stderr);
	CPP_DELETE_ARRAY(receptBuffer)

	//fprintf(stderr, "FileVideoAcquisition::%s:%d this=%p...\n", __func__, __LINE__, this); fflush(stderr);
	//CPP_DELETE_ARRAY(m_inbuff)
	CPP_DELETE_ARRAY(m_noPitchBuffer);
}


// START COPY FROM VideoPlayerTool.cpp
void FileVideoAcquisition::initPlayer() {
	initVirtualDevice(); // init inherited functions and vars

	m_videoFileName[0] = '\0';
	m_videoFileName[127] = '\0';
	m_pFormatCtx 	= NULL;
	m_pCodecCtx		= NULL;
	m_pCodec		= NULL;
	m_pFrame		= NULL;
	m_pFrameRGB		= NULL;
	m_origbuff = m_inbuff		= NULL;
	receptBuffer	= NULL;
	m_noPitchBuffer = NULL;

	m_isJpeg = false;
	playFrame = 0;
	m_prevPosition = 0;
	m_fileSize = 0;
	myVD_palette = -1;
	m_videoStream = -1;

	// Initialise with default value
	// (will be computed later with fps value)
	timeref_sec = 0;
	timeref_usec = 0;
	timestep_usec = 40000;
	timestep_sec = 0;

	myAcqIsInitialised = false;
	myVDIsInitialised = false;

	if(!m_bAvcodecIsInitialized)
	{
		// Register all formats and codecs
		av_register_all();

		m_bAvcodecIsInitialized = true;
	}
}

int FileVideoAcquisition::openDevice(const char * aDevice, tBoxSize )
{
	fprintf(stderr, "FileVA::%s:%d : OPENING VIDEO FILE '%s'............\n",
			__func__, __LINE__, aDevice);

	memset(&m_video_properties, 0, sizeof(t_video_properties));
	if(!aDevice) {
		return -1;
	}

	if(m_inbuff)
	{
		fprintf(stderr, "FileVA::%s:%d : purging old file...\n", __func__, __LINE__);

		purge();
		m_inbuff = NULL;
	}
	float lfps = 25.f;

   // Open video file
	if(av_open_input_file(&m_pFormatCtx, aDevice, NULL, 0, NULL)!=0) {
		fprintf(stderr, "FileVA::%s:%d : av_open_input_file failed ! cannot open file '%s'\n", __func__, __LINE__, aDevice);
		return -1; // Couldn't open file
	}

	// Retrieve stream information
	if(av_find_stream_info(m_pFormatCtx)<0)
	{
		fprintf(stderr, "FileVA::%s:%d : cannot find stream information for file '%s'\n", __func__, __LINE__, aDevice);
		return -1; // Couldn't find stream information
	}
	m_isJpeg = false;

	// Set no stream to the first video stream
	m_videoStream=-1;
	for(unsigned int i=0; i<m_pFormatCtx->nb_streams; i++)
	{
		if(m_pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO )
		{
			fprintf(stderr, "FileVA::%s:%d : choosed stream #%d for file '%s'\n",
					__func__, __LINE__,
					i, aDevice);

			m_videoStream = i;
			break;
		}
	}

	if(m_videoStream==-1) {
		fprintf(stderr, "FileVA::%s:%d : cannot find video stream in file '%s'\n",
			__func__, __LINE__, aDevice);
		return -1; // Didn't find a video stream
	}

	// Get a pointer to the codec context for the video stream
	m_pCodecCtx = m_pFormatCtx->streams[m_videoStream]->codec;
	if(m_pCodecCtx->codec_id == CODEC_ID_MJPEG) {
		m_isJpeg = true; }

	// Find the decoder for the video stream
	m_pCodec = avcodec_find_decoder(m_pCodecCtx->codec_id);
	if(m_pCodec==NULL) {
		fprintf(stderr, "FileVA::%s:%d : cannot find codec '%d'\n", __func__, __LINE__,
				(int)m_pCodecCtx->codec_id);
		return -1; // Codec not found
	}

	// Inform the codec that we can handle truncated bitstreams -- i.e.,
	// bitstreams where frame boundaries can fall in the middle of packets
	//if(pCodec->capabilities & CODEC_CAP_TRUNCATED)
	//    pCodecCtx->flags|=CODEC_FLAG_TRUNCATED;

	// Open codec
	if(avcodec_open(m_pCodecCtx, m_pCodec)<0) {
		fprintf(stderr, "FileVA::%s:%d : cannot open codec\n", __func__, __LINE__ );
		return -1; // Could not open codec
	}

	// Hack to correct wrong frame rates that seem to be generated by some
	// codecs
	/*
	if(m_pCodecCtx->frame_rate>1000 && m_pCodecCtx->frame_rate_base==1)
		m_pCodecCtx->frame_rate_base=1000;
	lfps = (float)m_pCodecCtx->frame_rate/m_pCodecCtx->frame_rate_base;
	*/
	/* rq OLV : code extrait de dump_format (utils.c) */
	if(m_pFormatCtx->streams[m_videoStream]->r_frame_rate.den
		&& m_pFormatCtx->streams[m_videoStream]->r_frame_rate.num) {
		lfps = (float)av_q2d(m_pFormatCtx->streams[m_videoStream]->r_frame_rate);

		fprintf(stderr, "FileVA::%s:%d : read fps = %f =(%d/%d)\n", __func__,__LINE__,
			lfps,
			(int)m_pFormatCtx->streams[m_videoStream]->r_frame_rate.num,
			(int)m_pFormatCtx->streams[m_videoStream]->r_frame_rate.den
			);
	}
	else {
		lfps = (float)(1/av_q2d(m_pCodecCtx->time_base));

		fprintf(stderr, "FileVA::%s:%d : read fps = %f\n", __func__,__LINE__,
			lfps);
	}
	if(lfps > 30) {
		fprintf(stderr, "FileVA::%s:%d : read fps = %f => 25 fps\n", __func__,__LINE__,
			lfps);
		lfps = 25;
	}

	if(lfps<1)
	{
		timestep_usec = 0;
		timestep_sec = 1/(long)floor(lfps);
	}
	else
	{
		timestep_usec = 1000000/(long)floor(lfps);
		timestep_sec = 0;
	}

	// Opening Video File is OK : declaring Video Device Initialised
	strncpy(m_videoFileName, aDevice, 128);
	myVDIsInitialised = true;

	// Allocate video frame
	m_pFrame = avcodec_alloc_frame();
	if(m_pFrame==NULL) {
		fprintf(stderr, "FileVA::%s:%d : cannot allocate frame\n", __func__, __LINE__);
		return -1;
	}
	// Allocate an AVFrame structure
	m_pFrameRGB = avcodec_alloc_frame();
	if(m_pFrameRGB==NULL){
		fprintf(stderr, "FileVA::%s:%d : cannot allocate frame in RGB space\n", __func__, __LINE__);
		return -1;
	}

	// Determine required buffer size and allocate buffer
	int numBytes = avpicture_get_size(m_pCodecCtx->pix_fmt, m_pCodecCtx->width,
		m_pCodecCtx->height);
	mImageSize = cvSize(m_pCodecCtx->width,
						m_pCodecCtx->height);
	receptBuffer = new uint8_t[numBytes];

	fprintf(stderr,"FileVA::%s:%d\tSize is %d x %d @ %d fps => buffer [ %d ]\n", __func__, __LINE__,
		mImageSize.width, mImageSize.height, (int)lfps, numBytes);

	m_period_ms = 40; // 40 ms at 25 fps
	m_fps = 25;
	if(lfps != 0) {
		m_period_ms = (int)roundf(1000.f / (float)lfps);
		m_fps = lfps;
	}

	// Assign appropriate parts of buffer to image planes in pFrame
	// rq. OLV : cette configuration doit remplacer les appels à calcBufferSize, display_frame, etc.
	//avpicture_fill((AVPicture *)m_pFrame, receptBuffer, mImageSize.pix_fmt,
	//    mImageSize.width, mImageSize.height);
	int numBytes_orig = avpicture_get_size(m_pCodecCtx->pix_fmt, m_pCodecCtx->width,
		m_pCodecCtx->height);

	m_origbuff = new uint8_t [ numBytes_orig ];
	avpicture_fill((AVPicture *)m_pFrame, m_origbuff, m_pCodecCtx->pix_fmt,
		m_pCodecCtx->width, m_pCodecCtx->height);
	/*fprintf(stderr, "FileVA::%s:%d : avpicture_fill %d x %d m_pFrame=%p => m_inbuff [ %d ] in RGBA32....\n",
			__func__, __LINE__,
			m_pCodecCtx->width,
			m_pCodecCtx->height,
			m_pFrame,
			numBytes_orig); fflush(stderr);*/

	// Determine required buffer size and allocate buffer
	numBytes = avpicture_get_size(PIX_FMT_RGBA32, m_pCodecCtx->width,
								  m_pCodecCtx->height);
	fprintf(stderr, "FileVA::%s:%d : allocating %d x %d => m_inbuff [ %d ] in RGBA32....\n",
			__func__, __LINE__,
			mImageSize.width,
			mImageSize.height,
			numBytes);

	m_inbuff = new uint8_t [ numBytes ];
	if(!m_inbuff) {
		fprintf(stderr, "FileVA::%s:%d : cannot allocate m_inbuff [ %d ]\n",
			__func__, __LINE__, numBytes);

		EXIT_ON_ERROR
	}
	/*fprintf(stderr, "FileVA::%s:%d : avpicture_fill %d x %d m_pFrame=%p m_pFrameRGB=%p => m_inbuff [ %d ] in RGBA32....\n",
			__func__, __LINE__,
			mImageSize.width,
			mImageSize.height,
			m_pFrame,
			m_pFrameRGB,
			numBytes); fflush(stderr);*/

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	avpicture_fill((AVPicture *)m_pFrameRGB, m_inbuff, PIX_FMT_RGBA32,
		mImageSize.width, mImageSize.height);
	myAcqIsInitialised = true;
	m_fileSize = url_fsize(URLPB(m_pFormatCtx->pb));

	fprintf(stderr, "FileVA::%s:%d : file size : %llu / img=%dx%d\n",
			__func__, __LINE__, m_fileSize, mImageSize.width, mImageSize.height);

	slotRewindMovie();
	return 1;
}


int FileVideoAcquisition::VAcloseVD()
{
	//fprintf(stderr, "FileVA::%s:%d : closing context\n", __func__, __LINE__);
	// Close the codec
	if(m_pCodecCtx)
	{
		fprintf(stderr, "FileVA::%s:%d : closing context\n", __func__, __LINE__);
		avcodec_close(m_pCodecCtx);
		m_pCodecCtx = NULL;
	}
	// Close the video file
	if(m_pFormatCtx)
	{
		av_close_input_file(m_pFormatCtx);
		m_pFormatCtx = NULL;
	}

	return 1;
}

bool FileVideoAcquisition::VDIsInitialised()
{
	return myVDIsInitialised;
}

bool FileVideoAcquisition::AcqIsInitialised()
{
	return myAcqIsInitialised;
}

CvSize FileVideoAcquisition::getImageSize()
{
#if 0
	fprintf(stderr, "[FileVA]::getImageSize : %lu x %lu\n",
		mImageSize.width, mImageSize.height);
#endif

	return mImageSize;
}

int FileVideoAcquisition::setChannel(int ch)
{
	if(!m_pFormatCtx)
	{
		return -1;
	}
	if((ch < (int)m_pFormatCtx->nb_streams) &&
		(m_pFormatCtx->streams[ch]->codec->codec_type==CODEC_TYPE_VIDEO)
		) {
		m_videoStream = ch;
	}
	return 1;
}

void FileVideoAcquisition::rewindMovie()
{
	if(!myAcqIsInitialised)
	{
		fprintf(stderr, "FileVA::%s:%d : not initialised : creating new one\n", __func__, __LINE__);
		purge();

		initPlayer();
		tBoxSize newSize; newSize.x=newSize.y=0;
		newSize.width = 640;
		newSize.height = 480;
		openDevice(m_videoFileName, newSize);
	}

	if(!myAcqIsInitialised)
	{
		fprintf(stderr, "[FilaVA]::%s:%d rewind File '%s' not found\n",
			__func__, __LINE__,
			m_videoFileName);
		return;
	}

	// rewind file
	if(m_pFormatCtx)
	{
		m_prevPosition = 0;
		av_seek_frame(m_pFormatCtx, m_videoStream, 0, 0);
		playFrame = 0;
		m_movie_pos.prevAbsPosition = 0;
		m_movie_pos.prevKeyFramePosition = 0;
		m_movie_pos.nbFramesSinceKeyFrame = 0;
	}

	// read first frame
	long theSize;
	readImageBuffer(&theSize);
}

t_video_properties FileVideoAcquisition::updateVideoProperties()
{
	m_video_properties.fps = m_fps;
	m_video_properties.frame_width = mImageSize.width;
	m_video_properties.frame_height = mImageSize.height;

	m_video_properties.frame_count = mPlayFrame;
	return m_video_properties;
}

