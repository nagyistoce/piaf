/***************************************************************************
	objectexplorer.cpp  -  components and tools explorer for Piaf
							 -------------------
	begin                : ven nov 29 15:53:48 UTC 2002
	copyright            : (C) 2002 by Olivier Viné & Christophe Seyve
	email                : olivier.vine@sisell.com
							cseyve@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "piaf-common.h"

// QT includes
#include <qpixmap.h>
#include <q3popupmenu.h>

//Added by qt3to4:
#include <QContextMenuEvent>

// PIAF includes
#include "workshopcomponent.h"
#include "workshoptool.h"
#include "videocapture.h"
#include "objectsexplorer.h"


ObjectsExplorer::ObjectsExplorer(Place p, QWidget *parent, const char *name, Qt::WFlags f)
				:Q3DockWindow(p, parent, name, f)
{
	setResizeEnabled(TRUE);
	pView = initListView();
	setWidget(pView);
}

ObjectsExplorer::ObjectsExplorer(QWidget *parent, const char *name, Qt::WFlags f)
				:Q3DockWindow(InDock, parent, name, f)
{
	setResizeEnabled(TRUE);
	pView = initListView();
	setWidget(pView);
}

Q3ListView *ObjectsExplorer::initListView()
{
	Q3ListView *lv;

	lv = new Q3ListView(this);
	lv->addColumn("Explorer");
	lv->setRootIsDecorated(TRUE);
	chdir(BASE_DIRECTORY);

	// init roots
	// ----------------------------------------------
	/*
	// Components items
	pRootComponents = new ExplorerItem(lv, ROOT_ITEM, ROOT_COMPONENT_ITEM);
	pRootComponents->setText(0, tr("Components"));
	iconPixmap.load("images/topLogos/components.png");
	pRootComponents->setPixmap(0, iconPixmap);

	pRootMeasure = new ExplorerItem(pRootComponents, ROOT_ITEM, ROOT_MEASURE_ITEM);
	pRootMeasure->setText(0, tr("Measures"));
	iconPixmap.load("images/topLogos/measures.png");
	pRootMeasure->setPixmap(0, iconPixmap);

	pRootImage = new ExplorerItem(pRootComponents, ROOT_ITEM, ROOT_IMAGE_ITEM);
	pRootImage->setText(0, tr("Images"));
	iconPixmap.load("images/topLogos/imagegallery.png");
	pRootImage->setPixmap(0, iconPixmap);

	pRootVideo = new ExplorerItem(pRootComponents, ROOT_ITEM, ROOT_VIDEO_ITEM);
	pRootVideo->setText(0, tr("Videos"));
	iconPixmap.load("images/topLogos/video.png");
	pRootVideo->setPixmap(0, iconPixmap);

	// Acquisitions items
	pRootAcquisition = new ExplorerItem(lv, ROOT_ITEM, ROOT_ACQ_ITEM);
	pRootAcquisition->setText(0, tr("Acquisitions"));
	iconPixmap.load("images/pixmaps/Cmesure.xpm");
	pRootAcquisition->setPixmap(0, iconPixmap);

	pRootDataAcq = new ExplorerItem(pRootAcquisition, ROOT_ITEM, ROOT_DATA_ACQ_ITEM);
	pRootDataAcq->setText(0, tr("Data acquisition"));
	iconPixmap.load("images/topLogos/dataAcq.png");
	pRootDataAcq->setPixmap(0, iconPixmap);

	pRootVideoAcq = new ExplorerItem(pRootAcquisition, ROOT_ITEM, ROOT_VIDEO_ACQ_ITEM);
	pRootVideoAcq->setText(0, tr("Video acquisition"));
	iconPixmap.load("images/topLogos/videoAcq.png");
	pRootVideoAcq->setPixmap(0, iconPixmap);

	// Tools items
	pRootTools = new ExplorerItem(lv, ROOT_ITEM, ROOT_TOOL_ITEM);
	pRootTools->setText(0, tr("Tools"));
	iconPixmap.load("images/pixmaps/tools.png");
	pRootTools->setPixmap(0, iconPixmap);
*/
	// Simplified version
	// File items
	pRootComponents = NULL;
	pRootMeasure = NULL;

	pRootImage = new ExplorerItem(lv, ROOT_ITEM, ROOT_IMAGE_ITEM);
	pRootImage->setText(0, tr("Images"));
	{
		QPixmap iconPixmap;
		iconPixmap.load("images/pixmaps/stillimage.png");
		pRootImage->setPixmap(0, iconPixmap);
	}
	pRootVideo = new ExplorerItem(lv, ROOT_ITEM, ROOT_MOVIE_ITEM);
	pRootVideo->setText(0, tr("Movies"));
	{
		QPixmap iconPixmap;
		iconPixmap.load("images/pixmaps/movie.png");
		pRootVideo->setPixmap(0, iconPixmap);
	}
	// Acquisitions items
	pRootAcquisition = NULL;
	pRootDataAcq = NULL;

	pRootVideoAcq = new ExplorerItem(lv, ROOT_ITEM, ROOT_VIDEO_ACQ_ITEM);
	pRootVideoAcq->setText(0, tr("Video acquisition"));
	{ QPixmap iconPixmap;
	iconPixmap.load("images/pixmaps/camera-web.png");
	pRootVideoAcq->setPixmap(0, iconPixmap);}
	// Tools items
	pRootTools = NULL;
