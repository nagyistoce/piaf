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



// default and only constructor.
VideoCaptureDoc::VideoCaptureDoc()
{
	imageRGB = NULL;
	currentPixel = 0;
	myVAcq=NULL;
	pImageTimer = NULL;
	m_running = m_run = false;
}


// default and only constructor.
VideoCaptureDoc::VideoCaptureDoc(SwVideoAcquisition * vAcq)
{
	imageRGB = NULL;
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
	pImageTimer = new QTimer(this);
	connect(pImageTimer, SIGNAL(timeout()), this, SLOT(loadImage()));
	//pImageTimer->start(1000 / Framerate);

	modified=false;

	// start background thread
	start();
}

// destructor
VideoCaptureDoc::~VideoCaptureDoc()
{
	fprintf(stderr, "VideoCaptureDoc::~VideoCaptureDoc()\n");
	fflush(stderr);
	// wait for finishing thread
	m_run = false;
	while(m_run) {
		sleep(1);
	}



	// RGB32 image
	if(imageRGB)
		delete [] imageRGB;
	imageRGB = NULL;
	// stop acquisition
	if(myVAcq)
		delete myVAcq;
	myVAcq = NULL;
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
	if(imageRGB)
	{
		delete imageRGB;
		imageRGB = NULL;
	}
	tBoxSize size;
#define DEFAULT_WIDTH 320
#define DEFAULT_HEIGHT 240

	size.width  = DEFAULT_WIDTH;
	size.height = DEFAULT_HEIGHT;
	char txt[128];
	sprintf(txt, "/dev/video%d", dev);

	printf("----- CREATE NEW WINDOW FOR DEVICE '%s' -----\n", txt);
	myVAcq = new SwVideoAcquisition(dev, size);

	if(!myVAcq->VDIsInitialised())
	{
		printf("(SwVideoAcquisition) : Video device init failed\n");
		return false;
	}
	else
		printf("(SwVideoAcquisition) : Video device Init OK \n");
	 // init de l'acquisition
	if(myVAcq->VAInitialise(true)<0) {
		printf("Error: canot initialize acquisition !\n");
		return false;
	}
	else
		printf("Initialization : %d\n",  myVAcq->AcqIsInitialised());

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

	fprintf(stderr, "VideoCaptureDoc::%s:%d : Image size : %ld x %ld \n",
			__func__, __LINE__,
			imageSize.width, imageSize.height);

	imageRGB = new unsigned char[currentPixel*4];
	if( !(imageRGB ))
	{
		printf("RGB image memory allocation failed\n");
		myVAcq->VAcloseVD();
		return 0;
	}
	// Clear buffer for security
	memset(imageRGB , 0, currentPixel * 4 * sizeof(unsigned char));

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

void VideoCaptureDoc::setDetectionMask(char * /*maskfile*/)
{
	// NOT IMPLEMENTED
}

int VideoCaptureDoc::setpicture(int br, int hue, int col, int cont, int white)
{
	return myVAcq->setpicture(br, hue, col, cont, white);
}
int VideoCaptureDoc::getpicture(video_picture * pic)
{
	return myVAcq->getpicture(pic);
}
int VideoCaptureDoc::getcapability(video_capability * vc)
{
	return myVAcq->getcapability(vc);
}
int VideoCaptureDoc::changeAcqParams(tBoxSize newSize, int ch)
{
	int ret = myVAcq->changeAcqParams(newSize, ch);
	if(ret)
	{
		if(imageRGB) delete [] imageRGB;
		if(mask) delete [] mask;

		allocateImages();
		return 1;
	}
	return 0;
}

//thomas.
int VideoCaptureDoc::setChannel(int ch)
{
	return myVAcq->setChannel(ch);
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


int VideoCaptureDoc::loadImage()
{
	if((myVAcq->readImageRGB32( imageRGB, NULL, true))<0) {
		fprintf(stderr, "VideoCapture::%s:%d : Acquisition problem : readImageRGB32() failed\n",
				__func__, __LINE__);
		return 0;
	}

	emit documentChanged();

	return 1;
}

unsigned char * VideoCaptureDoc::getCurrentImageRGB()
{
	return imageRGB;
}

tBoxSize VideoCaptureDoc::getImageSize()
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
	if(pMaskList->count()>0)
		return mask;
	else
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
