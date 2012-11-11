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
#include "ffmpeg_file_acquisition.h"

#include "piaf-common.h"
#include "PiafFilter.h"
#include <QFileInfo>

#include "moviebookmarkform.h"
#include "videocapture.h"
#include "vidacqsettingswindow.h"

#include "piafworkflow-settings.h"

#include "OpenCVEncoder.h"

#include <QDir>
#include <QMessageBox>

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
	mpFileVA = NULL;
	mpFilterSequencer = NULL;
	mpImageInfoStruct = NULL;

	mPlayGrayscale = false;
	mPlaySpeed = 1.f;
	m_editBookmarksForm = NULL;

	ui->stackedWidget->hide();
	m_pVideoCaptureDoc = NULL;

	connect(&mPlayTimer, SIGNAL(timeout()), this, SLOT(slot_mPlayTimer_timeout()));
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
int MainDisplayWidget::setMovieFile(QString moviePath,
									t_image_info_struct * pinfo)
{
	mpImageInfoStruct = pinfo;

	QFileInfo fi(moviePath);
	setWindowTitle(fi.baseName());
	mSourceName = fi.baseName();
	updateSnapCounter();

	m_pVideoCaptureDoc = NULL;

	m_listBookmarks.clear();
	if(mpFileVA)
	{
		MDW_printf(EMALOG_INFO, "Stop video acquisition");
		mpFileVA->stopAcquisition();
		delete mpFileVA; mpFileVA = NULL;
	}

	ui->timeLineWidget->setFilePosition(0);
	ui->mainImageWidget->setImage(m_fullImage, pinfo);
	ui->stackedWidget->setCurrentIndex(0);
	ui->stackedWidget->show();

	tBoxSize boxsize; memset(&boxsize, 0, sizeof(tBoxSize));

	/// \todo : FIXME : use factory
	mpFileVA = (FileVideoAcquisition*) new FFmpegFileVideoAcquisition(moviePath.toUtf8().data());
	if(!mpFileVA)
	{
		QMessageBox::critical(NULL, tr("File error"),
							  tr("Could not open file ") + moviePath + tr(" with movie player."));

		return -1;
	}

	int ret = 0;
	if(mpFileVA->isDeviceReady()) {
		m_fullImage = iplImageToQImage( mpFileVA->readImageRGB32() );
	} else {
		PIAF_MSG(SWLOG_ERROR, "Could not open file '%s'", moviePath.toUtf8().data());
		ret = -1;
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

		ui->timeLineWidget->setFileSize(pinfo->filesize);
	}
	else
	{
		ui->timeLineWidget->setFileSize(fi.size());
	}

	if(mpFileVA->getFrameRate()>1E-5)
	{
		mPlayTimer.setInterval((int)(1000.f /(mPlaySpeed * (float)mpFileVA->getFrameRate())));
	}

	return ret;
}

void MainDisplayWidget::appendBookmark(t_movie_pos pos)
{
	PIAF_MSG(SWLOG_INFO, "Append bmk at: abs=%llu key=%llu +%d frames",
			 pos.prevAbsPosition,
			 pos.prevKeyFramePosition,
			 pos.nbFramesSinceKeyFrame );
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
	ui->timeLineWidget->setBookmarkList(m_listBookmarks);

}

void MainDisplayWidget::slotNewBookmarkList(QList<video_bookmark_t> list) {
	if(!mpImageInfoStruct)
	{
		PIAF_MSG(SWLOG_ERROR, "No info struct for this file");
		return;
	}

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
	ui->timeLineWidget->setBookmarkList(m_listBookmarks);
	ui->timeLineWidget->setFileSize(mpImageInfoStruct->filesize);
}

void MainDisplayWidget::on_goFirstButton_clicked()
{
	if(mpFileVA)
	{
		PIAF_MSG(SWLOG_INFO, "Rewind FileVA");
		mpFileVA->rewindMovie();
	}

	updateDisplay();

	// to update navigation image widget
	emit signalImageChanged(m_fullImage);
}

void MainDisplayWidget::updateDisplay()
{
	IplImage * captureImage = ( mPlayGrayscale ?
									mpFileVA->readImageY() : mpFileVA->readImageRGB32() );
	PIAF_MSG(SWLOG_DEBUG, "setImage with IplImage %dx%dx%dx%d",
			 captureImage->width, captureImage->height, captureImage->nChannels, captureImage->depth
			 );
	t_image_info_struct info = mpFileVA->readImageInfo();


	ui->mainImageWidget->setImage(captureImage, NULL);
	if(mpegEncoder && mIsRecording)
	{
		PIAF_MSG(SWLOG_TRACE, "Recording...");
		mpegEncoder->encodeImage(ui->mainImageWidget->getOutputImage());
	}

	// Convert to QImage before display
	m_fullImage = iplImageToQImage( captureImage );



	//ui->mainImageWidget->setImage(m_fullImage, NULL);
	if(mpFileVA)
	{
		ui->timeLineWidget->setFilePosition(mpFileVA->getAbsolutePosition());
	}
}

