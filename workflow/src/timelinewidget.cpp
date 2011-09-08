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
#include <QPainter>
#include "inc/timelinewidget.h"

TimeLineWidget::TimeLineWidget(QWidget *parent) :
	QLabel(parent)
{
	clearImageInfoStruct(&m_image_info_struct);
	mMagnetic = true;
	mFilePos = 0;
	mCursorDisplayPos = 0;
	mLeftButton = false;
}

TimeLineWidget::~TimeLineWidget()
{

}

void TimeLineWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:

        break;
    default:
        break;
    }
}

/*Set position in file */
void TimeLineWidget::setFilePosition(unsigned long long filepos)
{
	mFilePos = filepos;
	update();
}

int TimeLineWidget::computeCursorDisplayPos(unsigned long long filepos)
{
	if(m_image_info_struct.filesize <= 0)
	{
		return 0;
	}

	int cursorPos = (int)roundf( size().width() * (double)filepos
								 / (double)m_image_info_struct.filesize );
	return cursorPos;
}

void TimeLineWidget::paintEvent( QPaintEvent * e)
{
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

	// Draw bookmarks
	p.setPen(QColor(qRgb(255,192,0)));

	int margin = 2;
	QList<t_movie_pos>::iterator it;
	for(it = m_image_info_struct.bookmarksList.begin(); it != m_image_info_struct.bookmarksList.end(); ++it)
	{
		t_movie_pos pos = (*it);
		int disppos = computeCursorDisplayPos(pos.prevAbsPosition);
		p.drawLine(disppos, margin, disppos, size().height()-1-margin);
	}

	// Then draw cursor
	mCursorDisplayPos = computeCursorDisplayPos(mFilePos);
	p.setPen(QColor(qRgb(0,255,0)));
	p.drawRect(mCursorDisplayPos - 1, 0, 2, size().height()-1);

	p.end();
	bitBlt( this, cr.topLeft(), &pix );
}

void TimeLineWidget::focusInEvent ( QFocusEvent * event )
{

}

void TimeLineWidget::focusOutEvent ( QFocusEvent * event )
{

}


void TimeLineWidget::keyPressEvent ( QKeyEvent * event )
{

}


void TimeLineWidget::keyReleaseEvent ( QKeyEvent * event )
{

}


void TimeLineWidget::mousePressEvent(QMouseEvent *e)
{
	if(!e) { return; }
	if(m_image_info_struct.filesize <= 0) { return; }

	mLeftButton = (e->button() == Qt::LeftButton);
	setCursorDisplayPos(e->pos().x());

}

void TimeLineWidget::setCursorDisplayPos(int posx)
{
	if(mLeftButton) {
		if(mMagnetic && mLeftButton) {
			t_movie_pos found;
			memset(&found, 0, sizeof(t_movie_pos));

			int dmin = 10;
			QList<t_movie_pos>::iterator it;
			for(it = m_image_info_struct.bookmarksList.begin(); it != m_image_info_struct.bookmarksList.end(); ++it)
			{
				t_movie_pos pos = (*it);
				int disppos = computeCursorDisplayPos(pos.prevAbsPosition);
				int dist = abs(posx -disppos);
				if(dist < dmin){
					dmin = dist;
					found = pos;
				}
			}
			if(found.prevAbsPosition>0)
			{
				// Emit this position
				mFilePos = found.prevAbsPosition;

//				emit signalCursorPositionChanged(mFilePos);
				fprintf(stderr, "TimeLineWidget::%s:%d : e->pos = %d / w=%d / filesize=%llu => emit signal for movie_pos.prevAbsPos= %llu\n",
					__func__, __LINE__,
					posx, size().width(),
					m_image_info_struct.filesize, found.prevAbsPosition
					);
				mFilePos = found.prevAbsPosition;
				emit signalCursorBookmarkChanged(found);
				repaint();

				return;
			}

		}

		mFilePos = posx / (double)size().width() * (double)m_image_info_struct.filesize;
		fprintf(stderr, "TimeLineWidget::%s:%d : e->pos = %d / w=%d / filesize=%llu => new pos=%llu\n",
			__func__, __LINE__,
			posx, size().width(),
			m_image_info_struct.filesize, mFilePos
			);

		emit signalCursorPositionChanged(mFilePos);
		repaint();
	}
}

void TimeLineWidget::mouseReleaseEvent(QMouseEvent *e)
{
	mLeftButton = false;
}

void TimeLineWidget::mouseMoveEvent(QMouseEvent *e)
{
	if(mLeftButton)
	{
		setCursorDisplayPos(e->pos().x());
	}

}

void TimeLineWidget::wheelEvent ( QWheelEvent * event )
{

}

