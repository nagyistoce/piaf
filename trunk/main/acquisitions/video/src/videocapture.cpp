/***************************************************************************
		   videocapture.cpp  -  doc class for video capture in Piaf
							 -------------------
	begin                : Wed Nov 13 10:07:22 CET 2002
	copyright            : (C) 2002 by Christophe Seyve
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


// include files for Qt
#include <qdir.h>
#include <qfileinfo.h>
#include <qwidget.h>
#include <qmessagebox.h>
#include <qtimer.h>
#include <qfiledialog.h>

// application specific includes
#include "swvideodetector.h"
#include "videocapture.h"
#include "piaf-common.h"

#include "opencvvideoacquisition.h"
#ifdef _V4L2
#include "V4L2Device.h"
#endif

// default and only constructor.
VideoCaptureDoc::VideoCaptureDoc()
{
	imageRGBA = NULL;
	currentPixel = 0;
	mpVAcq=NULL;
	pImageTimer = NULL;
	m_running = m_run = false;
}


// default and only constructor.
VideoCaptureDoc::VideoCaptureDoc(VirtualDeviceAcquisition * vAcq)
{
	imageRGBA = NULL;
	mask = NULL;
	currentPixel = 0;
	mpVAcq=vAcq;

	// alloc image size
	allocateImages();

	// Area mask list
	pMaskList = new QList<QRect *>;
//	pMaskList->setAutoDelete(false);

	/* For devices without select() we use a timer. */
	int Framerate = 25;  // default value
	pImageTimer = NULL;

//	pImageTimer = new QTimer(this);
//	connect(pImageTimer, SIGNAL(timeout()), this, SLOT(loadImage()));
	//pImageTimer->start(1000 / Framerate);

	modified = false;

	// start background thread
	if(mpVAcq->isAcquisitionRunning()) {
		start();
	}
}

// destructor
VideoCaptureDoc::~VideoCaptureDoc()
{
	fprintf(stderr, "VideoCaptureDoc::~VideoCaptureDoc()\n");
	fflush(stderr);

	// signal to observers that the doc will be deleted
	emit documentClosed(this);

	// wait for finishing thread
	m_run = false;
	while(m_run) {
		sleep(1);
	}



	// RGB32 image
	swReleaseImage(&imageRGBA);

	// stop acquisition
	if(mpVAcq) {
		delete mpVAcq;
		mpVAcq = NULL;
	}
}


void VideoCaptureDoc::setPathName(const QString &name)
{
	m_filename = name;
	m_title = QFileInfo(name).fileName();
}

const QString& VideoCaptureDoc::pathName() const
{
  return m_filename;
}

void VideoCaptureDoc::setTitle(const QString &title)
{
  m_title=title;
}

const QString &VideoCaptureDoc::title() const
{
  return m_title;
}


void VideoCaptureDoc::closeDocument()
{

}




bool VideoCaptureDoc::newDocument(int dev)
{
  /////////////////////////////////////////////////
  // TODO: Add your document initialization code here
  /////////////////////////////////////////////////
	// VIDEO INITIALISATION
	if(mpVAcq)
	{
		delete mpVAcq;
		mpVAcq = NULL;
	}
	swReleaseImage(&imageRGBA);

	tBoxSize size;
#define DEFAULT_WIDTH 320
#define DEFAULT_HEIGHT 240

	size.width  = DEFAULT_WIDTH;
	size.height = DEFAULT_HEIGHT;

	char txt[128];
	sprintf(txt, "/dev/video%d", dev);

	fprintf(stderr, "[VideoCapture]::%s:%d ----- CREATE NEW WINDOW FOR DEVICE '%s' -----\n",
			__func__, __LINE__, txt);
#ifndef _V4L2
	mpVAcq = new OpenCVVideoAcquisition(dev);
#else
	mpVAcq = new V4L2Device(dev);
#endif
	if(!mpVAcq->isDeviceReady())
	{
		printf("(VirtualVideoAcquisition) : Video device init failed\n");
		return false;
	}
	else {
		printf("(VirtualVideoAcquisition) : Video device Init OK \n");
	}

	// start acquisition
	fprintf(stderr, "[VidCapture]::%s:%d : mpVAcq->startAcquisition...\n", __func__, __LINE__);
	if(mpVAcq->startAcquisition() < 0) {
		fprintf(stderr, "[VidCapture]::%s:%d : Error: canot initialize acquisition !\n", __func__, __LINE__);
		return false;
	}
	else {
		printf("Initialization : %d\n",  mpVAcq->isAcquisitionRunning());
	}

	// alloc image size
	allocateImages();


	// Area mask list
	pMaskList = new QList<QRect *>;
//	pMaskList->setAutoDelete(false);

	/* For devices without select() we use a timer. */
	int Framerate = 20;  // default value
	if(!pImageTimer)
		pImageTimer = new QTimer(this);
	connect(pImageTimer, SIGNAL(timeout()), this, SLOT(loadImage()));
//	pImageTimer->start(1000 / Framerate);

	modified = false;

	return true;
}

