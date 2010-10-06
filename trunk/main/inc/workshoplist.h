/***************************************************************************
                          workshoplist.h  -  description
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

#ifndef WORKSHOPLIST_H
#define WORKSHOPLIST_H

#include <qwidget.h>
#include <q3listview.h>
#include <qpixmap.h>
#include <qobject.h>
#include <qstringlist.h>

#define NB_ITEM_SISELL_WSHP	15
#define ID_SISELL_WSHP			0
#define ID_FAST_VIEW				100
#define ID_ANALOG_IN				110
#define ID_DIGITAL_IN			120
#define ID_COUNTER				130
#define ID_SIGNAL_GEN			200
#define ID_ANALOG_OUT			210
#define ID_DIGITAL_OUT			220
#define ID_TIMER					230
#define ID_INSTRUMENTS			300
#define ID_OSCILLOSCOPE			310
#define ID_FFT_ANALYSER			320
#define ID_VIDEO					400
#define ID_VIDEO_VIEW			410
#define ID_MOTION					420
#define ID_RECORDER				430

#define START_ON_TOP		"ON_TOP"
#define STOP_ON_TOP		"END_ON_TOP"
#define START_LIBRARIES	"LIBRARIES"
#define STOP_LIBRARIES		"END_LIBRARIES"

#include "sw_library.h"

struct wlistItem
{
	unsigned long ident;
	unsigned long IDparent;
	const char *caption;
	const char *pixmap;
};

class WorkshopList : public Q3ListView
{
	Q_OBJECT

public:
	WorkshopList();
	WorkshopList(QWidget *parent, const char *name);
	~WorkshopList();
	
private:
	void createSisellWorkshop();
	void createTopLib();

	void createLibraries(Q3ListViewItem * p, QString tk);
	void createQTViewer(Q3ListViewItem *parent);
	
private:
	Q3ListViewItem *SWItemList[NB_ITEM_SISELL_WSHP];	// Sisell Workshop List of Items


};

#endif
