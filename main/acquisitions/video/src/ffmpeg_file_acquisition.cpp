/***************************************************************************
						  FFmpegFileVideoAcquisition.cpp  -  description
							 -------------------
	begin                : Tue Mar 12 2002
	copyright            : (C) 2002 by Olivier Viné & Christophe Seyve
	email                : olivier.vine@sisell.com  & cseyve@free.fr
	description 		 : class in charge of video acquisition
							(init, start and stop, get properties, acquire, ...)
 ***************************************************************************/
#include <math.h>
#include <sys/stat.h>


#include "file_video_acquisition_factory.h"

#include "ffmpeg_file_acquisition.h"

#include "swvideodetector.h"
#include "piaf-common.h"

#ifdef HAS_SWSCALE
extern "C" {
#include <swscale.h>
}
#endif // HAS_SWSCALE

#include <errno.h>

#define CALC_FFMPEG_VERSION(a,b,c) ( a<<16 | b<<8 | c )

#if LIBAVCODEC_VERSION_INT >=  ((51<<16)+(50<<8)+0)
#define URLPB(pb) (pb)
#else
#define URLPB(pb) &(pb)
#endif

#if LIBAVFORMAT_BUILD < CALC_FFMPEG_VERSION(53, 2, 0)
#define AVMEDIA_TYPE_VIDEO CODEC_TYPE_VIDEO
#endif


#define EXTENSION_FFMPEG	"avi,wmv,mov,mpg,mp4"
//#define EXTENSION_FFMPEG	"avi,wmv,mov,mpg"

std::string FFmpegFileVideoAcquisition::mRegistered =
	FileVideoAcquisitionFactory::RegisterCreatorFunction(
		(EXTENSION_FFMPEG), FFmpegFileVideoAcquisition::creatorFunction);


FileVideoAcquisition* FFmpegFileVideoAcquisition::creatorFunction(std::string path)
{
	PIAF_MSG(SWLOG_INFO, "Create FFmpegFileVideoAcquisition(path='%s')", path.c_str() );
	return (FileVideoAcquisition *)new FFmpegFileVideoAcquisition( path.c_str() );
}




FFmpegFileVideoAcquisition::FFmpegFileVideoAcquisition()
{
	m_bAvcodecIsInitialized = FALSE;

	// no device is wanted, try default device
	tBoxSize newSize;newSize.x=newSize.y=0;
	newSize.width = 640;
	newSize.height = 480;

	initPlayer();

	openDevice(NULL, newSize);
}

FFmpegFileVideoAcquisition::FFmpegFileVideoAcquisition(const char *device)
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

FFmpegFileVideoAcquisition::~FFmpegFileVideoAcquisition() {
	purge();
}

long FFmpegFileVideoAcquisition::getFrameIndex() {
	return (long)mPlayFrame;
}

unsigned long long FFmpegFileVideoAcquisition::getAbsolutePosition() {
	if(!m_pFormatCtx) {
		return 0;
	}

	return url_ftell(URLPB(m_pFormatCtx->pb));
}

void FFmpegFileVideoAcquisition::purge() {
	//fprintf(stderr, "FFmpegFileVideoAcquisition::%s:%d this=%p...\n", __func__, __LINE__, this); fflush(stderr);
	VAcloseVD();
	//fprintf(stderr, "FFmpegFileVideoAcquisition::%s:%d this=%p...\n", __func__, __LINE__, this); fflush(stderr);

	// Free the RGB image
	av_free(m_pFrameRGB);
	//fprintf(stderr, "FFmpegFileVideoAcquisition::%s:%d this=%p...\n", __func__, __LINE__, this); fflush(stderr);
	//fprintf(stderr, "FFmpegFileVideoAcquisition::%s:%d this=%p...\n", __func__, __LINE__, this); fflush(stderr);

	// Free the YUV frame
	//fprintf(stderr, "FFmpegFileVideoAcquisition::%s:%d this=%p...\n", __func__, __LINE__, this); fflush(stderr);
	av_free(m_pFrame);

	//fprintf(stderr, "FFmpegFileVideoAcquisition::%s:%d this=%p...\n", __func__, __LINE__, this); fflush(stderr);
	CPP_DELETE_ARRAY(receptBuffer)

	//fprintf(stderr, "FFmpegFileVideoAcquisition::%s:%d this=%p...\n", __func__, __LINE__, this); fflush(stderr);
	//CPP_DELETE_ARRAY(m_inbuff)
	CPP_DELETE_ARRAY(m_noPitchBuffer);
}

IplImage * FFmpegFileVideoAcquisition::readImageRaw()
{
	fprintf(stderr, "%s %s:%d : NOT IMPLEMENTED\n", __FILE__, __func__, __LINE__);
	return NULL; /// \todo FIXME : return raw value
}

// START COPY FROM VideoPlayerTool.cpp
void FFmpegFileVideoAcquisition::initPlayer() {
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
	mPlayFrame = 0;
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

	clearImageInfoStruct(&mImageInfo);

	if(!m_bAvcodecIsInitialized)
	{
		// Register all formats and codecs
		av_register_all();

		m_bAvcodecIsInitialized = true;
	}
}

int FFmpegFileVideoAcquisition::openDevice(const char * aDevice, tBoxSize )
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

	struct stat bufstat;
	if(stat(aDevice, &bufstat) == 0)
	{
		mImageInfo.Date = bufstat.st_ctime;
	}
	else
	{
		mImageInfo.Date = mImageInfo.Tick = 0;
	}

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
		if(m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
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
		lfps = (float)(1.f / av_q2d(m_pCodecCtx->time_base));

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
	int numBytes_orig = avpicture_get_size(m_pCodecCtx->pix_fmt,
										   m_pCodecCtx->width, m_pCodecCtx->height);

	m_origbuff = new uint8_t [ numBytes_orig ];
	avpicture_fill((AVPicture *)m_pFrame,
				   m_origbuff,
				   m_pCodecCtx->pix_fmt,
				   m_pCodecCtx->width, m_pCodecCtx->height);
	/*fprintf(stderr, "FileVA::%s:%d : avpicture_fill %d x %d m_pFrame=%p => m_inbuff [ %d ] in RGBA32....\n",
			__func__, __LINE__,
			m_pCodecCtx->width,
			m_pCodecCtx->height,
			m_pFrame,
			numBytes_orig); fflush(stderr);*/

	// Determine required buffer size and allocate buffer
	numBytes = avpicture_get_size(PIX_FMT_RGBA32,
								  m_pCodecCtx->width,
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

	rewindMovie();
	return 1;
}


int FFmpegFileVideoAcquisition::VAcloseVD()
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

bool FFmpegFileVideoAcquisition::VDIsInitialised()
{
	return myVDIsInitialised;
}

bool FFmpegFileVideoAcquisition::AcqIsInitialised()
{
	return myAcqIsInitialised;
}

CvSize FFmpegFileVideoAcquisition::getImageSize()
{
#if 0
	fprintf(stderr, "[FileVA]::getImageSize : %lu x %lu\n",
		mImageSize.width, mImageSize.height);
#endif

	return mImageSize;
}

int FFmpegFileVideoAcquisition::setChannel(int ch)
{
	if(!m_pFormatCtx)
	{
		return -1;
	}
	if((ch < (int)m_pFormatCtx->nb_streams) &&
		(m_pFormatCtx->streams[ch]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		) {
		m_videoStream = ch;
	}
	return 1;
}

void FFmpegFileVideoAcquisition::rewindMovie()
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
		mPlayFrame = 0;
		m_movie_pos.prevAbsPosition = 0;
		m_movie_pos.prevKeyFramePosition = 0;
		m_movie_pos.nbFramesSinceKeyFrame = 0;
	}

	// read first frame
	long theSize;
	readImageBuffer(&theSize);
}

