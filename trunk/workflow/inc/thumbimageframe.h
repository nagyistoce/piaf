/***************************************************************************
 *  thumbimageframe - Thumb display picture
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


#ifndef THUMBIMAGEFRAME_H
#define THUMBIMAGEFRAME_H

#include <QtGui/QFrame>
#include "imageinfo.h"

namespace Ui {
	class ThumbImageFrame;
}

class ThumbImageFrame : public QFrame {
	Q_OBJECT
	Q_DISABLE_COPY(ThumbImageFrame)
public:
	explicit ThumbImageFrame(QWidget *parent = 0);
	virtual ~ThumbImageFrame();

	/** @brief set mirror twin frame */
	void setTwin(ThumbImageFrame * pTwin) { mpTwin = pTwin; }

	/** @brief get mirror twin frame */
	ThumbImageFrame * getTwin() { return mpTwin ; }

	/** @brief Get file name */
	QString getFilename() { return m_imagePath; }

	/** @brief Set the image info */
	void setImageInfoStruct(t_image_info_struct * pinfo);

	/** @brief Set the image */
	void setImageFile(const QString &  imagePath, IplImage * img = NULL, int score = -1);

	/** @brief set the selected flag */
	void setSelected(bool selected);

	/** @brief tell if the frame is selected */
	bool isSelected() { return mSelected; }

	/** @brief set the active flag */
	void setActive(bool active);

	/** @brief tell if the frame is selected */
	bool isActive() { return mActive; }

//	/** @brief Shift key is pressed */
//	bool shiftPressed() { return mShift; }
//	/** @brief Ctrl key is pressed */
//	bool ctrlPressed() { return mCtrl; }

protected:
	virtual void changeEvent(QEvent *e);

	virtual void keyPressEvent ( QKeyEvent * e );
	virtual void keyReleaseEvent ( QKeyEvent * e );
	virtual void focusInEvent ( QFocusEvent * event );
	virtual void focusOutEvent ( QFocusEvent * event );
private:
	QString m_imagePath;
	ThumbImageFrame * mpTwin;
	Ui::ThumbImageFrame *m_ui;
	bool mSelected; ///< user selection flag
	bool mActive; ///< active file flag: this is the file seen in main display
	bool mCtrl, mShift;

private slots:
	void on_globalImageLabel_signalMousePressEvent(QMouseEvent * e);
	void on_globalImageLabel_signalMouseMoveEvent(QMouseEvent * e);
	void on_globalImageLabel_signalMouseDoubleClickEvent ( QMouseEvent * );

signals:
	void signalThumbDoubleClicked(QString);
	void signalThumbClicked(QString);
	void signalThumbSelected(QString);

private:
//	QString m_filename;
	/// update background color depending on mActive and mSelected
	void updateBackground();

protected:
	virtual void mouseDoubleClickEvent ( QMouseEvent * event );
	virtual void mouseMoveEvent ( QMouseEvent * event );
	virtual void mousePressEvent ( QMouseEvent * event );
	virtual void mouseReleaseEvent ( QMouseEvent * event );

signals:
//	void changeEvent(QEvent *e);
	void signal_mouseDoubleClickFile ( QString filename );

	void signal_mouseDoubleClickEvent ( QMouseEvent * event );
	void signal_mouseMoveEvent ( QMouseEvent * event );
	void signal_mousePressEvent ( QMouseEvent * event );
	void signal_mouseReleaseEvent ( QMouseEvent * event );

	void signal_keyPressEvent ( QKeyEvent * e );
	void signal_keyReleaseEvent ( QKeyEvent * e );

	void signal_click(ThumbImageFrame *);
	void signal_shiftClick(ThumbImageFrame *);
	void signal_ctrlClick(ThumbImageFrame *);

};

#endif // THUMBIMAGEFRAME_H