/*	pRootTools = new ExplorerItem(lv, ROOT_ITEM, ROOT_TOOL_ITEM);
	pRootTools->setText(0, tr("Tools"));
	iconPixmap.load("images/topLogos/tools.png");
	pRootTools->setPixmap(0, iconPixmap);
*/
	return lv;
}

ObjectsExplorer::~ObjectsExplorer()
{
	delete pView;
}


void ObjectsExplorer::addWorkshopComponent(WorkshopComponent *wc)
{
	ExplorerItem * qlvi;
	QPixmap pixmap;

	chdir(BASE_DIRECTORY);
	QString label = wc->getLabelString();
	int cClass = wc->getComponentClass();
	switch(cClass)
	{
		case CLASS_MEASURE:
			pixmap.load(BASE_DIRECTORY "images/mesurelogo_16.png");
			if(!pRootMeasure) return;

			qlvi = new ExplorerItem(pRootMeasure, COMPONENT_ITEM, MEASURE_ITEM);
			qlvi->setItemPtr(wc);
			qlvi->setText(0, label);
			qlvi->setPixmap(0, pixmap);
			pRootMeasure->setOpen(true);
			break;

		case CLASS_IMAGE:
			pixmap.load("images/pixmaps/stillimage16.png");
			qlvi = new ExplorerItem(pRootImage, COMPONENT_ITEM, IMAGE_ITEM);
			qlvi->setItemPtr(wc);
			qlvi->setText(0, label);
			qlvi->setPixmap(0, pixmap);
			pRootImage->setOpen(true);
			break;
		case CLASS_VIDEO:
			pixmap.load("images/pixmaps/movie16.png");
			qlvi = new ExplorerItem(pRootVideo, COMPONENT_ITEM, MOVIE_ITEM);
			qlvi->setItemPtr(wc);
			qlvi->setText(0, label);
			qlvi->setPixmap(0, pixmap);
			pRootVideo->setOpen(true);
			break;
		default:
			break;
	}
}

///////////////////////////////
void ObjectsExplorer::delWorkshopComponent(WorkshopComponent * /*wc (unused)*/)
{
//	QString label = wc->getlabel();
//	int cClass = wc->getComponentClass();

//	printf("delWorkshopComponent!\n");
}
///////////////////////////////




void ObjectsExplorer::addVideoAcquisition(VideoCaptureDoc *sva, const char *name)
{
	ExplorerItem * qlvi;
	QPixmap pixmap;

	pixmap.load("images/pixmaps/camera-web16.png");
	qlvi = new ExplorerItem(pRootVideoAcq, ACQUISITION_ITEM, VIDEO_ACQ_ITEM);
	qlvi->setItemPtr(sva);
	if(name)
		qlvi->setText(0, name);
	else
		qlvi->setText(0, "Video");

	qlvi->setPixmap(0, pixmap);
}

void ObjectsExplorer::contextMenuEvent( QContextMenuEvent *e )
{
	e->ignore();
}
