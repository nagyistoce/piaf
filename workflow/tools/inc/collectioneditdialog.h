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

#ifndef COLLECTIONEDITDIALOG_H
#define COLLECTIONEDITDIALOG_H

#include "workflowtypes.h"

#include <QDialog>

namespace Ui {
    class CollectionEditDialog;
}

class CollectionEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CollectionEditDialog(QWidget *parent = 0);
    ~CollectionEditDialog();
	void setCollection(EmaCollection *);
protected:
    void changeEvent(QEvent *e);

private slots:
	void on_buttonBox_accepted();

private:
	EmaCollection * mpCollec;
    Ui::CollectionEditDialog *ui;

signals:
	void signalNewCollection(EmaCollection);
	void signalCollectionChanged(EmaCollection *);
};

#endif // COLLECTIONEDITDIALOG_H
