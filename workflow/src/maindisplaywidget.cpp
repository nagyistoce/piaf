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

	mPlayGrayscale = false;
	mPlaySpeed = 1.f;

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
	mFileVA.stopAcquisition();

	tBoxSize boxsize; memset(&boxsize, 0, sizeof(tBoxSize));

	int ret = mFileVA.openDevice(moviePath.toUtf8().data(), boxsize);
	if(ret >= 0) {
		m_fullImage = iplImageToQImage( mFileVA.readImageRGB32() );
	} else {
		PIAF_MSG(SWLOG_ERROR, "Could not open file '%s'", moviePath.toUtf8().data());
	}

	ui->mainImageWidget->setImage(m_fullImage, pinfo);
	ui->stackedWidget->show();
	ui->stackedWidget->setCurrentIndex(0);
	return ret;
}


void MainDisplayWidget::on_goFirstButton_clicked()
{
	mFileVA.slotRewindMovie();
	updateDisplay();
}

void MainDisplayWidget::updateDisplay()
{
	m_fullImage = iplImageToQImage( mPlaySpeed ? mFileVA.readImageRGB32() : mFileVA.readImageY() );

	ui->mainImageWidget->setImage(m_fullImage, NULL);
}

void MainDisplayWidget::on_goPrevButton_clicked()
{
	mFileVA.slotRewindMovie();
	mPlayTimer.stop();

	ui->playButton->blockSignals(true);
	ui->playButton->setChecked(false);
	ui->playButton->blockSignals(false);

	updateDisplay();
}

void MainDisplayWidget::on_playButton_toggled(bool checked)
{
	if(checked)
	{
		mPlayTimer.start(1000 * mPlaySpeed / mFileVA.getFrameRate());
	} else {
		mPlayTimer.stop();
	}
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
}

void MainDisplayWidget::on_mPlayTimer_timeout()
{
	on_goNextButton_clicked();
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
	if(val.contains("/"))
	{
		QStringList split = val.split("/");
		if(split.count()>1)
		{
			mPlaySpeed = split[1].toInt();
		}
	} else {
		mPlaySpeed = val.toInt();
	}
	fprintf(stderr, "MainDisplayWidget::%s:%d : playspeed=%g\n",
			__func__, __LINE__, mPlaySpeed);
	if(mPlayTimer.isActive())
	{
		if(mFileVA.getFrameRate()>0) {
			mPlayTimer.setInterval(1000 * mPlaySpeed / mFileVA.getFrameRate());
		}
	}
}

void MainDisplayWidget::on_magneticButton_clicked()
{

}

void MainDisplayWidget::on_bookmarksButton_clicked()
{
	if(!m_editBookmarksForm)
	{
		m_editBookmarksForm = new MovieBookmarkForm(NULL);
		m_editBookmarksForm->setBookmarkList(m_listBookmarks);
	}
	m_editBookmarksForm->show();
}
