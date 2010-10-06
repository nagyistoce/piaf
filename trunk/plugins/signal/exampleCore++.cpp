/***************************************************************************
    main.cpp  -  description
                             -------------------
    begin                : Tue Jul  2 15:52:21 CEST 2002
    copyright            : (C) 2002 by Christophe Seyve
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "SwPluginCore++.h"


#define CATEGORY	"Signal"
#define SUBCATEGORY	"Testing"

// function invert
PIAF_PLUGIN(invert,)
{
	void proc()
	{
		unsigned char * imageIn  = (unsigned char *)imIn->buffer;
		unsigned char * imageOut = (unsigned char *)imOut->buffer;

		for(unsigned long r = 0; r < imIn->buffer_size; r++)
		{
			imageOut[r] = 255 - imageIn[r];
		}
	}
};

// function threshold
PIAF_PLUGIN(threshold,
            (unsigned char, threshold_min, 127)
            (unsigned char, threshold_max, 255)
           )
{
	void proc()
	{
		unsigned char * imageIn  = (unsigned char *)imIn->buffer;
		unsigned char * imageOut = (unsigned char *)imOut->buffer;

		for(unsigned long r = 0; r < imIn->buffer_size; r++)
		{
			if(imageIn[r] >= threshold_min && imageIn[r] <= threshold_max)
				imageOut[r] = 255;
			else
				imageOut[r] = 0;
		}
	}
};

// function example3
PIAF_PLUGIN(uselessexample,
            (unsigned char, v1, 127)
            (unsigned char, v2, 255)
           )
{
	unsigned char val;

	void proc()
	{
		unsigned char * imageIn  = (unsigned char *)imIn->buffer;
		unsigned char * imageOut = (unsigned char *)imOut->buffer;

		for(unsigned long r = 0; r < imIn->buffer_size; r++)
		{
			if(imageIn[r] >= threshold_min && imageIn[r] <= threshold_max)
				imageOut[r] = val;
			else
				imageOut[r] = imageIn[r];
		}
	}

	void onParameterChanged()
	{
		val = (v2 + v1) / 2;
	}
};

PIAF_MAIN(CATEGORY, SUBCATEGORY,
		(invert)
		(threshold)
		(uselessexample)
)
