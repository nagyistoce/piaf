/***************************************************************************
	SwPluginCore.h  -  core class for Piaf plugin manager (dialog utilities)
                             -------------------
    begin                : 
    copyright            : (C) 2002 - 2003 Christophe Seyve (CSE)
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

#ifndef __SWPLUGINCORE__
#define __SWPLUGINCORE__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sw_types.h>

#include <SwTypes.h>
#include <SwImage.h>


/**
    \brief Component for dialoging with plugins.

	Core class for managing communications between plugins
	and calling objects (such as SwFilter).
	
	\author Christophe SEYVE	\mail cseyve@free.fr
*/
class SwPluginCore
{
public:
	/// constructor
	SwPluginCore();
	/// destrcutor
	~SwPluginCore();

	// ------- plugin-side called functions
	/// registers category and subcategory (ex. : "Vision" / "Morphology").
	int registerCategory(char * category, char * subcategory);
	
	/// registers functions descriptions
	int registerFunctions(swFunctionDescriptor * funcDesc, int nbfunc);
	
	// loop for processing commands	
	int loop();

	/// buffers for data in
	unsigned char * data_in;
	/// buffers for data out
	unsigned char * data_out;

private:
    int size_in;
    int size_out;
    int quit;
    
	// read parameters from file
	int readConfigurationFile();

	// frame struct for dialog
	swFrame frame;
	char * buffer;
	
	// format output frame
	int formatFrame(char *frameDescriptor);
	
	// Export data to file or stdout
	int exportOutputToFile(char *filename);
	int exportOutputToStdout();
	int exportParameters();
	
	// entry points : treatFrame
	int treatFrame(char *frame, int framesize);
	
	/// writes function list to stdout
	char * sendFunctions();
	
	
	unsigned long calculateChecksum();
	int decodeFrame();
	int setFunctionParameters(char *frame, int len);
	int processFunction(char *frame, int len);

	// global properties
	char * name;
	char * category;
	char * subcategory;
	/// writes category frame to stdout
	void sendCategory(); 
	
	int sendMessage(char * msg);
	
	// 
	swFunctionDescriptor * funcList;
	int NbFunctions;

	
	// For any function, returns
	int sendFunctionDescriptor(int funcnum);

};

// Internal functions

int swAllocateFrame(swFrame *current, int size);
int swAddHeaderToFrame(swFrame * current);
int swAddStringToFrame(swFrame *current, char *txt);
int swAddBufferToFrame(swFrame * current, void * buffer, int size);
int swAddSeparatorToFrame(swFrame *current);
int swAddScalarToDescriptor(swFrame *current, char *name, unsigned char type, char *value);
int swAddBooleanToDescriptor(swFrame *current, char *name, unsigned char type, bool value);
int swAddListToDescriptor(swFrame *current, char *name, char ** itemnames, unsigned char type, char ** value, int nb_item);
int swCloseFrame(swFrame * current);
int swSendFrame(swFrame *current);
int swFreeFrame(swFrame * current);

// Data communication
int swAddImageToFrame(swFrame *current, swImageStruct * image);
int swAddMeasure1DToFrame(swFrame *current, swMeasure1DStruct * measure);

int swAllocValueFromType(swType type, void ** value);
int swGetValueFromType(swType type, // type
	void * val, // pointer to value 
	void * outputval // pointer to value output
	);
int swGetValueFromTypeAndString(
	swType type, // type
	char * val, // pointer to value 
	void * outputval // pointer to value output
	);
int swGetStringValueFromType(swType type, // type
	void * value, // pointer to value
	char * txt);


int swSendImage(int iFunc, swFrame * frame, swType outputType, void * data_out, FILE * fW);

/** @brief
  @param
  */
int swReceiveImage(void * data_out, FILE * fR, int timeout_ms, bool * pcontinue = NULL);
int debugWriteImage(char * filename, swImageStruct * pimage);

int swReadFromPipe(unsigned char * buffer, u32 size, FILE * pipe, int timeout_ms);

	
// FILE SECTION
char * nextTab( char * frame );
char * nextFileSeparator( char * frame );
FILE * swFopen(char * file, char *mode);


#endif
