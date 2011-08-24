/***************************************************************************
	   plugineditform.cpp  -  plugin sequence control widget
							 -------------------
	begin                : Fri Aug 12 2011
	copyright            : (C) 2002-2011 by Christophe Seyve
	email                : cseyve@free.fr
 ***************************************************************************/


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

#include "inc/plugineditorform.h"
#include "ui_plugineditorform.h"

#include "piaf-common.h"

#include <QMessageBox>
#include <QFileDialog>

extern bool g_debug_FilterSequencer;

AvailablePluginTreeWidgetItem::AvailablePluginTreeWidgetItem(
		QTreeWidgetItem * parent,
		PiafFilter * pFilter, int idx_function)
	: QTreeWidgetItem(parent), mpFilter(pFilter), mIndexFunction(idx_function)
{
	if(!mpFilter) { setText(0, ("NULL plugin")); return; }
	if(mIndexFunction >= mpFilter->nbfunctions()) { setText(0, ("idx > nbfunc")); return; }

	DEBUG_MSG("Creating AvailablePluginTreeWidgetItem([%d] = '%s')",
			  mIndexFunction, mpFilter->getFunction(mIndexFunction).name);

	// Set columns
	setText(0, QString(mpFilter->getFunction(mIndexFunction).name));
	//setText(1, QString::number(mpFilter->getFunction(mIndexFunction).nb_params));
}

AvailablePluginTreeWidgetItem::~AvailablePluginTreeWidgetItem()
{
	DEBUG_MSG("Deleting AvailablePluginTreeWidgetItem");
}

PluginEditorForm::PluginEditorForm(QWidget *parent) :
    QWidget(parent),
	ui(new Ui::PluginEditorForm)
{
    ui->setupUi(this);

	pWorkspace = NULL;
	mHasGUI = false;

	// Load available plugins
	mFilterSequencer.loadFilters();
	mpFilterSequencer = NULL;
	mOwnFilterSequencer = false;


	// Display the list of available filters
	QList<PiafFilter *> availList = mFilterSequencer.getAvailableFilters();
	QList<PiafFilter *>::iterator it;
	for(it = availList.begin(); it != availList.end(); ++it)
	{
		PiafFilter * filter = (*it);

		DEBUG_MSG("Adding binary '%s' : %d / %d functions", filter->exec_name,
				  filter->getIndexFunction(),
				  filter->nbfunctions());

		QStringList cols;
		cols << QString(filter->subcategory );

		QTreeWidgetItem * rootItem = new QTreeWidgetItem(ui->availablePluginsTreeWidget, cols);

		// Create a root tree for each filter binary, then one item per function
		for(int idx_function = 0; idx_function<filter->nbfunctions(); idx_function++)
		{
			// Create item
			new AvailablePluginTreeWidgetItem( rootItem,
											   filter, idx_function
											   );
		}
	}
	mFilterSequencer.unloadAllAvailable();

}


/* Change sequencer */
void PluginEditorForm::setFilterSequencer(FilterSequencer * seq)
{
	if(mpFilterSequencer) {
		disconnect(mpFilterSequencer);
		if(mOwnFilterSequencer) {
			delete mpFilterSequencer;
			mpFilterSequencer = NULL;
		}
	}
	mpFilterSequencer = seq;
	mOwnFilterSequencer = false;
	// connect refresh slot
	if(mpFilterSequencer) {
		connect(mpFilterSequencer, SIGNAL(selectedFilterChanged()), this,
				SLOT(on_filterSequencer_selectedFilterChanged()));
	}
}

FilterSequencer * PluginEditorForm::createFilterSequencer() {
	setFilterSequencer(new FilterSequencer());
	mOwnFilterSequencer = true;
	return mpFilterSequencer;
}

PluginEditorForm::~PluginEditorForm()
{
	purge();
	delete ui;
}

void PluginEditorForm::on_filterSequencer_selectedFilterChanged()
{
	// Update
	if(!mpFilterSequencer) return;

	fprintf(stderr, "PluginEditForm::%s:%d : update !!\n", __func__, __LINE__);
	updateSelectedView();
}

void PluginEditorForm::changeEvent(QEvent *e)
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


