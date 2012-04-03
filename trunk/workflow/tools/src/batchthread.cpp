/***************************************************************************
	batchthread.cpp  -  Movie / Image batch processor thread
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

#include "batchthread.h"
#include "FileVideoAcquisition.h"
#include "ffmpeg_file_acquisition.h"

#include "piaf-settings.h"
#include "piaf-common.h"
#include "OpenCVEncoder.h"

#include <QFileInfo>

/*******************************************************************************

					PROCESSING THREAD

  *****************************************************************************/

BatchFiltersThread::BatchFiltersThread()
{
	mRun = mRunning = mProcessing = false;
	mpFilterSequencer = NULL;


	mpBatchTask = NULL;	// no current batch task


	memset(&mTimeHistogram, 0, sizeof(t_time_histogram));
	mTimeHistogram.nb_iter = -4; // to prevent from counting first iterations

	mPause = false;

	if(mpBatchTask)
	{
		mpBatchTask->options.reload_at_change = true;
		mpBatchTask->options.use_grey = false;
	}
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
	if(mpBatchTask)
	{
		mpBatchTask->options = options;
	}
	}

void BatchFiltersThread::startProcessing(bool on)
{
	mProcessing = on;
}

void BatchFiltersThread::setPause(bool on)
{
	mPause = on;
}


bool g_debug_BatchFiltersThread = false;


void BatchFiltersThread::allocHistogram(float maxproctime_us)
{
	::allocHistogram(&mTimeHistogram, maxproctime_us);
	fprintf(stderr, "[BatchThread]::%s:%d: max proc time at init : %g us "
			"=> max=%d => scale=%g item/us\n",
			__func__, __LINE__, maxproctime_us,
			mTimeHistogram.max_histogram,
			mTimeHistogram.time_scale
			);
}

