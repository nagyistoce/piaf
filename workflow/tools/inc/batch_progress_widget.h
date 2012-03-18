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

#include <QTimer>
#include <QList>
#include <QTreeWidgetItem>
#include <QSettings>

#include "PiafFilter.h"
#include "batchthread.h"

#include "time_histogram.h"

#include "timehistogramwidget.h"
#include "swvideodetector.h"

namespace Ui {
    class BatchProgressWidget;
}

/// Settings for piaf batch processor
#define PIAFBATCHSETTINGS	"PiafBatch"


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

	/** \brief Set current batch thread, if not allocated */
	void setBatchFiltersThread(BatchFiltersThread  * );

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

	/// Allocate a new batch thread for standalone window
	void allocateBatchThread();
	BatchFiltersThread  * mpBatchThread; ///< Batch processing thread
	bool mBatchThreadAllocated;	///< tells if current batch thread has been allocated by the widget

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
