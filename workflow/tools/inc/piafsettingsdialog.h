/***************************************************************************
 *  piafsettingsdialog.h - Global settings for piaf : image and movie paths, ...
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

#ifndef PiafSettingsDialog_H
#define PiafSettingsDialog_H

#include <qvariant.h>

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PiafSettingsDialog
{
public:
    QLabel *textLabel1;
    QLabel *textLabel1_2;
    QPushButton *imagePushButton;
    QLabel *textLabel1_3;
    QWidget *Layout1;
    QHBoxLayout *hboxLayout;
    QSpacerItem *Horizontal_Spacing2;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;
    QPushButton *moviePushButton;
    QPushButton *measurePushButton;
    QLineEdit *defaultMovieLineEdit;
    QLineEdit *defaultImageLineEdit;
    QCheckBox *saveAtExitCheckBox;
    QLineEdit *defaultMeasureLineEdit;

	void setupUi(QDialog *PiafSettingsDialog)
    {
		if (PiafSettingsDialog->objectName().isEmpty())
			PiafSettingsDialog->setObjectName(QString::fromUtf8("PiafSettingsDialog"));
		PiafSettingsDialog->resize(571, 264);
		PiafSettingsDialog->setSizeGripEnabled(false);
		textLabel1 = new QLabel(PiafSettingsDialog);
        textLabel1->setObjectName(QString::fromUtf8("textLabel1"));
        textLabel1->setGeometry(QRect(10, 1, 211, 20));
        textLabel1->setWordWrap(false);
		textLabel1_2 = new QLabel(PiafSettingsDialog);
        textLabel1_2->setObjectName(QString::fromUtf8("textLabel1_2"));
        textLabel1_2->setGeometry(QRect(10, 70, 211, 20));
        textLabel1_2->setWordWrap(false);
		imagePushButton = new QPushButton(PiafSettingsDialog);
        imagePushButton->setObjectName(QString::fromUtf8("imagePushButton"));
        imagePushButton->setGeometry(QRect(510, 90, 50, 31));
		textLabel1_3 = new QLabel(PiafSettingsDialog);
        textLabel1_3->setObjectName(QString::fromUtf8("textLabel1_3"));
        textLabel1_3->setGeometry(QRect(10, 130, 211, 20));
        textLabel1_3->setWordWrap(false);
		Layout1 = new QWidget(PiafSettingsDialog);
        Layout1->setObjectName(QString::fromUtf8("Layout1"));
        Layout1->setGeometry(QRect(10, 220, 550, 33));
        hboxLayout = new QHBoxLayout(Layout1);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        Horizontal_Spacing2 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(Horizontal_Spacing2);

        buttonOk = new QPushButton(Layout1);
        buttonOk->setObjectName(QString::fromUtf8("buttonOk"));
        buttonOk->setAutoDefault(true);
        buttonOk->setDefault(true);

        hboxLayout->addWidget(buttonOk);

        buttonCancel = new QPushButton(Layout1);
        buttonCancel->setObjectName(QString::fromUtf8("buttonCancel"));
        buttonCancel->setAutoDefault(true);

        hboxLayout->addWidget(buttonCancel);

		moviePushButton = new QPushButton(PiafSettingsDialog);
        moviePushButton->setObjectName(QString::fromUtf8("moviePushButton"));
        moviePushButton->setGeometry(QRect(510, 150, 50, 31));
		measurePushButton = new QPushButton(PiafSettingsDialog);
        measurePushButton->setObjectName(QString::fromUtf8("measurePushButton"));
        measurePushButton->setGeometry(QRect(510, 30, 50, 31));
		defaultMovieLineEdit = new QLineEdit(PiafSettingsDialog);
        defaultMovieLineEdit->setObjectName(QString::fromUtf8("defaultMovieLineEdit"));
        defaultMovieLineEdit->setGeometry(QRect(10, 150, 490, 31));
		defaultImageLineEdit = new QLineEdit(PiafSettingsDialog);
        defaultImageLineEdit->setObjectName(QString::fromUtf8("defaultImageLineEdit"));
        defaultImageLineEdit->setGeometry(QRect(10, 90, 490, 31));
		saveAtExitCheckBox = new QCheckBox(PiafSettingsDialog);
        saveAtExitCheckBox->setObjectName(QString::fromUtf8("saveAtExitCheckBox"));
        saveAtExitCheckBox->setGeometry(QRect(10, 190, 280, 30));
        saveAtExitCheckBox->setChecked(true);
		defaultMeasureLineEdit = new QLineEdit(PiafSettingsDialog);
        defaultMeasureLineEdit->setObjectName(QString::fromUtf8("defaultMeasureLineEdit"));
        defaultMeasureLineEdit->setGeometry(QRect(10, 30, 490, 31));

		retranslateUi(PiafSettingsDialog);
		QObject::connect(buttonOk, SIGNAL(clicked()), PiafSettingsDialog, SLOT(accept()));
		QObject::connect(defaultImageLineEdit, SIGNAL(textChanged(QString)), PiafSettingsDialog, SLOT(slotImageChanged(QString)));
		QObject::connect(defaultMeasureLineEdit, SIGNAL(textChanged(QString)), PiafSettingsDialog, SLOT(slotMeasureChanged(QString)));
		QObject::connect(defaultMovieLineEdit, SIGNAL(textChanged(QString)), PiafSettingsDialog, SLOT(slotMovieChanged(QString)));
		QObject::connect(imagePushButton, SIGNAL(clicked()), PiafSettingsDialog, SLOT(slotImagePushed()));
		QObject::connect(measurePushButton, SIGNAL(clicked()), PiafSettingsDialog, SLOT(slotMeasurePushed()));
		QObject::connect(moviePushButton, SIGNAL(clicked()), PiafSettingsDialog, SLOT(slotMoviePushed()));
		QObject::connect(saveAtExitCheckBox, SIGNAL(toggled(bool)), PiafSettingsDialog, SLOT(slotSaveAtExit(bool)));
		QObject::connect(buttonCancel, SIGNAL(clicked()), PiafSettingsDialog, SLOT(reject()));

		QMetaObject::connectSlotsByName(PiafSettingsDialog);
    } // setupUi

	void retranslateUi(QDialog *PiafSettingsDialog)
    {
		PiafSettingsDialog->setWindowTitle(QApplication::translate("PiafSettingsDialog", "Piaf Configuration Dialog", 0, QApplication::UnicodeUTF8));
		textLabel1->setText(QApplication::translate("PiafSettingsDialog", "Default measure directory :", 0, QApplication::UnicodeUTF8));
		textLabel1_2->setText(QApplication::translate("PiafSettingsDialog", "Default image directory :", 0, QApplication::UnicodeUTF8));
		imagePushButton->setText(QApplication::translate("PiafSettingsDialog", "...", 0, QApplication::UnicodeUTF8));
		textLabel1_3->setText(QApplication::translate("PiafSettingsDialog", "Default movie directory :", 0, QApplication::UnicodeUTF8));
		buttonOk->setText(QApplication::translate("PiafSettingsDialog", "&OK", 0, QApplication::UnicodeUTF8));
        buttonOk->setShortcut(QString());
		buttonCancel->setText(QApplication::translate("PiafSettingsDialog", "&Cancel", 0, QApplication::UnicodeUTF8));
        buttonCancel->setShortcut(QString());
		moviePushButton->setText(QApplication::translate("PiafSettingsDialog", "...", 0, QApplication::UnicodeUTF8));
		measurePushButton->setText(QApplication::translate("PiafSettingsDialog", "...", 0, QApplication::UnicodeUTF8));
		saveAtExitCheckBox->setText(QApplication::translate("PiafSettingsDialog", "Save configuration at exit", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
	class PiafSettingsDialog: public Ui_PiafSettingsDialog {};
} // namespace Ui

QT_END_NAMESPACE

class PiafSettingsDialog : public QDialog, public Ui::PiafSettingsDialog
{
    Q_OBJECT

public:
	PiafSettingsDialog(QWidget* parent = 0, bool modal = false, Qt::WindowFlags fl = 0);
	~PiafSettingsDialog();

protected slots:
    virtual void languageChange();

    virtual void accept();


private:
    QString measureDir;
    QString imageDir;
    QString movieDir;
    bool saveAtExit;
    QString homeDir;

    virtual void init();

private slots:
    virtual void slotImageChanged( const QString & str );
    virtual void slotMeasureChanged( const QString & str );
    virtual void slotMovieChanged( const QString & str );
    virtual void slotImagePushed();
    virtual void slotMoviePushed();
    virtual void slotMeasurePushed();
    virtual void slotSaveAtExit( bool on );

	void on_saveAtExitCheckBox_toggled(bool checked);
	void on_measurePushButton_clicked();
};

#endif // PiafSettingsDialog_H
