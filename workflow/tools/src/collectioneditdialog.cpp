/***************************************************************************
 *  collectioneditdialog.h
 *
 *  Sun Oct 30 21:30:41 2011
 *  Copyright  2011  Christophe Seyve
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

#include "collectioneditdialog.h"
#include "ui_collectioneditdialog.h"

CollectionEditDialog::CollectionEditDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CollectionEditDialog)
{
    ui->setupUi(this);
	mpCollec = NULL;
}
void CollectionEditDialog::setCollection(EmaCollection * pcollec)
{
	if(!pcollec) return;
	mpCollec = pcollec;

	ui->titleLineEdit->setText(pcollec->title);
	ui->commentLineEdit->setText(pcollec->comment);
	ui->nbFilesLabel->setText( QString::number(pcollec->filesList.count()) );
	ui->nbSubCollectionsLabel->setText( QString::number(pcollec->subCollectionsList.count()) );
}

CollectionEditDialog::~CollectionEditDialog()
{
    delete ui;
}

void CollectionEditDialog::changeEvent(QEvent *e)
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

void CollectionEditDialog::on_buttonBox_accepted()
{
	if(mpCollec)
	{
		mpCollec->title = ui->titleLineEdit->text();
		mpCollec->comment = ui->commentLineEdit->text();
		emit signalCollectionChanged(mpCollec);

		return;
	}
	EmaCollection collec;
	collec.title = ui->titleLineEdit->text();
	collec.comment = ui->commentLineEdit->text();

	emit signalNewCollection(collec);
}