t_video_properties FFmpegFileVideoAcquisition::updateVideoProperties()
{
	m_video_properties.fps = m_fps;
	m_video_properties.frame_width = mImageSize.width;
	m_video_properties.frame_height = mImageSize.height;

	m_video_properties.frame_count = mPlayFrame;
	return m_video_properties;
}

void FFmpegFileVideoAcquisition::setAbsoluteFrame(int frame)
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
		m_prevPosition = frame;
		av_seek_frame(m_pFormatCtx, m_videoStream, 0, 0);
		mPlayFrame = frame;
	}

	// read first frame
	long theSize;

	readImageBuffer(&theSize);
}


void FFmpegFileVideoAcquisition::setAbsolutePosition(unsigned long long newpos)
{
	fprintf(stderr, "\tFileVA::%s:%d : newpos = %llu\n", __func__, __LINE__, newpos);
	rewindToPosMovie(newpos);
}

#define JUMP_BUFFERSIZE	500000
#define DEFAULT_FRAME_SIZE	10000
void FFmpegFileVideoAcquisition::rewindToPosMovie(unsigned long long l_position)
{
	if(!m_pFormatCtx) {
		DEBUG_ALL("m_pFormatCtx not allocated")
		return;
	}

	if(l_position <= 0)
	{
		DEBUG_ALL("position <= 0 => rewindMovie")

		rewindMovie();
		return;
	}

	fprintf(stderr,"FileVA::%s:%d Jump in file position=%llu\n",
				__func__, __LINE__,
				l_position);

	unsigned long long dpos = 0;
	if(l_position>JUMP_BUFFERSIZE) {
		dpos = l_position-JUMP_BUFFERSIZE;
	}

	fprintf(stderr,"FileVA::%s:%d starting at jump=%llu (jump buffer size=%d)\n",
				__func__, __LINE__,
				dpos, JUMP_BUFFERSIZE);
	url_fseek(URLPB(m_pFormatCtx->pb), dpos, SEEK_SET);

	m_movie_pos.nbFramesSinceKeyFrame = -1; // tell that we cannot know the last key frame
	m_movie_pos.prevKeyFramePosition = 0;

	// FIXME : update mPlayFrame
	mPlayFrame -= JUMP_BUFFERSIZE / DEFAULT_FRAME_SIZE;
	unsigned long long curpos = url_ftell(URLPB(m_pFormatCtx->pb));
	while(curpos<l_position && !endOfFile())
	{
		GetNextFrame();
		curpos = url_ftell(URLPB(m_pFormatCtx->pb));
#if 0
		fprintf(stderr,"\tFileVA::%s:%d reading frame "
				"=====> position=%llu (/wanted=%llu)\n",
				__func__, __LINE__,
				curpos, l_position);
#endif

	}
}

bool FFmpegFileVideoAcquisition::endOfFile(){
//	if(url_feof(URLPB(m_pFormatCtx->pb)))
//		fprintf(stderr, "FileVA::%s:%d : url_feof says EOF !!\n", __func__,__LINE__);
//	if(url_ftell(URLPB(m_pFormatCtx->pb))>m_fileSize)
//		fprintf(stderr, "FileVA::%s:%d : url_ftell=%d > m_fileSize=%ld !!\n",
//			__func__,__LINE__, url_ftell(URLPB(m_pFormatCtx->pb)), m_fileSize);

	return ( url_feof(URLPB(m_pFormatCtx->pb)) ||
		((unsigned long)url_ftell(URLPB(m_pFormatCtx->pb)) > m_fileSize)
		);
}


#if 0
bool FFmpegFileVideoAcquisition::GetNextFrame()
{
	static int      bytesRemaining=0;
	static uint8_t  *rawData;
	static bool     fFirstTime=true;
	int             bytesDecoded;
	int             frameFinished;


	// First time we're called, set packet.data to NULL to indicate it
	// doesn't have to be freed
	if(fFirstTime)
	{
		fFirstTime=false;
		packet.data=NULL;
	}

	// Decode packets until we have decoded a complete frame
	while(true)
	{
		// Work on the current packet until we have decoded all of it
		while(bytesRemaining > 0)
		{
			// Decode the next chunk of data
			bytesDecoded = avcodec_decode_video(m_pCodecCtx, m_pFrame,
				&frameFinished, rawData, bytesRemaining);

			// Was there an error?
			if(bytesDecoded < 0)
			{
				// Free old packet
				if(packet.data!=NULL)
					av_free_packet(&packet);

				bytesRemaining=0;

				fprintf(stderr, "Error while decoding frame\n");
				return false;
			}

			bytesRemaining-=bytesDecoded;
			rawData+=bytesDecoded;

			// Did we finish the current frame? Then we can return
			if(frameFinished)
			{
				// fill recept buffer
				fill_buffer();
				return true;
			}
		}

		// Read the next packet, skipping all packets that aren't for this
		// stream
		do
		{
			// Free old packet
			if(packet.data!=NULL)
				av_free_packet(&packet);

			// Read new packet
			if(av_read_packet(m_pFormatCtx, &packet)<0)
				goto loop_exit;
		} while(packet.stream_index!=m_videoStream);

		bytesRemaining=packet.size;
		rawData=packet.data;
	}

loop_exit:
	PRINT_FIXME
	// Decode the rest of the last frame
	bytesDecoded = avcodec_decode_video(m_pCodecCtx, m_pFrame, &frameFinished,
										rawData, bytesRemaining);

	if(frameFinished!=0)
	{
		// fill recept buffer
		fill_buffer();
	}

	// Free last packet
	if(packet.data!=NULL) {
		av_free_packet(&packet);
	}

	return frameFinished!=0;
}

#else

