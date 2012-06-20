/***************************************************************************
	batch_progress_widget.cpp  -  Movie / Image batch processor
								to process a list of file using a pre-saved
								plugins list + parameters
							 -------------------
	begin                : Fri Mar 25 2011
	copyright            : (C) 2011 by Christophe Seyve (CSE)
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
#include "piaf-common.h"
#include "piaf-settings.h"

#include "batch_progress_widget.h"
#include "ui_batch_progress_widget.h"

#include "batchthread.h"
#include "swopencv.h"

#include <QFileDialog>
#include <QMessageBox>

#include "FileVideoAcquisition.h"

#include "imgutils.h"
#include "OpenCVEncoder.h"

#include "timehistogramwidget.h"


BatchProgressWidget::BatchProgressWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BatchProgressWidget)
{
    ui->setupUi(this);

	mpBatchTask = NULL;	// no current batch task
	mBatchTaskAllocated = false;	//

	mpBatchThread = NULL; // no current Batch processing thread
	mBatchThreadAllocated = false;	/// tells if current batch thread has been allocated by the widget


	// dont' show warning on plugin crash
	mFilterSequencer.enableWarning(false);
	mFilterSequencer.enableAutoReloadSequence(true);


	mPreviewFilterSequencer.enableWarning(false);
	mPreviewFilterSequencer.enableAutoReloadSequence(true);

//	ui->controlGroupBox->setEnabled(false);

	allocateBatchTask();

	mpBatchThread = NULL;


	connect(&mDisplayTimer, SIGNAL(timeout()), this, SLOT(on_mDisplayTimer_timeout()));
	//	if(mpBatchThread) mpBatchThread->start(QThread::HighestPriority);
	mDisplayIplImage = NULL;

	mLastPluginsDirName = g_pluginDirName;
	mLastDirName = g_imageDirName;

	loadSettings();
}

void BatchProgressWidget::allocateBatchThread()
{
	if(mpBatchThread) { return; }
	mpBatchThread = new BatchFiltersThread();

	// assign a filter manager to background processing thread
//	mpBatchThread->setFilterSequencer(&mFilterSequencer);
}


void BatchProgressWidget::allocateBatchTask()
{
	// Read batch options in GUI (defaults)
	if(!mpBatchTask)
	{
		PIAF_MSG(SWLOG_INFO, "create new watch task");
		mpBatchTask = new t_batch_task;
		mpBatchTask->sequencePath = "(none)";
	}

	mpBatchTask->options.reload_at_change = ui->reloadPluginCheckBox->isChecked();
	mpBatchTask->options.use_grey = ui->greyButton->isChecked();
	mpBatchTask->options.record_output = ui->recordButton->isChecked();
	mpBatchTask->options.view_image = ui->viewButton->isChecked() ;

	if(mpBatchThread)
	{
		mpBatchThread->setOptions(mpBatchTask->options);
	}
}

/*
 * Delete task and its file items
 */
void purgeBatchTask(t_batch_task * pTask)
{
	if(!pTask) { return; }
//	QList<t_batch_item *> mItemsList;	///< List of files
//	QString mSequencePath;				///< Path to plugins sequence
//	t_batch_options options;			///< processing options
//	} t_batch_task;

	fprintf(stderr, "[%s] %s:%d: delete batch task '%s' + %d files\n",
			__FILE__, __func__, __LINE__,
			pTask->sequencePath.toAscii().data(),
			pTask->itemsList.count()
			);
	QList<t_batch_item *>::iterator it;
	for(it = pTask->itemsList.begin();
		it != pTask->itemsList.end();
		) {
		t_batch_item * pitem = (*it);
		if(pitem)
		{
			if(pitem->treeItem)
			{
				delete pitem->treeItem;
			}
			delete pitem;
		}
		it = pTask->itemsList.erase(it);
	}

	delete pTask;
}

BatchProgressWidget::~BatchProgressWidget()
{
	if(mBatchTaskAllocated && mpBatchTask)
	{
		// purge
		purgeBatchTask(mpBatchTask);
		mpBatchTask = NULL;
	}
	if(mpBatchThread && mBatchThreadAllocated)
	{
		delete mpBatchThread;
	}
	mpBatchThread = NULL;
	mBatchThreadAllocated = false;

	swReleaseImage(&mDisplayIplImage);

	delete ui;
}

void BatchProgressWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}









#define BATCHSETTING_LASTPLUGINDIR "batch.lastPluginDir"
#define BATCHSETTING_LASTFILEDIR "batch.lastFileDir"

void BatchProgressWidget::loadSettings()
{
	// overwrite with batch settings
	mLastPluginsDirName = mSettings.value(BATCHSETTING_LASTPLUGINDIR, mLastPluginsDirName).toString();
	mLastDirName = mSettings.value(BATCHSETTING_LASTFILEDIR, mLastDirName).toString();
}

void BatchProgressWidget::saveSettings()
{
	// overwrite with batch settings
	mSettings.setValue(BATCHSETTING_LASTPLUGINDIR, mLastPluginsDirName);
	mSettings.setValue(BATCHSETTING_LASTFILEDIR, mLastDirName);
}




void BatchProgressWidget::on_loadButton_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open plugin sequence file"),
													 mLastPluginsDirName,
													 tr("Piaf sequence (*.flist)"));
	setPluginSequence(fileName);
}

/*
 * Set the plugin sequence */
void BatchProgressWidget::setPluginSequence(QString fileName)
{
	if(!mpBatchTask)
	{
		mpBatchTask = new t_batch_task;
		mBatchTaskAllocated = true;
	}

	QFileInfo fi(fileName);
	if(fi.isFile() && fi.exists())
	{
		mLastPluginsDirName = fi.absoluteDir().absolutePath();

		if(mFilterSequencer.loadFilterList(fi.absoluteFilePath().toUtf8().data()) < 0)
		{
			ui->sequenceLabel->setText(tr("Invalid file"));
			//ui->controlGroupBox->setEnabled(false);
		} else {
			//ui->controlGroupBox->setEnabled(true);
			ui->sequenceLabel->setText(fi.baseName());
			mpBatchTask->options.sequence_name = fi.baseName();
			if(mpBatchThread) {
				mpBatchThread->setOptions(mpBatchTask->options);
			}

			mPreviewFilterSequencer.loadFilterList(fi.absoluteFilePath().toUtf8().data());
		}

		saveSettings();
	}
	else
	{
		QMessageBox::critical(NULL, tr("Invalid name"),
							  tr("Invalid file name '") + fileName
							  + tr("': this is not an existing file."));
	}
}

/** \brief Set the pointer on current task

  This function enables to display a task which is waiting in stack, finished or in progress
  */
int BatchProgressWidget::setBatchTask(t_batch_task * pTask)
{
	purgeBatchTask(mpBatchTask);
	mBatchTaskAllocated = false; // keep in mind we received this pointer from external

	// Show files list
	mpBatchTask = pTask;
	if(pTask)
	{
		PIAF_MSG(SWLOG_INFO, "Received new task = %p : %d files, sequence='%s'",
				 mpBatchTask, mpBatchTask->itemsList.count(),
				 mpBatchTask->sequencePath.toAscii().data()
				 );
	} else {
		PIAF_MSG(SWLOG_INFO, "Received new task = %p", pTask);
	}
	return 0;
}

/*
 * Set the list of files */
void BatchProgressWidget::setFilesList(QStringList files)
{
	if(!mpBatchTask)
	{
		allocateBatchTask();
	}

	QFileInfo fi;
	QStringList list = files;
	QStringList::Iterator it = list.begin();
	while(it != list.end()) {
		QString fileName = (*it);
		++it;

		// Append file
		fi.setFile(fileName);
		if(fi.exists() && fi.isFile()) {
			t_batch_item * item = new t_batch_item;
			item->progress = 0.f;
			item->absoluteFilePath = fi.absoluteFilePath();
			item->is_movie = fi.isFile(); // FIXME
			item->processing_state =
					item->former_state = UNPROCESSED;


			QStringList strings; strings.append(fi.completeBaseName());
			item->treeItem = new QTreeWidgetItem(ui->filesTreeWidget, strings);
			item->treeItem->setIcon(1, QIcon(":/images/16x16/waiting.png"));

			mpBatchTask->itemsList.append(item);

			mLastDirName = fi.absoluteDir().absolutePath();

			saveSettings();
		}
	}

}


