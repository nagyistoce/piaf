/***************************************************************************
    workshopmovie.cpp  -  video file (movie) component for Piaf
                             -------------------
    begin                : fri nov 29 15:53:48 UTC 2002
    copyright            : (C) 2002 by Christophe SEYVE
    email                : christophe.seyve@sisell.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "workshopmovie.h"
#include <stdlib.h>

void WorkshopMovie::save()
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


	swcFileName="";

	if(swcPathName.isNull())
	{
		printf("bad Path!\n");
		return;
	}

    // saving data in .avi files
	QString completeFile=swcPathName+swcFileName;


	QDir dir;
	
	dir.rename(FilePath, completeFile.latin1());
	
	QFileInfo fi(completeFile);
	setLabel(fi.fileName());
}

void WorkshopMovie::saveAs()
{
	selectPathName();
	save();
}


void WorkshopMovie::openFile(const QString filename)
{
	printf("Movie file : %s\n", filename.latin1());
//	this->QImage::load(filename);
	QFileInfo fi( filename );
	QString completeFile = fi.dirPath()+"/"+fi.baseName() + ".imdata";
	printf("Movie data file : %s\n", completeFile.latin1());
	QFile file(completeFile);
    if(file.open( QIODevice::ReadOnly ))
	{
		QDataStream stream( &file ); // we will serialize the data into the file
		stream >> swcLabel >> swcShortLabel >> swcDate >> swcValidity >> swcType;	
		file.close();
	}
	setLabel(fi.baseName(TRUE));
}