bool FFmpegFileVideoAcquisition::GetNextFrame()
{
	if(!m_pFormatCtx) {
		fprintf(stderr, "FileVA::%s:%d : invalid m_pFormatCtx : NULL !!\n", __func__, __LINE__);

		return false;
	}

	int frameFinished = 0;
	int counter=0;
	int ret_val;
	int old_pos;

	m_movie_pos.prevAbsPosition =
		m_prevPosition = url_ftell(URLPB(m_pFormatCtx->pb));
	old_pos = m_prevPosition;

	if(m_video_properties.fps <= 0)
	{
		mImageInfo.Tick += 40000; ///< \bug FIXME
	}
	else
	{
		mImageInfo.Tick += 1000000.f / m_video_properties.fps;
	}

	while(mImageInfo.Tick > 1000000)
	{
		mImageInfo.Tick -= 1000000;
		mImageInfo.Date ++;
	}

	PIAF_MSG(SWLOG_TRACE, "FPS=%g Date=%lu.%lu",
			 m_fps,
			 (ulong)mImageInfo.Date,  (ulong)mImageInfo.Tick );

	while(counter<25)
	{
		// Read a frame/packet.
//		fprintf(stderr, "FileVA::%s:%d m_pFormatCtx=%p m_pCodecCtx=%p %dx%d\n", __func__, __LINE__, m_pFormatCtx,
//			m_pCodecCtx, (int)m_pCodecCtx->width, (int)m_pCodecCtx->height
//			);

		ret_val = av_read_frame(m_pFormatCtx, &packet);
		if(endOfFile()) {
			//fprintf(stderr, "FileVA::%s:%d EOF\n", __func__, __LINE__);
			return false;
		}

		if(ret_val<0)
		{
			if(old_pos == url_ftell(URLPB(m_pFormatCtx->pb)))
			{
				url_fseek(URLPB(m_pFormatCtx->pb), old_pos+2500,SEEK_SET);
			}
			counter++;
		} else {
			// If it's a video packet from our video stream...
			if(packet.stream_index == m_videoStream)
			{
/*
#if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(53, 2, 0)
				avcodec_decode_video2(video_st->codec, picture, &got_picture, &packet);
#elif LIBAVFORMAT_BUILD > 4628
				avcodec_decode_video(video_st->codec,
									 picture, &got_picture,
									 packet.data, packet.size);
#else
				avcodec_decode_video(&video_st->codec,
									 picture, &got_picture,
									 packet.data, packet.size);
#endif


*/

#if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(53, 2, 0)
				avcodec_decode_video2(m_pCodecCtx, m_pFrame, &frameFinished, &packet);
#elif LIBAVFORMAT_BUILD > 4628
				avcodec_decode_video(m_pCodecCtx, m_pFrame, &frameFinished,
									 packet.data, packet.size);
#else
				avcodec_decode_video(&m_pCodecCtx, m_pFrame, &frameFinished,
									 packet.data, packet.size);
#endif
				/*
				PRINT_FIXME
				FILE * fpacket = FileOpen("/tmp/packet.jpg", "wb");
				if(fpacket) {
					fwrite(packet.data, 1, packet.size, fpacket);
					FileClose(fpacket);
				}
				*/

				if( frameFinished )
				{
					// fill recept buffer
					fill_buffer();

					// FIXME : OLD !!! increaseTimeFileVA();
					// update mPlayFrame
					mPlayFrame++;

					//
					if(m_pFrame->key_frame)
					{
						m_movie_pos.nbFramesSinceKeyFrame = 0;
						m_movie_pos.prevKeyFramePosition = m_prevPosition;

					//	fprintf(stderr, "FileVA::%s:%d : key frame = %d : frame=%d\n",
					//			__func__, __LINE__, m_pFrame->key_frame, mPlayFrame);
					} else {
						if(m_movie_pos.nbFramesSinceKeyFrame>=0) {
							m_movie_pos.nbFramesSinceKeyFrame++;
						} // else is < 0 we don't know where we are
					}

					return true;
				} else {
					counter++;
				}
			}

			av_free_packet(&packet);
		}
	}

	//    return frameFinished!=0;
	fprintf(stderr, "FileVA::%s:%d NO FRAME FOUND counter=%d mPlayFrame=%ld Fpos=%lu)\n",
		__func__, __LINE__,
		counter, mPlayFrame,
		(unsigned long)url_ftell(URLPB(m_pFormatCtx->pb)));

	return false;
}
#endif

unsigned char * FFmpegFileVideoAcquisition::readImageBuffer(long * buffersize)
{
	if(!myAcqIsInitialised) {
		fprintf(stderr, "[FVA]::%s:%d: '%s': myAcqIsInitialised is FALSE\n",
				__func__, __LINE__, m_videoFileName);
		return NULL;
	}

	bool ret = GetNextFrame();
	// read next frame until end of file
	int fail_counter = 0;
	while( !ret && fail_counter < 25) {
		if(endOfFile()) {
			fprintf(stderr, "[FVA]::%s:%d : '%s' GetNextFrame EOF !\n",
					__func__, __LINE__, m_videoFileName);
			return NULL;
		}
		fail_counter++;
		fprintf(stderr, "[FVA]::%s:%d : GetNextFrame on '%s' failed (for %d times)\n",
				__func__, __LINE__, m_videoFileName, fail_counter);
		ret = GetNextFrame();
	}
	if(!ret) {
		return NULL;
	}

	static unsigned short oldSig = 0xffff;
	// calcul d'une signature sur 20 octets de l'image
	unsigned short sig = 0;

	if(oldSig>0) {
		do {
			sig = 0;
			for(int i=10000; i<10100; i+=4)
				sig += (unsigned short)receptBuffer[i];

			if(sig == oldSig) {
				ret = GetNextFrame();
				// read next frame
				while( !ret ) {
					if(endOfFile()) {
						fprintf(stderr, "[FVA]::%s:%d : endOfFile !\n", __func__, __LINE__);
						return NULL;
					}
					fprintf(stderr, "[FVA]::%s:%d GetNextFrame failed\n", __func__, __LINE__);
					ret = GetNextFrame();
				}
			}
		}
		while(sig == oldSig);
	}
	//
	oldSig = sig;

	if(!ret) {
		fprintf(stderr, "FileVA::%s:%d : ret = %d\n", __func__, __LINE__, ret);
		return NULL;
	}
	//*buffersize = mImageSize.width*mImageSize.height;
	*buffersize = avpicture_get_size(m_pCodecCtx->pix_fmt,
									 mImageSize.width, mImageSize.height);

	//fprintf(stderr, "FileVA::%s:%d : read image %d\n", __func__, __LINE__, mPlayFrame);

	return receptBuffer;
}

unsigned char * FFmpegFileVideoAcquisition::bufY() {
	if(!m_pFrame) return NULL;
	return m_pFrame->data[0];
}
unsigned char * FFmpegFileVideoAcquisition::bufU() {
	if(!m_pFrame) return NULL;
	return m_pFrame->data[1];
}
unsigned char * FFmpegFileVideoAcquisition::bufV() {
	if(!m_pFrame) return NULL;
	return m_pFrame->data[2];
}
int FFmpegFileVideoAcquisition::get_xsize() {
	if(!m_pCodecCtx) return 0;
	return mImageSize.width;
}
int FFmpegFileVideoAcquisition::get_ysize() {
	if(!m_pCodecCtx) return 0;
	return mImageSize.height;
}
int FFmpegFileVideoAcquisition::get_pitch() {
	if(!m_pFrame) return 0;
	return m_pFrame->linesize[0];
}
int FFmpegFileVideoAcquisition::get_uW() {
	if(!m_pFrame) return 0;
	return m_pFrame->linesize[1];
}
int FFmpegFileVideoAcquisition::get_vW() {
	if(!m_pFrame) return 0;
	return m_pFrame->linesize[2];
}

