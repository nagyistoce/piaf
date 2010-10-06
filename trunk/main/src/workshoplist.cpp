/***************************************************************************
                          workshoplist.cpp  -  description
                             -------------------
    begin                : Mon Apr 8 2002
    copyright            : (C) 2002 by Olivier Vine
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

#include "workshoplist.h"
//Added by qt3to4:
#include <QPixmap>
#include <Q3TextStream>
const wlistItem swList[NB_ITEM_SISELL_WSHP]=
	{
	 {ID_FAST_VIEW, 	ID_SISELL_WSHP, 	"Viewers",		 		"images/signal.png"},
    {ID_SIGNAL_GEN,	ID_SISELL_WSHP,	"Signal Generation",	"images/signalGen.png"},
    {ID_INSTRUMENTS,	ID_SISELL_WSHP,	"Instruments",			"images/instruments.png"},
    {ID_VIDEO,			ID_SISELL_WSHP,	"Video Acquisition",	"images/video.png"},
    {ID_ANALOG_IN, 	ID_FAST_VIEW, 		"Analog In", 			"images/analogIn.png"},
    {ID_DIGITAL_IN,	ID_FAST_VIEW, 		"Digital In", 			"images/digitalIn.png"},
    {ID_COUNTER,   	ID_FAST_VIEW, 		"Counter",				"images/counter.png"},
    {ID_ANALOG_OUT,	ID_SIGNAL_GEN,		"Analog Out",			"images/analogOut.png"},
    {ID_DIGITAL_OUT,	ID_SIGNAL_GEN,		"Digital Out",			"images/digitalOut.png"},
    {ID_TIMER,			ID_SIGNAL_GEN,		"Timer",					"images/timer.png"},
    {ID_OSCILLOSCOPE,ID_INSTRUMENTS,	"Oscilloscope",		"images/oscilloscope.png"},
    {ID_FFT_ANALYSER,ID_INSTRUMENTS,	"FFT Analyser",		"images/fft.png"},
    {ID_VIDEO_VIEW,	ID_VIDEO,			"Video Viewer",		"images/videoView.png"},
    {ID_MOTION,		ID_VIDEO,			"Motion Detector",	"images/motionDetection.png"},
    {ID_RECORDER,		ID_VIDEO,			"Video Recorder",		"images/videoRec.png"},
   };


WorkshopList::WorkshopList(QWidget *parent, const char *name):Q3ListView(parent, name)
{
	for(int i=0; i<NB_ITEM_SISELL_WSHP; i++)
		SWItemList[i] = NULL;
	addColumn( "" );
   setRootIsDecorated( TRUE );

	createSisellWorkshop();
	
	createTopLib();

}

WorkshopList::~WorkshopList()
{
	for(int i=0; i<NB_ITEM_SISELL_WSHP; i++)
	{
		if(SWItemList[i])	
			delete SWItemList[i];
	}
}

void WorkshopList::createSisellWorkshop()
{
	unsigned long i, j;
	Q3ListViewItem *tmpParent;
	QPixmap tmpPixmap;
		
	// Create first Item (SISELL WorkShop)
	Q3ListViewItem *sisellItem = new Q3ListViewItem( this, "\tSisell WorkShop");
	tmpPixmap.load("images/sisell.png");
	sisellItem->setPixmap(0, tmpPixmap);

	for(i=0; i<NB_ITEM_SISELL_WSHP; i++)
	{
		// search parent
		tmpParent = NULL;
		if(swList[i].IDparent==ID_SISELL_WSHP)
			tmpParent = sisellItem;
		else
		{
			if(i==0)
				tmpParent = sisellItem;
			else
			{
				// searching parent in already build items
				for(j=0;j<i;j++)
				{
					if(swList[j].ident==swList[i].IDparent)
					{
						tmpParent = SWItemList[j];
						break;
					}
				}
				if(!tmpParent)
					tmpParent = sisellItem;
			}
		}
		
		// create new item
		SWItemList[i] = new Q3ListViewItem(tmpParent, swList[i].caption);
						
		// adding logo
		tmpPixmap.load(swList[i].pixmap);
		SWItemList[i]->setPixmap(0, tmpPixmap);
			
	}		
}

void WorkshopList::createLibraries(Q3ListViewItem * parent, QString topKey)
{
	QFile f("database/libItems.dat");
	QPixmap			tmpPixmap;
	QStringList 	tmpList;
	if( f.open(QIODevice::ReadOnly) ){
		Q3TextStream t(&f);

		QString s;

		while( (!t.atEnd()) && (s != START_LIBRARIES) ){
			s = t.readLine();
		}

		while( (!t.atEnd()) && (s != STOP_LIBRARIES) ){
			s = t.readLine();
			if (s != STOP_LIBRARIES){				
				tmpList = QStringList::split("\t", s);
				if(tmpList[LIBPARENT_INDEX]==topKey){
					SW_Library * tmpLib = new SW_Library( parent, tmpList);
					// for debug purpose
					if(tmpList[LIBNAME_INDEX]=="QT")
						tmpLib->expand();
				}
			}
		}

		f.close();
	}
}

void WorkshopList::createTopLib()
{
	// Create first Item : LIBRARIES
	Q3ListViewItem *libItem = new Q3ListViewItem( this, "\tLibraries");
	QPixmap libImage("images/libraries.png");
	libItem->setPixmap(0, libImage);

	// Create Top Items
	QFile f("database/libItems.dat");
	QStringList		topItemsList;
	QPixmap			tmpPixmap;

	if( f.open(QIODevice::ReadOnly) ){
		Q3TextStream t(&f);

		QString s;

		while( (!t.atEnd()) && (s != START_ON_TOP) ){
			s = t.readLine();
		}

		while( (!t.atEnd()) && (s != STOP_ON_TOP) ){
			s = t.readLine();
			if (s != STOP_ON_TOP)
				topItemsList.append(s);
		}

		f.close();

		for(QStringList::Iterator it = topItemsList.begin(); it != topItemsList.end(); ++it){
			QStringList tmpList = QStringList::split("\t",*it);
			Q3ListViewItem * topItem = new Q3ListViewItem( libItem, tmpList[TOPNAME_INDEX]);
			QString sp = TOPLOGOS_PATH + tmpList[TOPLOGO_INDEX];
			if( tmpPixmap.load(sp) )
				topItem->setPixmap(0, tmpPixmap);

			createLibraries(topItem, tmpList[TOPKEY_INDEX]);
		}
	}
}
