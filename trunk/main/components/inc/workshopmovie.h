/***************************************************************************
    workshopmovie.h  -  video file (movie) component for Piaf
                             -------------------
    begin                : ven nov 29 15:53:48 UTC 2002
    copyright            : (C) 2002 by Christophe SEYVE
    email                : christophe.seyve@sisell.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef WORKSHOPMOVIE_H
#define WORKSHOPMOVIE_H

// include files for Qt
#include <qobject.h>
#include <qwidget.h>
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


// Sisell Workshop include files
#include "sw_types.h"
#include "workshopcomponent.h"

/**
	This handles mainly the path of the file saved on disk.
	
	\brief Recorded video component.
	\author Christophe SEYVE \mail christophe.seyve@sisell.com
*/
class WorkshopMovie: public WorkshopComponent
{
	Q_OBJECT
	
public:
	//! Constructor
	///////////////////////
	WorkshopMovie(char * file, QString label = "Untitled",int type = VIRTUAL_VALUE)
		: WorkshopComponent(label, type)
	{
		setComponentClass(CLASS_VIDEO);
		if(strlen(file)<512)
			strcpy(FilePath, file);
		else
			FilePath[0] = '\0';
	}
	////////////////////////

	WorkshopMovie(QString label = "Untitled",int type = VIRTUAL_VALUE)
		: WorkshopComponent(label, type)
	{
		setComponentClass(CLASS_VIDEO);
		FilePath[0] = '\0';
	}
	
	char * file() { return FilePath; };
	~WorkshopMovie(){}
	// overloaded functions
	// open function
	//bool open(const QString &filename, const char *format=0);
    /** saves the document under filename and format.*/	
    void save();
	void close(){ printf("WorkshopMovie close !\n"); }
	void saveAs();
	void openFile(const QString);
private:
	char FilePath[512];
protected:
    //! Calls itemChanged()
    //virtual void curveChanged() { itemChanged(); }	
};

#endif
