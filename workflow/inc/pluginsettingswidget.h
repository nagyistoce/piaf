/***************************************************************************
	pluginsettingswidget.h - plugin parameters control widget
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

#ifndef PLUGINSETTINGSWIDGET_H
#define PLUGINSETTINGSWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>

namespace Ui {
    class PluginSettingsWidget;
}

class PiafFilter;

class PluginSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PluginSettingsWidget(QWidget *parent = 0);
    ~PluginSettingsWidget();

	/** @brief Set pointer to PiafFilter */
	int setPiafFilter(PiafFilter * );
protected:
    void changeEvent(QEvent *e);

private:
    Ui::PluginSettingsWidget *ui;
	PiafFilter * mpPiafFilter;

	int mNbParams;		///< numbr of parameters -> number of
	QLabel ** mLabels;
	QLineEdit ** mParamsEditLines ;
	QComboBox ** mComboEdit ;

	void clearDisplay();
	void updateDisplay();

private slots:
	void on_revertButton_clicked();
	void on_applyButton_clicked();

signals:
	/// Signal that current plugin params has changed
	void selectedFilterChanged(PiafFilter *);
};

#endif // PLUGINSETTINGSWIDGET_H
