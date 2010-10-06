/***************************************************************************
      workshopimage.cpp  -  image component for Piaf
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
#include "workshopimage.h"
#include <qcolor.h>
//Added by qt3to4:
#include <QPixmap>
#include <QPaintEvent>

unsigned char RthermicBlack2Red(int p) {
	int dp = p-170; 
	if(dp<0) dp = -dp;
	unsigned char R = (p<170 ? 0 : (unsigned char)(dp*3));
	return R;
}
unsigned char GthermicBlack2Red(int p) {
	int dp = p-170; 
	if(dp<0) dp = -dp;
	unsigned char G = (p<85 ? 0 : (unsigned char)(255 - dp*3));
	return G;
}

unsigned char BthermicBlack2Red(int p) {
	int dp = p-85; 
	if(dp<0) dp = -dp;
	unsigned char B = (p>170 ? 0 : (unsigned char)(255-dp*3));
	return B;
}

unsigned char RthermicBlue2Red(int p) {
	if(p<128) return 0;
		
	int dp = p-128;
	if(dp<0) dp = -dp;
	unsigned char R = (p<128 ? 0 : (unsigned char)(dp*2));
	return R;
}

unsigned char GthermicBlue2Red(int p) {
	int dp = p-127; 
	if(dp<0) dp = -dp;
	unsigned char G = (unsigned char)(255 - dp*2);
	return G;
}

unsigned char BthermicBlue2Red(int p) {
	if(p>128) return 0;
	
	unsigned char B = (unsigned char)(255-p*2);
	return B;
}

void ImageWidget::setColorMode(int mode) {
	if(mode <= COLORMODE_THERMIC_BLUE2RED )
		m_colorMode = mode;
	else
		m_colorMode = COLORMODE_GREY;
	if(dImage) {
		if(dImage->depth() > 8) return;
		
		dImage->setNumColors(256);
		QColor col;
fprintf(stderr, "ImageWidget::%s : colorMode = %d\n", __func__, m_colorMode);
		// Change color mode
		switch(m_colorMode) {
		default:
		case COLORMODE_GREY:
			for(int i=0; i<256; i++) {
				col.setHsv(0,0,i);
				dImage->setColor(i, col.rgb());
			}
			break;
		case COLORMODE_GREY_INVERTED:
			for(int i=0; i<256; i++) {
				col.setHsv(0,0,255-i);
				dImage->setColor(i, col.rgb());
			}
			break;
		case COLORMODE_THERMIC_BLUE2RED: // "Thermic" blue to red
			for(int i=0; i<256; i++) {
				dImage->setColor(i, qRgb(RthermicBlue2Red(i), GthermicBlue2Red(i), BthermicBlue2Red(i)));
			}
			break;
		case COLORMODE_THERMIC_BLACK2RED: // "Thermic" black to red
			for(int i=0; i<256; i++) {
				dImage->setColor(i, qRgb(RthermicBlack2Red(i), GthermicBlack2Red(i), BthermicBlack2Red(i)));
			}
			break;
		}
	}
	update();
}


void ImageWidget::paintEvent( QPaintEvent * e)
{
//fprintf(stderr, "ImageWidget::paintEvent\n");
	if(!dImage)
		return;
	QRect cr;
	if(!e) {
		cr = rect();
	} else {
		// Use double-buffering
		cr = e->rect();
	}
	QPixmap pix( cr.size() );
	pix.fill( this, cr.topLeft() );
    QPainter p( &pix);
	if(cr != this->rect())
		p.drawImage(0,0, dImage->copy(cr)); 
	else 
		p.drawImage(0,0, *dImage); 

    p.end();
    bitBlt( this, cr.topLeft(), &pix );
}

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
