/***************************************************************************
 *  batchqueuewidget.h Manage a queue of batch tasks and display the progress
 *						of current processing task
 *
 *  Fri Mar 09 08:17:41 2012
 *  Copyright  2012  Christophe Seyve
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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef BATCHQUEUEWIDGET_H
#define BATCHQUEUEWIDGET_H

#include <QWidget>
#include <QMutex>
#include <QTreeWidgetItem>

#include "batch_progress_widget.h"

namespace Ui {
    class BatchQueueWidget;
}

/** @brief Display one batch task in tree widget */
class BatchQueueTreeWidgetItem : public QTreeWidgetItem
{
public:
	BatchQueueTreeWidgetItem(QTreeWidget * pparent, t_batch_task * pTask);

	t_batch_task * getTask() { return mpBatchTask; }
private:
	t_batch_task * mpBatchTask;
};




/** @brief Display the items in queue and their progress, and enalble to change
	order, delete...
  */
class BatchQueueWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BatchQueueWidget(QWidget *parent = 0);
    ~BatchQueueWidget();

	/** @brief Append a new batch task to managed tasks list */
	void appendBatchTask(t_batch_task * pTask);

protected:
    void changeEvent(QEvent *e);

private slots:
	void on_batchPlayButton_toggled(bool checked);

	void on_batchMoveUpButton_clicked();

	void on_batchMoveDownButton_clicked();

	void on_batchDeleteButton_clicked();

	void on_batchQueueTreeWidget_itemClicked(QTreeWidgetItem *item, int column);

	void on_nbThreadsSpinBox_valueChanged(int arg1);

private:
    Ui::BatchQueueWidget *ui;

	/// List of batch tasks
	QList<t_batch_task *> mBatchTasksList;

	/// Batch processing thread list: since htere are many core,
	/// there can be several batch tasks
	QList<BatchFiltersThread *> mBatchThreadList;

	/// Display the batch task which is processed now
	BatchProgressWidget * mpBatchProgressWidget;
};

#endif // BATCHQUEUEWIDGET_H
