/***************************************************************************
    workshopmeasure.cpp  -  1D measure component for Piaf
                             -------------------
    begin                : ven nov 29 15:53:48 UTC 2002
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
#include "workshopmeasure.h"


void WorkshopMeasure::save()
{
	/*
	QDir dir(swcPathName);
	if((swcPathName.isNull())||(!dir.exists()))
	{
		// selectPathName();
		if((swcPathName.isNull())||(!dir.exists()))
			return;
	}
	*/
	
	swcFileName="";
	
	
	if(swcPathName.isNull())
	{
		printf("bad Path!\n");
		return;
	}
	
	//QString completeFile=swcPathName+swcFileName+".meas";
	QString completeFile=swcPathName;
	
	
 	QFile file(completeFile);
    if(file.open( QIODevice::WriteOnly ))
	{
		QDataStream stream( &file ); // we will serialize the data into the file
		stream << swcLabel << swcShortLabel << swcDate << swcValidity << swcType;
		stream << Q3ValueVector<double>::count(); 
		Q3ValueVector<double>::iterator it;

		for( it = this->begin(); it != this->end(); ++it )
			stream << *it;
	
		file.close();
		
		const char* chemin = swcPathName.ascii();
	    printf("%s -> measure saved!!\n",chemin);
	}
	
}

void WorkshopMeasure::saveAs()
{
	// QString path;

	selectPathName();
	/*
	  QDir dir(swcPathName);
	  if((swcPathName.isNull())||(!dir.exists()))
	  	 return;
	  else
	*/
	
	save();
}


void WorkshopMeasure::openFile(const QString filename)
{
	QFile file(filename);
	printf("%s.\n",filename.latin1());
    if(file.open( QIODevice::ReadOnly ))
	{
		QDataStream stream( &file ); // we will serialize the data into the file
		stream >> swcLabel >> swcShortLabel >> swcDate >> swcValidity >> swcType;
		unsigned int lsize;
		stream >> lsize;
		resize(lsize, 0.);
		Q3ValueVector<double>::iterator it;
		for( it = this->begin(); it != this->end(); ++it )
			stream >> *it;
		
		file.close();
	}
	
}
