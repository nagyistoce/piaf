/***************************************************************************
      previewplot2d.h  -  2D plotting preview for Piaf project
                             -------------------
    begin                : July 2002
    copyright            : (C) 2002-2003 by Olivier VINE, Christophe SEYVE - SISELL
    email                : olivier.vine@sisell.com / christophe.seyve@sisell.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PREVIEWPLOT2D_H
#define PREVIEWPLOT2D_H

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
#include <qwt_math.h>
//Added by qt3to4:
#include <Q3ActionGroup>
#include <QMouseEvent>
#include <QCloseEvent>
#include "workshopmeasure.h"
#include "workshoptool.h"

// Zoom modes
#define NO_ZOOM    0
#define H_ZOOM     1
#define V_ZOOM     2
#define REC_ZOOM   3
#define ELL_ZOOM   4
#define MOV_ZOOM   5

// Max distance to select a curve
#define MAX_SELECT_DIST   10

/** \brief Preview window for measure (1D signal).
	\author Olivier VINE \mail olivier.vine@sisell.com
*/
class PreviewPlot2D: public WorkshopTool
{
	Q_OBJECT
	
public:
	//! Constructor
	PreviewPlot2D(QWidget* parent, const char *name, Qt::WidgetAttribute wflags);
	PreviewPlot2D(WorkshopMeasure* pDoc, QWidget* parent, const char *name, Qt::WidgetAttribute wflags);
 	~PreviewPlot2D(){}

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
	//void onDelMeasure(int ptr);

protected:
	virtual void closeEvent(QCloseEvent*) {}

private:
	//QMainWindow *pWin;
	Q3HBox* view_back;
	QwtExtPlot *pPlot;	
	long currKey;
	int zoomMode;
	//QValueList<long> highlightList;
	//QIntDict<WorkshopMeasure> curveDict;
	//QIntDict<QPen> HPenDict;
    QPoint memPoint;
	bool autoscaleX, autoscaleY;
	long zoomMarkerKey;
		
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
		
	// Toolbars
	Q3ToolBar *zoomToolBar;

private:
	void initTool();
	void initPlot();
	void initVars();
	void initActions();
	void initToolBar();

	// handling with highlights
	/*
	void clearHighlight();
	void removeHighlight(long key);
	void addHighlight(long key);
	bool isHighlighted(long key);
	void onDoHighlight(const QMouseEvent& e);
	*/
	// handling with zooms
	void onZoomRec(const QMouseEvent& e);
	void onHZoom(const QMouseEvent& e);
	void onVZoom(const QMouseEvent& e);
};

#endif
