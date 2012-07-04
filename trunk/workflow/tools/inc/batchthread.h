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

#ifndef BATCHTHREAD_H
#define BATCHTHREAD_H

#include "piaf-common.h"
#include "swvideodetector.h"

#include "PiafFilter.h"

#include <QString>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include <QTreeWidgetItem>

typedef enum {
	UNPROCESSED,
	PROCESSING,
	PROCESSED,
	ERROR,
	ERROR_READ,	///< Error while reading the file
	ERROR_PROCESS	///< Error while processing the file
} enum_proc_state;



/** \brief File item for batch processing

  */
typedef struct {
	QString absoluteFilePath;			///< Absolute path to file
	bool is_movie;						///< flag to tell if it's a movie. If false, it's an image
	enum_proc_state former_state;		///< state of processing at previous check
	enum_proc_state processing_state;	///< state of processing :

	QTreeWidgetItem * treeItem;			///< Item in file list treewidget
	float progress;						///< Processing progress in 0..1
} t_batch_item;

/** \brief Processing and display options
*/
typedef struct {
	bool use_grey;						///< use grayscale input image
	bool reload_at_change;				///< reload plugin when file changes (usefull when all files aren't the same size)
	bool record_output;					///< record output
	bool view_image;					///< periodically display image when processing
	QString sequence_name;				///< name of sequence file
} t_batch_options;

/** @brief Print task and its file items */
void printBatchOptions(t_batch_options * pOptions);


/** @brief Batch processing task
  */
typedef struct {
	QString title;						///< Title for identifying task in widget
	int progress;						///< Progress in [0..100]
	QList<t_batch_item *> itemsList;	///< List of files
	QString sequencePath;				///< Path to plugins sequence
	t_batch_options options;			///< processing options
} t_batch_task;

/** @brief Print task and its file items */
void printBatchTask(t_batch_task * pTask);

/** @brief Delete task and its file items */
void purgeBatchTask(t_batch_task * pTask);



/** \brief Threaded batch processing

  */
class BatchFiltersThread : public QThread
{
	Q_OBJECT
public:
	BatchFiltersThread();
	~BatchFiltersThread();

	/** @brief Return true if the thread is running */
	bool isRunning() { return mRunning; }

	/** \brief Set the pointer on current task

	  This function enables to display a task which is waiting in stack, finished or in progress
	  */
	int setBatchTask(t_batch_task * pTask);

//	void run();

	/** @brief Set the processing filter manager */
	void setFilterSequencer(FilterSequencer * fm) { mpFilterSequencer = fm; }

	/** @brief Set the processing list (OBSOLETE) */
	void setFileList(QList<t_batch_item *> * pFileList);

	/** @brief start/stop the processing */
	void startProcessing(bool on );
	/** @brief pause the processing */
	void setPause(bool on);

	/** @brief Set filter options */
	void setOptions(t_batch_options options);

	/** @brief Get the image for display */
	IplImage * getDisplayImage() { return mDisplayImage; }

	void lockDisplay(bool on) {
		if(on) {
			mDisplayMutex.lock();
		} else {
			mDisplayMutex.unlock();
		}
	}

	/// \brief get time histogram
	t_time_histogram getTimeHistogram() { return mTimeHistogram; }

protected:
	void run();

private:
	bool mRun; ///< run command (loop while run is true)
	bool mRunning; ///< run status (false at end of loop)
	bool mProcessing; ///< run command (process while run is true)
	bool mPause; ///< user asked for a pause


	t_time_histogram mTimeHistogram; ///< time histogram
	void appendTimeUS(float proctime_us); ///< Append processing time in us
	void allocHistogram(float maxproctime_us);

	t_batch_task * mpBatchTask;	///< current batch task
//	QList<t_batch_item *> * mpFileList; ///< pointer to processing list
//	t_batch_options mBatchOptions;	///< batch processing options: greyscale, reload plugins, ...

	QMutex mProcMutex; ///< Processing mutex
	QMutex mLoopMutex; ///< Mutex for loop wait condition
	QWaitCondition mWaitCondition; ///< wait condition for waiting loop

	QMutex mDisplayMutex; ///< Mutex for display


	FilterSequencer * mpFilterSequencer;
	IplImage * mDisplayImage;


signals:
	void signalFileProcessed(t_batch_item *);
};

#endif // BATCHTHREAD_H
