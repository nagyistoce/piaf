/***************************************************************************
	batchfiltersmainwindow.cpp  -  Movie / Image batch processor
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

#include "batchfiltersmainwindow.h"
#include "ui_batchfiltersmainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include "FileVideoAcquisition.h"
#include "piaf-settings.h"
#include "OpenCVEncoder.h"

BatchFiltersMainWindow::BatchFiltersMainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::BatchFiltersMainWindow),
	mSettings(PIAFBATCHSETTINGS)
{
	this->setAttribute(Qt::WA_DeleteOnClose, true);
    ui->setupUi(this);

	// dont' show warning on plugin crash
	mFilterManager.hide();
	mFilterManager.enableWarning(false);

	mPreviewFilterManager.hide();
	mPreviewFilterManager.enableWarning(false);

	ui->controlGroupBox->setEnabled(false);

	mBatchOptions.reload_at_change = true;
	mBatchOptions.use_grey = false;
	mBatchOptions.record_output = false;
	mBatchOptions.view_image = true;

	mBatchThread.setOptions(mBatchOptions);
	mBatchThread.setFilterManager(&mFilterManager);
	mBatchThread.setFileList(&mFileList);

	connect(&mDisplayTimer, SIGNAL(timeout()), this, SLOT(on_mDisplayTimer_timeout()));
	//	mBatchThread.start(QThread::HighestPriority);
	mDisplayIplImage = NULL;

	mLastPluginsDirName = g_pluginDirName;
	mLastDirName = g_imageDirName;
}


BatchFiltersMainWindow::~BatchFiltersMainWindow()
{
	if(mDisplayIplImage) {
		cvReleaseImage(&mDisplayIplImage);
	}
    delete ui;
}


void BatchFiltersMainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}


void BatchFiltersMainWindow::on_loadButton_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open plugin sequence file"),
													 mLastPluginsDirName,
													 tr("Piaf sequence (*.flist)"));
	QFileInfo fi(fileName);
	if(fi.isFile() && fi.exists())
	{
		mLastPluginsDirName = fi.absoluteDir().absolutePath();

		if(mFilterManager.loadFilterList(fi.absoluteFilePath().toUtf8().data()) < 0)
		{
			ui->sequenceLabel->setText(tr("Invalid file"));
			ui->controlGroupBox->setEnabled(false);
		} else {
			ui->controlGroupBox->setEnabled(true);
			ui->sequenceLabel->setText(fi.baseName());
			mBatchOptions.sequence_name = fi.baseName();
			mBatchThread.setOptions(mBatchOptions);

			mPreviewFilterManager.loadFilterList(fi.absoluteFilePath().toUtf8().data());
		}
	}
}

void BatchFiltersMainWindow::on_addButton_clicked()
{
	QStringList files = QFileDialog::getOpenFileNames(
							 NULL,
							 tr("Select one or more image or movie files to open"),
							 mLastDirName,
							 tr("Images (*.png *.xpm *.jpg *.jpeg *.bmp *.tif*);;"
								"Movies (*.mpg *.avi *.wmv *.mov)"));
/*
	fileName = Q3FileDialog::getOpenFileName(imageDirName,
											 "Images (*.png *.jpg *.jpeg *.bmp *.tif*)", this,
											 "open image dialog", "Choose an image to open" );
											 */
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

			mFileList.append(item);

			mLastDirName = fi.absoluteDir().absolutePath();
		}
	}

}