// update selected filters QListView.
void PluginEditorForm::updateSelectedView()
{
	fprintf(stderr, "\n=========================================\n"
			"PluginEditorForm::%s:%d : update list !", __func__, __LINE__);
	if(!mpFilterSequencer) { return; }

	// clear listview then add filters
	ui->selectedPluginsTreeWidget->clear();


	// --- add new items for current loaded plugins ---
	QList<PiafFilter *> filterList = mpFilterSequencer->getLoadedFilters();

	bool refresh = true;
	bool a_plugin_crashed = false;

	while(refresh) {
		refresh = false;

		int i = filterList.count()-1;

		if(!filterList.isEmpty()) {
			QList<PiafFilter *>::iterator it;

			for(it = filterList.begin();
				it != filterList.end(); ++it)
			{
				PiafFilter * curF = (*it);

				//if(g_debug_FilterSequencer)
				{
					fprintf(stderr, "PiafFilterManager:%s:%d : creating items for filter %p\n",
							__func__, __LINE__,
						   curF); fflush(stderr);
				}

				if(!curF->plugin_died)
				{
					new LoadedPluginTreeWidgetItem (
							ui->selectedPluginsTreeWidget,
							curF);

				}
				else
				{
					fprintf(stderr, "PiafFilterManager:%s:%d : filter %p died, removing it\n",
							__func__, __LINE__,
						   curF);
					a_plugin_crashed = true;

					if(!mNoWarning ) {
						QString filename = QString(curF->exec_name);
						QMessageBox::critical(NULL,
							tr("Filter Error"),
							tr("Filter process ") + filename + tr(" died."),
							QMessageBox::Abort,
							QMessageBox::NoButton,
							QMessageBox::NoButton);
					}
					else {
						fprintf(stderr, "[PiafFilterMng]::%s:%d : dead filter (but no warning)...\n",
								__func__, __LINE__);
					}

					mpFilterSequencer->removeFilter(curF);
					refresh = true;
					break;
				}

//				if(curF && curF->funcList) {

//					curF->loaded = true;

//					if(mHasGUI) { // create display for selected item
//						curF->selItem = new Q3ListViewItem(selectedListView,
//							curF->funcList[curF->indexFunction].name, "" );
//						if(curF->enabled) {
//							curF->selItem->setPixmap(1, check);
//						} else {
//							curF->selItem->setPixmap(1, uncheck);
//						}

//						if(i == idViewPlugin) {
//							curF->selItem->setPixmap(2, eye);
//						}
//						else
//							if(i<idViewPlugin)
//								curF->selItem->setPixmap(2, eye_grey);

//						if(i == idEditPlugin)
//							selectedListView->setSelected(curF->selItem, true);
//					}

//					i--;
//				}
//				else
//				{
//					fprintf(stderr, "[PiafFilters] %s:%d : invalid function in filter\n", __func__, __LINE__);
//				}
			}
		}
	}

	if(mHasGUI) {	// create an item to visualize original item
		fprintf(stderr, "PiafFilterManager:%s:%d : creating root item after %d filters\n",
				__func__, __LINE__,
			  filterList.count() ); fflush(stderr);
		QStringList cols;
		cols << tr("Original");
		originalItem = new QTreeWidgetItem( ui->selectedPluginsTreeWidget, cols );
	} else {
		originalItem = NULL;
	}

	if(g_debug_FilterSequencer) {
		fprintf(stderr, "PiafFilterManager:%s:%d : update display after %d filters\n",
				__func__, __LINE__,
				filterList.count() );
		fflush(stderr);
	}

	if(a_plugin_crashed) {
		if(mAutoReloadSequence)
		{
			fprintf(stderr, "[PiafFilterManager]::%s:%d : a plugin crashed & auto-reload is on "
					"=> unload then reload plugin sequence '%s'\n",
					__func__, __LINE__,
					mpFilterSequencer->getPluginSequenceFile());
			// unload previously loaded filters
			mpFilterSequencer->unloadAllLoaded();
			// reload same file
			mpFilterSequencer->loadSequence(mpFilterSequencer->getPluginSequenceFile());
		}
	}

	update();
}

void PluginEditorForm::setWorkspace(QWorkspace * wsp) {
	pWorkspace = wsp;

	if(pWorkspace) {
		((QWorkspace *)pWorkspace)->addWindow((QWidget *)this);
		mHasGUI = true;

		QPixmap winIcon = QPixmap(":images/pixmaps/IconFilters.png");
		setIcon(winIcon);

		show();
	}
}

/*  Initialisation :
	- initializes buffers, reset pointers
	- load filter list
	- creates widgets */
void PluginEditorForm::init()
{
	mHasGUI = false;
	mNoWarning = false;
	mAutoReloadSequence = false; // do not reload buggy plugins
	pWorkspace = NULL;

	show();
}


// destructor.
void PluginEditorForm::purge()
{
	fprintf(stderr, "PluginEditorForm::%s:%d DELETE PluginEditorForm\n", __func__, __LINE__);

	if(mpFilterSequencer) {
		delete mpFilterSequencer;
	}

	ui->selectedPluginsTreeWidget->clear();
}






/********************************************************************

					FILTER LIST MANAGEMENT

 ********************************************************************/


