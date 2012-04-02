/***************************************************************************
	   sequence_select_dialog.cpp  -  plugin sequence selection dialog
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

#include "sequence_select_dialog.h"
#include "ui_sequence_select_dialog.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>

SequenceSelectDialog::SequenceSelectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SequenceSelectDialog)
{
    ui->setupUi(this);

	// Load plugins
	QString lastPluginDir = QString("/usr/local/piaf");
	char home[512]="/usr/local/piaf";

	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
		lastPluginDir = QString(home);
	}

	QDir dir("/usr/local/piaf/plugins/");
	QStringList filters; filters << "*.flist";
	QFileInfoList filtersList = dir.entryInfoList(filters, QDir::Files);
	QFileInfoList::iterator it;
	for(it = filtersList.begin(); it != filtersList.end(); ++it)
	{
		QFileInfo fi = (*it);
		QFile file(fi.absoluteFilePath());

		if(file.exists()) {
			new SequenceTreeWidgetItem(ui->sequenceTreeWidget,
									   fi.absoluteFilePath());
		} else {
			fprintf(stderr, "Cannot read file '%s' \n", home);
		}
	}
}

SequenceTreeWidgetItem::SequenceTreeWidgetItem(QTreeWidget * pparent, QString path)
	: QTreeWidgetItem(pparent)
{
	QFileInfo fi(path);
	QFile file(path);
	if(file.open(QIODevice::ReadOnly))
	{
		char str[1024]="";
/* format is

/home/tof/Vision/piaf-googlecode/piaf/plugins/vision/opencv_contour     1
SWSTART FUNCDESC=       0       Canny   3       low tresh       "       1500    high tresh      "       2000    kernel size     ^R      5
SWEND
/home/tof/Vision/piaf-googlecode/piaf/plugins/vision/opencv_histogram   3
SWSTART FUNCDESC=       0       View hist       1       Height scale    !       0.4     SWEND
/home/tof/Vision/piaf-googlecode/piaf/plugins/vision/opencv_histogram   0
SWSTART FUNCDESC=       0       View hist       1       Height scale    !       0.4     SWEND

*/
		QString funcDesc;

		int lineidx = 0;
		while(file.readLine(str, 512) >= 0) {

			// split
			QString lineStr = QString(str);
			QStringList lst = lineStr.split( "\t" );

			if(lineidx%2 == 1)
			{
				/* Read
SWSTART FUNCDESC=       0       View hist       1       Height scale    !       0.4     SWEND
				  */
				if(lst.count()>3)
				{
					funcDesc = lst.at(3) + " (";
					mDescription += lst.at(3) + " (";
					for(int arg = 5; arg<lst.count()-2; arg+=3)
					{
						if(arg >= 8) {
							funcDesc += ", ";
							mDescription += ", ";
						}
						mDescription += lst.at(arg) + "=" + lst.at(arg+2);
						funcDesc += lst.at(arg) + "=" + lst.at(arg+2);
						// if there are still arguments, add a comma

					}
					funcDesc += ")";
					mDescription += ")\n";
				}


			}
			lineidx++;
		}
		file.close();

		mName = fi.baseName();
		setText(0, mName);
		setText(1, funcDesc);
		setToolTip(1, mDescription);
	}
	else
	{
		setText(0, fi.baseName());
		setText(1, QObject::tr("Error: file ") + path + QObject::tr(" not opened"));
	}
}

SequenceSelectDialog::~SequenceSelectDialog()
{
    delete ui;
}

void SequenceSelectDialog::changeEvent(QEvent *e)
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

void SequenceSelectDialog::on_addButton_clicked()
{
	// Open a dialog to select a .flist file
	QString path = QFileDialog::getOpenFileName(NULL,
												tr("Open sequence"),
												tr("Sequences (*.flist)"));
	if(path.isEmpty()) return;

	// Create its item

}

void SequenceSelectDialog::on_delButton_clicked()
{
	// Remove selected sequences

}

void SequenceSelectDialog::on_buttonBox_accepted()
{

}

void SequenceSelectDialog::on_sequenceTreeWidget_itemActivated(QTreeWidgetItem *item, int column)
{
	if(!item)
	{
		ui->delButton->setEnabled(false);
		ui->descriptionLabel->setText("");
		return;
	}

	SequenceTreeWidgetItem * seqItem = (SequenceTreeWidgetItem *)item;

	ui->delButton->setEnabled(true);
	ui->descriptionLabel->setText(seqItem->getDescription());

}

void SequenceSelectDialog::on_sequenceTreeWidget_itemSelectionChanged()
{
	if(ui->sequenceTreeWidget->selectedItems().isEmpty()) { return; }

	on_sequenceTreeWidget_itemActivated(ui->sequenceTreeWidget->selectedItems().at(0), 0);
}