void BatchProgressWidget::on_addButton_clicked()
{
	QStringList files = QFileDialog::getOpenFileNames(
							 NULL,
							 tr("Select one or more image or movie files to open"),
							 mLastDirName,
							 tr("Images or movies") + "(*.png *.xpm *.jpg *.jpeg *.bmp *.tif*"
								"*.mpg *.avi *.wmv *.mov);;"
								+ tr("Images") + " (*.png *.xpm *.jpg *.jpeg *.bmp *.tif*);;"
								+ tr("Movies") + " (*.mpg *.avi *.wmv *.mov)"
								);
	setFilesList(files);


}

void BatchProgressWidget::on_resetButton_clicked()
{
	if(!mpBatchTask) { return; }

	QList<t_batch_item *>::iterator it;
	for(it = mpBatchTask->itemsList.begin(); it != mpBatchTask->itemsList.end(); ++it)
	{
		// remove selected
		t_batch_item * item = (*it);
		if(item->treeItem->isSelected()) {
			item->processing_state = UNPROCESSED;

			if(item->treeItem)
			{
				item->treeItem->setIcon(1, QIcon());
			}
		}
	}

}

void BatchProgressWidget::on_delButton_clicked()
{
	QList<t_batch_item *>::iterator it;
	int idx = 0;
	for(it = mpBatchTask->itemsList.begin(); it != mpBatchTask->itemsList.end(); )
	{
		// remove selected
		t_batch_item * item = (*it);
		if(item->treeItem->isSelected())
		{
			ui->filesTreeWidget->takeTopLevelItem(idx);
			it = mpBatchTask->itemsList.erase(it);
			delete item;
		}
		else { ++it; idx++; }
	}
}

void BatchProgressWidget::on_recordButton_toggled(bool checked)
{
	mpBatchTask->options.record_output = checked;
	if(mpBatchThread) mpBatchThread->setOptions(mpBatchTask->options);
}

void BatchProgressWidget::on_viewButton_toggled(bool checked)
{
	mpBatchTask->options.view_image = checked;

	fprintf(stderr, "[Batch]::%s:%d : display = %c\n",
			__func__, __LINE__, mpBatchTask->options.view_image ? 'T':'F');

	if(mpBatchThread) mpBatchThread->setOptions(mpBatchTask->options);
}

void BatchProgressWidget::on_greyButton_toggled(bool checked)
{
	mpBatchTask->options.use_grey = checked;
	if(checked)
	{
		ui->greyButton->setText(tr("Grey"));
	}
	else
	{
		ui->greyButton->setText(tr("Color"));
	}

	if(mpBatchThread) mpBatchThread->setOptions(mpBatchTask->options);
}


/*
 * Enable or not the interactive mode: add/remove files, ...

	on non-interactive mode, the user can't add or remove files,
	can't change the filters' sequence
*/
void BatchProgressWidget::enableInteractive(bool on)
{
	ui->loadButton->setVisible(on);
	ui->filesButtonsWidget->setVisible(on);
}

void BatchProgressWidget::on_playPauseButton_toggled(bool checked)
{
	PIAF_MSG(SWLOG_INFO, "checked=%c", checked ? 'T':'F');

	ui->recordButton->setEnabled(!checked);
	ui->greyButton->setEnabled(!checked);
	ui->filesButtonsWidget->setEnabled(!checked);

	// Start thread and timer
	if(!mDisplayTimer.isActive()) {
		mDisplayTimer.start(1000);
	}

	if(checked)
	{
		if(mpBatchThread) {
			if(!mpBatchThread->isRunning())
			{
				if(mpBatchThread) mpBatchThread->start();
			}

			//
			mpBatchThread->startProcessing(true);

			mpBatchThread->setPause(false); // set pause on
		}
		else
		{
			/// \todo allocate if needed
		}
	} else {
		if(mpBatchThread) {
			mpBatchThread->setPause(true); // set pause on
		}
		else
		{
			/// \todo allocate if needed
		}

	}
}


void BatchProgressWidget::on_reloadPluginCheckBox_stateChanged(int )
{
	mpBatchTask->options.reload_at_change = ui->reloadPluginCheckBox->isChecked();
	if(mpBatchThread) {
		mpBatchThread->setOptions(mpBatchTask->options);
	}
	else
	{
		/// \todo allocate if needed
	}

}



