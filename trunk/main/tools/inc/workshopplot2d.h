/***************************************************************************
                          workshopPlot2D.h  -  description
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

#ifndef WORKSHOPPLOT2D_H
#define WORKSHOPPLOT2D_H

// include files for Qt
#include <qwidget.h>
#include <q3mainwindow.h>
#include <qpen.h>
#include <qaction.h>
#include <q3toolbar.h>

//#include <qptrlist.h>
#include <q3intdict.h>
#include <q3hbox.h>

// Sisell Workshop include files
#include "qwt_ext_plot.h"
#include "qwt_ext_cursor.h"
#include <qwt_math.h>
//Added by qt3to4:
#include <Q3ActionGroup>
#include <QMouseEvent>
#include <Q3ValueList>
#include <QCloseEvent>
#include "workshopmeasure.h"
#include "workshoptool.h"
#include "SwFilters.h"


/////////////////////////
#include "marker_dialog.h"
#include "curve_properties.h"

// Zoom modes
#define NO_ZOOM       0
#define H_ZOOM        1
#define V_ZOOM        2
#define REC_ZOOM      3
#define ELL_ZOOM      4
#define MOV_ZOOM      5
#define MARKER_FORM   6
#define SELECT        7
#define CURSOR_FOLLOW 8

// Max distance to select a curve
#define MAX_SELECT_DIST   10

/**
	\brief 2D display widget.
	\author Olivier VINE \mail olivier.vine@sisell.com
	*/
class WorkshopPlot2D: public WorkshopTool
{
	Q_OBJECT
	
public:
	//! Constructor
	WorkshopPlot2D(QWidget* parent, const char *name, Qt::WidgetAttribute wflags);
	WorkshopPlot2D(WorkshopMeasure* pDoc, QWidget* parent, const char *name, Qt::WidgetAttribute wflags );
 	~WorkshopPlot2D();

	//QMainWindow *win() { return pWin; } 	
	QwtExtPlot *plotArea() { return pPlot; }
		
	// handling zoom mode
	void set_zoomMode(int mode) { zoomMode = mode; }
	int  get_zoomMode()         { return zoomMode; }

public slots:
	void plotMouseMoved( const QMouseEvent&);
	void plotMousePressed( const QMouseEvent&);
	void plotMouseReleased( const QMouseEvent&);
	void slotZoom(QAction*);
	void slotAutoX(bool);
	void slotAutoY(bool);
	void onNewMeasure(int ptr);
	void onDelMeasure(int ptr);
    void slotMarkerForm();
    void slotPropertiesForm();

signals:
	void newMeasure(WorkshopMeasure *swm);
protected:

	virtual void closeEvent(QCloseEvent*) {}

    //! Calls itemChanged()
    //virtual void curveChanged() { itemChanged(); }
private:
	//QMainWindow *pWin;
	Q3HBox* view_back;
	QwtExtPlot *pPlot;	
	int zoomMode;
	Q3ValueList<long> highlightList;
	Q3IntDict<WorkshopMeasure> curveDict;
	Q3IntDict<QPen> HPenDict;
    QPoint memPoint;
	bool autoscaleX, autoscaleY;
	long zoomMarkerKey;
	WorkshopMeasure *pNewMeasure;
    /////////
    long crv;
    ///////
    QWidget *pWorkspace;

	/////////////////////////////
	MarkerDialogForm * markerForm;
	PropertiesForm * properties;
    QwtExtCursor * my_cursor;
	double xval, yval;
    /////////////////////////////

	// Actions
	Q3ActionGroup *grpZoom;
	QAction *Select;
	QAction *ZoomR;
	QAction *ZoomV;
	QAction *ZoomH;
	QAction *ZoomE;
	QAction *Move;
	QAction *AutoScaleX;
	QAction *AutoScaleY;
	QAction *MarkForm;
	QAction *PropForm;
	QAction *CursorForm;
		
	// Toolbars
	Q3ToolBar *zoomToolBar;

private:
	void initTool();
	void initPlot();
	void initVars();
	void initActions();
	void initToolBar();

	// handling with highlights
	void clearHighlight();
	void removeHighlight(long key);
	void addHighlight(long key);
	bool isHighlighted(long key);
	void onDoHighlight(const QMouseEvent& e);

	// handling with zooms
	void onZoomRec(const QMouseEvent& e);
	void onHZoom(const QMouseEvent& e);
	void onVZoom(const QMouseEvent& e);

private:
	QAction * filtersAction;
    Q3MainWindow * filtersWindow;

	// ---- FILTER MANAGEMENT SECTION ----
  public:
	int loadFilterManager();

  private:
    SwFilterManager * filterManager;

  protected slots:
    void slotFilters();


	
};

#endif
