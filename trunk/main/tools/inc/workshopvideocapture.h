/***************************************************************************
	workshopvideocapture.h  -  live video capture/display/processing tool for Piaf
							 -------------------
	begin                : Wed Nov 13 10:07:22 CET 2002
	copyright            : (C) 2002 by Christophe Seyve
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

#ifndef WORKSHOPVIDEOCAPTURE_H
#define WORKSHOPVIDEOCAPTURE_H

// include files for Qt
#include <qwidget.h>
#include <qpushbutton.h>

#include <qimage.h>
#include <qpainter.h>

#include <q3vbox.h>
#include <qslider.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qpushbutton.h>
#include <q3buttongroup.h>
#include <qaction.h>
#include <q3popupmenu.h>
#include <q3toolbar.h>


#include <q3listview.h>
//Added by qt3to4:
#include <QCloseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWorkspace>

#include <QThread>


#include "videocapture.h"
#include "OpenCVEncoder.h"
#include "SwFilters.h"

// Sisell Workshop include files
#include "workshopmeasure.h"
#include "workshoptool.h"
#include "vidacqsettingswindow.h"

class VideoCaptureDoc;
class WorkshopImage;
class WorkshopImageTool;

/** \brief Thread just to wait for grabbing from video doc */
class WorkshopVideoCaptureThread : public QThread
{
	Q_OBJECT
public:
	WorkshopVideoCaptureThread(VideoCaptureDoc* pDoc);
	~WorkshopVideoCaptureThread();

	void run();

private:
	VideoCaptureDoc* m_pDoc;
	bool m_run;
	bool m_running;

signals:
	void documentChanged();
};


/**
	\brief Live video view tool

	This class provides live video display utilities, device control,
	snapshot and video encoding features , and access
	to plugin manager.

	\author Christophe SEYVE \mail cseyve@free.fr
 */
class WorkshopVideoCaptureView : public WorkshopTool
{
	Q_OBJECT

public:
	/** Constructor for the view
		* @param pDoc  your document instance that the view represents. Create a document before calling the constructor
		* or connect an already existing document to a new MDI child widget.*/
	WorkshopVideoCaptureView(VideoCaptureDoc* pDoc, QWidget* parent, const char *name, Qt::WidgetAttribute wflags);

	/** Constructor for the view
		* @param va existing SwVideoAcquisition input
		* or connect an already existing document to a new MDI child widget.*/
	WorkshopVideoCaptureView(VirtualDeviceAcquisition * va, QWidget *parent, const char* name, Qt::WidgetAttribute  wflags);
	/** Destructor for the main view */
	~WorkshopVideoCaptureView();
    /** returns a pointer to the document connected to the view*/
	VideoCaptureDoc *getDocument() const;
	/** gets called to redraw the document contents if it has been modified */
    void update(WorkshopVideoCaptureView* pSender);
	/** contains the implementation for printing functionality and gets called by MultiVideoApp::slotFilePrint() */
	void print(QPrinter *pPrinter);

	void setWorkspace(QWorkspace * wksp);
protected:
	virtual void closeEvent(QCloseEvent*);

	VideoCaptureDoc * doc;
	VidAcqSettingsWindow * mVidAcqSettingsWindow;

	// -------------------- VIDEO SPECIFIC SECTION ---------------------
public:
	tBoxSize getImageSize();
private:
    void init();

    void setToolbar();
    void refreshDisplay();

private :
	QWorkspace * pWorkspace;
	bool m_isLocked; // display refresh is locked
	// inspired vu workshopplot2d
	void initTool();

	WorkshopVideoCaptureThread * m_pWorkshopVideoCaptureThread;

	Q3VBox *vBox;
	Q3HBox *hBox;
	QPushButton * DeviceButton;

	// Device menu
	QMenu * deviceMenu;
	QPushButton * bDevice;
	QAction * actDevEdit;
	QAction * actDevParams;

	void updateToolbar();

	// play/pause and record buttons
	QPushButton * bPlayPause;
	bool freeze;

	// image parameters
	int contrast;
	int brightness;
	int hue;
	int white;
	int color;

	int exposure; ///< exposure in us
	int gain; ///< gain
	Q3ToolBar * toolBar;
	QMenuBar * menuBar;

	// Display current image
	WorkshopImage * detailsImage;
	WorkshopImageTool * detailsView;

	QWidget * deviceSettingsWidget;
	QWidget * deviceParamsWidget;
	bool playGrayscale;
  protected slots:
	void slotDocumentChanged();
	void slotResizeTool(QResizeEvent *e);
	// device slots
	void slotDeviceParams();
	void slotDeviceSize();
	void slotSetContrast(int cont);
	void slotSetColor(int col);
	void slotSetHue(int h);
	void slotSetBrightness(int br);
	void slotSetWhite(int w);

	void slotSetExposure(int exp);
	void slotSetGain( int );

	// change size
	void slotSetSize( const QString & str );
	void slotSetChannel( const QString &str);

	// play/pause
	void slotPlayPause();
};

#endif

