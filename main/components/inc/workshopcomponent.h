/***************************************************************************
                          workshopcomponent.h  -  description
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

#ifndef WORKSHOPCOMPONENT_H
#define WORKSHOPCOMPONENT_H

// QT include 
#include <qobject.h>
#include <qstring.h>
#include <qdir.h>
#include <q3filedialog.h>
#include <QDate>

// classes of components
#define NB_COMPONENT_CLASS 3
#define CLASS_UNDEFINED -1
#define CLASS_MEASURE  	0
#define CLASS_IMAGE    	1
#define CLASS_VIDEO    	2

// types of component
#define REAL_VALUE     0
#define VIRTUAL_VALUE  1

/**
	This class handles components type, date ... 
	It is inherited by all component classes (for sigal, image, ...).

	\brief Base component class for Piaf data storage. 
	\author Olivier VINE  \mail olivier.vine@sisell.com
*/	

class WorkshopComponent : public QObject
{
	Q_OBJECT
public:
	WorkshopComponent(QString label = "Untitled", int type = REAL_VALUE);
	~WorkshopComponent() {
		
		//if(savedialog)
		//	delete savedialog;
		}

	// handling label and short label of component
	// ---------------------------------------------------
	void setLabel(const QString newLabel)		{ swcLabel = newLabel; }
	const char *getLabel()						{ return swcLabel.latin1(); }
	const QString getLabelString() 					{ return swcLabel; };
	void setShortLabel(const QString newLabel)	{ swcShortLabel=newLabel; }
	const char *getShortLabel()					{ return swcShortLabel.latin1(); }

	// Date handling
	// ---------------------------------------------------
	void actuateDate()	{ swcDate.currentDate(); }
	void setDate(QDate src)  { swcDate = src; }
	QDate getDate() { return swcDate; }

	// Validity handling
	// ---------------------------------------------------
	void validate() 	{ swcValidity = TRUE; }
	void unvalidate()	{ swcValidity = FALSE; }
	bool isValid()		{ return swcValidity; }

	// handling type of component
	// ---------------------------------------------------
	void setCType(int type) { swcType = type; }
	int  getCType()			{ return swcType; }
	bool isReal() 			{ return (swcType==REAL_VALUE); }

	// handling file path
	// ---------------------------------------------------
	void setPathName(const QString &name) 	{ swcPathName = name;}
    const QString& getPathName() const 		{ return swcPathName; }
	
	// handling file name
	// ---------------------------------------------------	
    void setFileName(const QString &name)	{ swcFileName = name; }
    const QString& getFileName() const 		{ return swcFileName; }
	void selectPathName();	

	// handling modified flag
	// ---------------------------------------------------
	void setModified(bool _m=true){ swcModified=_m; };
	bool isModified(){ return swcModified; };

	// handling component class
	// ---------------------------------------------------
	int setComponentClass(int newClass);
	int getComponentClass() { return swcClass; }
	
protected :
	// General properties (name, date of creation/modification...)
	QString swcLabel;
	QString swcShortLabel;
	QDate   swcDate;
	int     swcValidity;

    bool 	swcModified;
    QString swcPathName;
    QString swcFileName;
	int 	swcType;  // REAL or VIRTUAL	

    /////////////////////
   // QFileDialog *savedialog;
    ////////////////////
private:
	// Class of component : CLASS_MEASURE, CLASS_IMAGE 
	int     swcClass;
	
private:
	void initDefaults();

};
#endif
