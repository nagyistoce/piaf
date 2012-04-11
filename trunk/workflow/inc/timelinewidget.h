/***************************************************************************
	   timelinewidget.h  -  Timeline for movie navigation
							 -------------------
	begin                : Thu Sept 01 2011
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
#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QLabel>
#include <QMouseEvent>
#include <QKeyEvent>
#include "imageinfo.h"

/** @brief Timeline for movie navigation */
class TimeLineWidget : public QLabel
{
    Q_OBJECT

public:
    explicit TimeLineWidget(QWidget *parent = 0);
    ~TimeLineWidget();



	/** @brief Activate magnetic mode: the cursor will be attrcated by bookmarks */
	void setMagneticMode(bool on) { mMagnetic = on; }

	/** @brief Set the absolute position in file */
	void setFilePosition(unsigned long long filepos);

	/** @brief Set the file size */
	int setFileSize(unsigned long long fileSize)
	{
		mFileSize = fileSize;
		repaint();
		return 0;
	}

	/** @brief compute display position from position in file */
	int computeCursorDisplayPos(unsigned long long filepos);

	/** @brief Set the list of bookmarks */
	void setBookmarkList(QList<video_bookmark_t> bkmkList) {
		m_bookmarksList = bkmkList;
		repaint();
	}

protected:
    void changeEvent(QEvent *e);
	virtual void focusInEvent ( QFocusEvent * event );
	virtual void focusOutEvent ( QFocusEvent * event );
	virtual void keyPressEvent ( QKeyEvent * event );
	virtual void keyReleaseEvent ( QKeyEvent * event );
	bool mShift, mCtrl;

	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);
	virtual void wheelEvent ( QWheelEvent * event );

	/** @brief Overloaded paintEvent */
	void paintEvent( QPaintEvent * );

private:
	/// Info about the file: bookmarks, ...
	t_image_info_struct m_image_info_struct;

	/// Cursor file position
	unsigned long long mFilePos;

	/// Bookmarks list
	QList<video_bookmark_t> m_bookmarksList;

	/// File size
	unsigned long long mFileSize;
	/// magnetic mode. Default=true
	bool mMagnetic;

	bool mLeftButton;

	/// Return the cursor position in display reference
	int computeCursorPos(unsigned long long filepos);

	/// Set cursor position in display
	void setCursorDisplayPos(int posx);

	/// Current position
	int mCursorDisplayPos;

signals:
	void signalCursorPositionChanged(unsigned long long);
	void signalCursorBookmarkChanged(t_movie_pos );

};

#endif // TIMELINEWIDGET_H
