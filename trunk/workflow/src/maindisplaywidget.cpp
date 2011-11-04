/***************************************************************************
 *      Main image display in middle of GUI
 *
 *  Sun Aug 30 14:06:41 2011
 *  Copyright  2011  Christophe Seyve
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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "inc/maindisplaywidget.h"
#include "ui_maindisplaywidget.h"


#include "piaf-common.h"
#include "PiafFilter.h"

#include "moviebookmarkform.h"

MainDisplayWidget::MainDisplayWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainDisplayWidget)
{
    ui->setupUi(this);
	mpFilterSequencer = NULL;
	mpImageInfoStruct = NULL;
	mPlayGrayscale = false;
	mPlaySpeed = 1.f;
	m_editBookmarksForm = NULL;

	connect(&mPlayTimer, SIGNAL(timeout()), this, SLOT(on_mPlayTimer_timeout()));

}

MainDisplayWidget::~MainDisplayWidget()
{
    delete ui;
	mpFilterSequencer = NULL;

}

void MainDisplayWidget::setFilterSequencer(FilterSequencer * pFS)
{
	if(mpFilterSequencer)
	{
		mpFilterSequencer->disconnect();
	}
	mpFilterSequencer = pFS;

	ui->mainImageWidget->setFilterSequencer(pFS);
}

int MainDisplayWidget::setImageFile(QString imagePath,
								   t_image_info_struct * pinfo )
{
	m_listBookmarks.clear();
	mpImageInfoStruct = pinfo;

	int ret = 0;
	// print allocated images
	tmPrintIplImages();
	ui->stackedWidget->hide();
	fprintf(stderr, "NavImageWidget::%s:%d ('%s')\n",
			__func__, __LINE__,
			imagePath.toAscii().data());

	if(!m_fullImage.load(imagePath)) {
		fprintf(stderr, "NavImageWidget::%s:%d could not load '%s'\n",
				__func__, __LINE__,
				imagePath.toAscii().data());
		ret = -1;
		m_fullImage = QImage(":/icons/error.png");
	}

	ui->mainImageWidget->setImage(m_fullImage, pinfo);

	return ret;
}

void  MainDisplayWidget::zoomOn(int x, int y, int scale) {

	ui->mainImageWidget->zoomOn(x,y,scale);
}

/******************************************************************************

							MOVIE PLAYER SECTION

  ******************************************************************************/
/* @brief Set the movie file */
int MainDisplayWidget::setMovieFile(QString moviePath, t_image_info_struct * pinfo)
{
	mpImageInfoStruct = pinfo;

	m_listBookmarks.clear();
	mFileVA.stopAcquisition();

	tBoxSize boxsize; memset(&boxsize, 0, sizeof(tBoxSize));

	int ret = mFileVA.openDevice(moviePath.toUtf8().data(), boxsize);
	if(ret >= 0) {
		m_fullImage = iplImageToQImage( mFileVA.readImageRGB32() );
	} else {
		PIAF_MSG(SWLOG_ERROR, "Could not open file '%s'", moviePath.toUtf8().data());
	}

	if(pinfo)
	{
		QList<t_movie_pos>::iterator it;
		int idx=0;
		for(it=pinfo->bookmarksList.begin(); it!=pinfo->bookmarksList.end(); ++it, ++idx)
		{
			t_movie_pos pos = (*it);
			appendBookmark(pos);
		}
		ui->timeLineWidget->setFileInfo(*pinfo);
	}

	ui->timeLineWidget->setFilePosition(0);
	ui->mainImageWidget->setImage(m_fullImage, pinfo);
	ui->stackedWidget->show();
	ui->stackedWidget->setCurrentIndex(0);

	if(mFileVA.getFrameRate()>0) {
		mPlayTimer.setInterval((int)(1000.f /(mPlaySpeed * (float)mFileVA.getFrameRate())));
	}

	return ret;
}
void MainDisplayWidget::appendBookmark(t_movie_pos pos)
{
	video_bookmark_t bmk;
	bmk.pAction = NULL;
	bmk.index = m_listBookmarks.count();
	bmk.movie_pos = pos;
	if(mpImageInfoStruct && mpImageInfoStruct->filesize > 0) {
		bmk.percent = (double)pos.prevAbsPosition / (double)mpImageInfoStruct->filesize * 100.;
	}
	else {
		bmk.percent = -1;
	}
	m_listBookmarks.append(bmk);

}

void MainDisplayWidget::slotNewBookmarkList(QList<video_bookmark_t> list) {
	// clear actions
	mpImageInfoStruct->bookmarksList.clear();

	QList<video_bookmark_t>::iterator it;
	for(it = m_listBookmarks.begin(); it != m_listBookmarks.end(); it++) {

		// delete action
		//menuBookmarks->removeAction((*it).pAction);
	}
	m_listBookmarks.clear();

	QList<t_movie_pos> bmklist;
	for(it = list.begin(); it != list.end(); it++) {
		mpImageInfoStruct->bookmarksList.append((*it).movie_pos);

		appendBookmark((*it).movie_pos);
	}

	ui->timeLineWidget->setFileInfo(*mpImageInfoStruct);
}

void MainDisplayWidget::on_goFirstButton_clicked()
{
	mFileVA.slotRewindMovie();
	updateDisplay();

	// to update navigation image widget
	emit signalImageChanged(m_fullImage);

}

void MainDisplayWidget::updateDisplay()
{
	m_fullImage = iplImageToQImage( mPlayGrayscale ? mFileVA.readImageY() : mFileVA.readImageRGB32() );

	ui->mainImageWidget->setImage(m_fullImage, NULL);
	ui->timeLineWidget->setFilePosition(mFileVA.getAbsolutePosition());
}