enum PixelFormat FFmpegFileVideoAcquisition::get_pixfmt()
{
	if(!m_pCodecCtx) return (enum PixelFormat)PIX_FMT_NONE;
	return m_pCodecCtx->pix_fmt;
}

void FFmpegFileVideoAcquisition::fill_buffer()
{
	unsigned char *buf = m_pFrame->data[0];
	unsigned char *buf2 = m_pFrame->data[1];
	unsigned char *buf3 = m_pFrame->data[2];
	int xsize = mImageSize.width;
	int ysize = mImageSize.height;
	int pitch = m_pFrame->linesize[0];
	int uW = m_pFrame->linesize[1];
	int vW = m_pFrame->linesize[2];

	int i;

	unsigned char * image = receptBuffer;

	unsigned char * yline = image;
	unsigned char * uline = image + mImageSize.width*mImageSize.height;
	int width_div_2 = mImageSize.width/2;
	unsigned char * vline = uline + width_div_2*(mImageSize.height/2);

//	fprintf(stderr, "%s:%d : %dx%d => yline=%p uline=y+%d, vline=uline+%d=vline+%d\n",
//			__func__, __LINE__,
//			xsize, ysize,
//			yline, (int)(uline-yline), (int)(vline-uline), (int)(vline-yline));

	if(buf2 && buf3) {
		for(i=0;i<ysize;i++) {
			memcpy(yline, buf, xsize);

			buf += pitch;
			yline += mImageSize.width;

			if(i%2==0) {
				memcpy(uline, buf2, width_div_2);
				uline += width_div_2;
				buf2 += uW;

				memcpy(vline, buf3, width_div_2);
				vline += width_div_2;
				buf3 += vW;
			}
		}
	}else {
		// y only
		for(i=0;i<ysize;i++) {
			memcpy(yline, buf, xsize);

			buf += pitch;
			yline += mImageSize.width;
		}
	}
}
/** \brief Get the list of output format */
QList<t_video_output_format> FFmpegFileVideoAcquisition::getOutputFormats()
{
	t_video_output_format RGB32format;
	RGB32format.id = 0;
	strcpy(RGB32format.description, "BGR32");
	RGB32format.ipl_depth = IPL_DEPTH_8U;
	RGB32format.ipl_nchannels = 4;
	QList<t_video_output_format> out;
	out.append(RGB32format);
	DEBUG_MSG("NOT IMPLEMENTED => only RGB32");

	return out;
}

/** \brief Set the output format */
int FFmpegFileVideoAcquisition::setOutputFormat(int id)
{
	DEBUG_MSG("NOT IMPLEMENTED => id=%d", id);

	return id;
}

/** @brief Read image as data of selected format */
IplImage * FFmpegFileVideoAcquisition::readImage()
{
	DEBUG_MSG("NOT IMPLEMENTED => return readimageRGB32()");
	return readImageRGB32();
}

int FFmpegFileVideoAcquisition::readImageRaw(unsigned char ** rawimage,
									   unsigned char ** compbuffer , long * compsize)
{
	int ret = readImageYUV(m_noPitchBuffer /* no buffer copy */, compsize);

	if(packet.size > 0 && m_isJpeg) {
		* compbuffer = packet.data;
		* compsize = packet.size;
	} else {
		* compbuffer = NULL;
		* compsize = 0;
	}
	// Allocate buffer with no pitch
	* rawimage = m_noPitchBuffer;

	return ret;
}

int FFmpegFileVideoAcquisition::readImageRaw(unsigned char * rawimage,
	unsigned char * , long * compsize, bool )
{
	return readImageY(rawimage, compsize);



	// ************** BOUCHONNEE ******************
	if(!myAcqIsInitialised)
		return -1;

	unsigned char * imageBrute = NULL;

	if(!myAcqIsInitialised) {
		fprintf(stderr, "FFmpegFileVideoAcquisition::readImageRaw: Device not initalised\n");
		return -1;
	}

	if((imageBrute = readImageBuffer(compsize))==NULL)
	{
		fprintf(stderr, "FFmpegFileVideoAcquisition::readImageRaw: readImageBuffer returned NULL\n");
		return -1;
	}

	if(rawimage == NULL)
		return 0;
fprintf(stderr, "FileVA::%s:%d : read image %ld\n", __func__, __LINE__, mPlayFrame);
	// no conversion, return raw image
	AVPicture dummypic;
	int imsize = avpicture_fill(&dummypic, NULL, m_pCodecCtx->pix_fmt, mImageSize.width, mImageSize.height);

	if(*compsize<imsize)
		return -1;

	memcpy(rawimage,  imageBrute, imsize);

	return 0;
}

unsigned char * FFmpegFileVideoAcquisition::getImageRawNoAcq()
{
	return receptBuffer;
}

