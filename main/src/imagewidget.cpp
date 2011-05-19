/***************************************************************************
	  imagewidget.cpp  -  image display widget for Piaf
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

#include "imagewidget.h"
#include <stdio.h>
#include <QPainter>


#include <qcolor.h>

//Added by qt3to4:
#include <QPixmap>
#include <QPaintEvent>

unsigned char RthermicBlack2Red(int p) {
	int dp = p-170;
	if(dp<0) dp = -dp;
	unsigned char R = (p<170 ? 0 : (unsigned char)(dp*3));
	return R;
}
unsigned char GthermicBlack2Red(int p) {
	int dp = p-170;
	if(dp<0) dp = -dp;
	unsigned char G = (p<85 ? 0 : (unsigned char)(255 - dp*3));
	return G;
}

unsigned char BthermicBlack2Red(int p) {
	int dp = p-85;
	if(dp<0) dp = -dp;
	unsigned char B = (p>170 ? 0 : (unsigned char)(255-dp*3));
	return B;
}

unsigned char RthermicBlue2Red(int p) {
	if(p<128) return 0;

	int dp = p-128;
	if(dp<0) dp = -dp;
	unsigned char R = (p<128 ? 0 : (unsigned char)(dp*2));
	return R;
}

unsigned char GthermicBlue2Red(int p) {
	int dp = p-127;
	if(dp<0) dp = -dp;
	unsigned char G = (unsigned char)(255 - dp*2);
	return G;
}

unsigned char BthermicBlue2Red(int p) {
	if(p>127) return 0;

	unsigned char B = (unsigned char)(255 - p*2);
	return B;
}




ImageWidget::ImageWidget(QWidget *parent, const char *name, Qt::WFlags f)
	: QWidget(parent, name, f)
{
	mZoomFit = true; // fit to size by default
	mZoomFitFactor = -1.; // don't know yet the zoom factor

	xOrigine = yOrigine = 0;

	dImage=NULL;
	ZoomScale = 1;
	m_colorMode = 0;
}

void ImageWidget::setRefImage(QImage *pIm)
{
	dImage = pIm;
	if(dImage) {
		if(dImage->depth()==8) {
			setColorMode(m_colorMode);
		}
	}
	update();
}


void ImageWidget::setColorMode(int mode) {
	if(mode <= COLORMODE_MAX )
		m_colorMode = mode;
	else
		m_colorMode = COLORMODE_GREY;

	if(dImage) {
		if(dImage->depth() > 8) return;

		dImage->setNumColors(256);
		QColor col;
		fprintf(stderr, "ImageWidget::%s : colorMode = %d\n", __func__, m_colorMode);
		// Change color mode
		switch(m_colorMode) {
		default:
		case COLORMODE_GREY:
			for(int i=0; i<256; i++) {
				col.setHsv(0,0,i);
				dImage->setColor(i, col.rgb());
			}
			break;
		case COLORMODE_GREY_INVERTED:
			for(int i=0; i<256; i++) {
				col.setHsv(0,0,255-i);
				dImage->setColor(i, col.rgb());
			}
			break;
		case COLORMODE_THERMIC_BLUE2RED: // "Thermic" blue to red
			for(int i=0; i<256; i++) {
				dImage->setColor(i, qRgb(RthermicBlue2Red(i), GthermicBlue2Red(i), BthermicBlue2Red(i)));
			}
			break;
		case COLORMODE_THERMIC_BLACK2RED: // "Thermic" black to red
			for(int i=0; i<256; i++) {
				dImage->setColor(i, qRgb(RthermicBlack2Red(i), GthermicBlack2Red(i), BthermicBlack2Red(i)));
			}
			break;
		case COLORMODE_INDEXED:
			col.setHsv(0,0,0);
			dImage->setColor(0, col.rgb());

			int hue = 17;
			for(int i=1; i<256; i++, hue+=37) {
				col.setHsv(hue%360, 255, 127);
				dImage->setColor(i, col.rgb());
			}
			break;
		}
	}

	update();
}


void ImageWidget::wheelEvent ( QWheelEvent * e )
{
	if(!e) return;
	if(!mZoomFit) return;

	float curZoom = mZoomFitFactor;
	int numDegrees = e->delta() / 8;
	int numSteps = numDegrees / 15;

	mZoomFitFactor += numSteps;

	int x = e->pos().x(), y =  e->pos().y();
	int xZoomCenter = xOrigine + x * curZoom;
	int yZoomCenter = yOrigine + y * curZoom;

	int disp_w =  size().width();
	int disp_h =  size().height();
	xOrigine = xZoomCenter - disp_w*0.5f/mZoomFitFactor;
	yOrigine = yZoomCenter - disp_h*0.5f/mZoomFitFactor;

//	fprintf(stderr, "%s:%d : x,y=%d,%d\n", __func__, __LINE__,

//	        );
	update();

	emit signalWheelEvent( e );
}

void ImageWidget::paintEvent( QPaintEvent * e)
{
	fprintf(stderr, "ImageWidget::paintEvent\n");
	if(!dImage) {
		return;
	}
	QRect cr;
	if(!e) {
		cr = rect();
	} else {
		// Use double-buffering
		cr = e->rect();
	}

	QPixmap pix( cr.size() );
	pix.fill( this, cr.topLeft() );
	QPainter p( &pix);

	if(mZoomFit)
	{
		fprintf(stderr, "ImageWidget::%s:%d : FIT!\n", __func__, __LINE__);
		int wdisp = size().width()-2;
		int hdisp = size().height()-2;
		if(mZoomFitFactor<0.) {
			mZoomFitFactor = std::min( (float)wdisp / (float)dImage->width(),
									   (float)hdisp / (float)dImage->height() );

			m_displayImage = dImage->scaled(size(), Qt::KeepAspectRatio);
		}
		else {
			QImage cropImage = dImage->copy(
						xOrigine, yOrigine,
						wdisp/mZoomFitFactor, hdisp/mZoomFitFactor);

			m_displayImage = cropImage.scaled( wdisp, hdisp,
										Qt::KeepAspectRatio );
		}
		p.drawImage(0,0, m_displayImage);

	}
	else
	{
		if(cr != this->rect())
			p.drawImage(0,0, dImage->copy(cr));
		else
			p.drawImage(0,0, *dImage);
	}


	if(m_overlayRect.width()>0) {
		p.setPen(m_overlayColor);
		p.drawRect(m_overlayRect.x()-cr.x(), m_overlayRect.y()-cr.y(),
				   m_overlayRect.width(), m_overlayRect.height());
	}

	p.end();
	bitBlt( this, cr.topLeft(), &pix );
}
