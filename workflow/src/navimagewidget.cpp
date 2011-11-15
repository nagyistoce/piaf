/***************************************************************************
 *  navimagewidget - Navigation display for main image
 *
 *  Jul 2 21:10:56 2009
 *  Copyright  2009  Christophe Seyve
 *  Email cseyve@free.fr
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "navimagewidget.h"
#include "ui_navimagewidget.h"
#include "piaf-common.h"
#include <stdio.h>

NavImageWidget::NavImageWidget(QWidget *parent) :
	QWidget(parent),
	m_ui(new Ui::NavImageWidget)
{
	m_last_zoom = QPoint( m_fullRect.width()/2,
						   m_fullRect.height()/2);
	m_zoom_scale = -1;
	m_ui->setupUi(this);
}

NavImageWidget::~NavImageWidget()
{
	delete m_ui;
}

void NavImageWidget::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		m_ui->retranslateUi(this);
		break;
	default:
		break;
	}
}


void NavImageWidget::on_zoomFitButton_clicked()
{
	m_zoom_scale = -1;
	m_last_zoom = QPoint( m_fullRect.width()/2,
						   m_fullRect.height()/2);
	if(m_displayImage.isNull()) { return; }
	emit signalZoomOn(0, 0, 0);
}

void NavImageWidget::on_zoomx1Button_released()
{
	m_zoom_scale = 1;
	emit signalZoomOn( m_last_zoom.x(),
					   m_last_zoom.y(), 1);
}

void NavImageWidget::on_zoomx2Button_released()
{
	m_zoom_scale = 2;
	emit signalZoomOn( m_last_zoom.x(),
					   m_last_zoom.y(), 2);
}

void NavImageWidget::slot_signalImageChanged(QImage imageIn)
{
	setImage(imageIn);
}

void NavImageWidget::slot_mainImageWidget_signalZoomRect(QRect r)
{
	if(m_fullRect.width()<=0 || m_fullRect.height()<=0) { return; }

	QRect scaled = QRect(r.x() * (float)m_displayImage.width() / (float)m_fullRect.width(),
						 r.y() * (float)m_displayImage.height() / (float)m_fullRect.height(),
						 r.width() * (float)m_displayImage.width() / (float)m_fullRect.width(),
						 r.height() * (float)m_displayImage.height() / (float)m_fullRect.height()
						 );
	m_zoomRect = r;

	m_ui->globalImageLabel->setOverlayRect(scaled, QColor(qRgb(0,0,255)));
	m_ui->globalImageLabel->setPixmap( m_displayImage );
	m_ui->globalImageLabel->repaint();
}

void NavImageWidget::slot_mainImageWidget_signalZoomChanged(float l_zoom_scale)
{
	if(m_fullRect.width()<=0 || m_fullRect.height()<=0) { return; }

	int disp_w = m_displayImage.width();
	int disp_h = m_displayImage.height();
	float zoom_min = std::min((float)disp_w/m_fullRect.width(),
							  (float)disp_h/m_fullRect.height());
	float zoom_max = 2.f;

	//	l_zoom_scale = zoom_min + (zoom_max-zoom_min)*position/100.f;
	// => position = (l_zoom_scale - zoom_min)*100/(zoom_max-zoom_min)
	int position = (l_zoom_scale - zoom_min)*100.f/(zoom_max-zoom_min);
	fprintf(stderr, "NavImW::%s:%d : cropRect=%d,%d+%dx%d "
			"=> lzoomscale=%g / zmin/max=%g,%g"
			" => position=%d\n",
			__func__, __LINE__,
			m_zoomRect.x(), m_zoomRect.y(), m_zoomRect.width(), m_zoomRect.height(),
			l_zoom_scale, zoom_min, zoom_max,
			position
			);
	m_zoom_scale = l_zoom_scale;
	if(position != m_ui->zoomSlider->value())
	{
		m_ui->zoomSlider->blockSignals(true);
		m_ui->zoomSlider->setValue(position);
		m_ui->zoomSlider->blockSignals(false);

	}
}
void NavImageWidget::setImage(QImage fullImage)
{
	PIAF_MSG(SWLOG_DEBUG, "NavImagewidget: fullImage=%dx%dx%d\n",
			fullImage.width(), fullImage.height(), fullImage.depth());

	if(fullImage.isNull())
	{
		PIAF_MSG(SWLOG_ERROR, "input image is null");
		m_displayImage.fill(127);
		m_fullRect = QRect(0, 0, 0, 0);
		return;
	}
	else
	{
		int wdisp = m_ui->globalImageLabel->width()-2;
		int hdisp = m_ui->globalImageLabel->height()-2;

		m_fullRect = QRect(0, 0, fullImage.width(), fullImage.height());

		m_displayImage = QPixmap::fromImage(fullImage.scaled( wdisp, hdisp,
										  Qt::KeepAspectRatio ));
		m_inputImage = m_displayImage;
	}

	m_ui->globalImageLabel->setPixmap( m_displayImage );
	m_ui->globalImageLabel->repaint();

	// reset zoom scale
//	m_zoom_scale = 0;

}

void NavImageWidget::setImageFile(const QString & imagePath)
{
	// "NavImageWidget::%s:%d ('%s')\n",
	//		__func__, __LINE__,
	//		imagePath);

	QImage fullImage(imagePath);
	setImage(fullImage);
}

void  NavImageWidget::zoomOn(int x, int y, float scale)
{
	if(m_displayImage.isNull()) { return; }

	int center_x = x * m_fullRect.width() / m_displayImage.width();
	int center_y = y * m_fullRect.height() / m_displayImage.height();

	fprintf(stderr, "NavImW::%s:%d: zoomOn(%d, %d, sc=%g) => emit signalZoomOn(%d, %d, scale=%g)",
			__func__, __LINE__,
			x,y, scale,
			center_x, center_y, scale
			);
	emit signalZoomOn(center_x, center_y, scale);

	//  display a rectangle
	m_zoom_scale = scale;

	m_ui->globalImageLabel->setPixmap(m_displayImage);
}

void NavImageWidget::on_globalImageLabel_signalMousePressEvent(QMouseEvent * e)
{
	if(m_displayImage.isNull()) { return; }
	if(!e) return;

	if(m_zoom_scale == 0) // toggle view
	{
		// Select the zoomx1button
//		m_ui->zoomx1Button->set
		fprintf(stderr, "NavImW::%s:%d: sc=%g => toogle zoom 0/1",
				__func__, __LINE__, m_zoom_scale
				);
		m_zoom_scale = 1;
	}

	m_lastClick = e->pos();
	int disp_w = m_displayImage.width();
	int disp_h = m_displayImage.height();
	int glob_w = m_ui->globalImageLabel->width();
	int glob_h = m_ui->globalImageLabel->height();
	int xoff = (glob_w - disp_w)/2;// offset on X on widget
	int yoff = (glob_h - disp_h)/2;// offset on Y on widget
	int x = e->pos().x();
	int y = e->pos().y();
	x = (int)( (float) (x - xoff) * (float)m_fullRect.width() / (float)disp_w
			   );//use center instead of topleft - (float)m_zoomRect.width()*0.5f);
	y = (int)( (float) (y - yoff) * (float)m_fullRect.height() / (float)disp_h
			   );//use center instead of topleft - (float)m_zoomRect.height()*0.5f);
	m_last_zoom = QPoint(x,y);

	fprintf(stderr, "NavImageWidget::%s:%d : "
			"clic on %d,%d / widg=%dx%d => %d,%d in dispImg=%dx%d / full=%dx%d"
			" => zoom center = %d,%d sc=%g\n",
			__func__ ,__LINE__,
			e->pos().x(), e->pos().y(), glob_w, glob_h,
			e->pos().x()-xoff, e->pos().y()-yoff,disp_w, disp_h,
			m_fullRect.width(), m_fullRect.height(),
			x, y, m_zoom_scale
			);
	emit signalZoomOn(x, y, m_zoom_scale);
}

void NavImageWidget::on_globalImageLabel_signalMouseReleaseEvent(QMouseEvent * e)
{

}

void NavImageWidget::on_globalImageLabel_signalMouseMoveEvent(QMouseEvent * e)
{
	if(!e) return;
	if(e->buttons()==Qt::LeftButton) {
		int x = e->pos().x(), y =  e->pos().y();
		int disp_w = m_displayImage.width();
		int disp_h = m_displayImage.height();

		if(disp_w<=0 || disp_h<=0) { return; }

		int glob_w = m_ui->globalImageLabel->width();
		int glob_h = m_ui->globalImageLabel->height();
		int xoff = (glob_w - disp_w)/2;// offset on X on widget
		int yoff = (glob_h - disp_h)/2;// offset on Y on widget

		x = (int)( (float) (x - xoff) * (float)m_fullRect.width() / (float)disp_w
				   );//use center instead of topleft - (float)m_zoomRect.width()*0.5f);
		y = (int)( (float) (y - yoff) * (float)m_fullRect.height() / (float)disp_h
				   );//use center instead of topleft - (float)m_zoomRect.height()*0.5f);
		m_last_zoom = QPoint(x,y);

		fprintf(stderr, "NavImageWidget::%s:%d : "
				"clic on %d,%d / widg=%dx%d => %d,%d in dispImg=%dx%d / full=%dx%d"
				" => zoom center = %d,%d sc=%g",
				__func__ ,__LINE__,
				e->pos().x(), e->pos().y(),glob_w, glob_h,
				e->pos().x()-xoff, e->pos().y()-yoff,disp_w, disp_h,
				m_fullRect.width(), m_fullRect.height(),
				x, y, m_zoom_scale
				);
		emit signalZoomOn(x, y, m_zoom_scale);
	}
}

void NavImageWidget::on_globalImageLabel_signalWheelEvent( QWheelEvent * e )
{
	if(!e) return;
	int numDegrees = e->delta() / 8;
	int numSteps = numDegrees / 15;
	m_zoom_scale += numSteps;

	int x = e->pos().x(), y =  e->pos().y();
	m_lastClick = e->pos();

	int glob_w = m_ui->globalImageLabel->width();
	int glob_h = m_ui->globalImageLabel->height();
	int disp_w = m_displayImage.width();
	int disp_h = m_displayImage.height();

	x = (x - (glob_w - disp_w)/2) * m_fullRect.width() / disp_w;
	y = (y - (glob_h - disp_h)/2) * m_fullRect.height() / disp_h;

	m_last_zoom = QPoint(x,y);

	fprintf(stderr, "NavImW::%s:%d: => emit signalZoomOn(%d, %d, scale=%g)",
			__func__, __LINE__,
			x,y, m_zoom_scale
			);
	emit signalZoomOn(x, y, m_zoom_scale);
}

void NavImageWidget::on_zoomSlider_sliderMoved(int position)
{
	if(m_fullRect.width() == 0 || m_fullRect.height()==0) return;
	int disp_w = m_displayImage.width();
	int disp_h = m_displayImage.height();
	float zoom_min = std::min((float)disp_w/m_fullRect.width(),
							  (float)disp_h/m_fullRect.height());
	float zoom_max = 2.f;

	m_zoom_scale = zoom_min + (zoom_max-zoom_min)*position/100.f;

	// position = 0 => m_zoom_scale = disp_w/m_fullRect.width()
	fprintf(stderr, "NavImageWidget::%s:%d: slider moved to %d"
			" => zoom min/max=%g/%g => zoom=%g\n",
			__func__, __LINE__, position,
			zoom_min, zoom_max, m_zoom_scale);
	emit signalZoomOn(m_last_zoom.x(), m_last_zoom.y(), m_zoom_scale);
}

void NavImageWidget::on_zoomSlider_sliderReleased()
{
	fprintf(stderr, "NavImageWidget::%s:%d: slider at pos=%d\n",
			__func__, __LINE__, m_ui->zoomSlider->value());

}

/* Set the zoom position */
void NavImageWidget::setZoomRect(QRect cropRect)
{
	if(m_fullRect.width() == 0 || m_fullRect.height()==0) return;

	m_displayRect = cropRect;

}


