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
#include <q3filedialog.h>

// application specific includes
#include "swvideodetector.h"
#include "videocapture.h"

#include "opencvvideoacquisition.h"
#ifdef _V4L2
#include "V4L2Device.h"
#endif

// default and only constructor.
VideoCaptureDoc::VideoCaptureDoc()
{
	imageRGBA = NULL;
	currentPixel = 0;
	myVAcq=NULL;
	pImageTimer = NULL;
	m_running = m_run = false;
}


// default and only constructor.
VideoCaptureDoc::VideoCaptureDoc(VirtualDeviceAcquisition * vAcq)
{
	imageRGBA = NULL;
	mask = NULL;
	currentPixel = 0;
	myVAcq=vAcq;

	// alloc image size
	allocateImages();

	// Area mask list
	pMaskList = new Q3PtrList<QRect>;
	pMaskList->setAutoDelete(false);

	/* For devices without select() we use a timer. */
	int Framerate = 25;  // default value
	pImageTimer = NULL;

//	pImageTimer = new QTimer(this);
//	connect(pImageTimer, SIGNAL(timeout()), this, SLOT(loadImage()));
	//pImageTimer->start(1000 / Framerate);

	modified = false;

	// start background thread
	start();
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
	if(myVAcq) {
		delete myVAcq;
		myVAcq = NULL;
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
	if(myVAcq)
	{
		delete myVAcq;
		myVAcq = NULL;
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
	myVAcq = new OpenCVVideoAcquisition(dev);
#else
	myVAcq = new V4L2Device(dev);
#endif
	if(!myVAcq->isDeviceReady())
	{
		printf("(VirtualVideoAcquisition) : Video device init failed\n");
		return false;
	}
	else {
		printf("(VirtualVideoAcquisition) : Video device Init OK \n");
	}

	// start acquisition
	if(myVAcq->startAcquisition() < 0) {
		fprintf(stderr, "[VidCapture]::%s:%d : Error: canot initialize acquisition !\n", __func__, __LINE__);
		return false;
	}
	else {
		printf("Initialization : %d\n",  myVAcq->isAcquisitionRunning());
	}

	// alloc image size
	allocateImages();


	// Area mask list
	pMaskList = new Q3PtrList<QRect>;
	pMaskList->setAutoDelete(false);

	/* For devices without select() we use a timer. */
	int Framerate = 20;  // default value
	if(!pImageTimer)
		pImageTimer = new QTimer(this);
	connect(pImageTimer, SIGNAL(timeout()), this, SLOT(loadImage()));
//	pImageTimer->start(1000 / Framerate);

	modified = false;

	return true;
}

void VideoCaptureDoc::run()
{
	if(!myVAcq) return;

	m_run = m_running = true;
	while(m_run)
	{
		// grab images
//		fprintf(stderr, "VideoCaptureDoc::%s:%d : (threaded) load image size : %ld x %ld \n",
//				__func__, __LINE__,
//				imageSize.width, imageSize.height);
		loadImage();
	}

	m_run = m_running = false;
}


int VideoCaptureDoc::allocateImages()
{
	imageSize = myVAcq->getImageSize();
	currentPixel = (int)( imageSize.width * imageSize.height);

	fprintf(stderr, "VideoCaptureDoc::%s:%d : Image size : %d x %d x 4\n",
			__func__, __LINE__,
			imageSize.width, imageSize.height);

	imageRGBA = swCreateImage(imageSize, IPL_DEPTH_8U, 4);
	if( !(imageRGBA ))
	{
		printf("RGB image memory allocation failed\n");
		myVAcq->stopAcquisition();
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
	if(!myVAcq) {
		t_video_properties empty;
		memset(&empty, 0, sizeof(t_video_properties));
		return empty;
	}

	t_video_properties props = myVAcq->updateVideoProperties();
	return props;
}

int VideoCaptureDoc::setVideoProperties(t_video_properties props)
{
	if(!myVAcq) {
		fprintf(stderr, "VidCapDoc::%s:%d : no vid acq\n", __func__, __LINE__);
		return -1;
	}

	return myVAcq->setVideoProperties(props);
}
void VideoCaptureDoc::setDetectionMask(char * /*maskfile*/)
{
	// NOT IMPLEMENTED
}
/*  Set exposure value in us */
int VideoCaptureDoc::setexposure( int exposure_us )
{
	if(!myVAcq) return -1;
	t_video_properties props = myVAcq->updateVideoProperties();
	props.exposure = exposure_us/100; // in V4L2, exposure in in 100us step
	return myVAcq->setVideoProperties(props);
}
int VideoCaptureDoc::setgain( int gain)
{
	if(!myVAcq) return -1;
	t_video_properties props = myVAcq->updateVideoProperties();
	props.gain = gain; // in V4L2,
	return myVAcq->setVideoProperties(props);
}

int VideoCaptureDoc::setpicture(int br, int hue, int col, int cont, int white)
{
	if(!myVAcq) return -1;

//	return myVAcq->setpicture(br, hue, col, cont, white);
	t_video_properties props = myVAcq->updateVideoProperties();
	props.brightness = br;
	props.hue = hue;
	props.saturation = col;
	props.contrast = cont;
	props.hue = hue;

	return myVAcq->setVideoProperties(props);
}

int VideoCaptureDoc::getpicture(video_picture * pic)
{
	t_video_properties props = myVAcq->updateVideoProperties();
	pic->brightness = props.brightness * 16384;
	pic->hue = props.hue * 16384;
	pic->colour = props.saturation * 16384;
	pic->contrast = props.contrast * 16384;
	// FIXME
	return 0;
	//return myVAcq->getpicture(pic);
}
int VideoCaptureDoc::getcapability(video_capability * vc)
{
	t_video_properties props = myVAcq->updateVideoProperties();
	vc->maxwidth = props.max_width;
	vc->minwidth = props.min_width;
	vc->maxheight = props.max_height;
	vc->minheight = props.min_height;

	return 0;

	//return myVAcq->getcapability(vc);
}

int VideoCaptureDoc::changeAcqParams(tBoxSize newSize, int ch)
{
	t_video_properties props = myVAcq->updateVideoProperties();
	props.frame_width = newSize.width;
	props.frame_height = newSize.height;
	props.mode = ch;

	return myVAcq->setVideoProperties(props);
}

//thomas.
int VideoCaptureDoc::setChannel(int ch)
{
	t_video_properties props = myVAcq->updateVideoProperties();
	props.mode = ch; // FIXME
	return myVAcq->setVideoProperties(props);
}


bool VideoCaptureDoc::openDocument(const QString &filename, const char * /*format =0*/)
{

	QFile f( filename );
	if ( !f.open( QIODevice::ReadOnly ) )
		return false;
  /////////////////////////////////////////////////
  // TODO: Add your document opening code here
  /////////////////////////////////////////////////
	f.close();

  modified=false;
  m_filename=filename;
	m_title=QFileInfo(f).fileName();
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
	bool ret = mWaitCondition.wait(&mMutex,
								  200 // ms
								  );
	if(ret) {
		IplImage * imageIn = myVAcq->readImageRGB32();
		if(imageIn)
		{
//			fprintf(stderr, "VideoCapture::%s:%d : Acquisition ok : readImageRGB32() ok "
//					": %dx%dx%dx%d\n",
//					__func__, __LINE__,
//					imageIn->width, imageIn->height,
//					imageIn->nChannels, imageIn->depth);

			if(imageIn->width>0 && imageIn->height>0)
			{
				if(imageIn->width != imageRGBA->width
				   || imageIn->height != imageRGBA->height)
				{
					swReleaseImage(&imageRGBA);
					imageRGBA = swCreateImage(cvGetSize(imageIn), IPL_DEPTH_8U,
											  imageIn->nChannels == 1 ? 1 : 4 );
					// update input size to force display change
					imageSize = cvGetSize(imageRGBA);
				}

				swConvert(imageIn, imageRGBA);
			}
			else
			{
				fprintf(stderr, "VideoCapture::%s:%d : Acquisition problem : readImageRGB32() failed (SIZE PB)\n",
						__func__, __LINE__);
				usleep(500000);
				return -1;
			}
		}
		else
		{
			fprintf(stderr, "VideoCapture::%s:%d : Acquisition problem : readImageRGB32() failed\n",
					__func__, __LINE__);
			usleep(500000);
			return -1;
		}

		return 0;
	}

	return -1;
}

int VideoCaptureDoc::loadImage()
{
	myVAcq->grab(); // FIXME

	// unlock wait condition to unlock every WorkshopVideoCaptureThread
	mWaitCondition.wakeAll();

	return 1;
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
	QRect * area, *closestArea = NULL;
	int dmin = 10000000;
	for(area = pMaskList->first(); area; area = pMaskList->next())
	{
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
	pMaskList->remove(area);
	saveMask();
}

unsigned char * VideoCaptureDoc::getMaskBuffer()
{
	if(pMaskList->count()>0) {
		return mask;
	}

	return NULL;
}

void VideoCaptureDoc::clearMask() {
	memset(mask, 255, currentPixel);
	SavePPMFile("full.pgm", false, imageSize, mask);
	setDetectionMask("full.pgm");

	QRect * area;
	for(area = pMaskList->first(); area; area = pMaskList->next())
		pMaskList->remove(area);

}

void VideoCaptureDoc::saveMask() {
	memset(mask, 0, currentPixel * sizeof(unsigned char));

	// Add each area
	QRect * area;
	for(area = pMaskList->first(); area ; area = pMaskList->next())
		for(int i=area->y(); i< area->y()+area->height(); i++)
		{
			int dec = i * imageSize.width + area->x();
			memset(mask+dec, 255, area->width() * sizeof(unsigned char));
		}

	// save file in PGM image and call SwMotionDetector::LoadDetectionMask()
	SavePPMFile("mask.pgm", false /* = grayscaled */, imageSize, mask);
	setDetectionMask("mask.pgm");
}
