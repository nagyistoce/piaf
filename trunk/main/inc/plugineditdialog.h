/***************************************************************************
	   plugineditdialog.h  -  plugin registration dialog for Piaf
							 -------------------
	begin                : Fri Aug 12 2011
	copyright            : (C) 2002-2011 by Christophe Seyve
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

#ifndef PLUGINEDITDIALOG_H
#define PLUGINEDITDIALOG_H

#include <QDialog>

namespace Ui {
    class PluginEditDialog;
}
/** @brief Plugin registration dialog for Piaf

  All the plugins listed here will be accessible in plugin manager
*/
class PluginEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PluginEditDialog(QWidget *parent = 0);
    ~PluginEditDialog();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::PluginEditDialog *ui;
	QString lastPluginDir;
private slots:
	void on_saveButton_clicked();
	void on_delButton_clicked();
	void on_iconButton_clicked();
	void on_addButton_clicked();
};

#endif // PLUGINEDITDIALOG_H
