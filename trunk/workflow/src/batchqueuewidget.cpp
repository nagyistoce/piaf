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

#include "inc/batchqueuewidget.h"
#include "ui_batchqueuewidget.h"

#include "piaf-common.h"

BatchQueueWidget::BatchQueueWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BatchQueueWidget)
{
    ui->setupUi(this);

	// =================== Create batch status progress widget =================
	mpBatchProgressWidget = new BatchProgressWidget(ui->batchProgressScrollAreaContents);
	ui->batchProgressScrollAreaContents->layout()->addWidget(mpBatchProgressWidget);
	// Disable interactions for user to show only the state of progress of batch
	mpBatchProgressWidget->enableInteractive(false);

	// Create the first Batch thread
	BatchFiltersThread * firstThread = new BatchFiltersThread();
	mBatchThreadList.append(firstThread);

}

BatchQueueWidget::~BatchQueueWidget()
{
    delete ui;
}

void BatchQueueWidget::changeEvent(QEvent *e)
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

void BatchQueueWidget::on_batchPlayButton_toggled(bool checked)
{

}

void BatchQueueWidget::on_batchMoveUpButton_clicked()
{

}

void BatchQueueWidget::on_batchMoveDownButton_clicked()
{

}

void BatchQueueWidget::on_batchDeleteButton_clicked()
{

}

void BatchQueueWidget::on_batchQueueTreeWidget_itemClicked(QTreeWidgetItem *item, int column)
{

}


/** @brief Append a new batch task to managed tasks list */
void BatchQueueWidget::appendBatchTask(t_batch_task * pTask)
{
	if(!pTask) { return; }

	PIAF_MSG(SWLOG_INFO, "Adding new task { title='%s' %d files}",
			 pTask->title.toAscii().data(),
			 pTask->itemsList.count()
			 );

	mBatchTasksList.append(pTask);

	// create item
	new BatchQueueTreeWidgetItem(ui->batchQueueTreeWidget, pTask);
}



BatchQueueTreeWidgetItem::BatchQueueTreeWidgetItem(QTreeWidget * pparent, t_batch_task * pTask)
	: QTreeWidgetItem(pparent),
	  mpBatchTask(pTask)
{
	if(mpBatchTask)
	{
		PIAF_MSG(SWLOG_INFO, "Adding new task { title='%s' %d files}",
				 pTask->title.toAscii().data(),
				 pTask->itemsList.count()
				 );
		setText(0, mpBatchTask->title );
		setText(2, QString::number(mpBatchTask->itemsList.count()) + QObject::tr(" files") );
	}
	else
	{
		setText(0, QObject::tr("Error: Null task") );
	}
}

void BatchQueueWidget::on_nbThreadsSpinBox_valueChanged(int nb)
{
	PIAF_MSG(SWLOG_INFO, "Changed number of threads: %d to %d",
			 mBatchThreadList.count(), nb
			 );
	/// @todo : handle destruction of threads, and allocation of its processing task to another

}
