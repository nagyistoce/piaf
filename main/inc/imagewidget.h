/***************************************************************************
	  imagewidget.h  -  image display widget for Piaf
							 -------------------
	begin                : ven nov 29 15:53:48 UTC 2002
	copyright            : (C) 2002-2011 by Christophe Seyve
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

#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include <QWidget>

// Color modes (palette for 8bit colors)
#define COLORMODE_GREY 					0
#define COLORMODE_GREY_INVERTED			1
#define COLORMODE_THERMIC_BLACK2RED 	2
#define COLORMODE_THERMIC_BLUE2RED		3
#define COLORMODE_INDEXED				4

#define COLORMODE_MAX	COLORMODE_INDEXED

/** \brief Image display widget

Image display widget, modified for Piaf display use.

*/
class ImageWidget : public QWidget
{
	Q_OBJECT
public:
	ImageWidget(QWidget *parent=0, const char *name=0, Qt::WFlags f=0);

	~ImageWidget()
	{

	}

	QImage * getQImage() { return dImage; };
	void setRefImage(QImage *pIm);

	void setZoomFit(bool on)
	{
		mZoomFit = on;
		update();
	}

	void setZoomParams(int xO, int yO, int scale) {
		xOrigine = xO;
		yOrigine = yO;
		ZoomScale = scale;
		mZoomFit = false;
	}

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
	/*!
	  A signal which is emitted when the mouse wheel is moved
	  \param e Mouse event object, event coordinates referring
			   to the plotting area
	 */
	void signalWheelEvent ( QWheelEvent * event );

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
	virtual void wheelEvent ( QWheelEvent * event );

private:
	QImage		*dImage;
	int xOrigine;
	int yOrigine;
	int ZoomScale;			///< Integer zoom scale : 1 = 1:1 : 2=2:1 ; 0=1:2 ...
	int m_colorMode;
	bool mZoomFit;			///< Fit zoom to size (not integer zoom factor). Default= true
	float mZoomFitFactor;	///< Fit zoom to size (floating point zoom factor)
	QImage m_displayImage;

	QRect m_overlayRect;
	QColor m_overlayColor;
};


#endif // IMAGEWIDGET_H
