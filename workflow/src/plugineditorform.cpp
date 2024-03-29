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
#include "piafworkflow-settings.h"

#include <QMessageBox>
#include <QFileDialog>

extern bool g_debug_FilterSequencer;

bool g_debug_PluginEditorForm = false;

/******************************************************************************
					AVAILABLE PLUGINS TREE
  ******************************************************************************/
AvailablePluginTreeWidgetItem::AvailablePluginTreeWidgetItem(
		QTreeWidgetItem * parent,
		PiafFilter * pFilter, int idx_function)
	: QTreeWidgetItem(parent), mpFilter(pFilter), mIndexFunction(idx_function)
{
	if(!mpFilter) { setText(0, ("NULL plugin")); return; }

	if(mIndexFunction >= mpFilter->nbfunctions()) {
		setText(0, ("idx > nbfunc")); return;
	}

	if(g_debug_FilterSequencer)
	{
		DEBUG_MSG("\tAvailablePluginTreeWidgetItem([%d] = '%s')",
			  mIndexFunction, mpFilter->getFunction(mIndexFunction).name);
	}

	// Set columns
	if(strlen(mpFilter->getFunction(mIndexFunction).name) == 0)
	{
		QFileInfo fi(mpFilter->name());
		setText(0, QObject::tr("Error: ") + fi.baseName());
	} else {
		setText(0, QString(mpFilter->getFunction(mIndexFunction).name));
	}
	//setText(1, QString::number(mpFilter->getFunction(mIndexFunction).nb_params));
}

AvailablePluginTreeWidgetItem::~AvailablePluginTreeWidgetItem()
{
//	DEBUG_MSG("Deleting AvailablePluginTreeWidgetItem");
}
























/******************************************************************************
					PLUGIN LIST EDIT WIDGET
  ******************************************************************************/


PluginEditorForm::PluginEditorForm(QWidget *parent) :
    QWidget(parent),
	ui(new Ui::PluginEditorForm)
{
    ui->setupUi(this);

	pWorkspace = NULL;
	mHasGUI = false;

	mpSelectedFilter = NULL;
	//mFilterSequencer.loadFilters();
	mpFilterSequencer = NULL;
	mOwnFilterSequencer = false;

	// Load available plugins
	slot_refreshFilters();

	// show plugins by default
	on_pluginsButton_clicked();
}

