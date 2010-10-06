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
#include <qmainwindow.h>
#include <qpen.h>
#include <qaction.h>
#include <qtoolbar.h>

//#include <qptrlist.h>
#include <qintdict.h>
#include <qhbox.h>

// Sisell Workshop include files
#include "qwt_ext_plot.h"
#include "qwt_ext_cursor.h"
#include <qwt_math.h>
#include "workshopmeasure.h"
#include "workshoptool.h"
#include "SwFilters.h"

/////////////////////////
#include "marker_dialog.h"
#include "curve_properties.h"

// Zoom modes
#define NO_ZOOM     0
#define H_ZOOM      1
#define V_ZOOM      2
#define REC_ZOOM    3
#define ELL_ZOOM    4
#define MOV_ZOOM    5
#define MARKER_FORM 6
#define SELECT      7

// Max distance to select a curve
#define MAX_SELECT_DIST   10

//class WorkshopPlot2D: public QMainWindow, public WorkshopTool
class WorkshopPlot2D: public WorkshopTool
{
	Q_OBJECT
	
public:
	//! Constructor
    WorkshopPlot2D(QWidget* parent, const char *name, int wflags);
    WorkshopPlot2D(WorkshopMeasure* pDoc, QWidget* parent, const char *name, int wflags);
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
	QHBox* view_back;
	QwtExtPlot *pPlot;	
	int zoomMode;
	QValueList<long> highlightList;
	QIntDict<WorkshopMeasure> curveDict;
	QIntDict<QPen> HPenDict;
    QPoint memPoint;
	bool autoscaleX, autoscaleY;
	long zoomMarkerKey;
	WorkshopMeasure *pNewMeasure;
    /////////
    bool pressed;
    long crv;
    ///////
    //QWidget *pWorkspace;

	/////////////////////////////
	MarkerDialogForm * markerForm;
	PropertiesForm * properties;
    QwtExtCursor * my_cursor;
    /////////////////////////////

	// Actions
	QActionGroup *grpZoom;
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
		
	// Toolbars
	QToolBar *zoomToolBar;


private:
	QAction * filtersAction;
    QMainWindow * filtersWindow;

	// ---- FILTER MANAGEMENT SECTION ----
  public:
	int loadFilterManager();

  private:
    SwFilterManager * filterManager;
	QWidget * pWorkspace;

  protected slots:
    void slotFilters();

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
};

#endif