void BatchFiltersThread::appendTimeUS(float proctime_us)
{
	::appendTimeUS(&mTimeHistogram, proctime_us);
}

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
			if(!mpBatchTask) {
				PIAF_MSG(SWLOG_DEBUG, "no task, no processing");
				procnow = false;
			}
			else if(mpBatchTask->itemsList.isEmpty()) {
				PIAF_MSG(SWLOG_DEBUG, "no item in task");
				procnow = false;
			} else {
				PIAF_MSG(SWLOG_INFO, "%d items in task, processing...",
						 mpBatchTask->itemsList.count()
						 );
				// check if at least one needs to be processed
				bool at_least_one = false;
				QList<t_batch_item *>::iterator it;
				for(it = mpBatchTask->itemsList.begin();
					mRun && it != mpBatchTask->itemsList.end() && !at_least_one; ++it)
				{
					t_batch_item * item = (*it);
					if(item->processing_state == UNPROCESSED)
					{
						if(g_debug_BatchFiltersThread) {
							fprintf(stderr, "BatchFiltersThread::%s:%d: file '%s' is not processed yet.\n",
								__func__, __LINE__,
								item->absoluteFilePath.toUtf8().data() );
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
			bool still_processing = true;
			bool was_paused = false;

			while(still_processing) {
				still_processing = false;
				was_paused = false;
				for(it = mpBatchTask->itemsList.begin();
					mRun && !was_paused
					&& it != mpBatchTask->itemsList.end(); ++it)
				{
					t_batch_item * item = (*it);
					if(item->processing_state == UNPROCESSED)
					{
						item->processing_state = PROCESSING;

						if(g_debug_BatchFiltersThread) {
							fprintf(stderr, "BatchThread::%s:%d : processing '%s'...\n",
								__func__, __LINE__,
								item->absoluteFilePath.toUtf8().data());
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
									item->absoluteFilePath.toUtf8().data());
							item->processing_state = ERROR_READ;
						}

						if(!loadedQImage.isNull()
								&& item->processing_state != ERROR_READ)
						{
							if(g_debug_BatchFiltersThread) {
								fprintf(stderr, "BatchThread::%s:%d : "
										"loaded '%s' as an image...\n",
										__func__, __LINE__,
										item->absoluteFilePath.toUtf8().data());
							}

							if(mpBatchTask->options.reload_at_change
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
										mpBatchTask->options.reload_at_change ? 'T':'F',
										oldSize.width, oldSize.height, oldDepth,
										loadedQImage.width(), loadedQImage.height(), loadedQImage.depth()
										);
								}
								if(encoder) { delete encoder; encoder = NULL; }

								// unload previously loaded filters
								mpFilterSequencer->unloadAllLoaded();
								// reload same file
								mpFilterSequencer->loadFilterList(mpFilterSequencer->getPluginSequenceFile());
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
							int retproc = mpFilterSequencer->processImage(&image);
							if(retproc < 0)
							{
								item->processing_state = ERROR_PROCESS;

							} else {
								// Store statistics
								appendTimeUS(image.deltaTus);

								// Check if we need to record
								if(mpBatchTask->options.record_output) {
									QFileInfo fi(item->absoluteFilePath);
									QString outFile = item->absoluteFilePath
													  + "-" + mpBatchTask->options.sequence_name + "." + fi.suffix();
									loadedQImage.save(outFile);
								}

								// Check if we need to copy an image for display
								if(mpBatchTask->options.view_image) {
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
							/// @todo : use factory Load ad movie
							FileVideoAcquisition * fva = (FileVideoAcquisition *)new FFmpegFileVideoAcquisition(
									item->absoluteFilePath.toUtf8().data());
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
															mpBatchTask->options.use_grey ? 1:4);
								// Loop on images
								swImageStruct image;
								memset(&image, 0, sizeof(swImageStruct));

								image.width = loadedImage->widthStep / loadedImage->nChannels;// beware of pitch
								image.height = loadedImage->height;
								image.depth = loadedImage->nChannels;
								image.bytedepth = loadedImage->depth / 8;
								image.buffer_size =
										image.width * image.height
										* image.depth * image.bytedepth;

								image.buffer = loadedImage->imageData; // Buffer
								bool resume = true;
								while(resume && mRun)
								{
									// Check if we are in pause
									if(mPause) {

										fprintf(stderr, "Paused !");
										while(mPause && mRun && mProcessing)
										{
											fprintf(stderr, "Paused...");
											usleep(500000);
										}

										// if the item is no more in processing list
										if(mpBatchTask->itemsList.indexOf(item) < 0)
										{
											was_paused = true;
											still_processing = true;
											// Forget this item because it may have been destroyed
											item = NULL;
											break;
										}

									}



									bool read_frame = fva->GetNextFrame();
									long buffersize = image.buffer_size ;
									int ret = -1;
									if(read_frame) {
										if(mpBatchTask->options.use_grey)
										{
											//ret = fva->readImageYNoAcq((uchar *)image.buffer, &buffersize);
											IplImage * grayImage = fva->readImageY();
											if(grayImage)
											{
												ret = 0;
												image.buffer = grayImage->imageData; // Buffer
											}

										}
										else
										{
											//ret = fva->readImageRGB32NoAcq((uchar *)image.buffer, &buffersize);
											IplImage * bgr32Image = fva->readImageRGB32();
											if(bgr32Image)
											{
												ret = 0;
												image.buffer = bgr32Image->imageData; // Buffer
											}

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
										int retproc = mpFilterSequencer->processImage(&image);

										if(retproc < 0) {
											// Set the state to processing error
											item->processing_state = ERROR_PROCESS;
											// but it may be reloaded on next frame
										}
										else {
											// Store statistics
											appendTimeUS(image.deltaTus);

											item->processing_state = PROCESSING;

											// Check if we need to record
											if(mpBatchTask->options.record_output) {
												if(!encoder) {
													QFileInfo fi(item->absoluteFilePath);
													QString outputFile = item->absoluteFilePath
																	  + "-" + mpBatchTask->options.sequence_name + ".avi";
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
											if(mpBatchTask->options.view_image) {
												mDisplayMutex.lock();
												if(mDisplayImage && (mDisplayImage->widthStep!=loadedImage->widthStep || mDisplayImage->height!=loadedImage->height)) {
													swReleaseImage(&mDisplayImage);
												}

												if(!mDisplayImage) {
													mDisplayImage = cvCloneImage(loadedImage);
													fprintf(stderr, "clone disp=%dx%dx%d", mDisplayImage->width, mDisplayImage->height, mDisplayImage->nChannels);
												}
												else {
													//fprintf(stderr, "disp=%dx%dx%d", mDisplayImage->width, mDisplayImage->height, mDisplayImage->nChannels);
													cvCopy(loadedImage, mDisplayImage);
												}
												mDisplayMutex.unlock();

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
								if(item) {
									if(item->processing_state == PROCESSING) {
										item->processing_state = PROCESSED;
									}

									item->progress = 1.f;

									fprintf(stderr, "File '%s' %s : progress= %4.2f %%",
											item->absoluteFilePath.toAscii().data(),
											item->processing_state == PROCESSED ? "PROCESSED" : "ERROR_PROCESS",
											item->progress * 100.f
											);
								}

								swReleaseImage(&loadedImage);
							}
						}


						// check if we need to make a pause
						if(mPause) {
							fprintf(stderr, "Paused !");
							while(mPause && mRun && mProcessing)
							{
								fprintf(stderr, "Paused...");
								usleep(500000);
							}
							was_paused = true;
							still_processing = true;
						}
					}
				} // end of iterator on file list

				if(encoder) { delete encoder; encoder = NULL; }
			} // while still_processing

			// Unload plugins at end of list
			if(mpFilterSequencer) {
				if(g_debug_BatchFiltersThread) {
					fprintf(stderr, "BatchFiltersThread::%s:%d: Unloading plugins on %p...\n", __func__, __LINE__,
						mpFilterSequencer);
				}

				mpFilterSequencer->unloadAllLoaded();

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

