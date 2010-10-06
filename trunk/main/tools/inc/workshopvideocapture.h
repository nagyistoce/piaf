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

#include "videocapture.h"
#include "OpenCVEncoder.h"
#include "SwFilters.h"

// Sisell Workshop include files
#include "workshopmeasure.h"
#include "workshoptool.h"


class VideoCaptureDoc;



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
    WorkshopVideoCaptureView(SwVideoAcquisition * va, QWidget *parent, const char* name, Qt::WidgetAttribute  wflags);
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

	VideoCaptureDoc *doc;

	// -------------------- VIDEO SPECIFIC SECTION ---------------------
public:
	tBoxSize getImageSize();
    virtual void paintEvent(QPaintEvent *);
private:
    void init();
    unsigned char * originalImage;
    void setViewSize();
    void setToolbar();
    void changeViewSize();
    void refreshDisplay();

private :
		QWidget *dpw;
	QWidget *dsw;
	QWorkspace * pWorkspace;
	bool m_isLocked;
	// inspired vu workshopplot2d
	void initTool();

	unsigned char typeOfView;
	tBoxSize viewSize;
	tBoxSize imageSize;

	int viewPixel;
	int imagePixel;

	/// Ready-to-use QImage object for RGB image
	QImage ImgRGB;
	QImage savedImage;

	unsigned char * mask;

	Q3VBox *vBox;
	Q3HBox *hBox;
	ImageWidget * drawWidget;
	QPushButton * DeviceButton;

	// Device menu
	Q3PopupMenu * deviceMenu;
	QPushButton * bDevice;
	QAction * actDevEdit;
	QAction * actDevParams;

	// Area selection buttons
	Q3HButtonGroup * toolGroup;
	QPushButton * aMenuMask;
	Q3PopupMenu * maskMenu;

	QAction * actSelMask;
	QAction * actAddMask;
	QAction * actDelSelectedMask;
	QAction * actAllMask;
	QRect * selectedMask;

	// View selection buttons
	QPushButton * aMenuView;
	Q3PopupMenu * viewMenu;

	// actions for view mode selection
	QAction * actZoomInView;
	QAction * actZoomFitView;
	QAction * actZoomOutView;
	QAction * actMoveView;
	void updateToolbar();

	// play/pause and record buttons
	QPushButton * bPlayPause;
	bool freeze;
	QPushButton * bRecord;

	// export button
	QPushButton * bExport;

	// image parameters
	int contrast;
	int brightness;
	int hue;
	int white;
	int color;

	int ZoomScale;
	int xZoomOrigine; // always in original image coordinates !!!
	int yZoomOrigine;
	int xZoomCenter;
	int yZoomCenter;
	int moveDx;
	int moveDy;
	void CalculateZoomWindow();
	void WindowView2Absolute(int Vx, int Vy, int * Ax, int * Ay);
	void Absolute2View(int Ax, int Ay, int * Vx, int * Vy);

	Q3ToolBar * toolBar;

	QMenuBar * menuBar;



  protected slots:
	void slotDocumentChanged();

	///////////////////////
	void slotSaveImage();
	//////////////////////

	// device slots
	void slotDeviceParams();
	void slotDeviceSize();
	void slotSetContrast(int cont);
	void slotSetColor(int col);
	void slotSetHue(int h);
	void slotSetBrightness(int br);
	void slotSetWhite(int w);
	// change size
	void slotSetSize( const QString & str );
	void slotSetChannel( const QString &str);

	// detection mask slots
	void slotMenuMask();
	void slotSelMask();
	void slotClearMask();
	void slotAddMask();
	void slotDelSelectedMask();
	void slotMaskTools(int id);

	// view slots
	void slotMenuView();
	void slotZoomInMode();
	void slotZoomFit();
	void slotZoomOutMode();
	void slotMoveMode();

	// play/pause
	void slotPlayPause();
	// record
	void slotRecord();

	void mousePressEvent( QMouseEvent *e);
	void mouseReleaseEvent( QMouseEvent *e);
	void mouseMoveEvent( QMouseEvent *e);
	void resizeEvent ( QResizeEvent * e);

private:
	void onLButtonDown(Qt::ButtonState nFlags, const QPoint point);
	void onLButtonUp(Qt::ButtonState nFlags, const QPoint point);
	void onRButtonDown(Qt::ButtonState nFlags, const QPoint point);
	void onRButtonUp(Qt::ButtonState nFlags, const QPoint point);
	// drawing rectangle function
	void onMouseMove(Qt::ButtonState nFlags, const QPoint point);
	bool selectPressed;
	bool showMask;
	int myStartX;
	int myStartY;
	int myStopX;
	int myStopY;

	unsigned char ToolMode;
	QRect * getMaskFromPos(int x, int y);

private:
	QPushButton * filtersButton;
	//	SwFilterManager * filtersList;

	// ---- FILTER MANAGEMENT SECTION ----
public:
	int loadFilterManager();


private:
	SwFilterManager * filterManager;


  protected slots:
	void slotFilters();

// ---- VIDEO ENCODER SECTION -----

private:
	OpenCVEncoder * mpegEncoder;
	bool record;
	int recordNum;
	/** start a new record */
	void startRecording();
	/** stop current record */
	void stopRecording();
	/** returns true if recording */
	bool isRecording();
	char movieFile[512];

signals:
	void ImageSaved(QImage *);
	void VideoSaved(char *);
};

#endif