/** @brief Start acquisition loop */
void VideoCaptureDoc::start(Priority)
{

	if(mpVAcq)
	{
		fprintf(stderr, "VideoCaptureDoc::%s:%d : start video acquisition device",
						__func__, __LINE__);
		mpVAcq->startAcquisition();
	}

	if(isRunning())
	{
		fprintf(stderr, "VideoCaptureDoc::%s:%d: loop already running: isRuning()=%c "
				"m_running=%c m_run=%c\n",
				__func__, __LINE__,
				isRunning() ? 'T':'F',
				m_running ? 'T':'F',
				m_run ? 'T':'F'
						);
		return;
	}

	PIAF_MSG(SWLOG_INFO, "VideoCaptureDoc: start video acquisition thread");
	QThread::start();
}

void VideoCaptureDoc::stop()
{
	fprintf(stderr, "VideoCaptureDoc::%s:%d : stop video acquisition loop (running=%c / finished = %c)\n",
					__func__, __LINE__,
			QThread::isRunning() ? 'T':'F',
			QThread::isFinished() ? 'T':'F'
			);

	m_run = false;
}

void VideoCaptureDoc::run()
{
	if(!mpVAcq)
	{
		fprintf(stderr, "VideoCaptureDoc::%s:%d : no video acquisition\n",
						__func__, __LINE__);
		m_running = false;
		m_run = false;
		return;
	}
	PIAF_MSG(SWLOG_INFO, "VideoCaptureDoc: started video acquisition thread");
//	// Start acquisition
//	PIAF_MSG(SWLOG_INFO, "\tCall mpVAcq->startAcquisition();");
//	mpVAcq->startAcquisition();


	PIAF_MSG(SWLOG_INFO, "VideoCaptureDoc: start grabbing loop");
	m_run = m_running = true;
	while(m_run)
	{
		// grab images
		int ret = loadImage();
		if(ret)
		{
			IplImage * img = readImage();

//			fprintf(stderr, "VideoCaptureDoc::%s:%d: (threaded) mVAcq=%p "
//					"load image size : %d x %d ret=%d => readImage()=%dx%dx%dx%d\n",
//					__func__, __LINE__, mpVAcq,
//					imageSize.width, imageSize.height,
//					ret,
//					img->width, img->height, img->depth, img->nChannels
//					);
//			cvSaveImage(SHM_DIRECTORY "VideoCaptureDoc.jpg", img);

		} else {
			fprintf(stderr, "VideoCaptureDoc::%s:%d: acquisition failed\n",
					__func__, __LINE__
					);
		}
	}

	PIAF_MSG(SWLOG_INFO, "\tstopped thread. Stopping acquisition");
	if(mpVAcq)
	{
		PIAF_MSG(SWLOG_INFO, "\tCall mpVAcq->stopAcquisition();");
		mpVAcq->stopAcquisition();
	}

	m_run = m_running = false;
}


int VideoCaptureDoc::allocateImages()
{
	imageSize = mpVAcq->getImageSize();
	currentPixel = (int)( imageSize.width * imageSize.height);

	fprintf(stderr, "VideoCaptureDoc::%s:%d : Image size : %d x %d x 4\n",
			__func__, __LINE__,
			imageSize.width, imageSize.height);

	imageRGBA = swCreateImage(imageSize, IPL_DEPTH_8U, 4);
	if( !(imageRGBA) )
	{
		printf("RGB image memory allocation failed\n");
		mpVAcq->stopAcquisition();
		return 0;
	}

	// Clear buffer for security
	mask = new unsigned char [currentPixel];
	if( !(mask ))
	{
		printf("Mask image memory allocation failed\n");
		return 0;
	}

	// Clear buffer for security
	memset(mask, 0, currentPixel * sizeof(unsigned char));

	return 1;
}

t_video_properties VideoCaptureDoc::getVideoProperties()
{
	if(!mpVAcq) {
		PIAF_MSG(SWLOG_ERROR, "no video acquisition");
		t_video_properties empty;
		memset(&empty, 0, sizeof(t_video_properties));
		return empty;
	}

	t_video_properties props = mpVAcq->updateVideoProperties();
	return props;
}

