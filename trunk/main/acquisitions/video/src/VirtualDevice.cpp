/***************************************************************************
 *            VirtualDevice.cpp
 *	Only constructor and destructor functions
 * 
 *  Thu March 6 09:36:48 2006
 *  Copyright  2006  Christophe Seyve - SISELL
 *  cseyve@free.fr
 ****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "VirtualDevice.h"
#include <stdio.h>

VirtualDevice::VirtualDevice() {
	fprintf(stderr, "VirtualDevice::VirtualDevice() : %p\n", this);
}

VirtualDevice::~VirtualDevice(){
	fprintf(stderr, "VirtualDevice::~VirtualDevice() : %p\n", this);
}