int FFmpegFileVideoAcquisition::readImageRGB32(unsigned char * image,
		long * buffersize,
		bool )
{
	unsigned char * imageBrute;
	PRINT_FIXME
	if(!myAcqIsInitialised)
		return -1;

	if( (*buffersize) < (long)avpicture_get_size(PIX_FMT_RGBA32, mImageSize.width, mImageSize.height)) {

		return -1;
	}

	// read raw image
	if((imageBrute = readImageBuffer(buffersize))==NULL){

		return -1;
	}

	if(image == NULL){

		return -1;
	}
#if 0
	// conversion to RGB32 coding
/*	PRINT_FIXME
	img_convert((AVPicture *)m_pFrameRGB, PIX_FMT_RGBA32, (AVPicture*)m_pFrame,
				mImageSize.pix_fmt, mImageSize.width, mImageSize.height);
	PRINT_FIXME

	memcpy(image, m_inbuff, avpicture_get_size(PIX_FMT_RGBA32, mImageSize.width, mImageSize.height));
	PRINT_FIXME
	*/
#else // new code
	// use queue_picture
	fprintf(stderr, "FileVA::%s:%d : use queue_picture code\n", __func__, __LINE__);

	//int queue_picture(VideoState *is, AVFrame *pFrame, double pts) {
	struct SwsContext *img_convert_ctx = NULL;
	PixelFormat dst_pix_fmt ;
	AVPicture pict;

	if(1 ) { // vp->bmp) {

		//	  dst_pix_fmt = PIX_FMT_YUV420P;
		dst_pix_fmt = PIX_FMT_YUV420P;

		/* point pict at the queue */
		//	pict.data[0] = vp->bmp->pixels[0];
		//	pict.data[1] = vp->bmp->pixels[2];
		//	pict.data[2] = vp->bmp->pixels[1];

		pict.data[0] = m_pFrame->data[0];
		pict.data[1] = m_pFrame->data[1];
		pict.data[2] = m_pFrame->data[2];

		// linesize
		pict.linesize[0] = m_pFrame->linesize[0];
		pict.linesize[1] = m_pFrame->linesize[1];
		pict.linesize[2] = m_pFrame->linesize[2];

		//	pict.linesize[0] = vp->bmp->pitches[0];
		//	pict.linesize[1] = vp->bmp->pitches[2];
		//	pict.linesize[2] = vp->bmp->pitches[1];

		// Convert the image into YUV format that SDL uses
		if( img_convert_ctx == NULL ) {
			int w = mImageSize.width;
			int h = mImageSize.height;

			img_convert_ctx = sws_getContext(w, h,
											 m_pCodecCtx->pix_fmt,
											 w, h, dst_pix_fmt, SWS_BICUBIC,
											 NULL, NULL, NULL);
			if(img_convert_ctx == NULL) {
				fprintf(stderr, "Cannot initialize the conversion context!\n");
				exit(1);
			}
		}

		sws_scale(img_convert_ctx, m_pFrame->data,
				  m_pFrame->linesize, 0,
				  mImageSize.height,
				  pict.data, pict.linesize);
		if(0) {
			memcpy(image, m_inbuff,
				   avpicture_get_size(PIX_FMT_RGBA32, mImageSize.width, mImageSize.height));
			IplImage * testImage = cvCreateImageHeader(cvSize(mImageSize.width, mImageSize.height), 8, 4);
			testImage->imageData = (char *)m_inbuff;
			cvSaveImage("/dev/shm/FVA.jpg", testImage);
			cvReleaseImageHeader(&testImage);
		}
		else {
			int pitch = avpicture_get_size(PIX_FMT_RGBA32, mImageSize.width, mImageSize.height)/mImageSize.height;
			u32 * output = (u32 *)image;
			u32 * input = (u32 *)m_pFrameRGB->data[0];
			for(int r = 0; r<mImageSize.height; r++)
			{
				int pos = r * pitch / 4;
#if LIBAVFORMAT_BUILD < CALC_FFMPEG_VERSION(50, 0, 0)
				PIAF_MSG(SWLOG_INFO, "Fix inversion of pixels 2 by 2");
				if((r%2) == 0) {
					memcpy(output+pos, input+pos, pitch);

				} else {
					for(int c = 0; c<mImageSize.width; c+=2) {
						output[pos+c] = input[pos+c+1];
						output[pos+c+1] = input[pos+c];
					}
				}
#else
				memcpy(output+pos, input+pos, pitch);
#endif

			}
		}
	}
#endif
	return 0;
}

int FFmpegFileVideoAcquisition::readImageYUV(unsigned char* image, long * buffersize)
{
	int ret = readImageY(image, buffersize);
	if(ret < 0) {
		fprintf(stderr, "FileVA::%s:%d : readImageY(image=%p, buffersize=%ld) failed\n",
			__func__, __LINE__, image, *buffersize);
		return ret;
	}
	if(!image)
		image = m_noPitchBuffer;

	// Copy U/V buffers
	int l_palette = getPalette();
	int pitchU = m_pFrame->linesize[1],
		pitchV = m_pFrame->linesize[2],
		width = mImageSize.width,
		height = mImageSize.height;

	switch(l_palette) {
	default:
		fprintf(stderr, "FileVA::%s:%d : unsupported palette %d\n", __func__, __LINE__, l_palette);

		break;
	case VIDEO_PALETTE_YUV420P: {
		int u_offset = width*height;
		unsigned char * imageBruteU =  m_pFrame->data[1];
		unsigned char * imageBruteV =  m_pFrame->data[2];
		int height_2 = height/2;
		int width_2 = width/2;


		int v_offset = u_offset * 5/4;
		int offset = 0;

		for(int i=0; i<height_2; i++, offset+=pitchU, u_offset+=width_2)
		{
			memcpy(image+u_offset, imageBruteU+offset, width_2);
		}

		offset = 0;
		for(int i=0; i<height_2; i++, offset+=pitchV, v_offset+=width_2)
		{
			memcpy(image+v_offset, imageBruteV+offset, width_2);
		}

		}break;
	case VIDEO_PALETTE_YUV422P: {
		PRINT_FIXME
		int u_offset = width*height;
		unsigned char * imageBruteU =  m_pFrame->data[1];
		unsigned char * imageBruteV =  m_pFrame->data[2];
		int height_2 = height/2;
		int width_2 = width/2;


		int v_offset = u_offset * 5/4;
		int offset = 0;

		for(int i=0; i<height_2; i++, offset+=pitchU, u_offset+=width_2)
		{
			memcpy(image+u_offset, imageBruteU+offset, width_2);
		}

		offset = 0;
		for(int i=0; i<height_2; i++, offset+=pitchV, v_offset+=width_2)
		{
			memcpy(image+v_offset, imageBruteV+offset, width_2);
		}

		}break;
	}
	return ret;
}

int FFmpegFileVideoAcquisition::readImageY(unsigned char* image, long * buffersize)
{
	unsigned char * imageBrute;

	if(!myAcqIsInitialised) {
		fprintf(stderr, "FileVA::%s:%d : acquisition is not initialized\n", __func__, __LINE__);
		return -1;
	}

	// read raw image
	imageBrute = readImageBuffer(buffersize);
	if(imageBrute == NULL) {
		fprintf(stderr, "FileVA::%s:%d : readImageBuffer failed\n", __func__, __LINE__);

		return -1;
	}

	/*if(image == NULL){
		fprintf(stderr, "FileVA::%s:%d : output image is NULL\n", __func__, __LINE__);
		return -1;
	}*/
	if( (*buffersize) < (long)(mImageSize.width*mImageSize.height))
	{
		fprintf(stderr, "FileVA::%s:%d : (*buffersize)=%ld too short buffer (%dx%d = %ld)\n",
			__func__, __LINE__,
			(*buffersize),
			mImageSize.width, mImageSize.height,
			(long)(mImageSize.width*mImageSize.height));
		return -1;
	}
	int posout=0, posin=0;
	int pitch = m_pFrame->linesize[0],
		width = mImageSize.width,
		height = mImageSize.height;



	/*
	fprintf(stderr, "FileVA::%s:%d :mPlayFrame=%d image size: %d x %d pitch=%d\n", __func__, __LINE__,
		mPlayFrame,
		width, height, pitch);
	*/

	imageBrute =  m_pFrame->data[0];
	int l_palette = getPalette();

	if(!image)  {
		int pixsize = width * height;
		switch(l_palette) {
		case VIDEO_PALETTE_YUV410P:
		case VIDEO_PALETTE_YUV411:
		case VIDEO_PALETTE_YUV411P:
			PRINT_FIXME
		case VIDEO_PALETTE_YUV422P:
		case VIDEO_PALETTE_YUV420P:
			pixsize = pixsize * 3 / 2;
			break;
		default:
			fprintf(stderr, "FileVA::%s:%d : unsupported palette %d !!\n",
				__func__, __LINE__, l_palette);

			return -1;
		}
		if(m_noPitchBuffer) delete [] m_noPitchBuffer;
		m_noPitchBuffer = new unsigned char [ pixsize ];
		image = m_noPitchBuffer;
		m_noPitchBufferSize = pixsize;
	}
	else
		m_noPitchBufferSize = width * height;

	*buffersize = m_noPitchBufferSize;

	for(int i=0; i<height;
		i++, posout += width,
		posin += pitch) {
		memcpy(image+posout, imageBrute+posin, width);
	}

	/*
	FILE * fdebug = FileOpen("/tmp/readImageY.pgm", "wb");
	if(fdebug) {
		fprintf(fdebug, "P5\n%d %d\n255\n", width, height);
		fwrite(image, width, height, fdebug);
		FileClose(fdebug);
	}*/

	return mPlayFrame;
}