void BatchFiltersMainWindow::on_resetButton_clicked()
{
	QList<t_batch_item *>::iterator it;
	int idx = 0;
	for(it = mFileList.begin(); it != mFileList.end(); ++it)
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

void BatchFiltersMainWindow::on_delButton_clicked()
{
	QList<t_batch_item *>::iterator it;
	int idx = 0;
	for(it = mFileList.begin(); it != mFileList.end(); )
	{
		// remove selected
		t_batch_item * item = (*it);
		if(item->treeItem->isSelected())
		{
			ui->filesTreeWidget->takeTopLevelItem(idx);
			it = mFileList.erase(it);
			delete item;
		}
		else { ++it; idx++; }
	}
}

void BatchFiltersMainWindow::on_recordButton_toggled(bool checked)
{
	mBatchOptions.record_output = checked;
	mBatchThread.setOptions(mBatchOptions);
}

void BatchFiltersMainWindow::on_viewButton_toggled(bool checked)
{
	mBatchOptions.view_image = checked;
	mBatchThread.setOptions(mBatchOptions);
}

void BatchFiltersMainWindow::on_greyButton_toggled(bool checked)
{
	mBatchOptions.use_grey = checked;
	if(checked)
	{
		ui->greyButton->setText(tr("Grey"));
	}
	else
	{
		ui->greyButton->setText(tr("Color"));
	}

	mBatchThread.setOptions(mBatchOptions);
}

void BatchFiltersMainWindow::on_playPauseButton_toggled(bool checked)
{
	ui->recordButton->setEnabled(!checked);
	ui->greyButton->setEnabled(!checked);

	if(checked)
	{
		if(!mBatchThread.isRunning())
		{
			mBatchThread.start();
		}

		//
		mBatchThread.startProcessing(true);

		// Start thread and timer
		mDisplayTimer.start(1000);
	} else {
		mDisplayTimer.stop();
		// And refresh at last image
		mDisplayTimer.setSingleShot(100);
	}
}


void BatchFiltersMainWindow::on_reloadPluginCheckBox_stateChanged(int )
{
	mBatchOptions.reload_at_change = ui->reloadPluginCheckBox->isChecked();
	mBatchThread.setOptions(mBatchOptions);
}



void BatchFiltersMainWindow::on_filesTreeWidget_itemSelectionChanged()
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

void BatchFiltersMainWindow::on_filesTreeWidget_itemClicked(QTreeWidgetItem* treeItem, int column)
{
	if(!ui->viewButton->isChecked()) { return; }

	fprintf(stderr, "[Batch] %s:%d \n",	__func__, __LINE__);
	QList<t_batch_item *>::iterator it;
	int idx = 0;
	for(it = mFileList.begin(); it != mFileList.end(); ++it)
	{
		// remove selected
		t_batch_item * item = (*it);
		if(item->treeItem == treeItem)
		{
			CvSize oldSize = cvSize(mLoadImage.width(), mLoadImage.height());
			int oldDepth = mLoadImage.depth();

			if(mLoadImage.load(item->absoluteFilePath))
			{
				fprintf(stderr, "[Batch] %s:%d : loaded '%s'\n",
						__func__, __LINE__,
						item->absoluteFilePath.toAscii().data());
//				QPixmap pixmap;
//				pixmap = pixmap.fromImage(loadImage.scaled(ui->imageLabel->size(),
//														   Qt::KeepAspectRatio));
				ui->imageLabel->setRefImage(&mLoadImage);
				ui->imageLabel->switchToSmartZoomMode();// reset zooming

				if(strlen(mPreviewFilterManager.getPluginSequenceFile())>0)
				{
					// TODO : process image
					QImage loadedQImage;
					if(loadedQImage.load(item->absoluteFilePath))
					{

					}
					else
					{
						fprintf(stderr, "BatchThread::%s:%d : could not load '%s' as an image\n",
								__func__, __LINE__,
								item->absoluteFilePath.toAscii().data());
					}

					if(!loadedQImage.isNull())
					{
						if(mBatchOptions.reload_at_change
						   || oldSize.width != loadedQImage.width()
						   || oldSize.height != loadedQImage.height()
						   || oldDepth != loadedQImage.depth()
						   )
						{
							fprintf(stderr, "Batch Preview::%s:%d : reload_at_change=%c "
									"or size changed (%dx%dx%d => %dx%dx%d) "
									"=> reload filter sequence\n",
									__func__, __LINE__,
									mBatchOptions.reload_at_change ? 'T':'F',
									oldSize.width, oldSize.height, oldDepth,
									loadedQImage.width(), loadedQImage.height(), loadedQImage.depth()
									);
							// unload previously loaded filters
							mPreviewFilterManager.slotUnloadAll();
							// reload same file
							mPreviewFilterManager.loadFilterList(mPreviewFilterManager.getPluginSequenceFile());
						}

						oldSize.width = loadedQImage.width();
						oldSize.height = loadedQImage.height();
						oldDepth = loadedQImage.depth();

						swImageStruct image;
						memset(&image, 0, sizeof(swImageStruct));
						image.width = loadedQImage.width();
						image.height = loadedQImage.height();
						image.depth = loadedQImage.depth() / 8;
						image.buffer_size = image.width * image.height * image.depth;

						image.buffer = loadedQImage.bits(); // Buffer

						fprintf(stderr, "[Batch] %s:%d : process sequence '%s' on image '%s' (%dx%dx%d)\n",
								__func__, __LINE__,
								mPreviewFilterManager.getPluginSequenceFile(),
								item->absoluteFilePath.toAscii().data(),
								loadedQImage.width(), loadedQImage.height(), loadedQImage.depth()
								);
						// Process this image with filters
						mPreviewFilterManager.processImage(&image);

						fprintf(stderr, "[Batch] %s:%d : processd sequence '%s' => display image '%s' (%dx%dx%d)\n",
								__func__, __LINE__,
								mPreviewFilterManager.getPluginSequenceFile(),
								item->absoluteFilePath.toAscii().data(),
								loadedQImage.width(), loadedQImage.height(), loadedQImage.depth());



						// Copy into QImage
//						QImage qImage( (uchar*)imgRGBdisplay->imageData,
//									   imgRGBdisplay->width, imgRGBdisplay->height, imgRGBdisplay->widthStep,
//									   ( imgRGBdisplay->nChannels == 4 ? QImage::Format_RGB32://:Format_ARGB32 :
//										QImage::Format_RGB888 //Set to RGB888 instead of ARGB32 for ignore Alpha Chan
//										)
//									  );
						mLoadImage = loadedQImage.copy(); //qImage.copy();
						ui->imageLabel->setRefImage(&mLoadImage);
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

void BatchFiltersMainWindow::on_filesTreeWidget_itemActivated(QTreeWidgetItem* item, int column)
{
	fprintf(stderr, "[Batch] %s:%d \n",	__func__, __LINE__);
	on_filesTreeWidget_itemClicked(item, column);
}

void BatchFiltersMainWindow::on_filesTreeWidget_itemChanged(QTreeWidgetItem* item, int column)
{
//	fprintf(stderr, "[Batch] %s:%d %p column=%d\n",	__func__, __LINE__, item,column );
//	on_filesTreeWidget_itemClicked(item, column);
}

typedef struct {
	int nb_total;
	int nb_errors;
	int nb_processed;

} t_batch_progress;


void BatchFiltersMainWindow::on_mDisplayTimer_timeout()
{
	QList<t_batch_item *>::iterator it;
	t_batch_progress progress;
	memset(&progress, 0, sizeof(t_batch_progress));

	float processed = 0.f;
	for(it = mFileList.begin(); it != mFileList.end(); ++it)
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
					pixIcon = QIcon(":/images/16x16/dialog-error.png");
					stateStr = tr("Error proc");
					break;
				case ERROR_READ:
					pixIcon = QIcon(":/images/16x16/dialog-error.png");
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

	progress.nb_total = mFileList.count();

	QString procStr, errStr, totalStr;
	procStr.sprintf("%d", progress.nb_processed);
	ui->nbProcLabel->setText(procStr);

	errStr.sprintf("%d", progress.nb_errors);
	ui->nbErrLabel->setText(errStr);

	totalStr.sprintf("%d", progress.nb_total);
	ui->nbFilesLabel->setText(totalStr);

	int progress_percent = (int)roundf(processed *100.f / (float)mFileList.count());
	ui->progressBar->setValue(progress_percent);


	// display current processed image if needed
	if(mBatchOptions.view_image)
	{
		IplImage * imgRGBdisplay = mBatchThread.getDisplayImage();

		if(imgRGBdisplay) {

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
			}

			// Copy into QImage
			QImage qImage( (uchar*)mDisplayIplImage->imageData,
						   mDisplayIplImage->width, mDisplayIplImage->height, mDisplayIplImage->widthStep,
						   ( mDisplayIplImage->nChannels == 4 ? QImage::Format_RGB32://:Format_ARGB32 :
							QImage::Format_RGB888 //Set to RGB888 instead of ARGB32 for ignore Alpha Chan
							)
						  );

			mLoadImage = qImage.copy();
			ui->imageLabel->setRefImage(&mLoadImage);
			ui->imageLabel->update();
		}
	}

}





/*******************************************************************************

					PROCESSING THREAD

  *****************************************************************************/

BatchFiltersThread::BatchFiltersThread()
{
	mRun = mRunning = mProcessing = false;
	mpFilterManager = NULL;
	mpFileList = NULL;

	mPause = false;

	mBatchOptions.reload_at_change = true;
	mBatchOptions.use_grey = false;
	mDisplayImage = NULL;
}

BatchFiltersThread::~BatchFiltersThread()
{
	mRun = false; // tell thread to stop
	while(mRunning)
	{
		fprintf(stderr, "[BatchThread]::%s:%d : waiting for thread to end up...\n",
				__func__, __LINE__);
		sleep(1);
	}

	swReleaseImage(&mDisplayImage);
}

void BatchFiltersThread::setOptions(t_batch_options options)
{
	mBatchOptions = options;
}
void BatchFiltersThread::setFileList(QList<t_batch_item *> * pFileList)
{
	mpFileList = pFileList;
}

void BatchFiltersThread::startProcessing(bool on)
{
	mProcessing = on;
}

bool g_debug_BatchFiltersThread = false;

void BatchFiltersThread::run()
{
	mRunning = true;
	mRun = true;

	while(mRun)
	{
		if(g_debug_BatchFiltersThread) {
			fprintf(stderr, "BatchThread::%s:%d : processing='%c'\n", __func__, __LINE__,
					mProcessing ? 'T':'F');
		}
		bool procnow = mProcessing;
		if(procnow) {
			if(!mpFileList) {
				procnow = false;
			}
			else if(mpFileList->isEmpty()) {
				procnow = false;
			} else {
				// check if at least one needs to be processed
				bool at_least_one = false;
				QList<t_batch_item *>::iterator it;
				for(it = mpFileList->begin();
					mRun && it != mpFileList->end() && !at_least_one; ++it)
				{
					t_batch_item * item = (*it);
					if(item->processing_state == UNPROCESSED)
					{
						if(g_debug_BatchFiltersThread) {
							fprintf(stderr, "BatchFiltersThread::%s:%d: file '%s' is not processed yet.\n",
								__func__, __LINE__,
								item->absoluteFilePath.toAscii().data() );
						}
						at_least_one = true;
					}
				}

				if(!at_least_one)
				{
					procnow = false;
				}
			}
		}

		if(g_debug_BatchFiltersThread) {
			fprintf(stderr, "BatchThread::%s:%d => procnow='%c'\n", __func__, __LINE__,
					procnow ? 'T':'F');
		}

		if(!procnow)
		{
			::sleep(1);
			if(g_debug_BatchFiltersThread) {
				fprintf(stderr, "BatchFiltersThread::%s:%d: wait for unlock\n", __func__, __LINE__);
			}
		}
		else
		{
			// Output encoder
			OpenCVEncoder * encoder = NULL;
			CvSize oldSize = cvSize(0,0);
			int oldDepth = -1;

			// Process file
			QList<t_batch_item *>::iterator it;
			bool still_processing = false;
			bool was_paused = false;

			while(still_processing) {
				still_processing = false;
				was_paused = false;
				for(it = mpFileList->begin(); mRun && !was_paused && it != mpFileList->end(); ++it)
				{
					t_batch_item * item = (*it);
					if(item->processing_state == UNPROCESSED)
					{
						item->processing_state = PROCESSING;

						if(g_debug_BatchFiltersThread) {
							fprintf(stderr, "BatchThread::%s:%d : processing '%s'...\n",
								__func__, __LINE__,
								item->absoluteFilePath.toAscii().data());
						}

						// Create movie player or load image
						//IplImage * loadedImage = cvLoadImage => does not work
						// There is a conflict between Qt and opencv for PNGs
						// try like in Piaf with Qt's loading
						QImage loadedQImage;
						if(loadedQImage.load(item->absoluteFilePath))
						{

						}
						else
						{
							fprintf(stderr, "BatchThread::%s:%d : could not load '%s' as an image\n",
									__func__, __LINE__,
									item->absoluteFilePath.toAscii().data());
							item->processing_state = ERROR_READ;
						}

						if(!loadedQImage.isNull()
								&& item->processing_state != ERROR_READ)
						{
							if(g_debug_BatchFiltersThread) {
								fprintf(stderr, "BatchThread::%s:%d : "
										"loaded '%s' as an image...\n",
										__func__, __LINE__,
										item->absoluteFilePath.toAscii().data());
							}

							if(mBatchOptions.reload_at_change
							   || oldSize.width != loadedQImage.width()
							   || oldSize.height != loadedQImage.height()
							   || oldDepth != loadedQImage.depth()
							   )
							{
								if(g_debug_BatchFiltersThread) {
									fprintf(stderr, "BatchThread::%s:%d : reload_at_change=%c "
										"or size changed (%dx%dx%d => %dx%dx%d) "
										"=> reload filter sequence\n",
										__func__, __LINE__,
										mBatchOptions.reload_at_change ? 'T':'F',
										oldSize.width, oldSize.height, oldDepth,
										loadedQImage.width(), loadedQImage.height(), loadedQImage.depth()
										);
								}
								if(encoder) { delete encoder; encoder = NULL; }

								// unload previously loaded filters
								mpFilterManager->slotUnloadAll();
								// reload same file
								mpFilterManager->loadFilterList(mpFilterManager->getPluginSequenceFile());
							}

							//
							oldDepth = loadedQImage.depth();


							swImageStruct image;
							memset(&image, 0, sizeof(swImageStruct));
							image.width = loadedQImage.width();
							image.height = loadedQImage.height();
							image.depth = loadedQImage.depth() / 8;
							image.buffer_size = image.width * image.height * image.depth;
							image.buffer = loadedQImage.bits(); // Buffer

							// Process this image with filters
							int retproc = mpFilterManager->processImage(&image);
							if(retproc < 0)
							{
								item->processing_state = ERROR_PROCESS;

							} else {

								// Check if we need to record
								if(mBatchOptions.record_output) {
									QFileInfo fi(item->absoluteFilePath);
									QString outFile = item->absoluteFilePath
													  + "-" + mBatchOptions.sequence_name + "." + fi.extension();
									loadedQImage.save(outFile);
								}

								// Check if we need to copy an image for display
								if(mBatchOptions.view_image) {
									if(mDisplayImage &&
											( mDisplayImage->width!=loadedQImage.width()
											 || mDisplayImage->height!=loadedQImage.height()
											 || mDisplayImage->nChannels!=loadedQImage.depth()/8
											 )) {
										swReleaseImage(&mDisplayImage);
									}

									if(!mDisplayImage) {
										mDisplayImage = swCreateImage(cvSize(loadedQImage.width(), loadedQImage.height()),
																	  IPL_DEPTH_8U, loadedQImage.depth()/8);
										cvSet(mDisplayImage, cvScalarAll(255));
									}

									IplImage * imgHeader =  swCreateImageHeader(cvSize(loadedQImage.width(), loadedQImage.height()),
																		  IPL_DEPTH_8U, loadedQImage.depth()/8);
									cvSetData(imgHeader, loadedQImage.bits(),
											  loadedQImage.width()* loadedQImage.depth()/8);
									cvCopy(imgHeader, mDisplayImage);
									swReleaseImageHeader(&imgHeader);
								}

								// Set the state to processed
								item->processing_state = PROCESSED;
							}
							item->progress = 1.f;

						}
						else {
							// Load ad movie
							FileVideoAcquisition * fva = new FileVideoAcquisition(item->absoluteFilePath.toUtf8().data());

							CvSize size = cvSize(fva->getImageSize().width,
												 fva->getImageSize().height);
							if(size.width <=0 || size.height <=0)
							{
								fprintf(stderr, "BatchThread::%s:%d : invalid image size : %dx%d "
										"=> file='%s' has processing ERROR\n",
										__func__, __LINE__,
										size.width, size.height,
										item->absoluteFilePath.toUtf8().data()
										);

								// Set the state to processed
								item->processing_state = ERROR;
								item->progress = 1.f;

							} else {
								IplImage * loadedImage = swCreateImage(size,
															IPL_DEPTH_8U,
															mBatchOptions.use_grey ? 1:4);
								// Loop on images
								swImageStruct image;
								memset(&image, 0, sizeof(swImageStruct));
								image.width = loadedImage->widthStep / loadedImage->nChannels;// beware of pitch
								image.height = loadedImage->height;
								image.depth = loadedImage->nChannels;
								image.buffer_size = image.width * image.height * image.depth;

								image.buffer = loadedImage->imageData; // Buffer
								bool resume = true;
								while(resume && mRun)
								{
									bool read_frame = fva->GetNextFrame();
									long buffersize = image.buffer_size ;
									int ret = -1;
									if(read_frame) {
										if(mBatchOptions.use_grey)
										{
											ret = fva->readImageYNoAcq((uchar *)image.buffer, &buffersize);
										} else {
											ret = fva->readImageRGB32NoAcq((uchar *)image.buffer, &buffersize);
										}
									}


									if(!read_frame)
									{
										if(g_debug_BatchFiltersThread) {
											fprintf(stderr, "BatchThread::%s:%d : File '%s' EOF\n",
												__func__, __LINE__, item->absoluteFilePath.toAscii().data()
												);fflush(stderr);
										}
										resume = false;
									} else {
										int retproc = mpFilterManager->processImage(&image);

										if(retproc < 0) {
											// Set the state to processing error
											item->processing_state = ERROR_PROCESS;

										}
										else {

											// Check if we need to record
											if(mBatchOptions.record_output) {
												if(!encoder) {
													QFileInfo fi(item->absoluteFilePath);
													QString outputFile = item->absoluteFilePath
																	  + "-" + mBatchOptions.sequence_name + ".avi";
													encoder = new OpenCVEncoder(loadedImage->width,
																				loadedImage->height,
																				(int)roundf(fva->getFrameRate()));
													encoder->startEncoder(outputFile.toUtf8().data());
												}

												if(encoder) {
													switch(loadedImage->nChannels)
													{
													default:
														break;
													case 3:
														encoder->encodeFrameRGB24((uchar *)loadedImage->imageData);
														break;
													case 4:
														encoder->encodeFrameRGB32((uchar *)loadedImage->imageData);
														break;
													case 1:
														encoder->encodeFrameY((uchar *)loadedImage->imageData);
														break;
													}

												}
											}

											// Check if we need to copy an image for display
											if(mBatchOptions.view_image) {
												if(mDisplayImage && (mDisplayImage->widthStep!=loadedImage->widthStep || mDisplayImage->height!=loadedImage->height)) {
													swReleaseImage(&mDisplayImage);
												}
												if(!mDisplayImage) {
													mDisplayImage = cvCloneImage(loadedImage);
												}
												else {
													//fprintf(stderr, "disp=%dx%dx%d", mDisplayImage->width, mDisplayImage->height, mDisplayImage->nChannels);
													cvCopy(loadedImage, mDisplayImage);
												}
											}
										}

										// compute progress
										if(fva->getFileSize()>0) {
											item->progress = (float)fva->getAbsolutePosition()/
														 (float)fva->getFileSize();

	//										fprintf(stderr, "\rProgress '%s' = %4.2f %%",
	//												item->absoluteFilePath.toAscii().data(),
	//												item->progress * 100.f
	//												);
										}
									}
								}

								// Set the state to processed
								if(item->processing_state == PROCESSING) {
									item->processing_state = PROCESSED;
								}

								item->progress = 1.f;

								fprintf(stderr, "File '%s' %s : progress= %4.2f %%",
										item->absoluteFilePath.toAscii().data(),
										item->processing_state == PROCESSED ? "PROCESSED" : "ERROR_PROCESS",
										item->progress * 100.f
										);

								swReleaseImage(&loadedImage);
							}
						}


						// check if wee need to make a pause
						if(mPause) {
							while(mPause && mRun && mProcessing)
							{
								fprintf(stderr, "Paused...");
								usleep(500);
							}
							was_paused = true;
							still_processing = true;
						}
					}
				} // end of iterator on file list

				if(encoder) { delete encoder; encoder = NULL; }
			} // while still_processing

			// Unload plugins at end of list
			if(mpFilterManager) {
				if(g_debug_BatchFiltersThread) {
					fprintf(stderr, "BatchFiltersThread::%s:%d: Unloading plugins on %p...\n", __func__, __LINE__,
						mpFilterManager);
				}

				mpFilterManager->slotUnloadAll();

				if(g_debug_BatchFiltersThread) {
					fprintf(stderr, "BatchFiltersThread::%s:%d: Unloaded plugins.\n", __func__, __LINE__); fflush(stderr);
				}
			}
		}


	}

	if(g_debug_BatchFiltersThread) {
		fprintf(stderr, "BatchFiltersThread::%s:%d: Thread ended.\n", __func__, __LINE__);
	}
	mRunning = false;
}



