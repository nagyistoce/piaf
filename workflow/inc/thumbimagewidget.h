/***************************************************************************
 *  thumbimagewidget - Thumb display picture with a few editing capabilities
 *
 *  Jul 2 21:10:56 2009
 *  Copyright  2007  Christophe Seyve
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

#ifndef THUMBIMAGEWIDGET_H
#define THUMBIMAGEWIDGET_H

#include <QtGui/QWidget>
#include "imageinfo.h"

namespace Ui {
    class ThumbImageWidget;
}

class ThumbImageWidget : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(ThumbImageWidget)
public:
    explicit ThumbImageWidget(QWidget *parent = 0);
    virtual ~ThumbImageWidget();
	/** @brief Set the image */
	void setImageFile(const QString &  imagePath, IplImage * img = NULL, int score = -1);

protected:
    virtual void changeEvent(QEvent *e);
	virtual void mouseDoubleClickEvent ( QMouseEvent * event );
	virtual void mouseMoveEvent ( QMouseEvent * event );
	virtual void mousePressEvent ( QMouseEvent * event );
	virtual void mouseReleaseEvent ( QMouseEvent * event );
private:
    Ui::ThumbImageWidget *m_ui;
	QString m_imagePath;

signals:
	void signal_mouseDoubleClickFile ( QString filename );

	void signal_mouseDoubleClickEvent ( QMouseEvent * event );
	void signal_mouseMoveEvent ( QMouseEvent * event );
	void signal_mousePressEvent ( QMouseEvent * event );
	void signal_mouseReleaseEvent ( QMouseEvent * event );
};

#endif // THUMBIMAGEWIDGET_H