void MainDisplayWidget::on_goPrevButton_clicked()
{
	if(mpFileVA)
	{
		mpFileVA->rewindMovie();
	}
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
		if(mpFileVA)
		{
			mPlayTimer.start((int)(1000.f / (mPlaySpeed * (float)mpFileVA->getFrameRate())));
		}
		else // Do not toggle
		{
			ui->playButton->blockSignals(true);
			ui->playButton->setChecked(false);
			ui->playButton->blockSignals(false);
		}
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
	bool got_picture = mpFileVA->GetNextFrame();
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

void MainDisplayWidget::slot_mPlayTimer_timeout()
{
	bool got_picture = false;
	if(m_pVideoCaptureDoc) // LIVE CAPTURE MODE
	{
		PIAF_MSG(SWLOG_DEBUG, "Live mode");

//		got_picture = (m_pVideoCaptureDoc->waitForImage() >= 0);
		got_picture = true;
		if(got_picture)
		{
			IplImage * captureImage = m_pVideoCaptureDoc->readImage();

//			fprintf(stderr, "MainDisplayW::%s:%d: saving /dev/shm/MainDisplayWidget-slot_mPlayTimer_timeout.png : %dx%dx%d\n",
//					__func__, __LINE__,
//					captureImage->width, captureImage->height, captureImage->nChannels
//					);

//			cvSaveImage(SHM_DIRECTORY "MainDisplayWidget-slot_mPlayTimer_timeout.png", captureImage);
			ui->mainImageWidget->setImage( captureImage, NULL);


			if(mpegEncoder && mIsRecording)
			{
				PIAF_MSG(SWLOG_INFO, "Recording...");
				mpegEncoder->encodeImage(ui->mainImageWidget->getOutputImage());
			}

			m_fullImage = iplImageToQImage( captureImage );
		}
		else
		{
			sleep(1);
			static int grab_failed_count;
			if(grab_failed_count++ > 10)
			{
				grab_failed_count = 0;
				PIAF_MSG(SWLOG_ERROR, "Live mode: grab failed => start capture loop");
				m_pVideoCaptureDoc->start();
			}
			else {
				PIAF_MSG(SWLOG_ERROR, "Live mode: grab failed");
			}
		}

	} else { // WE ARE IN MOVIE MODE
		PIAF_MSG(SWLOG_DEBUG, "Movie mode");

		got_picture = mpFileVA->GetNextFrame();
		if(got_picture)
		{
			updateDisplay();
		}
		else
		{
			PIAF_MSG(SWLOG_ERROR, "GeTNextFrame failed");
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
	PIAF_MSG(SWLOG_INFO, "Movie player speed changed x%g to '%s'",
			 mPlaySpeed, val.toAscii().data());
	if(val.contains("/"))
	{
		QStringList split = val.split("/");
		if(split.count()>1)
		{
			int denom = split[1].toInt(&ok);
			if(ok) {
				if(denom > 0)
				{
					mPlaySpeed = 1.f / (float)denom;
					PIAF_MSG(SWLOG_INFO, "Movie player speed changed to ratio %g",
							 mPlaySpeed);
				}
				else
				{
					PIAF_MSG(SWLOG_ERROR, "Invalid speed denominator read : %d => do not change speed",
							 denom);
				}
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

	PIAF_MSG(SWLOG_INFO, "Movie player speed changed to x%g * movie fps=%g => play at %g fps",
			 mPlaySpeed,
			 mpFileVA->getFrameRate(),
			 mpFileVA->getFrameRate() > 0 ? mPlaySpeed * (float)mpFileVA->getFrameRate()
										  : mPlaySpeed * 25.f);
	if(mpFileVA->getFrameRate()>0)
	{
		mPlayTimer.setInterval((int)roundf(1000.f /(mPlaySpeed * (float)mpFileVA->getFrameRate())));
	} else {
		PIAF_MSG(SWLOG_WARNING, "Movie player FPS is %g => change timer as if it were 25 fps",
				 mpFileVA->getFrameRate());
		mPlayTimer.setInterval((int)(1000.f /(mPlaySpeed * 25.f)));
	}
}

void MainDisplayWidget::on_timeLineWidget_signalCursorBookmarkChanged(
		t_movie_pos movie_pos)
{
	fprintf(stderr, "MainDisplayWidget::%s:%d received !\n", __func__, __LINE__);
	if(movie_pos.nbFramesSinceKeyFrame <= 1) {
		mpFileVA->setAbsolutePosition(movie_pos.prevAbsPosition);

		// display
		on_goNextButton_clicked();

	} // else go some frames before
	else {
		mpFileVA->setAbsolutePosition(movie_pos.prevKeyFramePosition);
		fprintf(stderr, "[VidPlay]::%s:%d : skip %d frames to reach the bookmark frame\n",
				__func__, __LINE__, movie_pos.nbFramesSinceKeyFrame-1);
		// display
		for(int nb = 0; nb < movie_pos.nbFramesSinceKeyFrame-1; nb++) {
			mpFileVA->GetNextFrame();
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
	mpFileVA->setAbsolutePosition(filepos);
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
	t_movie_pos bkmk = mpFileVA->getMoviePosition();

	PIAF_MSG(SWLOG_INFO, "New bookmark = {name='%s', prevAbsPosition=%llu, key=%llu +%d frames}",
			 bkmk.name.toAscii().data(),
			 bkmk.prevAbsPosition, bkmk.prevKeyFramePosition, bkmk.nbFramesSinceKeyFrame);
	appendBookmark( bkmk );

	if(mpImageInfoStruct) {
		mpImageInfoStruct->bookmarksList.append(mpFileVA->getMoviePosition());
		saveImageInfoStruct(mpImageInfoStruct);
		ui->timeLineWidget->setFileSize(mpImageInfoStruct->filesize);
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
		PIAF_MSG(SWLOG_INFO, "Starting device '%s' ...", m_pVideoCaptureDoc->getVideoProperties().devicename);
		m_pVideoCaptureDoc->start();
	}
	else
	{
		PIAF_MSG(SWLOG_INFO, "Device '%s' already running", m_pVideoCaptureDoc->getVideoProperties().devicename);
	}

	mSourceName = QString( m_pVideoCaptureDoc->getVideoProperties().devicename );
	updateSnapCounter();

	double fps = m_pVideoCaptureDoc->getVideoProperties().fps;
	if(fps <= 5)
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
	emit signalPluginsButtonClicked();
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
		PIAF_MSG(SWLOG_INFO, "create movie encoder %dx%d @ %dfps",
				 m_fullImage.width(), m_fullImage.height(),
				 25 );
		mpegEncoder = new OpenCVEncoder(m_fullImage.width(), m_fullImage.height(),
										25);
		mpegEncoder->setQuality(75);
	}

	QString fileext;
	fileext.sprintf(FORMAT_AVI, mMovieCounter);
	QString movieFile = mSourceName + fileext;

	PIAF_MSG(SWLOG_INFO, "Starting encoding file '%s'\n",
			movieFile.toUtf8().data());

	if(mpegEncoder->startEncoder( movieFile.toUtf8().data() ))
	{
		mIsRecording = true;
	} else {
		mIsRecording = false;
	}

	PIAF_MSG(SWLOG_INFO, "Starting encoding file '%s' => recording=%c\n",
			 movieFile.toUtf8().data() ,
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
	// don't save swice on same file

	QString number;

	QDir savePath( g_workflow_settings.defaultImageDir );

	number.sprintf("-%03d.png", mSnapCounter);
	QString absPath = savePath.absoluteFilePath(mSourceName + number);

	QFileInfo fi;
	do {
		fi.setFile(absPath);
		if( fi.exists() )
		{
			MDW_printf(EMALOG_INFO, "file '%s' already exists", absPath.toAscii().data());
			mSnapCounter++; //  -042.png already exists, so next will be 0043 ... until the file does not exists
			number.sprintf("-%03d.png", mSnapCounter);
			absPath = savePath.absoluteFilePath(mSourceName + number);
		}
		else
		{
			bool saved = snap.save(absPath);
			if(saved) {
				mSnapCounter++; // Save -042.png, so next will be 0043, we can increase counter now
				MDW_printf(EMALOG_INFO, "=> saved as '%s'", absPath.toAscii().data());
			} else {
				QMessageBox::critical(NULL, tr("Can't save image"),
									  tr("Can't save image file ") + absPath);
			}
			return;
		}
	} while(mSnapCounter<10000);

	MDW_printf(EMALOG_INFO, "Snapcounter overflow: %d and it could not save image", mSnapCounter);
}