void BatchProgressWidget::on_filesTreeWidget_itemSelectionChanged()
{
	fprintf(stderr, "[Batch] %s:%d \n",	__func__, __LINE__);

	QList<QTreeWidgetItem *> selectedItems = ui->filesTreeWidget->selectedItems ();
	if(selectedItems.count() != 1) // more or less than one item selecte, do nothing
	{
		return;
	}

	// Display or process selected items
	on_filesTreeWidget_itemClicked(selectedItems.at(0), 0);
}

void BatchProgressWidget::on_filesTreeWidget_itemClicked(
		QTreeWidgetItem* treeItem, int /*column*/)
{
	if(!ui->viewButton->isChecked()) { return; }
	if(!mpBatchTask) { return; }
	IplImage * imageOut = NULL; // temp image for storing data output

	fprintf(stderr, "[Batch] %s:%d \n",	__func__, __LINE__);
	QList<t_batch_item *>::iterator it;
	for(it = mpBatchTask->itemsList.begin(); it != mpBatchTask->itemsList.end(); ++it)
	{
		// remove selected
		t_batch_item * item = (*it);
		if(item->treeItem == treeItem)
		{

			CvSize oldSize = mLoadImage ? cvSize(mLoadImage->width, mLoadImage->height)
										: cvSize(0,0);
			int oldNChannels = mLoadImage ? mLoadImage->nChannels : 0;
			int oldDepth = IPL_DEPTH_8U;

			swReleaseImage(&mLoadImage);
			mLoadImage = cvLoadImage(item->absoluteFilePath.toUtf8().data());
			if(mLoadImage)
			{
				fprintf(stderr, "[Batch] %s:%d : loaded '%s'\n",
						__func__, __LINE__,
						item->absoluteFilePath.toAscii().data());
//				QPixmap pixmap;
//				pixmap = pixmap.fromImage(loadImage.scaled(ui->imageLabel->size(),
//														   Qt::KeepAspectRatio));
				ui->imageLabel->setRefImage(mLoadImage);
				ui->imageLabel->switchToSmartZoomMode();// reset zooming

				if(strlen(mPreviewFilterSequencer.getPluginSequenceFile())>0)
				{
					// TODO : process image
					IplImage * loadedImage = cvLoadImage(item->absoluteFilePath.toUtf8().data());

					if(loadedImage) {

					}
					else
					{
						fprintf(stderr, "BatchThread::%s:%d : could not load '%s' as an image\n",
								__func__, __LINE__,
								item->absoluteFilePath.toAscii().data());
					}

					if(loadedImage)
					{
						if(mpBatchTask->options.reload_at_change
								|| oldSize.width != loadedImage->width
								|| oldSize.height != loadedImage->height
								|| oldDepth != loadedImage->depth
								|| oldNChannels != loadedImage->nChannels
						   )
						{
							fprintf(stderr, "Batch Preview::%s:%d : reload_at_change=%c "
									"or size changed (%dx%dx%d => %dx%dx%d) "
									"=> reload filter sequence\n",
									__func__, __LINE__,
									mpBatchTask->options.reload_at_change ? 'T':'F',
									oldSize.width, oldSize.height, oldDepth,
									loadedImage->width, loadedImage->height,
									loadedImage->depth
									);

							// unload previously loaded filters
							mPreviewFilterSequencer.unloadAllLoaded();

							// reload same file
							mPreviewFilterSequencer.loadFilterList(mPreviewFilterSequencer.getPluginSequenceFile());
						}

						oldSize.width = loadedImage->width;
						oldSize.height = loadedImage->height;
						oldDepth = loadedImage->depth;
						oldNChannels = loadedImage->nChannels;

						fprintf(stderr, "[Batch] %s:%d : process sequence '%s' on image '%s' (%dx%dx%dx%d)\n",
								__func__, __LINE__,
								mPreviewFilterSequencer.getPluginSequenceFile(),
								item->absoluteFilePath.toAscii().data(),
								loadedImage->width, loadedImage->height,
								loadedImage->depth, loadedImage->nChannels
								);
						// Process this image with filters
						mPreviewFilterSequencer.processImage(loadedImage, &imageOut);

						fprintf(stderr, "[Batch] %s:%d : processd sequence '%s' "
								"=> display image '%s' (%dx%dx%dx%d)\n",
								__func__, __LINE__,
								mPreviewFilterSequencer.getPluginSequenceFile(),
								item->absoluteFilePath.toAscii().data(),
								loadedImage->width, loadedImage->height,
								loadedImage->depth, loadedImage->nChannels);

						// Copy into QImage
						//mLoadImage = iplImageToQImage(imageOut);
						ui->imageLabel->setRefImage(mLoadImage);
						ui->imageLabel->update();
					}
				}
			}
			else
			{
				ui->imageLabel->setRefImage(NULL);
				fprintf(stderr, "[Batch] %s:%d : could not load '%s'\n",
						__func__, __LINE__,
						item->absoluteFilePath.toAscii().data());
			}
		}
	}
}

