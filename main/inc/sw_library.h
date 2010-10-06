/***************************************************************************
                          sw_library.h  -  description
                             -------------------
    begin                : Thu May 2 2002
    copyright            : (C) 2002 by Olivier Viné
    email                : olivier.vine@sisell.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SW_LIBRARY_H
#define SW_LIBRARY_H

#include <q3listview.h>
#include <qfile.h>
#include <qpixmap.h>
#include <q3textstream.h>
#include <qstringlist.h>

#include "sw_structure.h"
#include "sw_component.h"

class SW_Library : public Q3ListViewItem
{
public: 
	SW_Library(Q3ListViewItem * parent, QStringList p);
	~SW_Library();
	QString text (int column) const;
	void expand();

private:
	QString	LName;
	QString	logoFile;
	QString	DBFile;
	QString	helpFile;
};

#endif