void MainDisplayWidget::on_goPrevButton_clicked()
{
	mFileVA.slotRewindMovie();
	mPlayTimer.stop();

	ui->playButton->blockSignals(true);
	ui->playButton->setChecked(false);
	ui->playButton->blockSignals(false);

	updateDisplay();
	// to update navigation image widget
	emit signalImageChanged(m_fullImage);
}

void MainDisplayWidget::on_playButton_toggled(bool checked)
{
	if(checked)
	{
		mPlayTimer.start((int)(1000.f / (mPlaySpeed * (float)mFileVA.getFrameRate())));
	} else {
		mPlayTimer.stop();
	}

	// to update navigation image widget
	emit signalImageChanged(m_fullImage);
}

void MainDisplayWidget::on_mainImageWidget_signalZoomRect(QRect cropRect)
{
	fprintf(stderr, "MainDisplayWidget::%s:%d : received signalZoomRect(cropRect=%d,%d+%dx%d);\n",
			__func__, __LINE__,
			cropRect.x(), cropRect.y(), cropRect.width(), cropRect.height());
	emit signalZoomRect(cropRect);
}

void MainDisplayWidget::on_goNextButton_clicked()
{
	bool got_picture = mFileVA.GetNextFrame();
	if(got_picture)
	{
		updateDisplay();
	}
	else if(mPlayTimer.isActive())
	{
		ui->playButton->setChecked(false);
	}
	// to update navigation image widget
	emit signalImageChanged(m_fullImage);

}

void MainDisplayWidget::on_mPlayTimer_timeout()
{
	bool got_picture = mFileVA.GetNextFrame();
	if(got_picture)
	{
		updateDisplay();
	}
	else if(mPlayTimer.isActive())
	{
		ui->playButton->setChecked(false);
	}
}

void MainDisplayWidget::on_goLastButton_clicked()
{

}

void MainDisplayWidget::on_grayscaleButton_toggled(bool gray)
{
	mPlayGrayscale = gray;

	updateDisplay();
}

void MainDisplayWidget::on_speedComboBox_currentIndexChanged(QString val)
{
	bool ok = false;
	val.replace("x", "");

	if(val.contains("/"))
	{
		QStringList split = val.split("/");
		if(split.count()>1)
		{
			int denom = split[1].toInt(&ok);
			if(ok) {
				mPlaySpeed = 1.f / (float)denom;
			}
		}
		else {
			fprintf(stderr, "MainDisplayWidget::%s:%d : cannot split val='%s' playspeed=%g\n",
					__func__, __LINE__,
					val.toAscii().data(),
					mPlaySpeed);

		}
	} else {
		mPlaySpeed = val.toInt(&ok);
	}

	fprintf(stderr, "MainDisplayWidget::%s:%d : val='%s' playspeed=%g ok=%c\n",
			__func__, __LINE__,
			val.toAscii().data(),
			mPlaySpeed, ok?'T':'F');
	if(mFileVA.getFrameRate()>0) {
		mPlayTimer.setInterval((int)(1000.f /(mPlaySpeed * (float)mFileVA.getFrameRate())));
	}
}

void MainDisplayWidget::on_timeLineWidget_signalCursorBookmarkChanged(
		t_movie_pos movie_pos)
{
	fprintf(stderr, "MainDisplayWidget::%s:%d received !\n", __func__, __LINE__);
	if(movie_pos.nbFramesSinceKeyFrame <= 1) {
		mFileVA.setAbsolutePosition(movie_pos.prevAbsPosition);

		// display
		on_goNextButton_clicked();

	} // else go some frames before
	else {
		mFileVA.setAbsolutePosition(movie_pos.prevKeyFramePosition);
		fprintf(stderr, "[VidPlay]::%s:%d : skip %d frames to reach the bookmark frame\n",
				__func__, __LINE__, movie_pos.nbFramesSinceKeyFrame-1);
		// display
		for(int nb = 0; nb < movie_pos.nbFramesSinceKeyFrame-1; nb++) {
			mFileVA.GetNextFrame();
		}

		// Display & process last one
		fprintf(stderr, "[MainDispW]::%s:%d : Display & process last one\n",
				__func__, __LINE__);
		on_goNextButton_clicked();
		fprintf(stderr, "[MainDispW]::%s:%d : jump done & processed\n",
				__func__, __LINE__);

	}

}

void MainDisplayWidget::on_timeLineWidget_signalCursorPositionChanged(unsigned long long filepos)
{
	fprintf(stderr, "MainDisplayWidget::%s:%d received !\n", __func__, __LINE__);
	mFileVA.setAbsolutePosition(filepos);
	on_goNextButton_clicked();

}

void MainDisplayWidget::on_magneticButton_toggled(bool on)
{
	ui->timeLineWidget->setMagneticMode(on);
}

void MainDisplayWidget::on_bookmarksButton_clicked()
{
	if(!m_editBookmarksForm)
	{
		m_editBookmarksForm = new MovieBookmarkForm(NULL);

		connect(m_editBookmarksForm, SIGNAL(signalNewBookmarkList(QList<video_bookmark_t>)),
				this, SLOT(slotNewBookmarkList(QList<video_bookmark_t>)));
	}

	m_editBookmarksForm->setBookmarkList(m_listBookmarks);
	m_editBookmarksForm->show();
}

void MainDisplayWidget::on_addBkmkButton_clicked()
{
	appendBookmark(mFileVA.getMoviePosition());
	if(mpImageInfoStruct) {
		mpImageInfoStruct->bookmarksList.append(mFileVA.getMoviePosition());
		ui->timeLineWidget->setFileInfo(*mpImageInfoStruct);
	}
}