int FFmpegFileVideoAcquisition::readImageYNoAcq(unsigned char* image, long * buffersize)
{
	if(!myAcqIsInitialised){
		fprintf(stderr, "FVA::%s:%d : acquisition is not started !!\n", __func__, __LINE__);
		return -1;
	}
	if(!buffersize) {
		fprintf(stderr, "FVA::%s:%d : buffersize is null !!\n", __func__, __LINE__);
		return -1;
	}
	if(!m_pFrame) {
		fprintf(stderr, "FVA::%s:%d : m_pFrame is null !!\n", __func__, __LINE__);
		return -1;
	}

	unsigned char *imageBrute = m_pFrame->data[0];

	if( imageBrute==NULL ) {
		fprintf(stderr, "FVA::%s:%d : imageBrute is null !!\n", __func__, __LINE__);

		return -1;
	}
	if(image == NULL){
		fprintf(stderr, "FVA::%s:%d : image is null !!\n", __func__, __LINE__);

		return -1;
	}

	int pitch = m_pFrame->linesize[0];
	if(mImageSize.width == pitch) {
		memcpy(image, imageBrute, mImageSize.width*mImageSize.height);
	} else {
		for(int r = 0; r<mImageSize.height; r++) {
			memcpy(image + r*mImageSize.width,
				   imageBrute+r*pitch, mImageSize.width);
		}
	}
/*
	FILE * fdebug = fopen("/dev/shm/pictY.pgm", "wb");
	if(fdebug) {
			fprintf(fdebug, "P5\n%d %d\n255\n", m_pFrame->linesize[0], mImageSize.height);
			fwrite(m_pFrame->data[0], m_pFrame->linesize[0], mImageSize.height, fdebug);
			fclose(fdebug);
	}
*/
	*buffersize = mImageSize.width*mImageSize.height;

	return 0;
}

int FFmpegFileVideoAcquisition::readImageRGB32NoAcq(unsigned char* image, long * buffersize)
{
	if(!myAcqIsInitialised){
		fprintf(stderr, "FVA::%s:%d : acquisition is not started !!\n", __func__, __LINE__);
		return -1;
	}
	if(!buffersize) {
		fprintf(stderr, "FVA::%s:%d : buffersize is null !!\n", __func__, __LINE__);
		return -1;
	}
	if( (*buffersize) < (long)avpicture_get_size(PIX_FMT_RGBA32, mImageSize.width, mImageSize.height)){
		fprintf(stderr, "FVA::%s:%d : buffersize (%ld bytes) is not big enough : should be %ld !!\n",
			__func__, __LINE__,
			(*buffersize),
			(long)avpicture_get_size(PIX_FMT_RGBA32,
				mImageSize.width, mImageSize.height)
			);
		*buffersize = (long)avpicture_get_size(PIX_FMT_RGBA32,
				mImageSize.width, mImageSize.height);
		return -1;
	}

	// conversion to RGB32 coding

	/*fprintf(stderr, "FileVA::%s:%d : m_pFrame=%p m_pFrameRGB=%p m_pCodecCtx=%p size=%d\n", __func__, __LINE__,
		m_pFrame, m_pFrameRGB, m_pCodecCtx,
		(int)avpicture_get_size(PIX_FMT_RGBA32, mImageSize.width, mImageSize.height));
	*/
#if 0
	img_convert((AVPicture *)m_pFrameRGB, PIX_FMT_RGBA32, (AVPicture*)m_pFrame,
					m_pCodecCtx->pix_fmt, mImageSize.width, mImageSize.height);
/*FIXME	img_convert((AVPicture *)m_pFrameRGB, PIX_FMT_RGBA32, (AVPicture*)m_pFrame,
				m_pCodecCtx->pix_fmt, mImageSize.width, mImageSize.height);
*/
	memcpy(image, m_inbuff,
		   avpicture_get_size(PIX_FMT_RGBA32, mImageSize.width, mImageSize.height));

#else // new code
	// use queue_picture
//	fprintf(stderr, "FileVA::%s:%d : use queue_picture code\n", __func__, __LINE__);

	//int queue_picture(VideoState *is, AVFrame *pFrame, double pts) {
	struct SwsContext *img_convert_ctx = NULL;
	PixelFormat dst_pix_fmt ;
	AVPicture pict;

	if(1 ) { // vp->bmp) {

            //	  dst_pix_fmt = PIX_FMT_YUV420P;
            dst_pix_fmt = PIX_FMT_RGB32;

            // point pict at the queue
            pict.data[0] = m_pFrameRGB->data[0];
            pict.data[1] = m_pFrameRGB->data[1];
            pict.data[2] = m_pFrameRGB->data[2];

            // linesize
            pict.linesize[0] = m_pFrameRGB->linesize[0];
            pict.linesize[1] = m_pFrameRGB->linesize[1];
            pict.linesize[2] = m_pFrameRGB->linesize[2];

            // Convert the image into YUV format that SDL uses
            if( img_convert_ctx == NULL ) {
				int w = mImageSize.width;
				int h = mImageSize.height;

                img_convert_ctx = sws_getContext(w, h,
                                                 m_pCodecCtx->pix_fmt,
                                                 w, h, dst_pix_fmt, SWS_FAST_BILINEAR, // SWS_BICUBIC,
                                                 NULL, NULL, NULL);
                if(img_convert_ctx == NULL) {
                    fprintf(stderr, "Cannot initialize the conversion context!\n");
                    exit(1);
                }
            }
            sws_scale(img_convert_ctx,
                      m_pFrame->data,
                      m_pFrame->linesize, 0,
					  mImageSize.height,
                      pict.data, pict.linesize);
            /*
                FILE * fdebug = fopen("/dev/shm/readImageRGB.pgm", "wb");
                if(fdebug) {
						fprintf(fdebug, "P5\n%d %d\n255\n", mImageSize.width*4, mImageSize.height);
						fwrite(m_pFrameRGB->data[0], mImageSize.width*4, mImageSize.height, fdebug);
                        fclose(fdebug);
				}
				FILE * fdebug = fopen("/dev/shm/pict.pgm", "wb");
                if(fdebug) {
						fprintf(fdebug, "P5\n%d %d\n255\n", m_pFrame->linesize[0], mImageSize.height);
						fwrite(m_pFrame->data[0], m_pFrame->linesize[0], mImageSize.height, fdebug);
                        fclose(fdebug);
				}
*/
            // Correct image
            if(0)
            {
                memcpy(image, m_pFrameRGB->data[0],
					   avpicture_get_size(PIX_FMT_RGBA32, mImageSize.width, mImageSize.height));
            } else {
				int pitch = avpicture_get_size(PIX_FMT_RGBA32,
											   mImageSize.width, mImageSize.height)
										/ mImageSize.height;
                u32 * output = (u32 *)image;
                u32 * input = (u32 *)m_pFrameRGB->data[0];
				for(int r = 0; r<mImageSize.height; r++)
                {
					int posout = r * pitch / 4;
#if LIBAVFORMAT_BUILD < CALC_FFMPEG_VERSION(50, 0, 0)
					PIAF_MSG(SWLOG_INFO, "Fix inversion of pixels 2 by 2");

                    if((r%2) == 0) {
						memcpy(output+posout, input+posout, pitch);
                    } else {
						for(int c = 0; c<mImageSize.width; c+=2) {
							output[posout+c] = input[posout+c+1];
							output[posout+c+1] = input[posout+c];
                        }
                    }
#else
					memcpy(output+posout, input+posout, pitch);

#endif

                }
            }
	}
#endif

	return 0;
}




