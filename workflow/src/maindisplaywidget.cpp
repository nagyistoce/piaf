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
#include <QFileInfo>

#include "moviebookmarkform.h"
#include "videocapture.h"
#include "vidacqsettingswindow.h"

#include "piafworkflow-settings.h"

#include "OpenCVEncoder.h"

#include <QDir>

#define FORMAT_IMG		"-%03d.png"
#define FORMAT_AVI		"-%03d.avi"

int g_MDW_debug_mode = EMALOG_INFO;

#define MDW_printf(a,...)  { \
				if(g_MDW_debug_mode>=(a)) { \
					fprintf(stderr,"MainDisplayWidget::%s:%d : ",__func__,__LINE__); \
					fprintf(stderr,__VA_ARGS__); \
					fprintf(stderr,"\n"); \
				} \
			}
MainDisplayWidget::MainDisplayWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainDisplayWidget)
{
    ui->setupUi(this);
	mpegEncoder = NULL;
	mIsRecording = false;

	mpFilterSequencer = NULL;
	mpImageInfoStruct = NULL;

	mPlayGrayscale = false;
	mPlaySpeed = 1.f;
	m_editBookmarksForm = NULL;

	ui->stackedWidget->hide();
	m_pVideoCaptureDoc = NULL;

	connect(&mPlayTimer, SIGNAL(timeout()), this, SLOT(on_mPlayTimer_timeout()));
}

MainDisplayWidget::~MainDisplayWidget()
{
    delete ui;
	if(mpegEncoder)
	{
		stopRecording();
	}
	mpFilterSequencer = NULL;
	m_pVideoCaptureDoc = NULL;
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
	QFileInfo fi(imagePath);
	setWindowTitle(fi.baseName());
	mSourceName = fi.baseName();
	updateSnapCounter();

	m_listBookmarks.clear();
	mpImageInfoStruct = pinfo;
	m_pVideoCaptureDoc = NULL;
	mPlayTimer.stop();

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

void  MainDisplayWidget::zoomOn(int x, int y, float scale) {

	ui->mainImageWidget->zoomOn(x,y,scale);
}

/******************************************************************************

							MOVIE PLAYER SECTION

  ******************************************************************************/
/* @brief Set the movie file */
int MainDisplayWidget::setMovieFile(QString moviePath, t_image_info_struct * pinfo)
{
	mpImageInfoStruct = pinfo;
	QFileInfo fi(moviePath);
	setWindowTitle(fi.baseName());
	mSourceName = fi.baseName();
	updateSnapCounter();

	m_pVideoCaptureDoc = NULL;

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
			t_movie_pos bkmk = (*it);
			PIAF_MSG(SWLOG_INFO, "\t\tadded bookmark name='%s' "
					 "prevAbsPos=%lld prevKeyFrame=%lld nbFrameSinceKey=%d",
					 bkmk.name.toAscii().data(),
					 bkmk.prevAbsPosition,
					 bkmk.prevKeyFramePosition,
					 bkmk.nbFramesSinceKeyFrame
					 ); // the node really is an element.

			appendBookmark(bkmk);
		}
		ui->timeLineWidget->setFileInfo(*pinfo);
	}
	else
	{

	}

	ui->timeLineWidget->setFilePosition(0);
	ui->mainImageWidget->setImage(m_fullImage, pinfo);
	ui->stackedWidget->setCurrentIndex(0);
	ui->stackedWidget->show();

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
	saveImageInfoStruct(mpImageInfoStruct);

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
	IplImage * captureImage = ( mPlayGrayscale ? mFileVA.readImageY() : mFileVA.readImageRGB32() );
	m_fullImage = iplImageToQImage( captureImage );
	if(mpegEncoder && mIsRecording)
	{
		mpegEncoder->encodeImage(captureImage);
	}

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
//	fprintf(stderr, "MainDisplayWidget::%s:%d : received signalZoomRect(cropRect=%d,%d+%dx%d);\n",
//			__func__, __LINE__,
//			cropRect.x(), cropRect.y(), cropRect.width(), cropRect.height());
	emit signalZoomRect(cropRect);
}
void MainDisplayWidget::on_mainImageWidget_signalZoomChanged(float zoom_scale)
{
//	fprintf(stderr, "MainDisplayWidget::%s:%d : received signalZoomChanged(scale=%g);\n",
//			__func__, __LINE__,
//			zoom_scale);
	emit signalZoomChanged(zoom_scale);
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
	bool got_picture = false;
	if(m_pVideoCaptureDoc) // LIVE CAPTURE MODE
	{
		got_picture = (m_pVideoCaptureDoc->waitForImage() >= 0);
		if(got_picture)
		{
			IplImage * captureImage = m_pVideoCaptureDoc->readImage();
//			fprintf(stderr, "MainDisplayW::%s:%d: saving /dev/shm/capture.jpg : %dx%dx%d\n",
//					__func__, __LINE__,
//					captureImage->width, captureImage->height, captureImage->nChannels
//					);
			if(mpegEncoder && mIsRecording)
			{
				mpegEncoder->encodeImage(captureImage);
			}
			m_fullImage = iplImageToQImage( captureImage );
			ui->mainImageWidget->setImage( m_fullImage, NULL);
		}

	} else { // WE ARE IN MOVIE MODE
		got_picture = mFileVA.GetNextFrame();
		if(got_picture)
		{
			updateDisplay();
		}
	}

	if(got_picture)
	{
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
		saveImageInfoStruct(mpImageInfoStruct);
		ui->timeLineWidget->setFileInfo(*mpImageInfoStruct);
	}
}

