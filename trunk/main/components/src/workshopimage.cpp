/***************************************************************************
      workshopimage.cpp  -  image component for Piaf
                             -------------------
    begin                : ven nov 29 15:53:48 UTC 2002
    copyright            : (C) 2002 by Olivier Viné
    email                : olivier.vine@sisell.com
	copyright            : (C) 2002-2011 by Christophe Seyve
	email                : cseyve@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "workshopimage.h"


void WorkshopImage::save()
{
	/*
	QDir dir(swcPathName);
	if((swcPathName.isNull())||(!dir.exists()))
	{
		//selectPathName();

		if((swcPathName.isNull())||(!dir.exists()))
			return;
	}
	*/

	swcFileName = "";
	
	if(swcPathName.isNull())
	{
		printf("bad Path!\n");
		return;
	}
	
    // saving data in .imdata files
	QString completeFile=swcPathName+swcFileName+".imdata";
	
 	QFile file(completeFile);
    if(file.open( QIODevice::WriteOnly ))
	{
		QDataStream stream( &file ); // we will serialize the data into the file
		stream << swcLabel << swcShortLabel << swcDate << swcValidity << swcType;
		file.close();
	}
	// saving image in PNG format
	completeFile=swcPathName+swcFileName;
	
	QFileInfo fInfo(swcPathName+swcFileName);
	QString fileType = fInfo.extension(FALSE).upper();
	if(fileType.contains("jpg", FALSE)>0) 
		fileType = QString("JPEG");
	this->QImage::save(completeFile, fileType);
	
	setLabel(fInfo.fileName());
}

void WorkshopImage::saveAs()
{
	//QString path;

	selectPathName();
	
	/*
	  QDir dir(swcPathName);
	  if((swcPathName.isNull())||(!dir.exists()))
	  {
		 const char* chemin = swcPathName.ascii();
	     printf("%s.\n",chemin);
		 return;
	  }
	  else
	*/
	
	save();
	
}


void WorkshopImage::openFile(const QString filename)
{
	this->QImage::load(filename);
	QFileInfo fi( filename );
	QString completeFile = fi.dirPath()+"/"+fi.baseName() + ".imdata";
	QFile file(completeFile);
    if(file.open( QIODevice::ReadOnly ))
	{
		QDataStream stream( &file ); // we will serialize the data into the file
		stream >> swcLabel >> swcShortLabel >> swcDate >> swcValidity >> swcType;	
		file.close();
	}
	setLabel( fi.baseName(TRUE) );
	swcPathName = filename;
}
