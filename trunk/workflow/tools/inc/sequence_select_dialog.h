/***************************************************************************
	   sequence_select_dialog.h  -  plugin sequence selection dialog
							 -------------------
	begin                : Thu Mar 29 2012
	copyright            : (C) 2002-2012 by Christophe Seyve
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

#ifndef SEQUENCE_SELECT_DIALOG_H
#define SEQUENCE_SELECT_DIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>

namespace Ui {
    class SequenceSelectDialog;
}

/** \brief Sequence file tree item */
class SequenceTreeWidgetItem : public QTreeWidgetItem
{
//	Q_OBJECT
	friend class EmaMainWindow;
public:
	SequenceTreeWidgetItem(QTreeWidgetItem * treeWidgetItemParent, QString path);
	SequenceTreeWidgetItem(QTreeWidget * treeWidgetParent, QString path);
	~SequenceTreeWidgetItem() {};

	/** \brief Expand contents: read directory contents and display it */
	void expand();
	/** \brief collapse contents: collapse directory and clear data */
	void collapse();

	QString getName() { return mName; }
	QString getFullPath() { return mFullPath; }

	QString getDescription() { return mDescription; }

protected:

	void init(); ///< init internal data and fill nb files
	QString mName;
	QString mFullPath;

	QString mDescription;
private:

};


/** @brief Select a plugin in plugins list, and let user add new sequences from .flist files


*/
class SequenceSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SequenceSelectDialog(QWidget *parent = 0);
    ~SequenceSelectDialog();

protected:
    void changeEvent(QEvent *e);

private slots:
	void on_addButton_clicked();

	void on_delButton_clicked();

	void on_buttonBox_accepted();

	void on_sequenceTreeWidget_itemActivated(QTreeWidgetItem *item, int column);

	void on_sequenceTreeWidget_itemSelectionChanged();

private:
    Ui::SequenceSelectDialog *ui;
};

#endif // SEQUENCE_SELECT_DIALOG_H
