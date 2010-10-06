/***************************************************************************
           previewimage.h  -  image preview tool for Piaf
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

#ifndef PREVIEWIMAGE_H
#define PREVIEWIMAGE_H

// include files for Qt
#include <qwidget.h>
#include <q3mainwindow.h>
#include <qpen.h>
#include <qaction.h>
#include <q3toolbar.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qprinter.h>
#include <qpainter.h>
#include <qrect.h>
#include <qtimer.h>
//Added by qt3to4:
#include <QCloseEvent>
#include <QPaintEvent>

// Sisell Workshop include files
#include "workshoptool.h"
#include "workshopimage.h"
#include "SwVideoAcquisition.h"

/**
	\brief Simple image display widget.
	\author Olivier VINE \mail olivier.vine@sisell.com
	*/
class ImageView : public QWidget
{
    Q_OBJECT
public:
    ImageView(QWidget *parent=0, const char *name=0, Qt::WFlags f=0)
	: QWidget(parent, name, f)
	{
		printer = NULL;
	}
    ~ImageView()
	{ 
		if(printer)
			delete printer;
	}

	void setRefImage(QImage *pIm)
	{
		dImage = pIm;
		update();
	}
	
protected:
    void   paintEvent( QPaintEvent * )
	{
		// Use double-buffering
		QRect cr = this->rect();
        QPixmap pix( cr.size() );
        pix.fill( this, cr.topLeft() );
        QPainter p( &pix);
		p.drawImage(cr, *dImage); 
        p.end();
        bitBlt( this, cr.topLeft(), &pix );
	}
    //void   resizeEvent( QResizeEvent * );
private:
    QPrinter	*printer;
	QImage		*dImage;
};


class VideoCaptureDoc;

/** \brief Preview window for still and live video images.
	\author Olivier VINE \mail olivier.vine@sisell.com
*/	
class PreviewImage : public WorkshopTool
{
	Q_OBJECT
	
public:
	//! Constructor
	PreviewImage(QWidget* parent, const char *name, Qt::WidgetAttribute wflags);
    //PreviewImage(WorkshopMeasure* pDoc, QWidget* parent, const char *name, int wflags);
 	~PreviewImage(){}
	
	// connecting to video to display both static images or videos
	void connect2video(VideoCaptureDoc *src);
	
	void unconnectVideo();
	bool displayVideo() { return fDisplayVideo; }
	
public slots:
	void onNewImage(int ptr);
	void onNewVideoImage();
	
protected:
	virtual void closeEvent(QCloseEvent*) {}

private:
	ImageView *pView;
	QImage pImage;
	VideoCaptureDoc *pVideoAcq;
	bool fDisplayVideo;
	QTimer *pImageTimer;
	tBoxSize videoSize;
		
private:
	void initTool();
	void initPlot();
	void initVars();
};

#endif
