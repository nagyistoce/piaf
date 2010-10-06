/***************************************************************************
    example.cpp  -  Example plugin for piaf
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

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


// include componenet header
#include "SwPluginCore.h"

/* compile with :
gcc -Wall -I/usr/local/sisell/include SwPluginCore.cpp -c
gcc -Wall -I/usr/local/sisell/include main.cpp -c
gcc main.o SwPluginCore.o -o swcorecomponent 
# needs SwPluginCore.o
*/


/********************** GLOBAL SECTION ************************
DO NOT MODIFY THIS SECTION
plugin.data_in  : input data. You should cast this pointer to read the
			data with the correct type
plugin.data_out : output data. You should cast this pointer to write the
			data with the correct type
***************************************************************/
SwPluginCore plugin;

/******************* END OF GLOBAL SECTION ********************/

/********************** USER SECTION ************************
YOU SHOULD MODIFY THIS SECTION
Declare your variables, function parameters and function list
	

***************************************************************/
#define CATEGORY	"Vision"
#define SUBCATEGORY	"Example"

// functions parameters


// circular buffer median
unsigned char median = 5;

swFuncParams median_params[] = {
	{"iteration", swU8, (void *)&median}
};

float low_update_rate = 0.1f;
swFuncParams low_update_params[] = {
	{"rate", swFloat, (void *)&low_update_rate}
};


// threshold
unsigned char threshold_min = 127;
unsigned char threshold_max = 255;

swFuncParams threshold_params[] = {
	{"min", swU8, (void *)&threshold_min},
	{"max", swU8, (void *)&threshold_max},
};

swFuncParams invert_params[] = {
};













float erode_radius = 3.1415927;
short erode_iterations = 1;

swFuncParams erode_params[] = {
	{"radius", swFloat, (void *)&erode_radius},
	{"iteration", swS16, (void *)&erode_iterations}
};


void invert();
void threshold();
void erode();
void median_func();
void low_update();
/* swFunctionDescriptor : 
	char * : function name
	int : number of parameters
	swFuncParams : function parameters
	swType : input type
	swType : output type
	void * : procedure
	*/
swFunctionDescriptor functions[] = {
	{"Circ. Median", 1, 	median_params, 		swImage, swImage, &median_func, NULL},
	{"Low update",	1, 		low_update_params, swImage, swImage, &low_update, NULL},
	{"Invert", 		0, 	NULL, 				swImage, swImage, &invert, NULL},
	{"Threshold", 	2, 	threshold_params, 	swImage, swImage, &threshold, NULL},
	{"Lighten", 	2,	erode_params,  		swImage, swImage, &erode, NULL}
};
int nb_functions = 5;

/******************* END OF USER SECTION ********************/

// function circular median 
unsigned char ** median_buf = NULL;
unsigned char oldmedian = 0;
int pos = 0;

void median_func()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	if((!median_buf || oldmedian != median) && median > 0) {
		if(median_buf) {
			for(int i=0; i<oldmedian; i++)
				delete [] median_buf[i];
			delete []  median_buf;
		}
		median_buf = new unsigned char * [median];
		for (int b=0; b<median; b++) {
			median_buf[b] = new unsigned char [imIn->buffer_size];
			
			memcpy(median_buf[b], imageIn, imIn->buffer_size);
		}
		pos = 0;
		oldmedian = median;
	}
	else {
		memcpy(median_buf[pos], imageIn, imIn->buffer_size);
		pos++;
		if(pos == (int)median) pos = 0;
	}
	for(unsigned long r = 0; r < imIn->buffer_size; r++)
	{
		unsigned short total = 0;
		for(int i=0; i<(int)median; i++)
			total += (unsigned short)median_buf[i][r];
		
		imageOut[r] = (unsigned char)(total / median);
	}
}

float * low_update_buf = NULL;
void low_update() {
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;
	unsigned long r;
	
	if(!low_update_buf) {
		low_update_buf = new float [ imIn->buffer_size ];
		for(r = 0; r < imIn->buffer_size; r++)
			low_update_buf[r] = (float)imageIn[r];
	}
	
	float rest = 1.f - low_update_rate;
	for(r = 0; r < imIn->buffer_size; r++)
	{
		low_update_buf[r] = low_update_rate * (float)imageIn[r] + rest * low_update_buf[r];
		imageOut[r] = (unsigned char)low_update_buf[r];
	}
}



// function invert
void invert()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;
	
	
	for(unsigned long r = 0; r < imIn->buffer_size; r++)
	{
		imageOut[r] = 255 - imageIn[r];
	}
}

// function threshold
void threshold()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
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

// function erode
void erode()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	memcpy(imageOut, imageIn, imIn->buffer_size);
		
	for(int i=0; i<erode_iterations;i++)
	for(unsigned long r = 0; r < imIn->buffer_size; r++)
	{
		imageOut[r] = ((imageOut[r] & 0xFE) >> 1) | 0x80;
	}
}


// DO NOT MODIFY MAIN PROC
void signalhandler(int sig)
{
	fprintf(stderr, "================== RECEIVED SIGNAL %d = '%s' From process %d ==============\n", sig, sys_siglist[sig], getpid());
	signal(sig, signalhandler);
	
	if(sig != SIGUSR1)
		exit(0);
}
int main(int argc, char *argv[])
{
	// SwPluginCore load
	for(int i=0; i<NSIG; i++)
		signal(i, signalhandler);
		
	fprintf(stderr, "registerCategory...\n");
	plugin.registerCategory(CATEGORY, SUBCATEGORY);
	
	// register functions 
	fprintf(stderr, "registerFunctions...\n");
	plugin.registerFunctions(functions, nb_functions );

	// process loop
	fprintf(stderr, "loop...\n");
	plugin.loop();

	fprintf(stderr, "exit(EXIT_SUCCESS). Bye.\n");
  	return EXIT_SUCCESS;
}
