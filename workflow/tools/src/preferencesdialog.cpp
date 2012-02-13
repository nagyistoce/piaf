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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "piaf-settings.h"
#include "piafworkflow-settings.h"
#include <QFileDialog>

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);
	setSettings( &m_workflow_settings );
}

void PreferencesDialog::setSettings(t_workflow_settings * pworkflow_settings)
{
	mp_workflow_settings = pworkflow_settings;

	ui->defaultMeasureLineEdit->setText(mp_workflow_settings->defaultMeasureDir);
	ui->defaultImageLineEdit->setText(mp_workflow_settings->defaultImageDir);
	ui->defaultMovieLineEdit->setText(mp_workflow_settings->defaultMovieDir);
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void PreferencesDialog::on_measurePushButton_clicked()
{
	if(!mp_workflow_settings)
	{
		return;
	}
	mp_workflow_settings->defaultMeasureDir = QFileDialog::getExistingDirectory(NULL,
														 tr("Select measures directory"),
														 mp_workflow_settings->defaultMeasureDir);
	ui->defaultMeasureLineEdit->setText(mp_workflow_settings->defaultMeasureDir);
}

void PreferencesDialog::on_imagePushButton_clicked()
{
	if(!mp_workflow_settings)
	{
		return;
	}
	mp_workflow_settings->defaultImageDir = QFileDialog::getExistingDirectory(NULL,
														 tr("Select images directory"),
														 mp_workflow_settings->defaultImageDir);

	ui->defaultImageLineEdit->setText(mp_workflow_settings->defaultImageDir);
}

void PreferencesDialog::on_moviePushButton_clicked()
{
	if(!mp_workflow_settings)
	{
		return;
	}

	mp_workflow_settings->defaultMovieDir = QFileDialog::getExistingDirectory(NULL,
														 tr("Select movies directory"),
														 mp_workflow_settings->defaultMovieDir);

	ui->defaultMovieLineEdit->setText(mp_workflow_settings->defaultMovieDir);
}