void BatchProgressWidget::on_filesTreeWidget_itemActivated(QTreeWidgetItem* item, int column)
{
	fprintf(stderr, "[Batch] %s:%d \n",	__func__, __LINE__);
	on_filesTreeWidget_itemClicked(item, column);
}

void BatchProgressWidget::on_filesTreeWidget_itemChanged(QTreeWidgetItem* /*item*/, int /*column*/)
{
//	fprintf(stderr, "[Batch] %s:%d %p column=%d\n",	__func__, __LINE__, item,column );
//	on_filesTreeWidget_itemClicked(item, column);
}

/// Progression structure
typedef struct {
	int nb_total;
	int nb_errors;
	int nb_processed;
	int nb_unprocessed;

} t_batch_progress;


void BatchProgressWidget::on_mDisplayTimer_timeout()
{
	QList<t_batch_item *>::iterator it;
	t_batch_progress progress;
	memset(&progress, 0, sizeof(t_batch_progress));

	float processed = 0.f;
	for(it = mpBatchTask->itemsList.begin(); it != mpBatchTask->itemsList.end(); ++it)
	{
		// remove selected
		t_batch_item * item = (*it);
		processed += item->progress;
		if(item->former_state != item->processing_state)
		{
			item->former_state = item->processing_state;
			QIcon pixIcon;
			QString stateStr;
			if(item->treeItem)
			{
				// update icon
				switch(item->processing_state)
				{
				default:
				case UNPROCESSED:
					pixIcon = QIcon(":/images/16x16/waiting.png");
					stateStr = tr("Unprocessed");
					break;
				case PROCESSED:
					pixIcon = QIcon(":/images/16x16/dialog-ok-apply.png");
					stateStr = tr("Processed");
					break;
				case ERROR:
					pixIcon = QIcon(":/images/16x16/dialog-error.png");
					stateStr = tr("Error");
					break;
				case ERROR_PROCESS:
					pixIcon = QIcon(":/images/16x16/dialog-error-process.png");
					stateStr = tr("Error proc");
					break;
				case ERROR_READ:
					pixIcon = QIcon(":/images/16x16/dialog-error-read.png");
					stateStr = tr("Error file");
					break;
				case PROCESSING:
					pixIcon = QIcon(":/images/16x16/system-run.png");
					stateStr = tr("Processing");
					break;
				}


				if(!pixIcon.isNull())
				{
					item->treeItem->setIcon(1, pixIcon);
				}
				else
				{
					item->treeItem->setText(1, stateStr);
				}

			}
		}



		// update stats
		switch(item->processing_state)
		{
		default:
			break;
		case UNPROCESSED:
			progress.nb_unprocessed++;
			break;
		case PROCESSED:
			progress.nb_processed++;
			break;
		case ERROR:
		case ERROR_READ:
		case ERROR_PROCESS:
			progress.nb_errors++;
			break;
		}
	}

	progress.nb_total = mpBatchTask->itemsList.count();

	QString procStr, errStr, totalStr;
	procStr.sprintf("%d", progress.nb_processed);
	ui->nbProcLabel->setText(procStr);

	errStr.sprintf("%d", progress.nb_errors);
	ui->nbErrLabel->setText(errStr);

	totalStr.sprintf("%d", progress.nb_total);
	ui->nbFilesLabel->setText(totalStr);

	int progress_percent = 100;
	if(progress.nb_total>0) {
		progress_percent = (int)roundf(processed *100.f / (float)mpBatchTask->itemsList.count());
	}
	ui->progressBar->setValue(progress_percent);

	fprintf(stderr, "[Batch]::%s:%d : display = %c\n",
			__func__, __LINE__,
			mpBatchTask->options.view_image ? 'T':'F');

	if(progress.nb_unprocessed == 0) {
		// we're done
		ui->playPauseButton->setChecked(false);
	}

	// display current processed image if needed
	if(mpBatchTask->options.view_image)
	{
		IplImage * imgRGBdisplay = NULL;
		if(mpBatchThread)
		{
			mpBatchThread->lockDisplay(true); // lock display for thread-safe execution
			IplImage * imgRGBdisplay = mpBatchThread->getDisplayImage();
		}
		else {
			/// \todo allocate if needed
		}

		if(!imgRGBdisplay) {
			if(mpBatchThread) {
				mpBatchThread->lockDisplay(false);
			}
			fprintf(stderr, "[Batch]::%s:%d : no display image\n",
					__func__, __LINE__);
		} else {
//			if(imgRGBdisplay->nChannels == 4 && imgRGBdisplay->imageData[0]==0)
//			{	// FORCE ALPHA CHANNEL
//				for(int i = 0 ; i<imgRGBdisplay->widthStep * imgRGBdisplay->height; i+=4)
//				{
//					imgRGBdisplay->imageData[i] = 255;
//				}
//			}

			// Convert to display
			if(mDisplayIplImage &&
					(imgRGBdisplay->widthStep != mDisplayIplImage->widthStep
					 || imgRGBdisplay->height != mDisplayIplImage->height) ) {
				swReleaseImage(&mDisplayIplImage);
				mDisplayIplImage = NULL;
			}

			if(!mDisplayIplImage) {
				mDisplayIplImage = swCreateImage(cvGetSize(imgRGBdisplay), IPL_DEPTH_8U, 4);
			}

			if(mDisplayIplImage->nChannels != imgRGBdisplay->nChannels) {
				//
				switch(imgRGBdisplay->nChannels) {
				default:
					break;
				case 1:
					cvCvtColor(imgRGBdisplay, mDisplayIplImage, CV_GRAY2RGBA);
					break;
				case 3:
					cvCvtColor(imgRGBdisplay, mDisplayIplImage, CV_RGB2RGBA);
					break;
				}
			} else {
				cvCopy(imgRGBdisplay, mDisplayIplImage);
			}
			if(mpBatchThread) mpBatchThread->lockDisplay(false);// image is copied, so we can unlock it
			fprintf(stderr, "[Batch]::%s:%d : display image: %dx%dx%d => %dx%dx%d\n",
					__func__, __LINE__,
					imgRGBdisplay->width, imgRGBdisplay->height, imgRGBdisplay->nChannels,
					mDisplayIplImage->width, mDisplayIplImage->height, mDisplayIplImage->nChannels
					);

			// Copy into QImage
//			QImage qImage( (uchar*)mDisplayIplImage->imageData,
//						   mDisplayIplImage->width, mDisplayIplImage->height, mDisplayIplImage->widthStep,
//						   ( mDisplayIplImage->nChannels == 4 ? QImage::Format_RGB32://:Format_ARGB32 :
//							QImage::Format_RGB888 //Set to RGB888 instead of ARGB32 for ignore Alpha Chan
//							)
//						  );

			swReleaseImage(&mLoadImage);
			mLoadImage = tmCloneImage(mDisplayIplImage);

			//mLoadImage = qImage.copy();
			ui->imageLabel->setRefImage(mLoadImage);
			ui->imageLabel->update();
		}
	}



	// display time histogram
	if(mpBatchThread) {
		t_time_histogram time_histo = mpBatchThread->getTimeHistogram();
		ui->timeHistoWidget->displayHisto(time_histo);
	}
}
void BatchProgressWidget::setBatchFiltersThread(BatchFiltersThread  * pBatchThread)
{
	if(mpBatchThread && mBatchThreadAllocated)
	{
		delete mpBatchThread;
	}
	mpBatchThread = pBatchThread;
	mBatchThreadAllocated = false;
}









void BatchProgressWidget::on_playPauseButton_clicked()
{

}
