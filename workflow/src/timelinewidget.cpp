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
#include "piaf-common.h"

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
	if(mFileSize <= 0)
	{
		return 0;
	}

	int cursorPos = (int)roundf( size().width() * (double)filepos
								 / (double)mFileSize );
	return cursorPos;
}


void TimeLineWidget::paintEvent( QPaintEvent * e)
{
	QPainter p( this );

	// Draw bookmarks
	p.setPen( QColor( qRgb(255,192,0) ) );

	int margin = 2;
	QList<video_bookmark_t>::iterator it;
	for(it = m_bookmarksList.begin(); it != m_bookmarksList.end(); ++it)
	{
		t_movie_pos bkmk = (*it).movie_pos;

		PIAF_MSG(SWLOG_TRACE, "\t\tadded bookmark name='%s' "
				 "prevAbsPos=%lld prevKeyFrame=%lld nbFrameSinceKey=%d",
				 bkmk.name.toAscii().data(),
				 bkmk.prevAbsPosition,
				 bkmk.prevKeyFramePosition,
				 bkmk.nbFramesSinceKeyFrame
				 ); // the node really is an element.

		int disppos = computeCursorDisplayPos(bkmk.prevAbsPosition);

		p.drawLine(disppos, margin, disppos, size().height()-1-margin);
	}

	// Then draw cursor
	mCursorDisplayPos = computeCursorDisplayPos(mFilePos);
	p.setPen(QColor(qRgb(0,255,0)));
	p.drawRect(mCursorDisplayPos - 1, 0, 2, size().height()-1);

	QString frameStr;
	frameStr.sprintf("%5d", mCursorDisplayPos);
	p.drawText(QPoint(0,0), frameStr);

	p.end();
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
	if(mFileSize == 0) {
		fprintf(stderr, "TimeLineWidget::%s:%d invalid filesize = %llu !\n",
				__func__, __LINE__,
				mFileSize);
		printImageInfoStruct(&m_image_info_struct);

		return;
	}

	mLeftButton = (e->button() == Qt::LeftButton);
	setCursorDisplayPos(e->pos().x());

}

void TimeLineWidget::setCursorDisplayPos(int posx)
{
	if(mLeftButton) {
		if(mMagnetic && mLeftButton) {
			t_movie_pos found;
			found.nbFramesSinceKeyFrame = found.prevAbsPosition = found.prevKeyFramePosition = 0;

			int dmin = 10;
			QList<video_bookmark_t>::iterator it;
			for(it = m_bookmarksList.begin(); it != m_bookmarksList.end(); ++it)
			{
				t_movie_pos pos = (*it).movie_pos;
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
					mFileSize, found.prevAbsPosition
					);
				mFilePos = found.prevAbsPosition;
				emit signalCursorBookmarkChanged(found);
				repaint();

				return;
			}

		}

		mFilePos = posx / (double)size().width() * (double)mFileSize;
		fprintf(stderr, "TimeLineWidget::%s:%d : e->pos = %d / w=%d / filesize=%llu => new pos=%llu\n",
			__func__, __LINE__,
			posx, size().width(),
			mFileSize, mFilePos
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


