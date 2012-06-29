/***************************************************************************
	pluginsettingswidget.cpp  -  plugin parameters control widget
							 -------------------
	begin                : Sat Oct 1 2011
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

#include "inc/pluginsettingswidget.h"
#include "ui_pluginsettingswidget.h"

#include "piaf-common.h"
#include "PiafFilter.h"
#include <QLineEdit>
#include <QComboBox>
#include <QMessageBox>

PluginSettingsWidget::PluginSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PluginSettingsWidget)
{
    ui->setupUi(this);
	mpPiafFilter = NULL;

	mNbParams = 0;

	mLabels = NULL;
	mParamsEditLines = NULL;
	mComboEdit = NULL;
}

PluginSettingsWidget::~PluginSettingsWidget()
{
    delete ui;
}
/* Set pointer to PiafFilter */
int PluginSettingsWidget::setPiafFilter(PiafFilter * filter)
{
	if(mpPiafFilter != filter) {
		clearDisplay();
	}
	mpPiafFilter = filter;

	// update display
	updateDisplay();

	return 0;
}

void PluginSettingsWidget::changeEvent(QEvent *e)
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

void PluginSettingsWidget::clearDisplay()
{
	PIAF_MSG(SWLOG_DEBUG, "clear display : fix clearing");

	// for each parameter :
	if(mLabels) {
		for(int row = 0; row < mNbParams; row++)
		{
			if(mLabels[row])
			{
				delete mLabels[row];
			}
		}
		delete [] mLabels;
		mLabels = NULL;
	}
	if(mParamsEditLines)
	{
		for(int row = 0; row < mNbParams; row++)
		{
			if(mParamsEditLines[row])
			{
				delete mParamsEditLines[row];
			}
		}
		delete [] mParamsEditLines;
		mParamsEditLines = NULL;
	}
	if(mComboEdit) {
		for(int row = 0; row < mNbParams; row++)
		{
			if(mComboEdit[row])
			{
				delete mComboEdit[row];
			}
		}
		delete [] mComboEdit;
		mComboEdit = NULL;
	}
}

void PluginSettingsWidget::updateDisplay()
{
	ui->scrollArea->setEnabled((mpPiafFilter != NULL));
	ui->applyButton->setEnabled((mpPiafFilter != NULL));
	ui->revertButton->setEnabled((mpPiafFilter != NULL));

	if(!mpPiafFilter) {
		return;
	}


	swFunctionDescriptor * func = mpPiafFilter->updateFunctionDescriptor();

	if(func)
	{
		// ---------- creates window for editing parameters ------------
		QString plugName = tr("Name: ") + QString(func->name);
		ui->pluginNameLabel->setText(plugName);

		ui->pluginPIDLabel->setText(
				tr("PID: ")+ QString::number(mpPiafFilter->getChildPid()));

		QString nbParamsStr;
		nbParamsStr = QString::number(func->nb_params) + tr(" params");
		ui->nbParamsLabel->setText(nbParamsStr);
		QGridLayout * fgrid = ui->gridLayout;

		clearDisplay();


		if(func->nb_params > 0)
		{
			mNbParams = func->nb_params;

			mLabels = new QLabel * [ func->nb_params ]; // for lists
			memset(mLabels, 0, sizeof(QLabel *) * func->nb_params);

			mParamsEditLines = new QLineEdit * [ func->nb_params ]; // for scalars
			memset(mParamsEditLines, 0, sizeof(QLineEdit *) * func->nb_params);

			mComboEdit = new QComboBox * [ func->nb_params ]; // for lists
			memset(mComboEdit, 0, sizeof(QComboBox *) * func->nb_params);

			int txtcount = 0;
			int combocount = 0;

			for(int i=0; i<func->nb_params; i++)
			{
				// Add param name then param value
				mLabels[i] = new QLabel(QString(func->param_list[i].name),
										   this);
				fgrid->addWidget(mLabels[i], i, 0);

				char txt[1024]="";
				swGetStringValueFromType(func->param_list[i].type,
										 func->param_list[i].value, txt);

				switch(func->param_list[i].type) {
				case swStringList: {
					mComboEdit[combocount] = new QComboBox(this);
					fgrid->addWidget(mComboEdit[combocount], i, 1);

					swStringListStruct * s = (swStringListStruct *)func->param_list[i].value;
					mComboEdit[combocount]->setFixedHeight(28);
					for(int item=0;item<s->nbitems;item++) {
						fprintf(stderr, "[PluginSettingsW] %s:%d insert item [%d/%d]='%s'\n",
								__func__, __LINE__, item, s->nbitems, s->list[item]);
						mComboEdit[combocount]->insertItem(item, s->list[item]);
					}
					combocount++;
					}
					break;
				default:
					mParamsEditLines[txtcount] = new QLineEdit( QString(txt), this);
					fgrid->addWidget(mParamsEditLines[txtcount], i, 1);
					mParamsEditLines[txtcount]->setFixedHeight(28);
					mParamsEditLines[txtcount]->setReadOnly(false);
					txtcount++;
					break;
				}
			}

		}
	}

}

void PluginSettingsWidget::on_applyButton_clicked()
{
	if(!mpPiafFilter) { return; }
	swFunctionDescriptor * func = mpPiafFilter->updateFunctionDescriptor();
	if(!func) { return; }

	// check if values are coherent with types
	int i;
	int txtcount=0;
	int combocount = 0;
	char par[2048]="";// big buffer because may be an accumulation of many strings
	for(i=0; i < func->nb_params;i++)
	{
		switch(func->param_list[i].type) {
		case swStringList: {
			// no problem with coherence for combo
			// read value from combo
			swStringListStruct *s = (swStringListStruct *)func->param_list[i].value;
			s->curitem = mComboEdit[combocount]->currentIndex();
			fprintf(stderr, "\t\t%s:%d : param[%d] = combo idx=%d stringList=%p\n",
					__func__, __LINE__,
					i, s->curitem, s);
			combocount++;
			}
			break;
		default:
			if(!swGetValueFromTypeAndString(func->param_list[i].type,
					(char *)mParamsEditLines[txtcount]->text().toAscii().data(),
					func->param_list[i].value))
			{
				fprintf(stderr, "PluginSettingsEdit::%s:%d: Error: incompatible type and value.\n",
						__func__, __LINE__);
			}

			swGetStringValueFromType(func->param_list[i].type,
									 func->param_list[i].value, par);
			mParamsEditLines[txtcount]->setText(QString(par));

			fprintf(stderr, "\t\t%s:%d : param[%d] = %p => '%s'\n",
					__func__, __LINE__,
					i, func->param_list[i].value, par);
			txtcount++;
			break;
		}
	}

	PIAF_MSG(SWLOG_INFO, "Send params to plugin");

	// Send params to remote plugin
	int ret = mpPiafFilter->sendParams();
	if(ret < 0)
	{
		QMessageBox::critical(NULL, tr("Plugin error"),
							  tr("The new parameters could not be sent to plugin."));
	}

	emit selectedFilterChanged(mpPiafFilter);
}

void PluginSettingsWidget::on_revertButton_clicked()
{
	// Refresh function descriptor from the plugin
	swFunctionDescriptor * func = mpPiafFilter->updateFunctionDescriptor();

	int ret = mpPiafFilter->sendParams();
	if(ret < 0)
	{
		QMessageBox::critical(NULL, tr("Plugin error"),
							  tr("The new parameters could not be sent to plugin."));

	}
	updateDisplay();
}
