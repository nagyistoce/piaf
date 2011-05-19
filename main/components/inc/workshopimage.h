/***************************************************************************
	  workshopimage.h  -  image component for Piaf
							 -------------------
	begin                : ven nov 29 15:53:48 UTC 2002
	copyright            : (C) 2002 by Olivier Viné
	email                : olivier.vine@sisell.com
	copyright            : (C) 2002-2011 by Christophe Seyve
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

#ifndef WORKSHOPIMAGE_H
#define WORKSHOPIMAGE_H

// include files for Qt
#include <qobject.h>
#include <qwidget.h>
#include <qpainter.h>
#include <q3ptrlist.h>
#include <qfile.h>
#include <qstring.h>
#include <qdatastream.h>
#include <q3popupmenu.h>
#include <q3filedialog.h>
#include <qdir.h>
#include <q3valuevector.h>
#include <qdatetime.h>
#include <qimage.h>
#include <qlist.h>
//Added by qt3to4:
#include <QPaintEvent>
#include <QMouseEvent>

#include "imagewidget.h"


// Sisell Workshop include files
#include "sw_types.h"
#include "workshopcomponent.h"

/**
	\brief Image component for Piaf
*/
class WorkshopImage: public WorkshopComponent, public QImage
{
	Q_OBJECT

public:
	//! Constructor
	///////////////////////
	WorkshopImage(QImage image, QString label = "Untitled",int type = VIRTUAL_VALUE)
		: WorkshopComponent(label, type), QImage(image)
	{
		static int WorkshopImage_last_id = 0;
		WorkshopImage_last_id++;
		m_ID = WorkshopImage_last_id;
		setComponentClass(CLASS_IMAGE);
	}
	////////////////////////

	WorkshopImage(QString label = "Untitled",int type = VIRTUAL_VALUE)
		: WorkshopComponent(label, type), QImage()
	{
		setComponentClass(CLASS_IMAGE);
	}
	int getID() { return m_ID; };

	~WorkshopImage(){}
	// overloaded functions
	// open function
	//bool open(const QString &filename, const char *format=0);
	/** saves the document under filename and format.*/
	void save();
	void close(){ printf("WorkshopImage close !\n"); }
	void saveAs();
	void openFile(const QString);

protected:
	int m_ID;
	//! Calls itemChanged()
	//virtual void curveChanged() { itemChanged(); }
};

#endif

