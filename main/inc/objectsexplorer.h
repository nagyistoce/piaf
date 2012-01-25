/***************************************************************************
    objectexplorer.h  -  components and tools explorer for Piaf
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
#ifndef OBJECTSEXPLORER_H
#define OBJECTSEXPLORER_H

#include <q3dockwindow.h>
#include <q3listview.h>
//Added by qt3to4:
#include <QContextMenuEvent>


// Item types definition
// --------------------------
#define ROOT_ITEM         	0
#define COMPONENT_ITEM    	1
#define TOOL_ITEM         	2
#define ACQUISITION_ITEM	3


// Component item
#define ROOT_COMPONENT_ITEM	100
#define ROOT_MEASURE_ITEM	110
#define ROOT_IMAGE_ITEM		120

#define ROOT_MOVIE_ITEM		130

#define MEASURE_ITEM      	111
#define IMAGE_ITEM			121
#define MOVIE_ITEM			131



// Acquisition classes
#define ROOT_ACQ_ITEM		200
#define ROOT_DATA_ACQ_ITEM	210
#define ROOT_VIDEO_ACQ_ITEM	220
#define DATA_ACQ_ITEM		211
#define VIDEO_ACQ_ITEM		221

// Tools classes
#define ROOT_TOOL_ITEM		300
#define PLOT2D_ITEM       	301
#define IMAGE_VIEW_ITEM   	302

class WorkshopComponent;
class VideoCaptureDoc;

/** 
	Main GUI explorer item, used for displaying and handling 
	an acquisition or data component or tool

	\brief Main GUI explorer
	\author Olivier VINE - Christophe SEYVE
*/
class ExplorerItem : public Q3ListViewItem
{
public:
	ExplorerItem(Q3ListView * parent, int type, int classe)
		: Q3ListViewItem(parent), itemType(type), itemClasse(classe)  { itemPtr = NULL; }
	ExplorerItem(Q3ListViewItem * parent, int type, int classe)
		: Q3ListViewItem(parent), itemType(type), itemClasse(classe)  { itemPtr = NULL; }
	~ExplorerItem() {};
	
	void *getItemPtr()	{ return itemPtr; }
	void setItemPtr(void *ptr) { itemPtr = ptr; }
	int getItemType() 	{ return itemType; }
	int getItemClass() 	{ return itemClasse; }
	
private:
	void *itemPtr;
	int itemType;
	int itemClasse;
};


/** 
	Main GUI explorer, for displaying and handling acquisition tools, 
	data components, processing tools.

	\brief Main GUI explorer
	\author Olivier VINE - Chrisotphe SEYVE
*/
class ObjectsExplorer : public Q3DockWindow
{
	Q_OBJECT

public:
	ObjectsExplorer(Place p=InDock, QWidget *parent=0, const char *name=0, Qt::WFlags f=0);	
	ObjectsExplorer(QWidget *parent=0, const char *name=0, Qt::WFlags f=0);
	~ObjectsExplorer();
	
	Q3ListView *view() { return pView; }
	void addWorkshopComponent(WorkshopComponent *wc);
	//////////////////////
	void delWorkshopComponent(WorkshopComponent *wc);
	//////////////////////
	void addVideoAcquisition(VideoCaptureDoc *sva, const char *name=0);
	
private:
	Q3ListView *pView;
	// Components 
	ExplorerItem *pRootComponents;
	ExplorerItem *pRootMeasure;
	ExplorerItem *pRootImage;
	ExplorerItem *pRootVideo;
	// Acquisitions
	ExplorerItem *pRootAcquisition;
	ExplorerItem *pRootDataAcq;
	ExplorerItem *pRootVideoAcq;
	// Tools
	ExplorerItem *pRootTools;

private:
	Q3ListView *initListView();
	void contextMenuEvent(QContextMenuEvent *);
};

#endif
