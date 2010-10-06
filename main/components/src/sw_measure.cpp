/***************************************************************************
                          sw_measure.cpp  -  description
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
#include "sw_measure.h"

SW_Measure::SW_Measure(void)
		:QValueVector<double>()
{
	mLabel = "Untitled Measure";
	mShortLabel = "Meas.";
	mDate.currentDate();
	mValidity = FALSE;
	CAN_size = 0;
	frequency = 0;
}

SW_Measure::SW_Measure(int size)
		:QValueVector<double>(size)
{
	mLabel = "Untitled Measure";
	mShortLabel = "Meas.";
	mDate.currentDate();
	mValidity = FALSE;
	CAN_size = 0;
	frequency = 0;
}

