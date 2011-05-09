/***************************************************************************
	  workshopimage.h  -  image component for Piaf
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

#ifndef WORKSHOPIMAGE_H
#define WORKSHOPIMAGE_H

// include files for Qt
#include <qobject.h>
#include <qwidget.h>
#include <qpainter.h>
#include <q3ptrlist.h>
#include <qfile.h>
#include <qstring.h>
#include <qdatastream.h>
#include <q3popupmenu.h>
#include <q3filedialog.h>
#include <qdir.h>
#include <q3valuevector.h>
#include <qdatetime.h>
#include <qimage.h>
#include <qlist.h>
//Added by qt3to4:
#include <QPaintEvent>
#include <QMouseEvent>

// Color modes (palette for 8bit colors)
#define COLORMODE_GREY 					0
#define COLORMODE_GREY_INVERTED			1
#define COLORMODE_THERMIC_BLACK2RED 	2
#define COLORMODE_THERMIC_BLUE2RED		3
#define COLORMODE_INDEXED				4

#define COLORMODE_MAX	COLORMODE_INDEXED

// Sisell Workshop include files
#include "sw_types.h"
#include "workshopcomponent.h"

/** \brief Image display widget

Image display widget, modified for Piaf display use.

*/
class ImageWidget : public QWidget
{
	Q_OBJECT
public:
	ImageWidget(QWidget *parent=0, const char *name=0, Qt::WFlags f=0)
		: QWidget(parent, name, f)
	{
		dImage=NULL;
		xOrigine = yOrigine = ZoomScale = 1;
		m_colorMode = 0;
	}

	~ImageWidget()
	{

	}

	QImage * getQImage() { return dImage; };
	void setRefImage(QImage *pIm)
	{
		dImage = pIm;
		if(dImage) {
			if(dImage->depth()==8) {
				setColorMode(m_colorMode);
			}
		}
		update();
	}

	void setZoomParams(int xO, int yO, int scale) {
		xOrigine = xO;
		yOrigine = yO;
		ZoomScale = scale;
	};

	void  paintEvent( QPaintEvent * );

	/** @brief Change color display mode for 8bit images */
	void setColorMode(int mode);

	/** @brief Set overlay rect */
	void setOverlayRect(QRect overlayRect, QColor col) {

		m_overlayRect = overlayRect;
		m_overlayColor = col;
	}
signals:


	/*!
	  A signal which is emitted when the mouse is pressed in the
	  plotting area.
	  \param e Mouse event object, event coordinates referring
			   to the plotting area
	 */
	void mousePressed( QMouseEvent *e);

	/*!
	  A signal which is emitted when a mouse button has been
	  released in the plotting area.
	  \param e Mouse event object, event coordinates referring
			   to the plotting area
	 */
	void mouseReleased(QMouseEvent *e);

	/*!
	  A signal which is emitted when the mouse is moved in the
	  plotting area.
	  \param e Mouse event object, event coordinates referring
			   to the plotting area
	 */
	void mouseMoved( QMouseEvent *e);

protected:
	virtual void mousePressEvent(QMouseEvent *e) {
		emit mousePressed(e);
	};
	virtual void mouseReleaseEvent(QMouseEvent *e){
		emit mouseReleased(e);
	};
	virtual void mouseMoveEvent(QMouseEvent *e){
		emit mouseMoved(e);
	};

private:
	QImage		*dImage;
	int xOrigine;
	int yOrigine;
	int ZoomScale;
	int m_colorMode;

	QRect m_overlayRect;
	QColor m_overlayColor;
};

/**
	\brief Image component for Piaf
*/
class WorkshopImage: public WorkshopComponent, public QImage
{
	Q_OBJECT

public:
	//! Constructor
	///////////////////////
	WorkshopImage(QImage image, QString label = "Untitled",int type = VIRTUAL_VALUE)
		: WorkshopComponent(label, type), QImage(image)
	{
		static int WorkshopImage_last_id = 0;
		WorkshopImage_last_id++;
		m_ID = WorkshopImage_last_id;
		setComponentClass(CLASS_IMAGE);
	}
	////////////////////////

	WorkshopImage(QString label = "Untitled",int type = VIRTUAL_VALUE)
		: WorkshopComponent(label, type), QImage()
	{
		setComponentClass(CLASS_IMAGE);
	}
	int getID() { return m_ID; };

	~WorkshopImage(){}
	// overloaded functions
	// open function
	//bool open(const QString &filename, const char *format=0);
	/** saves the document under filename and format.*/
	void save();
	void close(){ printf("WorkshopImage close !\n"); }
	void saveAs();
	void openFile(const QString);

protected:
	int m_ID;
	//! Calls itemChanged()
	//virtual void curveChanged() { itemChanged(); }
};

#endif