int FFmpegFileVideoAcquisition::getcapability(video_capability * vc)
{
	if(!vc) {
		DEBUG_ALL("vc is null")
		return -1;
	}

	memset(vc, 0, sizeof(video_capability));

	if(!m_pCodecCtx) {
		DEBUG_ALL("m_pCodecCtx is null")
		return -1;
	}

	vc->minwidth = mImageSize.width;
	vc->maxwidth = mImageSize.width;
	vc->minheight = mImageSize.height;
	vc->maxheight = mImageSize.height;
	vc->channels = 0;

	return 1;
}

/// Not really used, just for Video Device compatibility
int FFmpegFileVideoAcquisition::setNorm(char *norm)
{
	if(!norm)
		return -1;
	return 1;
}

int FFmpegFileVideoAcquisition::changeAcqParams(tBoxSize , int)
{
	// stop acquisition
	myAcqIsInitialised = false;

	// change acquisition size
	return -1;
}

int FFmpegFileVideoAcquisition::setpicture(int, int , int , int , int )
{
	return 1;
}


int FFmpegFileVideoAcquisition::getpicture(video_picture * pic)
{
	if(!pic) return -1;
	memset(pic, 0, sizeof(video_picture));
	pic->palette = (unsigned short)getPalette();
	return 1;
}



/*
 * Note OLV : a valider avec CSE
*/
int FFmpegFileVideoAcquisition::getPalette()
{
	if(!m_pCodecCtx) {
		fprintf(stderr, "FileVA::%s:%d : getPalette failed : m_pCodecCtx not allocated !\n",
			__func__, __LINE__);
		return -1;
	}
	myVD_palette = -1;

	switch(m_pCodecCtx->pix_fmt)
	{
		case PIX_FMT_YUV420P:   ///< Planar YUV 4:2:0 (1 Cr & Cb sample per 2x2 Y samples)
		case PIX_FMT_YUVJ420P:  ///< Planar YUV 4:2:0 full scale (jpeg)
			myVD_palette = VIDEO_PALETTE_YUV420P;
			//fprintf(stderr, "FileVA::%s:%d : palette is VIDEO_PALETTE_YUV420P\n", __func__, __LINE__);
			break;
	#ifdef PIX_FMT_YUV422
		case PIX_FMT_YUV422:
			fprintf(stderr, "FileVA::%s:%d : palette is VIDEO_PALETTE_YUV422\n", __func__, __LINE__);
			myVD_palette = VIDEO_PALETTE_YUV422;
			break;
	#endif
		case PIX_FMT_RGB24:     ///< Packed pixel, 3 bytes per pixel, RGBRGB...
			fprintf(stderr, "FileVA::%s:%d : palette is VIDEO_PALETTE_RGB24\n", __func__, __LINE__);
			myVD_palette = VIDEO_PALETTE_RGB24;
			break;
		case PIX_FMT_YUV422P:   ///< Planar YUV 4:2:2 (1 Cr & Cb sample per 2x1 Y samples)
			fprintf(stderr, "FileVA::%s:%d : palette is VIDEO_PALETTE_YUV422\n", __func__, __LINE__);
			myVD_palette = VIDEO_PALETTE_YUV422P;
			break;
		case PIX_FMT_RGBA32:    ///< Packed pixel, 4 bytes per pixel, BGRABGRA..., stored in cpu endianness
			fprintf(stderr, "FileVA::%s:%d : palette is VIDEO_PALETTE_RGB32\n", __func__, __LINE__);
			myVD_palette = VIDEO_PALETTE_RGB32;
			break;
		case PIX_FMT_YUV410P:   ///< Planar YUV 4:1:0 (1 Cr & Cb sample per 4x4 Y samples)
			fprintf(stderr, "FileVA: Palette is PIX_FMT_YUV410P\n");
			myVD_palette = VIDEO_PALETTE_YUV410P;
			break;
		case PIX_FMT_YUV411P:   ///< Planar YUV 4:1:1 (1 Cr & Cb sample per 4x1 Y samples)
			fprintf(stderr, "FileVA: Palette is PIX_FMT_YUV411P\n");
			myVD_palette = VIDEO_PALETTE_YUV411P;
			break;
		case PIX_FMT_RGB565:    ///< always stored in cpu endianness
			myVD_palette = VIDEO_PALETTE_RGB565;
			fprintf(stderr, "FileVA: Palette is PIX_FMT_RGB565\n");
			break;
		case PIX_FMT_RGB555:    ///< always stored in cpu endianness, most significant bit to 1
			fprintf(stderr, "FileVA: Palette is PIX_FMT_RGB55\n");
			myVD_palette = VIDEO_PALETTE_RGB555;
			break;
		case PIX_FMT_GRAY8:
			fprintf(stderr, "FileVA: Palette is PIX_FMT_GRAY8\n");
			myVD_palette = VIDEO_PALETTE_GREY;
			break;
		case PIX_FMT_NB:
			fprintf(stderr, "FileVA: Palette is PIX_FMT_NB\n");
			myVD_palette = VIDEO_PALETTE_RGB24;
			break;
		case PIX_FMT_BGR24:     ///< Packed pixel, 3 bytes per pixel, BGRBGR...
			fprintf(stderr, "FileVA: Palette is PIX_FMT_BGR24\n");
			myVD_palette = VIDEO_PALETTE_RGB24;
			break;
		case PIX_FMT_YUV444P:   ///< Planar YUV 4:4:4 (1 Cr & Cb sample per 1x1 Y samples)
			fprintf(stderr, "FileVA: Palette is PIX_FMT_YUV444P\n");
			myVD_palette = VIDEO_PALETTE_YUV422P;
			break;
		case PIX_FMT_MONOWHITE: ///< 0 is white
			fprintf(stderr, "FileVA: Palette is PIX_FMT_MONOWHITE\n");
			return -1;
		case PIX_FMT_MONOBLACK: ///< 0 is black
			fprintf(stderr, "FileVA: Palette is PIX_FMT_MONOBLACK\n");
			return -1;
		case PIX_FMT_PAL8:      ///< 8 bit with RGBA palette
			fprintf(stderr, "FileVA: Palette is PIX_FMT_PAL8\n");
			return -1;
		case PIX_FMT_YUVJ422P:  ///< Planar YUV 4:2:2 full scale (jpeg)
	//		fprintf(stderr, "FileVA: Palette is PIX_FMT_YUVJ422P\n");
			myVD_palette = VIDEO_PALETTE_YUV422P;
			break;
		case PIX_FMT_YUVJ444P:  ///< Planar YUV 4:4:4 full scale (jpeg)
			fprintf(stderr, "FileVA: Palette is PIX_FMT_YUVJ444P\n");
			return -1;
		case PIX_FMT_XVMC_MPEG2_MC:///< XVideo Motion Acceleration via common packet passing(xvmc_render.h)
			fprintf(stderr, "FileVA: Palette is PIX_FMT_XVMC_MPEG2_MC\n");
			return -1;
		case PIX_FMT_XVMC_MPEG2_IDCT:
			fprintf(stderr, "FileVA: Palette is PIX_FMT_XVMC_MPEG2_IDCT\n");
			return -1;
		default:
			fprintf(stderr, "FileVA::%s:%d : UNKNOWN PALETTE !!\n", __func__, __LINE__);

			return -1;
	}

	return myVD_palette;
}















