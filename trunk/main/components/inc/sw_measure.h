/***************************************************************************
                          sw_measure.h  -  1D measure class for Piaf
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
#ifndef SW_MEASURE_H
#define SW_MEASURE_H

#include <qobject.h>
#include <qstring.h>
#include <qdatetime.h>
#include <qvaluevector.h>

#include "sw_types.h"

class SW_Measure : public QValueVector<double>
{	
public:
    SW_Measure();
	SW_Measure(int size);
   ~SW_Measure() {}
	   
	// properties handling
	void setLabel(const QString newLabel)		{ mLabel=newLabel; }
	//QString getLabel() 						{ return mLabel; }
	const char *getLabel()						{ return mLabel.latin1(); }
	void setShortLabel(const QString newLabel)	{ mShortLabel=newLabel; }
	//QString getShortLabel()					{ return mShortLabel; }
	const char *getShortLabel()					{ return mShortLabel.latin1(); }

	// Date handling
	void actuateDate()	{ mDate.currentDate(); }
	void setDate(QDate src)  { mDate = src; }
	QDate getDate() { return mDate; }
	// Validity handling
	void validate() 	{ mValidity = TRUE; }
	void unvalidate()	{ mValidity = FALSE; }
	bool isValid()		{ return mValidity; }
		
protected:
	// General properties (name, date of creation/modification...)
	QString mLabel;
	QString mShortLabel;
	QDate   mDate;
	int     mValidity;
	// Acquisition properties
	int 	CAN_size;
	int 	frequency;
};

#endif