int VideoCaptureDoc::setVideoProperties(t_video_properties props)
{
	if(!mpVAcq) {
		fprintf(stderr, "VidCapDoc::%s:%d : no vid acq\n", __func__, __LINE__);
		return -1;
	}

	return mpVAcq->setVideoProperties(props);
}
void VideoCaptureDoc::setDetectionMask(char * /*maskfile*/)
{
	// NOT IMPLEMENTED
}
/*  Set exposure value in us */
int VideoCaptureDoc::setexposure( int exposure_us )
{
	if(!mpVAcq) return -1;
	t_video_properties props = mpVAcq->updateVideoProperties();
	props.exposure = exposure_us/100; // in V4L2, exposure in in 100us step
	return mpVAcq->setVideoProperties(props);
}
int VideoCaptureDoc::setgain( int gain)
{
	if(!mpVAcq) return -1;
	t_video_properties props = mpVAcq->updateVideoProperties();
	props.gain = gain; // in V4L2,
	return mpVAcq->setVideoProperties(props);
}

int VideoCaptureDoc::setpicture(int br, int hue, int col, int cont, int white)
{
	if(!mpVAcq) return -1;

//	return mpVAcq->setpicture(br, hue, col, cont, white);
	t_video_properties props = mpVAcq->updateVideoProperties();
	props.brightness = br;
	props.hue = hue;
	props.saturation = col;
	props.contrast = cont;
	props.hue = hue;

	return mpVAcq->setVideoProperties(props);
}

int VideoCaptureDoc::getpicture(video_picture * pic)
{
	t_video_properties props = mpVAcq->updateVideoProperties();
	pic->brightness = props.brightness * 16384;
	pic->hue = props.hue * 16384;
	pic->colour = props.saturation * 16384;
	pic->contrast = props.contrast * 16384;
	// FIXME
	return 0;
	//return mpVAcq->getpicture(pic);
}
int VideoCaptureDoc::getcapability(video_capability * vc)
{
	t_video_properties props = mpVAcq->updateVideoProperties();
	vc->maxwidth = props.max_width;
	vc->minwidth = props.min_width;
	vc->maxheight = props.max_height;
	vc->minheight = props.min_height;

	return 0;

	//return mpVAcq->getcapability(vc);
}

int VideoCaptureDoc::changeAcqParams(tBoxSize newSize, int ch)
{
	t_video_properties props = mpVAcq->updateVideoProperties();
	props.frame_width = newSize.width;
	props.frame_height = newSize.height;
	props.mode = ch;

	return mpVAcq->setVideoProperties(props);
}

//thomas.
int VideoCaptureDoc::setChannel(int ch)
{
	t_video_properties props = mpVAcq->updateVideoProperties();
	props.mode = ch; // FIXME
	return mpVAcq->setVideoProperties(props);
}


bool VideoCaptureDoc::openDocument(const QString &filename, const char * /*format =0*/)
{

	QFile f( filename );
	if ( !f.open( QIODevice::ReadOnly ) )
	{
		PIAF_MSG(SWLOG_ERROR, "cannot open file '%s'", filename.toAscii().data());
		return false;
	}
	/////////////////////////////////////////////////
	// TODO: Add your document opening code here
	/////////////////////////////////////////////////
	f.close();

	modified = false;
	m_filename = filename;
	m_title = QFileInfo(f).fileName();
	return true;
}

bool VideoCaptureDoc::saveDocument(const QString &filename, const char * /*format =0*/)
{
	QFile f( filename );
	if ( !f.open( QIODevice::WriteOnly ) )
		return false;

  /////////////////////////////////////////////////
  // TODO: Add your document saving code here
  /////////////////////////////////////////////////

  f.close();

  modified=false;
  m_filename=filename;
  m_title=QFileInfo(f).fileName();
  return true;
}

void VideoCaptureDoc::deleteContents()
{
  /////////////////////////////////////////////////
  // TODO: Add implementation to delete the document contents
  /////////////////////////////////////////////////

}


int VideoCaptureDoc::waitForImage()
{
	fprintf(stderr, "VideoCapture::%s:%d : wait for notify\n", __func__, __LINE__);
	bool ret = mWaitCondition.wait(&mMutex,
								  500 // ms
								  );
	if(!ret) {
		fprintf(stderr, "VideoCapture::%s:%d : timeout => return -1\n", __func__, __LINE__);

	} else {
		mImageBufferMutex.lock();

		IplImage * imageIn = mpVAcq->readImage();
		if(imageIn)
		{
//			fprintf(stderr, "VideoCapture::%s:%d : Acquisition ok : readImageRaw() ok "
//					": %dx%dx%dx%d\n",
//					__func__, __LINE__,
//					imageIn->width, imageIn->height,
//					imageIn->nChannels, imageIn->depth);

			if(imageIn->width > 0 && imageIn->height>0)
			{
				if(imageIn->widthStep != imageRGBA->widthStep
				   || imageIn->height != imageRGBA->height)
				{
					swReleaseImage(&imageRGBA);

					imageRGBA = swCreateImage(cvGetSize(imageIn),
											  imageIn->depth,
											  imageIn->nChannels );
					// update input size to force display change
					imageSize = cvGetSize(imageRGBA);
				}

				// convert to destination
				cvCopy(imageIn, imageRGBA);
			}
			else
			{
				fprintf(stderr, "VideoCapture::%s:%d : Acquisition problem : "
						"readImageRGB32() failed (SIZE PB: %dx%d)\n",
						__func__, __LINE__,
						imageIn->width, imageIn->height);
				usleep(500000);
				mImageBufferMutex.unlock();
				return -1;
			}
		}
		else
		{
			fprintf(stderr, "VideoCapture::%s:%d : Acquisition problem : readImageRaw() failed\n",
					__func__, __LINE__);
			usleep(500000);
			mImageBufferMutex.unlock();
			return -1;
		}
		mImageBufferMutex.unlock();

		return 0;
	}


	fprintf(stderr, "VideoCapture::%s:%d : timeout => return -1\n", __func__, __LINE__);
	return -1;
}

