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


#ifndef NAVIMAGEWIDGET_H
#define NAVIMAGEWIDGET_H

#include <QtGui/QWidget>
#include "qimagedisplay.h"

namespace Ui {
	class NavImageWidget;
}

/** @brief Global image navigation */
class NavImageWidget : public QWidget {
	Q_OBJECT
	Q_DISABLE_COPY(NavImageWidget)
public:
	explicit NavImageWidget(QWidget *parent = 0);
	virtual ~NavImageWidget();

	/** @brief Set the image file path */
	void setImageFile(const QString &  imagePath);

	/** @brief Set the image directly */
	void setImage(QImage fullImage);

	/** @brief Set the main display crop size  */
	void setMainDisplaySize(int x, int h);

	/** @brief Set the zoom position */
	void setZoomRect(QRect cropRect);

protected:
	virtual void changeEvent(QEvent *e);

private:
	Ui::NavImageWidget *m_ui;

	QRect m_fullRect;///< size of fullscale image
	QRect m_zoomRect; ///< Position of room rectangle
	QRect m_displayRect;
	QPixmap m_inputImage;
	QPixmap m_displayImage;

	/** @brief Zoom on a part of input image at a specified scale */
	void zoomOn(int x, int y, float scale);
	float m_zoom_scale; ///< zoom factor
	QPoint m_last_zoom; ///< last zoom position in original image size reference
	QPoint m_lastClick;

public slots:
	void slot_signalImageChanged(QImage);
	void slot_mainImageWidget_signalZoomRect(QRect);
	void slot_mainImageWidget_signalZoomChanged(float);

private slots:
	void on_zoomSlider_sliderReleased();
	void on_zoomSlider_sliderMoved(int position);
	void on_zoomx2Button_released();
	void on_zoomx1Button_released();
	void on_zoomFitButton_clicked();
	// Zoom options
	void on_globalImageLabel_signalMousePressEvent(QMouseEvent * e);
	void on_globalImageLabel_signalMouseReleaseEvent(QMouseEvent * e);
	void on_globalImageLabel_signalMouseMoveEvent(QMouseEvent * e);
	void on_globalImageLabel_signalWheelEvent( QWheelEvent * e );
signals:
	void signalZoomOn(int xcenter, int ycenter, float zoom_scale);
};

#endif // NAVIMAGEWIDGET_H
