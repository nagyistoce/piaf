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
	xMouseMoveStart = -1;
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

/* Switch to smart zooming mode */
void ImageWidget::switchToSmartZoomMode(bool on)
{
	mZoomFit = on;
	mZoomFitFactor = -1.f;
	xMouseMoveStart = -1;
	xOrigine = yOrigine = 0;

	clipSmartZooming();
}

void ImageWidget::setZoomParams(int xO, int yO, int scale) {
	xOrigine = xO;
	yOrigine = yO;

	mZoomFitFactor = scale;
	if(scale < 1)
	{
		mZoomFitFactor = 1.f / (2.f - scale);
	}
	ZoomScale = scale;
//		mZoomFit = false;
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

void ImageWidget::mousePressEvent(QMouseEvent *e) {
	emit mousePressed(e);
	if(!mZoomFit) return;

	xMouseMoveStart = e->pos().x();
	yMouseMoveStart = e->pos().y();
	xOriginMoveStart = xOrigine;
	yOriginMoveStart = yOrigine;
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *e){
	emit mouseReleased(e);
	if(!mZoomFit) return;
	xMouseMoveStart = -1;
}

void ImageWidget::mouseMoveEvent(QMouseEvent *e){
	emit mouseMoved(e);
	if(!mZoomFit) { return; }

	if(xMouseMoveStart<0) { return; } // not clicked

	// dx,dy is the move in pixel on display
	int dx = e->pos().x() - xMouseMoveStart ;
	int dy = e->pos().y() - yMouseMoveStart ;
	xOrigine = xOriginMoveStart - dx / mZoomFitFactor;
	yOrigine = yOriginMoveStart - dy / mZoomFitFactor;
	clipSmartZooming();
	update();
}
void ImageWidget::clipSmartZooming()
{
	if(!dImage) return;

	int width = size().width() / mZoomFitFactor;
	int height = size().height() / mZoomFitFactor;
	if(xOrigine + width > dImage->width()) {  xOrigine = dImage->width() - width; }
	if(yOrigine + height > dImage->height()) {  yOrigine = dImage->height() - height; }

	// do xO, yO at end to align on left and top
	if(xOrigine < 0) { xOrigine = 0; }
	if(yOrigine < 0) { yOrigine = 0; }
}

void ImageWidget::wheelEvent ( QWheelEvent * e )
{
	if(!e) return;
	if(!dImage) return;
	if(!mZoomFit) return;

	if(mZoomFitFactor<0)
	{
		update();
	}
	float curZoom = mZoomFitFactor;
	int numDegrees = e->delta() / 8;
	int numSteps = numDegrees / 15;

	int disp_w =  size().width();
	int disp_h =  size().height();
	float zoommin = std::min( (float)disp_w / (float)dImage->width(),
							  (float)disp_h / (float)dImage->height() );



	if(numSteps > 0)
		mZoomFitFactor *= 1.2;
	else
		mZoomFitFactor *= 0.8;
	if(mZoomFitFactor < zoommin) {
		// recompute minimal zoom
		xOrigine = yOrigine = 0;
		mZoomFitFactor = -1.f;
		update();
		return ;
	}
	int x = e->pos().x(), y =  e->pos().y();
	int xZoomCenter = xOrigine + x / curZoom;
	int yZoomCenter = yOrigine + y / curZoom;

	fprintf(stderr, "%s:%d : orig=%d,%d / %dx%d x,y=%d,%d zoom=%g => new=%g"
			" => ZoomCenter=%d,%d disp=%dx%d => xOrig=%dx%d\n", __func__, __LINE__,
			xOrigine, yOrigine, dImage->width(), dImage->height(),
			x,y, curZoom, mZoomFitFactor,
			xZoomCenter, yZoomCenter,
			disp_w, disp_h,
			xZoomCenter - disp_w*0.5f/mZoomFitFactor,
			yZoomCenter - disp_h*0.5f/mZoomFitFactor
			);

	xOrigine = xZoomCenter - disp_w*0.5f/mZoomFitFactor;
	yOrigine = yZoomCenter - disp_h*0.5f/mZoomFitFactor;
	clipSmartZooming();
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
			if(cropImage.isNull())
			{
				fprintf(stderr, "ImageWidget::%s:%d : pb with crop (%d,%d+%d/%gx%d/%g)!\n",
						__func__, __LINE__,
						xOrigine, yOrigine,
						wdisp, mZoomFitFactor, hdisp, mZoomFitFactor
						);
			}
			else {
				m_displayImage = cropImage.scaled( wdisp, hdisp,
												  Qt::KeepAspectRatio );
			}
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