int VideoCaptureDoc::loadImage()
{
	if(!mpVAcq) {
		fprintf(stderr, "VideoCapture::%s:%d : no acq => grab failed\n", __func__, __LINE__);
		return -1;
	}
	mImageBufferMutex.lock();

	if(mpVAcq->grab()< 0)
	{
		fprintf(stderr, "VideoCapture::%s:%d : grab failed\n", __func__, __LINE__);
		mImageBufferMutex.unlock();
		return 0;
	}
	else
	{
		// unlock wait condition to unlock every WorkshopVideoCaptureThread
		//fprintf(stderr, "VideoCapture::%s:%d : acq OK => notify all\n", __func__, __LINE__);
		mWaitCondition.wakeAll();
	}
	mImageBufferMutex.unlock();

	return 1;
}
/// \todo FIXME not thread-safe
IplImage * VideoCaptureDoc::readImage()
{
	if(mpVAcq) {
		return mpVAcq->readImage();
	}

	return NULL;
	//return imageRGBA;
}

unsigned char * VideoCaptureDoc::getCurrentImageRGB()
{
	if(!imageRGBA) { return NULL; }

	return (unsigned char *)imageRGBA->imageData;
}

CvSize VideoCaptureDoc::getImageSize()
{
	return imageSize;
}


//-------------------- MASK MANAGEMENT
void VideoCaptureDoc::addMask(int x, int y, int width, int height)
{
	QRect * area = new QRect(x,y,width, height);
	pMaskList->append(area);
	saveMask();
}

QRect * VideoCaptureDoc::getMaskFromPos(int selX, int selY)
{
	// search for closest QRect
	QRect *closestArea = NULL;
	int dmin = 10000000;
	QList<QRect *>::iterator it;
	for(it = pMaskList->begin(); it!=pMaskList->end(); ++it)
	{
		QRect * area = (*it);
		if( selX >= area->x() && selX <= area->x()+area->width()
			&& selY >= area->y() && selY <= area->y()+area->height() )
		{
			int dx = selX - ( area->x() + area->width()/2 );
			int dy = selY - ( area->y() + area->height()/2 );

			int d = dx*dx+dy*dy;
			if(d<dmin) {
				dmin = d;
				closestArea = area;
			}
		}
	}
	return closestArea;
}

void VideoCaptureDoc::removeMask(QRect * area)
{
	pMaskList->removeOne(area);
	saveMask();
}

unsigned char * VideoCaptureDoc::getMaskBuffer()
{
	if(pMaskList->count()>0) {
		return mask;
	}

	return NULL;
}

void VideoCaptureDoc::clearMask()
{
	memset(mask, 255, currentPixel);

	SavePPMFile("full.pgm", false, imageSize, mask);
	setDetectionMask("full.pgm");

	QList<QRect *>::iterator it;
	for(it = pMaskList->begin(); it!=pMaskList->end(); )
	{
		QRect * area = (*it);

		it = pMaskList->erase(it);
		delete area;
	}

}

void VideoCaptureDoc::saveMask()
{
	memset(mask, 0, currentPixel * sizeof(unsigned char));

	// Add each area
	QList<QRect *>::iterator it;
	for(it = pMaskList->begin(); it!=pMaskList->end(); ++it)
	{
		QRect * area = (*it);
		for(int i=area->y(); i< area->y()+area->height(); i++)
		{
			int dec = i * imageSize.width + area->x();
			memset(mask+dec, 255, area->width() * sizeof(unsigned char));
		}
	}

	// save file in PGM image and call SwMotionDetector::LoadDetectionMask()
	SavePPMFile("mask.pgm", false /* = grayscaled */, imageSize, mask);
	setDetectionMask("mask.pgm");
}
