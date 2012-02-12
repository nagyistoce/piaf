/***************************************************************************
	preferencedialog.h  - Piaf common settings
		-------------------
	begin                : Fri Feb 10 2012
	copyright            : (C) 2012 Christophe Seyve (CSE)
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


#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>


#include "piafworkflow-settings.h"

namespace Ui {
    class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = 0);
    ~PreferencesDialog();
	void setSettings(t_workflow_settings * pworkflow_settings);
protected:
    void changeEvent(QEvent *e);

	t_workflow_settings * mp_workflow_settings;
private slots:
	void on_measurePushButton_clicked();

	void on_imagePushButton_clicked();

	void on_moviePushButton_clicked();

private:
    Ui::PreferencesDialog *ui;
};

#endif // PREFERENCESDIALOG_H
