/***************************************************************************
	workshoptool.h  -  generic tool window for Piaf
							 -------------------
	begin                : ven nov 29 15:53:48 UTC 2002
	copyright            : (C) 2002 by Olivier Viné - Christophe SEYVE
	email                : olivier.vine@sisell.com / cseyve@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef WORKSHOPTOOL_H
#define WORKSHOPTOOL_H

// include files for Qt
#include <qwidget.h>
#include <q3mainwindow.h>
#include <qaction.h>
#include <q3toolbar.h>

//#include <qptrlist.h>
#include <q3intdict.h>
//Added by qt3to4:
#include <QResizeEvent>
#include <Q3PtrList>


#define MODE_SMARTZOOM		0
#define MODE_SELECT			1
#define MODE_ADD_RECT		2
#define MODE_ADD_FREE		3
#define MODE_ZOOM_IN		4
#define MODE_ZOOM_OUT		5
#define MODE_MOVE			6
#define MODE_PICKER			7


// Sisell Workshop include files
//#include "qwt_ext_plot.h"
//#include <qwt_math.h>
#include "workshopimage.h"
#include "workshopmeasure.h"
#include "workshopcomponent.h"


/**
	\brief Modified QWidget for tools' windows in MDI.
	\author Christophe SEYVE \mail cseyve@free.fr
	*/
class WorkshopMainWindow : public QWidget
{
	Q_OBJECT
public:
	WorkshopMainWindow(QWidget *parent=0, const char *name=0,
										 Qt::WidgetAttribute f=Qt::WA_DeleteOnClose)
		: QWidget(parent, name)
	{
		setAttribute(f, true);
	};

signals:

	/*!
	  A signal which is emitted when the window is resized.
	  \param e Resize event object
	 */
	void signalResizeEvent( QResizeEvent *e);
private:
	void resizeEvent( QResizeEvent * e)
	{
		emit signalResizeEvent(e);
	};
};

/**
	\brief Base class (inherited) for every tool.
	\author Olivier VINE \mail olivier.vine@sisell.com
	*/
class WorkshopTool : public QObject
{
	Q_OBJECT
public:
	WorkshopTool(QWidget* parent, const char *name, Qt::WidgetAttribute wflags);

	~WorkshopTool() {};

	// handling images
	void addNewImage(WorkshopImage *pWi);
	void removeImage(WorkshopImage *pWi);

	bool haveImage	(WorkshopImage *pWi);
	bool displayNewImage() { return flagDisplayNewImage; };
	void enableDisplayNewImage() { flagDisplayNewImage = true; };
	void disableDisplayNewImage() { flagDisplayNewImage = false; };

	void addNewMeasure(WorkshopMeasure *);
	void removeMeasure(WorkshopMeasure *);
	bool haveMeasure (WorkshopMeasure * pWm);
	void enableDisplayNewMeasure() { flagDisplayNewMeasure = true; };
	void disableDisplayNewMeasure() { flagDisplayNewMeasure = false; };
	bool displayNewMeasure() { return flagDisplayNewMeasure; };

	// handling components
	void addNewComponent(WorkshopComponent *pWc);
	void removeComponent(WorkshopComponent *pWc);
	bool haveComponent(WorkshopComponent *pWc);
	bool displayNewComponent(int cClass);
	void enableDisplayNewComponent(int cClass);
	void disableDisplayNewComponent(int cClass);

	// handling display
//	Q3MainWindow *display() { return pWin; };
	QWidget *display() { return pWin; };

	// General properties handling
	void enableDisplay(int compType)  { flagsDisplay[compType] = true; }
	void disableDisplay(int compType) { flagsDisplay[compType] = false; }
	void enableCreate(int compType)   { flagsCreate[compType] = true; }
	void disableCreate(int compType)  { flagsCreate[compType] = false; }
	bool canDisplay(int compType)	  { return flagsDisplay[compType]; }
	bool canCreate(int compType)	  { return flagsCreate[compType]; }
	int getID() { return m_ID; };
protected:
	int m_ID; // unique ID
	//QIntDict<WorkshopReport> WMDict;
	Q3PtrList<WorkshopImage>   WIList;
	Q3PtrList<WorkshopMeasure>   WMList;
	WorkshopMainWindow *pWin;
	QWidget *pWorkspace;

signals:
	void newMeasure(int ptrmeas);
	void rmMeasure(int ptrmeas);
	void newImage(int ptrimage);
	void rmImage(int ptrimage);
	void signalSaveSettings();
private:
	void initToolVars();

private:
	bool flagDisplayNewReport;
	bool flagDisplayNewImage;
	bool flagDisplayNewMeasure;
	bool flagsDisplay[NB_COMPONENT_CLASS];
	bool flagsCreate[NB_COMPONENT_CLASS];
};
#endif
