/***************************************************************************
	SWtypes.h  -  SISELL Workshop Types
                             -------------------
    begin                : Tue Jun 25 2002
    copyright            : (C) 2002 by Christophe Seyve
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

#ifndef _SW_TYPES_
#define _SW_TYPES_


// ***** STRUCTURES DECLARATIONS *****
// Type declarations for functions and their parameters description

// unsigned :	binary 0xxxxxxx
// signed : 	binary 1xxxxxxx
#define swType unsigned char

// non float
#define swU8	0x01
#define swS8	0x11
#define swU16	0x02
#define swS16	0x12
#define swU32	0x04
#define swS32	0x14
#define swSignedType 0x10

// float
#define swFloat		0x21
#define swDouble	0x22
#define swFloatingPointType 0x20

// sisell class defines
#define swNumeric 0x41
#define swMeasure1D 0x42
#define swMeasure2D 0x42
#define swImage 0x44

// String list
typedef struct _swStringListStruct {
	int nbitems;
	int curitem;
	char ** list;
	} swStringListStruct;

#define swStringList 0x48


// ------------- frame communication declarations --------------
#define SWFRAME_HEADER "SWSTART"
#define SWFRAME_END "SWEND"

#define SWFRAME_QUIT "QUIT"


// function list
#define SWFRAME_ASKFUNCLIST "FUNCLIST?"
#define SWFRAME_SETFUNCLIST "FUNCLIST="

// function description
#define SWFRAME_ASKCATEGORY "CATEGORY?"
#define SWFRAME_SETCATEGORY "CATEGORY="

#define SWFRAME_ASKFUNCTIONDESC "FUNCDESC?"
#define SWFRAME_SETFUNCTIONDESC "FUNCDESC="

// process function
#define SWFRAME_PROCESSFUNCTION "PROCESS"
#define SWFRAME_PROCESSRESULT "RESULT="

typedef struct _swFuncParams {
	char * name;
	swType type;
	void *value;
} swFuncParams;

typedef struct _swFunctionDescriptor {
    char *name; 		// name of function
    int nb_params;		// number of parameters
    swFuncParams * param_list;  // parameters list by name
    swType inputType;
    swType outputType;
    void (* procedure)();
    void (* edit_procedure)();
} swFunctionDescriptor;






int SwGetTypeSize(unsigned char type);

typedef struct _swMutex {
	unsigned char StatusPending : 1; // 1 if we can't write or read yet

    // Rights
	unsigned char WriteDisabled : 1; // 1 : cannot write
	unsigned char ReadDisabled : 1; // 1 : cannot write
	
	
    unsigned char WritePending : 1; // 1 if a write is pending
//    unsigned char
	
	} swMutex;


// For exchanges between agents
typedef struct _swFrame {
    char *buffer;
    int maxlen;
    int pos;
    } swFrame;

	
#endif

