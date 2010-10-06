/***************************************************************************
    workshopmeasure.h  -  1D measure component for Piaf
                             -------------------
    begin                : ven nov 29 15:53:48 UTC 2002
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

#ifndef WORKSHOPMEASURE_H
#define WORKSHOPMEASURE_H

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


// Sisell Workshop include files
#include "sw_types.h"
#include "workshopcomponent.h"

/**
	\brief Measure (1D data) component
	*/
class WorkshopMeasure: public WorkshopComponent, public Q3ValueVector<double> 
{
	Q_OBJECT
	
public:
	//! Constructor
	WorkshopMeasure(QString label = "Untitled", int type = REAL_VALUE, int size = 1000)
		: WorkshopComponent(label, type), Q3ValueVector<double>(size)
	{
		static int WorkshopMeasure_last_id = 0;
		WorkshopMeasure_last_id++;
		m_ID = WorkshopMeasure_last_id;
		setComponentClass(CLASS_MEASURE);
	}

	inline int getID() { return m_ID; };

	~WorkshopMeasure(){}
	// overloaded functions
	// open function
	//bool open(const QString &filename, const char *format=0);
 	/** saves the document under filename and format.*/	
	void save();
	
	void close(){ printf("WorkshopMeasure close !\n"); }
	void saveAs();
	void openFile(const QString);
		
protected:
    //! Calls itemChanged()
    //virtual void curveChanged() { itemChanged(); }	
private:
	int m_ID;
	// Acquisition properties
	int 	CAN_size;
	int 	frequency;
};

#endif