LoadedPluginTreeWidgetItem::LoadedPluginTreeWidgetItem(QTreeWidget * treeWidget, PiafFilter * pFilter)
	: QTreeWidgetItem(treeWidget), mpFilter(pFilter)
{
	if(!mpFilter) {
		setText(0, ("Error filter"));
		return;
	}
	fprintf(stderr, "Loaded::%s:%d : enabled=%c / final=%c\n",
			__func__, __LINE__,
			pFilter->isEnabled()?'T':'F',
			pFilter->isFinal()?'T':'F'
			);
	// Fill columns
	setText(0, QString( mpFilter->getFunction( mpFilter->getIndexFunction()).name ) );
	QIcon icon1, icon2;
	QString str1, str2;

	if(pFilter->isEnabled()) {
		icon1 = QIcon(":/icons/16x16/page-zoom.png");
		str1 = "T";
	} else {
		icon1 = QIcon(":/icons/16x16/edit-delete.png");
		str1 = "F";
	}
	setIcon(1, icon1);
	setText(1, str1);

	if(pFilter->isFinal()) {
		icon2 = QIcon(":/icons/16x16/page-zoom.png");
		str2 = "T";
	}
	else
	{
		str2 = "";
	}
	setIcon(2, icon2);
	setText(2, str2);

}

LoadedPluginTreeWidgetItem::~LoadedPluginTreeWidgetItem()
{

}

void PluginEditorForm::on_appendPluginButton_clicked()
{
	// Get selected item
	AvailablePluginTreeWidgetItem * item =
			(AvailablePluginTreeWidgetItem *)ui->availablePluginsTreeWidget->selectedItems().at(0);
	if(!item) { return; }
	if(!mpFilterSequencer) {
		fprintf(stderr, "[PluginEditForm]::%s:%d: creating FilterSequencer...\n", __func__, __LINE__);

		createFilterSequencer();
	}

	// Add this item
	item->getFilter()->indexFunction = item->getIndexFunction();
	PiafFilter * newFilter = mpFilterSequencer->addFilter(item->getFilter());
}


void PluginEditorForm::on_loadButton_clicked()
{
	if(!mpFilterSequencer) {
		QMessageBox::critical(NULL, tr("No sequencer"), tr("No filter sequencer = nothing to save"));
		return;
	}

	QString path = QFileDialog::getOpenFileName(NULL,
												tr("Open sequence"),
												tr("Sequences (*.flist)"));
	if(path.isEmpty()) return;

	mpFilterSequencer->loadSequence(path.toUtf8().data());
}

void PluginEditorForm::on_saveButton_clicked()
{
	if(!mpFilterSequencer) {
		QMessageBox::critical(NULL, tr("No sequencer"), tr("No filter sequencer = nothing to save"));
		return;
	}

	QString path = QFileDialog::getSaveFileName(NULL,
												tr("Save sequence"),
												tr("Sequences (*.flist)"));
	if(path.isEmpty()) return;

	mpFilterSequencer->saveSequence(path.toUtf8().data());
}

void PluginEditorForm::on_removePluginButton_clicked()
{
	if(ui->selectedPluginsTreeWidget->selectedItems().isEmpty()) return;

	LoadedPluginTreeWidgetItem * item =
			(LoadedPluginTreeWidgetItem *)ui->selectedPluginsTreeWidget->selectedItems().at(0);
	if(!item) {
		return; }
	if(!mpFilterSequencer) { return; }

	mpFilterSequencer->removeFilter(item->getFilter());
}

void PluginEditorForm::on_paramsButton_clicked()
{
	if(ui->selectedPluginsTreeWidget->selectedItems().isEmpty()) return;
	LoadedPluginTreeWidgetItem * item =
			(LoadedPluginTreeWidgetItem *)ui->selectedPluginsTreeWidget->selectedItems().at(0);
	if(!item) { return; }
	if(!mpFilterSequencer) { return; }

	//	mpFilterSequencer->edit(item->getFilter());

}

void PluginEditorForm::on_activateButton_clicked()
{
	LoadedPluginTreeWidgetItem * item =
			(LoadedPluginTreeWidgetItem *)ui->selectedPluginsTreeWidget->selectedItems().at(0);
	if(!item) { return; }
	mpFilterSequencer->toggleFilterDisableFlag(item->getFilter());
}

void PluginEditorForm::on_timeButton_clicked()
{

}

void PluginEditorForm::on_upButton_clicked()
{

}

void PluginEditorForm::on_downButton_clicked()
{

}

void PluginEditorForm::on_selectedPluginsTreeWidget_itemClicked(QTreeWidgetItem* item, int column)
{

}

void PluginEditorForm::on_selectedPluginsTreeWidget_itemDoubleClicked(QTreeWidgetItem* item, int column)
{
	if(!item) return;
	if(!mpFilterSequencer) return;

	LoadedPluginTreeWidgetItem * loaded =
			(LoadedPluginTreeWidgetItem *)item;
	mpFilterSequencer->setFinal(loaded->getFilter());

}
