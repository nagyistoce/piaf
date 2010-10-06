/***************************************************************************
	  previewimage.cpp  -  Image preview for Piaf project
							 -------------------
	begin                : July 2002
	copyright            : (C) 2002 by Olivier Viné
	email                : olivier.vine@sisell.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <QWorkspace>

#include "previewimage.h"
#include "videocapture.h"

PreviewImage::PreviewImage(QWidget* parent, const char *name, Qt::WidgetAttribute wflags):
		WorkshopTool(parent, name, wflags)
{
	pWin->setCaption(name);

	initTool();
	initVars();
	initPlot();

	((QWorkspace *)parent)->addWindow((QWidget *)pWin);

	connect(this, SIGNAL(newImage(int)), SLOT(onNewImage(int)));
	QPixmap winIcon = QPixmap(BASE_DIRECTORY "images/pixmaps/stillimage.png");
	display()->setIcon(winIcon);
	pWin->show();
}

void PreviewImage::initTool()
{
	// enable displaying measures
	enableDisplay(CLASS_IMAGE);
	// disable all ohter actions
	disableCreate(CLASS_IMAGE);
	disableDisplay(CLASS_MEASURE);
	disableCreate(CLASS_MEASURE);
	disableDisplay(CLASS_VIDEO);
	disableCreate(CLASS_VIDEO);
}

void PreviewImage::initPlot()
{
	pView = new ImageView(pWin,"image view", Qt::WRepaintNoErase);
	//only for Q3MainWin	pWin->setCentralWidget(pView);
	pWin->setFixedSize(200, 150);
	pView->setFixedSize(200, 150);
	pView->setRefImage(&pImage);
}

void PreviewImage::initVars()
{
	pImageTimer = NULL;
	fDisplayVideo = false;
}


void PreviewImage::connect2video(VideoCaptureDoc *src)
{

	/* For devices without select() we use a timer. */
	// Rq. (OLV) : ce n'est qu'un preview : on peut utiliser le timer en permanence...
	connect(src, SIGNAL(documentChanged()), this, SLOT(onNewVideoImage()));

	// updating pVideoAcq
	pVideoAcq = src;
	// setting flag
	fDisplayVideo = true;
	// creating videoimage if necessary
	videoSize = pVideoAcq->getImageSize();
	if((pImage.width()!=(int)videoSize.width)||(pImage.height()!=(int)videoSize.height))
		pImage.create(videoSize.width, videoSize.height, 32);
}

void PreviewImage::unconnectVideo()
{
	// stopping timer...
	fDisplayVideo = false;
}

// SLOTS
void PreviewImage::onNewImage(int ptr)
{
	if(!ptr) return; //

	// Get image from ID
	 WorkshopImage *pWi = NULL;
	for ( pWi = WIList.first(); pWi; pWi = WIList.next() ) {
		if(pWi->getID() == ptr) {
			break;
		}
	}
	if(!pWi) return;
	pImage = pWi->copy();

	if(pWi) {
		// change image ratio : max width or height is 200
		int twidth = pImage.width();
		int theight = pImage.height();
		if(twidth != 0 && theight != 0) {
			if(twidth > theight)
			{
				theight = 200 * theight / twidth;
				twidth = 200;
			}
			else
			{
				twidth = 200 * twidth / theight;
				theight = 200;
			}
			pView->setFixedSize(twidth, theight);
			pWin->setFixedSize(twidth, theight);
		}
	}

	pView->update();
}

void PreviewImage::onNewVideoImage()
{
	if(fDisplayVideo)
	{
		memcpy(pImage.bits(), pVideoAcq->getCurrentImageRGB(), pImage.width()*pImage.height()*4);
		if(!pView->isHidden())
			pView->update();
	}
}
