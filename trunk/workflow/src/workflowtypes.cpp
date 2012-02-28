/***************************************************************************
 *  workflowtypes.cpp
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

#include "piaf-common.h"
#include "workflowtypes.h"

#include <QFileInfo>

int g_debug_WorkflowTypes = SWLOG_DEBUG;



#define WKTYPES_MSG(a,...)       { \
			if( (a)>=g_debug_WorkflowTypes ) { \
					struct timeval l_nowtv; gettimeofday (&l_nowtv, NULL); \
					fprintf(stderr,"%03d.%03d %s [WkfTypes] [%s] %s:%d : ", \
							(int)(l_nowtv.tv_sec%1000), (int)(l_nowtv.tv_usec/1000), \
							SWLOG_MSG((a)), __FILE__,__func__,__LINE__); \
					fprintf(stderr,__VA_ARGS__); \
					fprintf(stderr,"\n"); \
			} \
		}


/******************************************************************************
								COLLECTIONS
  *****************************************************************************/
EmaCollection::EmaCollection()
{
	treeViewItem = NULL;
	title = QObject::tr("No title");
}

EmaCollection::~EmaCollection()
{
	/// \todo implement destructions of allocated structures
}
void EmaCollection::appendSubCollection(EmaCollection *sub)
{
	WKTYPES_MSG(SWLOG_INFO, "Append sub-collection '%s' to collection '%s'",
				sub ? sub->title.toAscii().data() : "null",
				title.toAscii().data()
				);
	this->subCollectionsList.append(sub);
}

void EmaCollection::appendFiles(QStringList listOfFiles)
{
	WKTYPES_MSG(SWLOG_INFO, "Append %d files to collection '%s'",
				listOfFiles.count(),
				title.toAscii().data()
				);

	QStringList::iterator it;
	for(it = listOfFiles.begin(); it != listOfFiles.end(); it++)
	{
		QFileInfo fi((*it));
		QString filepath = fi.absoluteFilePath();

		QList<t_collection_file *>::iterator fit;
		bool found = false;
		for(fit = filesList.begin(); fit!=filesList.end() && !found; ++fit)
		{
			if((*fit)->fullpath == filepath)
			{
				found = true;
			}
		}
		if(!found)
		{
			t_collection_file * pfile = new t_collection_file;
			pfile->filename = fi.baseName();
			pfile->fullpath = fi.absoluteFilePath();
			pfile->pCollection = this;
			pfile->type = 0;
			pfile->treeViewItem = NULL; /// \todo implement tree widgets item generation

			WKTYPES_MSG(SWLOG_DEBUG, "\tappend file '%s'", pfile->fullpath.toAscii().data());
			filesList.append(pfile);
		}
	}
}

void removeCollection(QList<EmaCollection *> * pCollList, EmaCollection * delCol)
{
	PIAF_MSG(SWLOG_DEBUG, "Search in collections list %p for '%s'...",
				 pCollList,
				 delCol->title.toAscii().data());

	QList<EmaCollection *>::iterator cit;
	for(cit = pCollList->begin(); cit != pCollList->end(); ++cit)
	{
		EmaCollection * pcol = (*cit);
		if(delCol == pcol)
		{
			// delete now
			if(delCol->treeViewItem)
			{
				delete delCol->treeViewItem;
			}
			pCollList->removeOne(delCol);
			delete delCol;
			return;
		}
		else
		{
			// Search in its sub-collection
			PIAF_MSG(SWLOG_DEBUG, "Search in '%s's sub-collections...",
						 pcol->title.toAscii().data());
			removeCollection(&pcol->subCollectionsList, delCol);
		}
	}

}


void EmaCollection::removeFiles(QStringList listOfFiles)
{
	WKTYPES_MSG(SWLOG_INFO, "Append %d files to collection '%s'",
				listOfFiles.count(),
				title.toAscii().data()
				);

	QStringList::iterator it;
	for(it = listOfFiles.begin(); it != listOfFiles.end(); it++)
	{
		QFileInfo fi((*it));
		QString filepath = fi.absoluteFilePath();
		t_collection_file * found = NULL;
		QList<t_collection_file *>::iterator fit;
		for(fit = filesList.begin(); fit!=filesList.end() && !found; ++fit)
		{
			if((*fit)->fullpath == filepath)
			{
				found = (*fit);
			}
		}
		if(found)
		{
			t_collection_file * pfile = found;
			WKTYPES_MSG(SWLOG_DEBUG, "\tremove file '%s'", pfile->fullpath.toAscii().data());
			filesList.removeOne(pfile);
			if(pfile->treeViewItem)
			{
				delete pfile->treeViewItem;
			}
			delete pfile;
		}
	}
}
void EmaCollection::clearFiles()
{
	WKTYPES_MSG(SWLOG_INFO, "Clear files list: %d files in collection '%s'",
				filesList.count(),
				title.toAscii().data()
				);
	while(!filesList.isEmpty())
	{
		t_collection_file * pfile = filesList.at(0);
		filesList.removeAt(0);
		if(pfile->treeViewItem)
		{
			delete pfile->treeViewItem;
		}
		delete pfile;
	}
}


/******************************************************************************
									FOLDERS
  *****************************************************************************/

