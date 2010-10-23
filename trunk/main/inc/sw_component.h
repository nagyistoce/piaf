/***************************************************************************
                          sw_component.h  -  description
                             -------------------
    begin                : Fri May 3 2002
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

#ifndef SW_COMPONENT_H
#define SW_COMPONENT_H

#include <q3listview.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qpixmap.h>

#include "sw_structure.h"

/** Base structure for generic component
  \author Olivier Viné
  */

class SW_Component : public Q3ListViewItem  {
public: 
	SW_Component(Q3ListViewItem * parent, QStringList p);
	~SW_Component();
	QString getComponentKey();
	QString getComponentAction();
	QString getComponentType();
	QString getComponentFile();
	QString text (int column) const;

private:
	QString	CKey;
	QString 	CName;
	QString	CFile;
	QString	CAction;
	QString	CType;
	QString	logoFile;
};

#endif