// FOR VirtualDeviceAcquisition PURE VIRTUAL API
void FFmpegFileVideoAcquisition::initVirtualDevice()
{
	mSequentialMode = true;
	m_imageRGB32 = m_imageBGR32 = m_imageY = NULL;
	memset(&m_video_properties, 0, sizeof(t_video_properties));

}

void FFmpegFileVideoAcquisition::purgeVirtualDevice()
{
	swReleaseImage(&m_imageRGB32);
	swReleaseImage(&m_imageBGR32);
	swReleaseImage(&m_imageY);
}

/* Use sequential mode
	If true, grabbing is done when a new image is requested.
	Else a thread is started to grab */
void FFmpegFileVideoAcquisition::setSequentialMode(bool on)
{
	mSequentialMode = on;
}

/* Function called by the doc (parent) thread */
int FFmpegFileVideoAcquisition::grab()
{
	// Read one frame
	long buffersize = 0;
	unsigned char * retgrab = readImageBuffer(&buffersize);

	PIAF_MSG(SWLOG_TRACE, "read buffer=%p / %ld bytes",
			 retgrab,
			 buffersize)

	return (retgrab ? 0 : -1);
}

/* Return true if acquisition device is ready to start */
bool FFmpegFileVideoAcquisition::isDeviceReady()
{
	return AcqIsInitialised();
}

/* Return true if acquisition is running */
bool FFmpegFileVideoAcquisition::isAcquisitionRunning() {
	return AcqIsInitialised();
}

/** \brief Start acquisition */
int FFmpegFileVideoAcquisition::startAcquisition()
{
	// FIXME
	return 0;

}

/* Stop acquisition */
int FFmpegFileVideoAcquisition::stopAcquisition()
{
	// FIXME
	return 0;

}

/* Grabs one image and convert to RGB32 coding format
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * FFmpegFileVideoAcquisition::readImageRGB32()
{
	if(m_imageRGB32 && (m_imageRGB32->width != mImageSize.width
						|| m_imageRGB32->height != mImageSize.height))
	{
		swReleaseImage(&m_imageRGB32);
		swReleaseImage(&m_imageBGR32);
	}

	if(!m_imageRGB32) {
		fprintf(stderr, "[FileVA]::%s:%d : create RGB32 image = %dx%d",
				__func__, __LINE__, mImageSize.width, mImageSize.height);
		m_imageRGB32 = swCreateImage(mImageSize, IPL_DEPTH_8U, 4);
		cvSet(m_imageRGB32, cvScalarAll(255)); // fill the alpha channel

		m_imageBGR32 = swCreateImage(mImageSize, IPL_DEPTH_8U, 4);
		cvSet(m_imageBGR32, cvScalarAll(255)); // fill the alpha channel
	}
	// Decode image
	long buffersize = m_imageRGB32->widthStep * m_imageRGB32->height;
	int ret = readImageRGB32NoAcq((unsigned char*)m_imageRGB32->imageData,
								  &buffersize);
	if(ret<0) {

		return NULL;
	}

//	fprintf(stderr, "[FileVA]::%s:%d : read BGR32 image = %dx%d : ret = %d\n",
//			__func__, __LINE__,
//			mImageSize.width, mImageSize.height, ret);
	return m_imageRGB32;

	// image is already in BGR32 format, so no need to convert -> comment following lines
//	cvCvtColor(m_imageRGB32, m_imageBGR32, CV_BGRA2RGBA);
//	return m_imageBGR32;
}

int FFmpegFileVideoAcquisition::setVideoProperties(t_video_properties props)
{
// FIXME
	return 0;
}

/* Grabs one image and convert to grayscale coding format
	if sequential mode, return last acquired image, else read and return image

	\return NULL if error
	*/
IplImage * FFmpegFileVideoAcquisition::readImageY()
{
	if(m_imageY && (m_imageY->width != mImageSize.width
						|| m_imageY->height != mImageSize.height) )
	{
		swReleaseImage(&m_imageY);
	}

	if(!m_imageY) {
		fprintf(stderr, "[FileVA]::%s:%d : create Y image = %dx%d", __func__, __LINE__,
				mImageSize.width, mImageSize.height);
		m_imageY = swCreateImage(mImageSize, IPL_DEPTH_8U, 1);
	}
	// Decode image
	long buffersize = m_imageY->widthStep * m_imageY->height;
	int ret = readImageYNoAcq((unsigned char*)m_imageY->imageData,
								  &buffersize);

//	fprintf(stderr, "[FileVA]::%s:%d : read YNoac image = %dx%d : ret = %d\n",
//			__func__, __LINE__,
//			mImageSize.width, mImageSize.height, ret);
	return m_imageY;

}
