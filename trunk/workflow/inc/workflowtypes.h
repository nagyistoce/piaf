/***************************************************************************
 *  workflowtypes.h
 *
 *  Sun Oct 30 21:32:11 2011
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

#ifndef WORKFLOWTYPES_H
#define WORKFLOWTYPES_H

#include <QtGui/QTreeWidgetItem>
#include <QString>

/** @brief Structure to define a mapping between a file and a tree item

 Beware: containing objects

  */
typedef struct _t_file {
	QString name;
	QString fullPath;
	QTreeWidgetItem * treeViewItem;
} t_file;

/** @brief Folder listed */
typedef struct {
	QString fullpath;	///< Full path to directory
	QString filename;	///< basename
	QString extension;	///< File extension
	bool expanded;		///< Expanded state on explorer

	QTreeWidgetItem * treeViewItem;
} t_folder;

/** @brief File known in collection */
typedef struct {
	QString fullpath;	///< Full path to file
	QString filename;	///< basename
	int type;			///< Type: movie, image
	struct _t_collection * pCollection; ///< Reverse pointer to its collection

	QTreeWidgetItem * treeViewItem;
} t_collection_file;

/** @brief Collection storage */
typedef struct _t_collection {
	QString title;
	QString comment;
	QList<t_collection_file *> filesList;	///< List of full path to their images
	QList<struct _t_collection *> subCollectionsList;

	QTreeWidgetItem * treeViewItem;
} t_collection;

typedef struct {
	QString node;
} t_device;

#endif // WORKFLOWTYPES_H
