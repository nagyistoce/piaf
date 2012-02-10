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

#include "piafsettingsdialog.h"

#include <qvariant.h>
#include <qimage.h>
#include <qpixmap.h>

#include <stdlib.h>

/*
 *  Constructs a PiafSettingsDialog as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
PiafSettingsDialog::PiafSettingsDialog(QWidget* parent, bool modal, Qt::WindowFlags fl)
	: QDialog(parent, fl)
{
    setupUi(this);

	setModal(modal);
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
PiafSettingsDialog::~PiafSettingsDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void PiafSettingsDialog::languageChange()
{
    retranslateUi(this);
}


void PiafSettingsDialog::on_saveAtExitCheckBox_toggled(bool checked)
{

}

void PiafSettingsDialog::on_measurePushButton_clicked()
{

}
