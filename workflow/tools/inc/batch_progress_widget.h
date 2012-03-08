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

#ifndef BATCH_PROGRESS_WIDGET_H
#define BATCH_PROGRESS_WIDGET_H

#include <QWidget>

#include <QThread>
#include <QTimer>
#include <QList>
#include <QTreeWidgetItem>
#include <QMutex>
#include <QWaitCondition>
#include <QSettings>

#include "PiafFilter.h"

#include "time_histogram.h"

#include "timehistogramwidget.h"
#include "swvideodetector.h"

namespace Ui {
    class BatchProgressWidget;
}


typedef enum {
	UNPROCESSED,
	PROCESSING,
	PROCESSED,
	ERROR,
	ERROR_READ,	///< Error while reading the file
	ERROR_PROCESS	///< Error while processing the file
} enum_proc_state;

/// Settings for piaf batch processor
#define PIAFBATCHSETTINGS	"PiafBatch"


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


/** @brief Batch processing task
  */
typedef struct {
	QList<t_batch_item *> itemsList;	///< List of files
	QString sequencePath;				///< Path to plugins sequence
	t_batch_options options;			///< processing options
} t_batch_task;

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

/** \brief Display/edit status of a batch task progress


*/

class BatchProgressWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BatchProgressWidget(QWidget *parent = 0);
    ~BatchProgressWidget();

	/** \brief Enable or not the interactive mode: add/remove files, ...

		on non-interactive mode, the user can't add or remove files,
		can't change the filters' sequence
	*/
	void enableInteractive(bool on);

	/** \brief Set the plugin sequence */
	void setPluginSequence(QString sequencepath);

	/** \brief Set the list of files */
	void setFilesList(QStringList fileList);

	/** \brief Set the pointer on current task

	  This function enables to display a task which is waiting in stack, finished or in progress
	  */
	int setBatchTask(t_batch_task * pTask);

protected:
    void changeEvent(QEvent *e);

private:
    Ui::BatchProgressWidget *ui;
	QTimer mDisplayTimer; ///< Display timer for periodic refresh
	QString mLastDirName; ///< Last opened directory with images
	QString mLastPluginsDirName; ///< Last opened directory with plugin sequence

	QImage mLoadImage; ///< Loaded image form list
	IplImage * mDisplayIplImage; ///< IplImage for display conversion

	/// Filter processing manager
	FilterSequencer mFilterSequencer;
	FilterSequencer mPreviewFilterSequencer;

	BatchFiltersThread mBatchThread; ///< Batch processing thread
//	t_batch_options mBatchOptions;	///< batch processing options: greyscale, reload plugins, ...

	/// Allocates a new batch task
	void allocateBatchTask();

	t_batch_task * mpBatchTask;	///< current batch task
	bool mBatchTaskAllocated;	///< tells if current batch task has been allocated by the widget



	/// Settings for saving last path, ...
	QSettings mSettings;

	void loadSettings();
	void saveSettings();

private slots:
	/// Load a plugins list (plugin files + function + parameters)
	void on_filesTreeWidget_itemSelectionChanged();
	void on_reloadPluginCheckBox_stateChanged(int );
	void on_filesTreeWidget_itemChanged(QTreeWidgetItem* item, int column);
	void on_filesTreeWidget_itemActivated(QTreeWidgetItem* item, int column);
	void on_filesTreeWidget_itemClicked(QTreeWidgetItem* item, int column);

	void on_playPauseButton_toggled(bool checked);
	void on_greyButton_toggled(bool checked);
	void on_viewButton_toggled(bool checked);
	void on_recordButton_toggled(bool checked);

	void on_resetButton_clicked();
	void on_delButton_clicked();
	void on_addButton_clicked();
	void on_loadButton_clicked();

	void on_mDisplayTimer_timeout();
};

#endif // BATCH_PROGRESS_WIDGET_H
