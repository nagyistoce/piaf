/***************************************************************************
                          sw_library.cpp  -  description
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

#include "sw_library.h"
//Added by qt3to4:
#include <Q3TextStream>
#include <QPixmap>

SW_Library::SW_Library(Q3ListViewItem * parent, QStringList param):Q3ListViewItem(parent){
	QPixmap tmpPixmap;

	LName =  param[LIBNAME_INDEX];
	this->setText(0, LName);
	DBFile = LIBDB_PATH + param[LIBDB_INDEX];
	helpFile = LIBHELP_PATH + param[LIBHELP_INDEX];
	logoFile = LIBLOGOS_PATH + param[LIBLOGO_INDEX];
	if(tmpPixmap.load(logoFile))
		this->setPixmap(0, tmpPixmap);
}

SW_Library::~SW_Library(){
}

QString SW_Library::text (int column) const
{
	if(column==0)
		return LName;
	else
		return "SW_Library";
}

void SW_Library::expand(){
	QFile f;
	QStringList		tmpList;
	QPixmap			tmpPixmap;
	SW_Component *child;
	unsigned int len;	
	// using DBFile as Current data file
	f.setName(DBFile);
	if( !f.open(QIODevice::ReadOnly) )
		return;

	Q3TextStream t(&f);

	QString s, key, pKey;

	while( !t.atEnd()) {
		// reading line by line
		s = t.readLine();
		// split string
		tmpList = QStringList::split("\t", s);
		// finding parent
		key = tmpList[COMPKEY_INDEX];
		len = key.length();
		if(len==1) {
			//SW_Component * tmpComp = 
				new SW_Component(this, tmpList);
		} else {		
			// finding parent key
			pKey = key;
	  		while(pKey.right(1)!="_"){
  				pKey.remove(len-1, 1);
  				len--;
  			}
  			// remove "_"
  			pKey.remove(len-1, 1);
  				
  			Q3ListViewItemIterator it(this);
  			for( ; it.current(); ++it) {
  				child = (SW_Component *)it.current();
  				if (child->getComponentKey()==pKey){
  					//SW_Component * tmpComp = 
						new SW_Component(child, tmpList);
					break;
  				}
  			}
		} //else
	}
	f.close();
}