void PluginEditorForm::slot_refreshFilters()
{
	PIAF_MSG(SWLOG_INFO, "Refreshing filters ...");
	ui->availablePluginsTreeWidget->clear();

	PIAF_MSG(SWLOG_INFO, "Reload filters ...");
	mFilterSequencer.loadFilters();

	// Display the list of available filters
	QList<PiafFilter *> availList = mFilterSequencer.getAvailableFilters();
	QList<PiafFilter *>::iterator it;
	for(it = availList.begin(); it != availList.end(); ++it)
	{
		PiafFilter * filter = (*it);

		DEBUG_MSG("Adding binary '%s' : %d / %d functions", filter->exec_name,
				  filter->getIndexFunction(),
				  filter->nbfunctions());

		QFileInfo fi(filter->name());
		QStringList cols;
		QString infoStr;
		if(filter->subcategory && strlen(filter->subcategory) > 0)
		{
			cols << QString(filter->subcategory );
			infoStr = tr("Plugin '") + fi.baseName() + tr("'' ")
					+ QString::number(filter->nbfunctions()) + tr (" functions");
			infoStr = tr("Plugin '") + fi.baseName() + tr("'' ")
					+ QString::number(filter->nbfunctions()) + tr (" functions");
		}
		else if(filter->name())
		{
			cols << tr("Error with file '") + fi.baseName() + "'";
			infoStr = tr("Error with file plugin '") + fi.absoluteFilePath() + tr("' = can't be loaded.");
		}
		else
		{
			cols << tr("Invalid filter");
		}

		QTreeWidgetItem * rootItem = new QTreeWidgetItem(ui->availablePluginsTreeWidget, cols);
		rootItem->setToolTip(0, infoStr);

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
		connect(mpFilterSequencer, SIGNAL(signalProcessingDone()), this,
				SLOT(on_filterSequencer_signalProcessingDone()));
		connect(mpFilterSequencer, SIGNAL(signalFilterDied(PiafFilter*)), this,
				SLOT(on_filterSequencer_signalFilterDied(PiafFilter *)));
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

void PluginEditorForm::on_filterSequencer_signalProcessingDone()
{
	if(!mpFilterSequencer) return;

	// Update time histogram
	if(g_debug_PluginEditorForm)
	{
		fprintf(stderr, "PluginEditForm::%s:%d : update !!\n", __func__, __LINE__);
	}
	updateTimeHistogram();

}

void PluginEditorForm::on_filterSequencer_selectedFilterChanged()
{
	// Update
	if(!mpFilterSequencer) return;

	if(g_debug_PluginEditorForm)
	{
		fprintf(stderr, "PluginEditForm::%s:%d : update !!\n", __func__, __LINE__);
	}

	updateSelectedView();
}

void PluginEditorForm::on_filterSequencer_signalFilterDied(PiafFilter * filter)
{
	if(!filter) return;
	disconnect(filter, NULL);

	if(filter == mpSelectedFilter)
	{
		mpSelectedFilter = NULL;
		// Back to filter list
		if(ui->stackedWidget->currentIndex() > 0)
		{	// we were on histogram or params, so we need to come back to avaliable plugins list
			ui->stackedWidget->setCurrentIndex(0);
		}
	}

	// Display message
	QMessageBox::warning(this, tr("Plugin ") + QString(filter->exec_name) + tr(" crashed"),
						 tr("The plugin ") + QString(filter->exec_name)
						 + tr(" crashed. It will not be reloaded automatically to let you correct the bug.")
						 );
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

	QTreeWidgetItem * selectedItem = NULL;
	while(refresh) {
		refresh = false;

		int i = filterList.count()-1;

		if(!filterList.isEmpty()) {
			QList<PiafFilter *>::iterator it;

			for(it = filterList.begin();
				it != filterList.end(); ++it)
			{
				PiafFilter * curF = (*it);

				if(g_debug_PluginEditorForm)
				{
					fprintf(stderr, "PiafFilterManager:%s:%d : creating items for filter %p\n",
							__func__, __LINE__,
						   curF); fflush(stderr);
				}

				if(!curF->plugin_died)
				{
					LoadedPluginTreeWidgetItem * item = new LoadedPluginTreeWidgetItem (
							ui->selectedPluginsTreeWidget,
							curF);

					if(mpSelectedFilter == curF)
					{
						ui->selectedPluginsTreeWidget->setCurrentItem(item);
					}

				}
				else
				{
					fprintf(stderr, "PiafFilterManager:%s:%d : filter %p died, removing it\n",
							__func__, __LINE__,
						   curF);
					a_plugin_crashed = true;

					if(!mNoWarning ) {
						QString filename = QString(curF->exec_name);
						QMessageBox::critical(this,
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
		setWindowIcon(winIcon);

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
					LOADED PLUGINS LIST MANAGEMENT
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
		icon1 = QIcon(":/icons/16x16/network-connect.png");
		str1 = "T";
	} else {
		icon1 = QIcon(":/icons/16x16/network-disconnect.png");
		str1 = "F";
	}
	if(!icon1.isNull())
		setIcon(1, icon1);
	else
		setText(1, str1);

	if(pFilter->isFinal()) {
		icon2 = QIcon(":/images/16x16/layer-visible-on.png");
		str2 = "T";
	}
	else
	{
		str2 = "";
	}
	if(!icon2.isNull())
		setIcon(2, icon2);
	else
		setText(2, str2);

}

LoadedPluginTreeWidgetItem::~LoadedPluginTreeWidgetItem()
{

}

void PluginEditorForm::on_availablePluginsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
	on_appendPluginButton_clicked();
}

void PluginEditorForm::on_appendPluginButton_clicked()
{
	if(ui->availablePluginsTreeWidget->selectedItems().isEmpty())
	{
		return;
	}

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

	// Select the last one
	ui->selectedPluginsTreeWidget->setCurrentItem(
				ui->selectedPluginsTreeWidget->topLevelItem(
					ui->selectedPluginsTreeWidget->topLevelItemCount() - 1));
}


void PluginEditorForm::on_loadButton_clicked()
{
	if(!mpFilterSequencer) {
		QMessageBox::critical(this, tr("No sequencer"),
							  tr("No filter sequencer = nothing to save"));
		return;
	}

	QString path = QFileDialog::getOpenFileName(NULL,
												tr("Open sequence"),
												g_workflow_settings.defaultSequenceDir,
												tr("Sequences (*.flist)"));
	if(path.isEmpty()) return;

	mpFilterSequencer->loadSequence(path.toUtf8().data());

	// Store last sequence directory
	QFileInfo fi(path);
	g_workflow_settings.defaultSequenceDir = fi.absoluteDir().absolutePath();

}

void PluginEditorForm::on_saveButton_clicked()
{
	if(!mpFilterSequencer) {
		QMessageBox::critical(this, tr("No sequencer"), tr("No filter sequencer = nothing to save"));
		return;
	}

	QString path = QFileDialog::getSaveFileName(NULL,
												tr("Save sequence"),
												g_workflow_settings.defaultSequenceDir,
												tr("Sequences (*.flist)"));
	if(path.isEmpty()) return;

	mpFilterSequencer->saveSequence(path.toUtf8().data());

	// Store last sequence directory
	QFileInfo fi(path);
	g_workflow_settings.defaultSequenceDir = fi.absoluteDir().absolutePath();

}

void PluginEditorForm::on_removePluginButton_clicked()
{
	if(ui->selectedPluginsTreeWidget->selectedItems().isEmpty())
	{
		return;
	}

	// remove from parameters settings widget
	ui->pluginSettingsWidget->setPiafFilter( NULL );

	// Check if a plugin is selected
	mpSelectedFilter = NULL;
	LoadedPluginTreeWidgetItem * item =
			(LoadedPluginTreeWidgetItem *)ui->selectedPluginsTreeWidget->selectedItems().at(0);
	if(!item) {
		return;
	}
	if(!mpFilterSequencer) { return; }

	mpFilterSequencer->removeFilter(item->getFilter());
}

void PluginEditorForm::on_paramsButton_clicked()
{
	ui->stackedWidget->setCurrentIndex(1);
	ui->appendPluginButton->setEnabled(false);

	if(ui->selectedPluginsTreeWidget->selectedItems().isEmpty()) return;
	LoadedPluginTreeWidgetItem * item =
			(LoadedPluginTreeWidgetItem *)ui->selectedPluginsTreeWidget->selectedItems().at(0);
	if(!item) { return; }

	if(!mpFilterSequencer) { return; }
	ui->selectedPluginsTreeWidget->setCurrentItem(item);

	//	mpFilterSequencer->edit(item->getFilter());

}

void PluginEditorForm::on_activateButton_clicked()
{
	if(ui->selectedPluginsTreeWidget->selectedItems().isEmpty()) { return; }
	LoadedPluginTreeWidgetItem * item =
			(LoadedPluginTreeWidgetItem *)ui->selectedPluginsTreeWidget->selectedItems().at(0);
	if(!item) { return; }

	mpFilterSequencer->toggleFilterDisableFlag(item->getFilter());
	ui->selectedPluginsTreeWidget->setCurrentItem(item);
}

void PluginEditorForm::on_pluginsButton_clicked()
{
	ui->stackedWidget->setCurrentIndex(0);
	ui->appendPluginButton->setEnabled(true);
}

void PluginEditorForm::on_timeButton_clicked()
{
	ui->stackedWidget->setCurrentIndex(2);
	ui->appendPluginButton->setEnabled(false);
}

void PluginEditorForm::on_upButton_clicked()
{
	if(!mpFilterSequencer) return;
	if(ui->selectedPluginsTreeWidget->selectedItems().isEmpty()) { return; }

	LoadedPluginTreeWidgetItem * item =
			(LoadedPluginTreeWidgetItem *)ui->selectedPluginsTreeWidget->selectedItems().at(0);
	if(!item) { return; }

	mpFilterSequencer->moveUp(item->getFilter());
	ui->selectedPluginsTreeWidget->setCurrentItem(item);
}

void PluginEditorForm::on_downButton_clicked()
{
	if(!mpFilterSequencer) return;
	if(ui->selectedPluginsTreeWidget->selectedItems().isEmpty()) { return; }
	LoadedPluginTreeWidgetItem * item =
			(LoadedPluginTreeWidgetItem *)ui->selectedPluginsTreeWidget->selectedItems().at(0);
	if(!item) { return; }

	mpFilterSequencer->moveDown(item->getFilter());
	ui->selectedPluginsTreeWidget->setCurrentItem(item);
}

/* Kill all processes running on computer (killall for all exec names) */
void PluginEditorForm::cleanAllPlugins()
{
	QList<PiafFilter *> availList = mFilterSequencer.getAvailableFilters();
	QList<PiafFilter *>::iterator it;
	for(it = availList.begin(); it != availList.end(); ++it)
	{
		PiafFilter * filter = (*it);
		QFileInfo fi(filter->exec_name);
		char command[1024];
		sprintf(command, "killall -9 %s %s", filter->exec_name, fi.baseName().toAscii().data());
		PIAF_MSG(SWLOG_DEBUG, "run cmd '%s'", command);
		system(command);
	}
}


void PluginEditorForm::on_pluginSettingsWidget_selectedFilterChanged(PiafFilter * selectedFilter)
{
	fprintf(stderr, "PluginEditorForm::%s:%d : update !\n", __func__, __LINE__);
	if(mpFilterSequencer)
	{
		mpFilterSequencer->update();
	}
	mpSelectedFilter = selectedFilter;
	updateSelectedView();
}

void PluginEditorForm::on_selectedPluginsTreeWidget_itemClicked(QTreeWidgetItem* item,
																int column)
{
	mpSelectedFilter = NULL;
	if(!item) { return; }

	LoadedPluginTreeWidgetItem * loaded = (LoadedPluginTreeWidgetItem *)item;

	mpSelectedFilter = loaded->getFilter();
	PIAF_MSG(SWLOG_INFO, "Selected: loaded=%p name='%s'", loaded,
			 mpSelectedFilter ? mpSelectedFilter->name() : "(null)"
			 );

	ui->pluginSettingsWidget->setPiafFilter( loaded->getFilter() );

	// update time histogram
	updateTimeHistogram();
}

void PluginEditorForm::updateTimeHistogram()
{
	if(ui->selectedPluginsTreeWidget->selectedItems().isEmpty()) { return; }

	LoadedPluginTreeWidgetItem * loaded = // load first selected item
			(LoadedPluginTreeWidgetItem *)ui->selectedPluginsTreeWidget->selectedItems().at(0);
	if(!loaded) { return; }

	if(g_debug_PluginEditorForm)
	{
		fprintf(stderr, "PluginEditForm::%s:%d : update !!\n", __func__, __LINE__);
	}

	// Show its histogram
	if(loaded->getFilter()) {
		ui->timeHistoWidget->displayHisto( loaded->getFilter()->getTimeHistogram() );
	}

}

void PluginEditorForm::on_selectedPluginsTreeWidget_itemDoubleClicked(QTreeWidgetItem* item, int column)
{
	if(!item) return;
	if(!mpFilterSequencer) return;

	LoadedPluginTreeWidgetItem * loaded =
			(LoadedPluginTreeWidgetItem *)item;
	mpFilterSequencer->setFinal(loaded->getFilter());

}


