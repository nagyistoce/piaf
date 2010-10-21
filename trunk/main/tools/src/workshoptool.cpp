/***************************************************************************
	workshoptool.cpp  -  generic tool for Piaf
							 -------------------
	begin                : ven nov 29 15:53:48 UTC 2002
	copyright            : (C) 2002 by Olivier Viné - Christophe SEYVE
	email                : olivier.vine@sisell.com christophe.seyve@sisell.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "workshoptool.h"
static int WorkshopTool_last_id = 0;

#include <QWorkspace>

WorkshopTool::WorkshopTool (QWidget * p_parent, const char *p_name, Qt::WidgetAttribute wflags)
	: QObject (p_parent, p_name)
{
	WorkshopTool_last_id++;

	m_ID = WorkshopTool_last_id;

	fprintf(stderr, "WorkshopTool::%s:%d : parent=%p\n", __func__, __LINE__, p_parent);
	// creating the main window
	pWin = new WorkshopMainWindow (p_parent, p_name, wflags);

	pWin->hide();

	// initialisation of the variables
	initToolVars ();
}

void WorkshopTool::initToolVars ()
{
	for (int i = 0; i < NB_COMPONENT_CLASS; i++)
	{
		disableDisplay (i);
		disableCreate (i);
	}

	disableDisplayNewMeasure ();
}

void WorkshopTool::addNewMeasure (WorkshopMeasure * pWm)
{
	if (pWm)
	{
		WMList.append (pWm);
		emit newMeasure ((int) pWm->getID());
	}
}

void WorkshopTool::addNewImage (WorkshopImage * pWi)
{
	if (pWi)
	{
		WIList.append (pWi);
		emit newImage ( pWi->getID() );
	}
}


void WorkshopTool::addNewComponent (WorkshopComponent * pWc)
{
	int cClass;
	if (pWc)
	{
		cClass = pWc->getComponentClass ();
		switch (cClass)
		{
		case CLASS_MEASURE:
			addNewMeasure ((WorkshopMeasure *) pWc);
			break;
		case CLASS_IMAGE:
			addNewImage ((WorkshopImage *) pWc);
			break;
		}
	}
}

void WorkshopTool::removeMeasure (WorkshopMeasure * pWm)
{
	if (WMList.isEmpty ())
		return;

	if (pWm)
	{
			WMList.remove (pWm);
		emit rmMeasure ((int) pWm->getID());
	}
}

void WorkshopTool::removeImage (WorkshopImage * pWi)
{
	if (WIList.isEmpty ())
		return;

	if (pWi)
	{
		WIList.remove (pWi);
		emit rmImage ((int) pWi->getID());
	}
}

void WorkshopTool::removeComponent (WorkshopComponent * pWc)
{
	int cClass;
	if (pWc)
	{
		cClass = pWc->getComponentClass ();
		switch (cClass)
		{
		case CLASS_MEASURE:
			removeMeasure ((WorkshopMeasure *) pWc);
			break;
		case CLASS_IMAGE:
			removeImage ((WorkshopImage *) pWc);
			break;
		}
	}
}

bool WorkshopTool::haveMeasure (WorkshopMeasure * pWm)
{
	if (WMList.isEmpty ())
		return false;

	return (WMList.find (pWm) != -1);
}

bool WorkshopTool::haveImage (WorkshopImage * pWi)
{
	if (WIList.isEmpty ())
		return false;

	return (WIList.find (pWi) != -1);
}

bool WorkshopTool::haveComponent (WorkshopComponent * pWc)
{
	int cClass;
	if (pWc)
	{
		cClass = pWc->getComponentClass ();
		switch (cClass)
		{
		case CLASS_MEASURE:
			return haveMeasure ((WorkshopMeasure *) pWc);
		case CLASS_IMAGE:
			return haveImage ((WorkshopImage *) pWc);
		}
	}

	return false;
}

bool WorkshopTool::displayNewComponent (int cClass)
{
	switch (cClass)
	{
	case CLASS_MEASURE:
		return displayNewMeasure ();
	case CLASS_IMAGE:
		return displayNewImage ();
	default:
		return false;
	}
}

void WorkshopTool::enableDisplayNewComponent (int cClass)
{
	switch (cClass)
	{
	case CLASS_MEASURE:
		enableDisplayNewMeasure ();
	case CLASS_IMAGE:
		enableDisplayNewImage ();
	default:
		break;
	}
}

void WorkshopTool::disableDisplayNewComponent (int cClass)
{
	switch (cClass)
	{
	case CLASS_MEASURE:
		disableDisplayNewMeasure ();
	case CLASS_IMAGE:
		disableDisplayNewImage ();
	default:
		break;
	}
}