/******************************************************************************

								VIDEO CAPTURE

*******************************************************************************/
void MainDisplayWidget::updateSnapCounter()
{
	mSnapCounter = mMovieCounter = 0;
	// Check name
	QString fileext;
	QFileInfo fi;
	do {
		fileext.sprintf(FORMAT_IMG, mSnapCounter);
		QString outname = mSourceName + fileext;
		fi.setFile(outname);
		if(fi.exists()) { mSnapCounter++; }
	} while(fi.exists());
	do {
		fileext.sprintf(FORMAT_AVI, mMovieCounter);
		QString outname = mSourceName + fileext;
		fi.setFile(outname);
		if(fi.exists()) { mMovieCounter++; }
	} while(fi.exists());
}

/* Set pointer to capture document */
int MainDisplayWidget::setVideoCaptureDoc(VideoCaptureDoc * pVideoCaptureDoc)
{

	ui->stackedWidget->setCurrentIndex(1);

	m_pVideoCaptureDoc = pVideoCaptureDoc;
	if(!m_pVideoCaptureDoc)
	{
		ui->stackedWidget->hide();
		mPlayTimer.stop();
		return 0;
	}

	if(!m_pVideoCaptureDoc->isRunning())
	{
		m_pVideoCaptureDoc->start();
	}
	mSourceName = QString( m_pVideoCaptureDoc->getVideoProperties().devicename );
	updateSnapCounter();

	double fps = m_pVideoCaptureDoc->getVideoProperties().fps;
	if(fps <= 0)
	{
		fps = 12.5;
		MDW_printf(EMALOG_WARNING, "fps = %g -> force %g",
				   m_pVideoCaptureDoc->getVideoProperties().fps, fps);
	}

	mPlayTimer.setInterval((int)(1000. / fps));

	ui->devicePlayButton->setChecked(true);
	mPlayTimer.start();

	ui->stackedWidget->show();

	return 0;
}

void MainDisplayWidget::on_devicePlayButton_toggled(bool checked)
{
	if(checked) {
		mPlayTimer.start();
	} else {
		mPlayTimer.stop();
	}
}

void MainDisplayWidget::on_limitFpsCheckBox_toggled(bool checked)
{
	ui->limitFpsSpinBox->setEnabled(checked);

}

void MainDisplayWidget::on_limitFpsSpinBox_valueChanged(int limitFps)
{
}

void MainDisplayWidget::on_deviceSettingsButton_clicked()
{
	if(!m_pVideoCaptureDoc) { return; }

	VidAcqSettingsWindow * settingsWidget = new VidAcqSettingsWindow(NULL);
	settingsWidget->setVideoCaptureDoc(m_pVideoCaptureDoc);
	settingsWidget->setAttribute(Qt::WA_DeleteOnClose);
	settingsWidget->show();
}


/*******************************************************************************

								RECORDING ACTIONS

  ******************************************************************************/


// Slots flot actions on image
void MainDisplayWidget::on_mainImageWidget_signalPluginsButtonClicked()
{
	MDW_printf(EMALOG_INFO, "Ask for plugins");
}

void MainDisplayWidget::on_mainImageWidget_signalRecordButtonToggled(bool on)
{
	MDW_printf(EMALOG_INFO, "Ask for record : %c", on?'T':'F');
	if(on)
	{
		startRecording();
	}
	else
	{
		stopRecording();
	}
}
/***************************************************************

				VIDEO RECORDING SECTION

****************************************************************/
void MainDisplayWidget::startRecording()
{
	if(!mpegEncoder)
	{
		mpegEncoder = new OpenCVEncoder(m_fullImage.width(), m_fullImage.height(),
										25);
		mpegEncoder->setQuality(75);
	}

	QString fileext;
	fileext.sprintf(FORMAT_AVI, mMovieCounter);
	QString movieFile = mSourceName + fileext;

	fprintf(stderr, "[MainDisplayWidget]::%s:%d: Starting encoding file '%s'\n",
			__func__, __LINE__, movieFile.toUtf8().data());

	if(mpegEncoder->startEncoder( movieFile.toUtf8().data() )) {
		mIsRecording = true;
	} else {
		mIsRecording = false;
	}

	fprintf(stderr, "[MainDisplayWidget]::%s:%d: Starting encoding file '%s' => record=%c\n",
			__func__, __LINE__, movieFile.toUtf8().data() ,
			mIsRecording?'T':'F');
}

void MainDisplayWidget::stopRecording()
{
	if(!mIsRecording) return;

	fprintf(stderr, "[MainDisplayWidget]::%s:%d : stop recording.\n", __func__, __LINE__);
	mIsRecording = false;
	if(mpegEncoder)
	{
		mpegEncoder->stopEncoder();
		delete mpegEncoder;
		mpegEncoder = NULL;
	}
}

void MainDisplayWidget::on_mainImageWidget_signalSnapButtonClicked()
{
	MDW_printf(EMALOG_INFO, "Snapshot!");
}


void MainDisplayWidget::on_mainImageWidget_signalSnapshot(QImage snap)
{
	MDW_printf(EMALOG_INFO, "Snapshot %dx%dx%d !",
			   snap.width(), snap.height(), snap.depth());
	QString number;
	number.sprintf("-%03d.png", mSnapCounter);

	QDir savePath( m_workflow_settings.defaultImageDir );
	QString absPath = savePath.absoluteFilePath(mSourceName + number);
	snap.save(absPath);

	MDW_printf(EMALOG_INFO, "=> saved as '%s'", absPath.toAscii().data());

	mSnapCounter++;
}

