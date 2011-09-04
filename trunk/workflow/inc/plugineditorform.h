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

#ifndef PLUGINEDITORFORM_H
#define PLUGINEDITORFORM_H

#include <QWidget>
#include "PiafFilter.h"

#include <QTreeWidgetItem>
#include <QWorkspace>

namespace Ui {
    class PluginEditorForm;
}

/** \brief Plugin item in available tree widget

  */
class AvailablePluginTreeWidgetItem : public QTreeWidgetItem
{

public:
	AvailablePluginTreeWidgetItem(QTreeWidgetItem * parent, PiafFilter * pFilter, int idx_function);
	~AvailablePluginTreeWidgetItem();

	/// Get filter
	PiafFilter * getFilter() { return mpFilter; }
	/// Get path of plugin
	QString getPluginPath() { return mPath; }

	/// Get index of function
	int getIndexFunction() { return mIndexFunction; }

private:
	bool mRoot;				///< root item = only the name of filter
	QString mName;			///< Name of plugin or function
	QString mPath;			///< Path of binary file
	PiafFilter * mpFilter;	///< pointer on plugin properties (not loaded)
	int mIndexFunction;		///< index of displayed function
};

/** \brief Plugin item in loaded plugins tree widget

  */
class LoadedPluginTreeWidgetItem : public QTreeWidgetItem
{
public:
	LoadedPluginTreeWidgetItem(QTreeWidget * treeWidget, PiafFilter * pFilter);
	~LoadedPluginTreeWidgetItem();
	PiafFilter * getFilter() { return mpFilter; }
private:
	bool mRoot; ///< root item = only the name of filter
	QString mName;	///< Name of plugin or function
	QString mPath;	///< Path of binary file
	PiafFilter * mpFilter;	///< pointer on loaded filter
	int mIndexFunction;
};


/** \brief Display a list of plugins and manage the sequence order

  */
class PluginEditorForm : public QWidget
{
    Q_OBJECT

public:
    explicit PluginEditorForm(QWidget *parent = 0);
    ~PluginEditorForm();

	/** @brief Set workspace in MDI mode */
	void setWorkspace(QWorkspace * wsp);

	/** @brief Change sequencer */
	void setFilterSequencer(FilterSequencer *);
	/** @brief Create its own sequencer */
	FilterSequencer * createFilterSequencer();

	/** @brief Kill all processes running on computer (killall for all exec names) */
	void cleanAllPlugins();
protected:
    void changeEvent(QEvent *e);

private:
    Ui::PluginEditorForm *ui;

	void init();
	void purge();

	/** @brief Refresh list of loaded plugins */
	void updateSelectedView();

	bool mHasGUI; ///< Show GUI

	QWorkspace * pWorkspace;

	/// Current active sequencer for processing filters
	FilterSequencer * mpFilterSequencer;
	bool mOwnFilterSequencer;
	/// Sequencer for listing available filters
	FilterSequencer mFilterSequencer;

	/// List of loaded items
	QList<LoadedPluginTreeWidgetItem *> mLoadedPluginTreeWidgetItemList;
	QTreeWidgetItem * originalItem; ///< fake first item

	bool mNoWarning; ///< do not display warnings when plugins crash
	bool mAutoReloadSequence;///< automatically reload sequence after a crash

private slots:
	/// Refresh function
	void on_pluginsButton_clicked();
 void on_selectedPluginsTreeWidget_itemDoubleClicked(QTreeWidgetItem* item, int column);
	void on_selectedPluginsTreeWidget_itemClicked(QTreeWidgetItem* item, int column);
	void on_downButton_clicked();
	void on_upButton_clicked();
	void on_timeButton_clicked();
	void on_activateButton_clicked();
	void on_paramsButton_clicked();
	void on_removePluginButton_clicked();
	void on_saveButton_clicked();
	void on_filterSequencer_selectedFilterChanged();

	void on_appendPluginButton_clicked();
	void on_loadButton_clicked();

};

#endif // PLUGINEDITORFORM_H
