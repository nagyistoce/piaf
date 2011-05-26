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
		ui(new Ui::BatchFiltersMainWindow)
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
			item->processing_state = UNPROCESSED;
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

void BatchFiltersMainWindow::on_filesTreeWidget_itemSelectionChanged()
{
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
	on_filesTreeWidget_itemClicked(item, column);
}

void BatchFiltersMainWindow::on_filesTreeWidget_itemChanged(QTreeWidgetItem* item, int column)
{
	on_filesTreeWidget_itemClicked(item, column);
}

void BatchFiltersMainWindow::on_mDisplayTimer_timeout()
{
	QList<t_batch_item *>::iterator it;
	float processed = 0.f;
	for(it = mFileList.begin(); it != mFileList.end(); ++it)
	{
		// remove selected
		t_batch_item * item = (*it);
		processed += item->progress;
		if(item->treeItem)
		{
			if(item->processing_state == UNPROCESSED)
			{
				item->treeItem->setIcon(1, QIcon(":/images/16x16/waiting.png"));
				break;
			}
			else {
				// update icon
				switch(item->processing_state)
				{
				default:
				case PROCESSED:
					item->treeItem->setIcon(1, QIcon(":/images/16x16/dialog-ok-apply.png"));
					break;
				case ERROR:
					item->treeItem->setIcon(1, QIcon(":/images/16x16/dialog-error.png"));
					break;
				case PROCESSING:
					item->treeItem->setIcon(1, QIcon(":/images/16x16/system-run.png"));
					break;
				}
			}
		}
	}

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
				cvReleaseImage(&mDisplayIplImage);
				mDisplayIplImage = NULL;
			}

			if(!mDisplayIplImage) {
				mDisplayIplImage = cvCreateImage(cvGetSize(imgRGBdisplay), IPL_DEPTH_8U, 4);
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
			QImage qImage( (uchar*)imgRGBdisplay->imageData,
						   imgRGBdisplay->width, imgRGBdisplay->height, imgRGBdisplay->widthStep,
						   ( imgRGBdisplay->nChannels == 4 ? QImage::Format_RGB32://:Format_ARGB32 :
							QImage::Format_RGB888 //Set to RGB888 instead of ARGB32 for ignore Alpha Chan
							)
						  );
			mLoadImage = qImage.copy();
			ui->imageLabel->setRefImage(&mLoadImage);
			ui->imageLabel->update();
		}
	}

	int progress = (int)(processed *100.f / (float)mFileList.count());
	ui->progressBar->setValue(progress);
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
	mRun = false;
	while(mRunning)
	{
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

void BatchFiltersThread::run()
{
	mRunning = true;
	mRun = true;

	while(mRun)
	{
		fprintf(stderr, "BatchThread::%s:%d : processing='%c'\n", __func__, __LINE__,
				mProcessing ? 'T':'F');
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
				for(it = mpFileList->begin(); it != mpFileList->end() && !at_least_one; ++it)
				{
					t_batch_item * item = (*it);
					if(item->processing_state == UNPROCESSED)
					{
						fprintf(stderr, "BatchFiltersThread::%s:%d: file '%s' is not processed yet.\n",
								__func__, __LINE__,
								item->absoluteFilePath.toAscii().data() );
						at_least_one = true;
					}
				}
			}
		}

		fprintf(stderr, "BatchThread::%s:%d => procnow='%c'\n", __func__, __LINE__,
				procnow ? 'T':'F');

		if(!procnow)
		{
			sleep(1);
			fprintf(stderr, "BatchFiltersThread::%s:%d: bip.\n", __func__, __LINE__);
		}
		else
		{
			// Output encoder
			OpenCVEncoder * encoder = NULL;
			CvSize oldSize = cvSize(0,0);
			int oldDepth = -1;

			// Process file
			QList<t_batch_item *>::iterator it;
			for(it = mpFileList->begin(); it != mpFileList->end(); ++it)
			{
				t_batch_item * item = (*it);
				if(item->processing_state == UNPROCESSED)
				{
					item->processing_state = PROCESSING;

					fprintf(stderr, "BatchThread::%s:%d : processing '%s'...\n",
							__func__, __LINE__,
							item->absoluteFilePath.toAscii().data());

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
						item->processing_state = ERROR;
					}

					if(!loadedQImage.isNull() && item->processing_state != ERROR)
					{
						fprintf(stderr, "BatchThread::%s:%d : loaded '%s' as an image...\n",
								__func__, __LINE__,
								item->absoluteFilePath.toAscii().data());
						if(mBatchOptions.reload_at_change
						   || oldSize.width != loadedQImage.width()
						   || oldSize.height != loadedQImage.height()
						   || oldDepth != loadedQImage.depth()
						   )
						{
							fprintf(stderr, "BatchThread::%s:%d : reload_at_change=%c "
									"or size changed (%dx%dx%d => %dx%dx%d) "
									"=> reload filter sequence\n",
									__func__, __LINE__,
									mBatchOptions.reload_at_change ? 'T':'F',
									oldSize.width, oldSize.height, oldDepth,
									loadedQImage.width(), loadedQImage.height(), loadedQImage.depth()
									);
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
						mpFilterManager->processImage(&image);

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
							//cvCopy(loadedImage, mDisplayImage);
						}

						// Set the state to processed
						item->processing_state = PROCESSED;
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
							while(resume)
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
									fprintf(stderr, "BatchThread::%s:%d : File '%s' EOF\n",
											__func__, __LINE__, item->absoluteFilePath.toAscii().data()
											);fflush(stderr);
									resume = false;
								} else {
									mpFilterManager->processImage(&image);

									if(fva->getFileSize()>0) {
										item->progress = (float)fva->getAbsolutePosition()/
													 (float)fva->getFileSize();

//										fprintf(stderr, "\rProgress '%s' = %4.2f %%",
//												item->absoluteFilePath.toAscii().data(),
//												item->progress * 100.f
//												);
									}

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


							}
							// Set the state to processed
							item->processing_state = PROCESSED;
							item->progress = 1.f;
							fprintf(stderr, "File '%s' PROCESSED : progress= %4.2f %%",
									item->absoluteFilePath.toAscii().data(),
									item->progress * 100.f
									);
							swReleaseImage(&loadedImage);
						}
					}


					// check if wee need to make a pause
					while(mPause && mRun)
					{
						fprintf(stderr, "Paused...");
						usleep(500);
					}

				}
			}

			if(encoder) { delete encoder; encoder = NULL; }

			// Unload plugins
			if(mpFilterManager) {
				fprintf(stderr, "BatchFiltersThread::%s:%d: Unloading plugins on %p...\n", __func__, __LINE__,
						mpFilterManager);
				mpFilterManager->slotUnloadAll();
				fprintf(stderr, "BatchFiltersThread::%s:%d: Unloaded plugins.\n", __func__, __LINE__); fflush(stderr);
			}
		}


	}

	fprintf(stderr, "BatchFiltersThread::%s:%d: Thread ended.\n", __func__, __LINE__);
	mRunning = false;
}


void BatchFiltersMainWindow::on_reloadPluginCheckBox_stateChanged(int )
{
	mBatchOptions.reload_at_change = ui->reloadPluginCheckBox->isChecked();
	mBatchThread.setOptions(mBatchOptions);
}

