/***************************************************************************
                          sw_component.cpp  -  description
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

#include "sw_component.h"
//Added by qt3to4:
#include <QPixmap>

SW_Component::SW_Component(Q3ListViewItem * parent, QStringList param):Q3ListViewItem(parent){
	QPixmap tmpPixmap;

	CName = param[COMPNAME_INDEX];
	this->setText(0, CName);
	CKey = param[COMPKEY_INDEX];
	CFile = param[COMPFILE_INDEX];
	CAction = param[COMPACTION_INDEX];
	CType = param[COMPTYPE_INDEX];
	logoFile = LOGOSCOM_PATH + param[COMPLOGO_INDEX];
	if(tmpPixmap.load(logoFile))
		this->setPixmap(0, tmpPixmap);
}

SW_Component::~SW_Component(){
}

QString SW_Component::getComponentKey(){
	return CKey;
}

QString SW_Component::getComponentAction(){
	return CAction;
}

QString SW_Component::getComponentType(){
	return CType;
}

QString SW_Component::getComponentFile(){
	return CFile;
}

QString SW_Component::text (int column) const
{
	if(column==0)
		return CName;
	else
		return "SW_Component";
}
