/***************************************************************************
				piaf-settings.h  - Piaf common settings header
	common paths, ... for Piaf widgets and tool
							 -------------------
	begin                : Sun Mar 27 2011
	copyright            : (C) 2011 Christophe Seyve (CSE)
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

#ifndef PIAFSETTINGS_H
#define PIAFSETTINGS_H

// Labels
#define COMP_IMAGE_LABEL    "Images"
#define COMP_VIDEO_LABEL    "Videos"
#define COMP_MEASURE_LABEL  "Measures"

/// Relative image directory
#define IMAGEDIR "images/pixmaps/"

/// Absolute image directory
#define FULLIMAGEDIR	BASE_DIRECTORY IMAGEDIR

#ifdef WORKSHOP_CPP
#define WKP_EXTERN
#else
#define WKP_EXTERN extern
#endif

// Path for saving files (snapshots, ...)
WKP_EXTERN QString g_imageDirName;
WKP_EXTERN QString g_measureDirName;
WKP_EXTERN QString g_movieDirName;

WKP_EXTERN QString g_pluginDirName;

#endif // PIAFSETTINGS_H
