/***************************************************************************
	SwPluginCore.cpp  -  core class for Piaf plugin manager (dialog utilities)
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

//#define __SWPLUGIN_DEBUG__

#include "SwPluginCore.h"
#include "sw_types.h"

/*
compiles with :
 gcc -Wall -I/usr/local/sisell/include SwPluginCore.cpp -c
 */
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

void swPurgePipe(FILE *fR);

//#define __SWPLUGIN_DEBUG__

#ifdef SWPLUGIN_SIDE
#define SWPLUGIN_SIDE_PRINT		"PIAF-GUI-SIDE\t"
#else
#define SWPLUGIN_SIDE_PRINT		"PLUGIN-SIDE\t"
#endif // 		SWPLUGIN_SIDE

// Default constructor
SwPluginCore::SwPluginCore()
{
	// Create function list
	NbFunctions = 0;

	// Read default parameters in file
	funcList = NULL;

	data_in = data_out = NULL;
	size_in = size_out = 0;

	category = subcategory = name = NULL;
#define BUFFER_SIZE 4096
	swAllocateFrame(&frame, BUFFER_SIZE);

	buffer = new char [BUFFER_SIZE];



}


// destructor
SwPluginCore::~SwPluginCore()
{
	if(buffer) {
		delete [] buffer;
	}

	if(funcList) {
		for(int i=0; i<NbFunctions; i++) {
			// delete descriptor
			if(funcList[i].name)	delete [] funcList[i].name;
			if(funcList[i].param_list) {
				for(int j=0; j<funcList[i].nb_params;j++) {
					if(funcList[i].param_list[j].name)
						delete [] funcList[i].param_list[j].name;
/*					if(funcList[i].param_list[j].value)
						delete funcList[i].param_list[j].value;
*/				}
				delete [] funcList[i].param_list;
			}
		}
		delete [] funcList;

		NbFunctions = 0;
	}

	if(name)
		delete [] name;
	if(category)
		delete [] category;
	if(subcategory)
		delete [] subcategory;

	if(data_in)
		delete [] data_in;
	if(data_out)
		delete [] data_out;

	swFreeFrame(&frame);
}



// registerCategory()
int SwPluginCore::registerCategory(char * cat, char * subcat)
{
	if(cat)
	{
		category = new char [ strlen(cat) + 1 ];
                strcpy(category, cat);
	}
	if(subcat)
	{
		subcategory = new char [ strlen(subcat) + 1 ];
                strcpy(subcategory, subcat);
	}
	return 1;
}

// Send category answer frame
void SwPluginCore::sendCategory()
{
	swAddHeaderToFrame(&frame);
	swAddSeparatorToFrame(&frame);

	swAddStringToFrame(&frame, SWFRAME_SETCATEGORY );
	swAddSeparatorToFrame(&frame);

	swAddStringToFrame(&frame, category);
	swAddSeparatorToFrame(&frame);

	swAddStringToFrame(&frame, subcategory);
	swAddSeparatorToFrame(&frame);


	// send
	swCloseFrame(&frame);
	swSendFrame(&frame);
}

// register function list and parameters
int SwPluginCore::registerFunctions(swFunctionDescriptor * list, int nbfunc)
{
	// allocation
	NbFunctions  = nbfunc;
	if(NbFunctions) {
		if(!funcList)
			funcList = new swFunctionDescriptor [ NbFunctions ];
	}
	else {
		funcList = NULL;
		return 1;
	}

	// allocation and copy for each function
	for(int i=0; i<NbFunctions; i++)
	{
		// name
		funcList[i].name = new char [ strlen(list[i].name)+1 ];
		if(!funcList[i].name) return 0;
		memcpy(funcList[i].name, list[i].name, strlen(list[i].name));
		funcList[i].name[ strlen(list[i].name) ] = '\0';

		// parameters
		funcList[i].nb_params = list[i].nb_params;
		funcList[i].inputType = list[i].inputType;
		funcList[i].outputType = list[i].outputType;
		funcList[i].procedure = list[i].procedure;
		funcList[i].edit_procedure = list[i].edit_procedure;

		if(funcList[i].nb_params) {
			funcList[i].param_list = new swFuncParams [ list[i].nb_params ];
			if(!funcList[i].param_list) return 0;

			for(int j=0; j< funcList[i].nb_params; j++)
			{
				funcList[i].param_list[j].type = list[i].param_list[j].type ;
				funcList[i].param_list[j].name = new char [ strlen(list[i].param_list[j].name)+1];
				if(!funcList[i].param_list[j].name) return 0;
				strcpy(funcList[i].param_list[j].name, list[i].param_list[j].name);

				funcList[i].param_list[j].value = list[i].param_list[j].value;
				// beware, we store only value address !!
//fprintf(stderr, SWPLUGIN_SIDE_PRINT "PLUGIN: register type [%d] '%s' = %d\n", j, funcList[i].param_list[j].name, list[i].param_list[j].type);
			}
		} else
			funcList[i].param_list = NULL;
	}

	return 1;
}

/** getFunctions.
 * Description : declare functions list
 * @author CSE
 */
char * SwPluginCore::sendFunctions()
{
	swAddHeaderToFrame(&frame);
	swAddSeparatorToFrame(&frame);

	swAddStringToFrame(&frame, SWFRAME_SETFUNCLIST );
	swAddSeparatorToFrame(&frame);

	// add number of functions
	char txt[64];
	sprintf(txt, "%d", NbFunctions);
	swAddStringToFrame(&frame, txt);
	swAddSeparatorToFrame(&frame);

	// add functions names
	for(int i=0; i<NbFunctions; i++)
	{
		swAddStringToFrame(&frame, funcList[i].name);
		swAddSeparatorToFrame(&frame);
	}

	// close and send
	swCloseFrame(&frame);
	swSendFrame(&frame);

	 // return nb of functions
	return NULL;
}


/***************************************************************************
 * Function : getFunctionDescriptor()
 * Description : read a function descriptor: name, parameters, output type...
 * Author : CSE
 ***************************************************************************/
int SwPluginCore::sendFunctionDescriptor(int funcnum)
{
	if(funcnum>=NbFunctions) return 0;

	swAddHeaderToFrame(&frame);
	swAddSeparatorToFrame(&frame);
	swAddStringToFrame(&frame, SWFRAME_SETFUNCTIONDESC);
	swAddSeparatorToFrame(&frame);

	// send func number, name, number of parameters
	char txt[2048];
	sprintf(txt, "%d\t%s\t%d\t",
			funcnum,
			funcList[funcnum].name,
			funcList[funcnum].nb_params
		);
	swAddStringToFrame(&frame, txt);

	// send each parameter
	for(int j=0; j< funcList[funcnum].nb_params; j++) {
		// format : param name \t type as uchar \t value as string
		sprintf(txt, "%s\t%c\t",
					funcList[funcnum].param_list[j].name,
					(char)funcList[funcnum].param_list[j].type);

		switch(funcList[funcnum].param_list[j].type)
		{
		case swU8:
			sprintf(txt, "%s%u\t", txt,
				(int)*(unsigned char *)funcList[funcnum].param_list[j].value);
			break;
		case swS8:
			sprintf(txt, "%s%d\t", txt,
				(int)*(char *)funcList[funcnum].param_list[j].value);
			break;
		case swU16:
			sprintf(txt, "%s%u\t", txt,
				(unsigned short)*(u16 *)funcList[funcnum].param_list[j].value);
			break;
		case swS16:
			sprintf(txt, "%s%d\t", txt,
				(short)*(i16 *)funcList[funcnum].param_list[j].value);
			break;
		case swU32:
			sprintf(txt, "%s%lu\t", txt,
                                (unsigned long)*(u32 *)funcList[funcnum].param_list[j].value);
			break;
		case swS32:
			sprintf(txt, "%s%ld\t", txt,
				(long)*(i32 *)funcList[funcnum].param_list[j].value);
			break;
		// floats
		case swFloat:
			sprintf(txt, "%s%g\t", txt,
				*(float *)funcList[funcnum].param_list[j].value);
			break;
		case swDouble:
			sprintf(txt, "%s%g\t", txt,
				*(double *)funcList[funcnum].param_list[j].value);
			break;
		case swStringList: {
			swStringListStruct *s =(swStringListStruct *)funcList[funcnum].param_list[j].value;
			sprintf(txt, "%s%d|%d", txt,s->nbitems, s->curitem);
			for(int i=0; i< s->nbitems;i++)
				sprintf(txt, "%s|%s", txt, s->list[i]);
			sprintf(txt, "%s\t", txt);

			}
			break;
		default:
			break;
		}
		swAddStringToFrame(&frame, txt);


	}

	// closa and send frame
	swCloseFrame(&frame);
	swSendFrame(&frame);

	return 1;
}


/* Function loop(). Loops until a quit order is received. For each received frame,
	it launches frame treatment with procedure treatFrame().
*/
int SwPluginCore::loop()
{
	quit = false;
	buffer[0] = '\0'; // clear buffer

	// reads standard input then close
	//fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_RDONLY & ~O_NONBLOCK);

	char *ret=NULL;
	while(!quit)
	{
		buffer[0]='\0';
		ret = fgets(buffer, BUFFER_SIZE-1, stdin);
#ifdef __SWPLUGIN_DEBUG__
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "PLUGIN: (BUFSIZ=%d) Read buffer='%s'\n", BUFSIZ, buffer); fflush(stderr);
#endif
		if(ret && buffer[0] != '\0') {
			buffer[strlen(buffer)] = '\0';
			treatFrame(buffer, strlen(ret));
		}
		else
			usleep(20000);
	}

#ifdef __SWPLUGIN_DEBUG__
	fprintf(stderr, SWPLUGIN_SIDE_PRINT "SwPluginCore: exit from loop !!\n");
#endif
	return 1;
}



// send a message to parent process (through stdout)
int SwPluginCore::sendMessage(char * msg)
{
	swAddHeaderToFrame(&frame);
	swAddSeparatorToFrame(&frame);

	swAddStringToFrame(&frame, "MSG" );
	swAddSeparatorToFrame(&frame);

	// add number of functions
	swAddStringToFrame(&frame, msg);
	swAddSeparatorToFrame(&frame);

	// close and send
	swCloseFrame(&frame);
	swSendFrame(&frame);

	return 1;
}



/***************************************************************************
 * Function : treatFrame()
 * Description : decompose and classify frame for examining
 * YOU DO NOT NEED TO MODIFY THIS FUNCTION
 *****************************************************************/
int SwPluginCore::treatFrame(char * framebuffer, int framesize)
{
	// Decompose frame
	//		look for header
	char *header = strstr(framebuffer, SWFRAME_HEADER);
	if(!header) // First arg =? SWFRAME_HEADER ??
	{
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "!!!!!!! SwPluginCore: Invalid header in treatFrame()\n");
		fflush(stderr);

		swPurgePipe(stdin);
		return 0;
	}

	// well, header is ok
	//	second arg: function name
	char *command = header + strlen(SWFRAME_HEADER) + 1;
	char *arg = nextTab( command );
#ifdef __SWPLUGIN_DEBUG__
fprintf(stderr, SWPLUGIN_SIDE_PRINT "\tCommand='%s'\n", command);
#endif
	// category ??
	if(strncmp(command, SWFRAME_ASKCATEGORY, strlen(SWFRAME_ASKCATEGORY)) == 0)
	{
#ifdef __SWPLUGIN_DEBUG__
fprintf(stderr, SWPLUGIN_SIDE_PRINT "\t====> sendCategory...\n");
#endif
		sendCategory();
		return 1;
	}

	// Function list ??
	if(strncmp(command, SWFRAME_ASKFUNCLIST, strlen(SWFRAME_ASKFUNCLIST)) == 0)
	{
#ifdef __SWPLUGIN_DEBUG__
fprintf(stderr, SWPLUGIN_SIDE_PRINT "sending function list...\n");
#endif
		sendFunctions();
		return 1;
	}

	//	quit command
	if(strncmp(command, SWFRAME_QUIT, strlen(SWFRAME_QUIT)) == 0)
	{
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "Quitting...\n"); fflush(stderr);
		quit = true;
		return 1;
	}

	//	Function descriptor ??
	if(strncmp(command, SWFRAME_ASKFUNCTIONDESC, strlen(SWFRAME_ASKFUNCTIONDESC)) == 0)
	{
		int nb = atoi(arg);
		if(nb > NbFunctions)
			return 0;

		// if no arg is declared
		if( funcList[nb].edit_procedure)
		{
			void (* pfunction)();
#ifdef __SWPLUGIN_DEBUG__
			fprintf(stderr, SWPLUGIN_SIDE_PRINT "Launch edit window.\n");
#endif
			pfunction = funcList[nb].edit_procedure;
			pfunction();
		}

		//
		sendFunctionDescriptor( nb);

		return 1;
	}

	// Function call
	if(strncmp(command, SWFRAME_SETFUNCTIONDESC, strlen(SWFRAME_SETFUNCTIONDESC)) == 0)
	{
		setFunctionParameters(arg, framesize - (arg - framebuffer) );
		return 1;
	}

	// Function call
	if(strncmp(command, SWFRAME_PROCESSRESULT, strlen(SWFRAME_PROCESSRESULT)) == 0)
	{
		processFunction(arg, framesize - (arg - framebuffer) );
		return 1;
	}

	return 0;
}

/*****************************************************************
 * Description : treat frame for executing an internal function
 *****************************************************************/
int SwPluginCore::processFunction(char *framebuffer, int )//unused len)
{
	// read function number, data type, data buffer
	int indexFunction = atoi(framebuffer);
	if(indexFunction >= NbFunctions)
		return 0;
	#ifdef __SWPLUGIN_DEBUG__
	fprintf(stderr, SWPLUGIN_SIDE_PRINT ">>>>> %s:%d : @@@@@@@@@@@@@@@@@ Processing function # %d ...\n\n",
			__func__, __LINE__,
			indexFunction);
	#endif

	// read data type
	char * arg = framebuffer;
	char * arg2 = nextTab(arg);
	arg = arg2;
	arg2 = nextTab(arg);

	swType inputType = *(swType *)arg;
	if( inputType != funcList[indexFunction].inputType)
	{
		fprintf(stderr, SWPLUGIN_SIDE_PRINT ">>>>> SwPluginCore::%s:%d: ERROR - Types do not match %d != %d\n",
				__func__, __LINE__,
				(int)inputType, (int)funcList[indexFunction].inputType);
		return 0;
	}
#ifdef __SWPLUGIN_DEBUG__
	fprintf(stderr, SWPLUGIN_SIDE_PRINT ">>>>> SwPluginCore::%s:%d: Input type = %d\n",
			__func__, __LINE__, (int)inputType);
#endif
	// read data
	switch (inputType) {
	case swImage: {
		// data is from this format : swImageStruct as binary field, then buffer
		// we must read image struct to get buffer size, then read size
		// read struct
		swImageStruct tmpStruct;
		int timeout_ms = 100;
		if(! swReadFromPipe((unsigned char *)&tmpStruct, (u32)sizeof(swImageStruct), stdin, timeout_ms)) {
			swPurgePipe(stdin);
			return 0;
		}

		// allocate buffer for input data
		int size = sizeof(swImageStruct) + tmpStruct.buffer_size + tmpStruct.metadata_size;

#ifdef __SWPLUGIN_DEBUG__
		// debug
		fprintf(stderr, SWPLUGIN_SIDE_PRINT ">>>>> SwPluginCore::%s:%d: Read image:"
				 "\tSize : %dx%d"
				 "\tbuffer_size=%ld"
				 "\tmetadata_size=%ld => size=%d\n",
				 __func__, __LINE__,
				 (int)tmpStruct.width, (int)tmpStruct.height,
				 (long)tmpStruct.buffer_size, (long)tmpStruct.metadata_size,
				 size
				 ); fflush(stderr);
#endif

		swImageStruct * pimage = (swImageStruct *)data_in;
		if(!data_in || size_in != size)
		{
			if(data_in)
				delete [] data_in;
			data_in = new unsigned char [ size ];
			memset(data_in, 0, sizeof(unsigned char)*size);

			size_in = size;
			pimage = (swImageStruct *)data_in;
			fprintf(stderr, SWPLUGIN_SIDE_PRINT "SwPluginCore::%s:%d Realloc data_in [ %d="
					"sizeof(swImageStruct)=%d + buffer_size=%d + metadata_size=%d] \n",
					__func__, __LINE__,
					size,
					(int)sizeof(swImageStruct),
					(int)tmpStruct.buffer_size, (int)tmpStruct.metadata_size
					); fflush(stderr);
		}

		memcpy(pimage, &tmpStruct, sizeof(swImageStruct));

		pimage->buffer = (unsigned char *)data_in + sizeof(swImageStruct); //restore pointer

#ifdef __SWPLUGIN_DEBUG__
		fprintf(stderr, SWPLUGIN_SIDE_PRINT ">>>>> SwPluginCore::%s:%d: copy header:"
				 "\tSize : %dx%d"
				 "\tbuffer_size=%ld metadata_size=%ld\n",
				 __func__, __LINE__,
				 (int)pimage->width,  (int)pimage->height,
				 (long)pimage->buffer_size,
				 (long)pimage->metadata_size
				 ); fflush(stderr);
#endif

		// read info from stdin to buffer
#ifdef __SWPLUGIN_DEBUG__
		fprintf(stderr, SWPLUGIN_SIDE_PRINT ">>>>> SwPluginCore::%s:%d: reading image buffers.\n", __func__, __LINE__);
#endif
		if(!swReadFromPipe((unsigned char *)pimage->buffer, (u32)(pimage->buffer_size),
						   stdin, timeout_ms)) {
			swPurgePipe(stdin);
			return 0;
		}
		if(pimage->metadata_size >0 ) {
			pimage->metadata = (unsigned char *)data_in + sizeof(swImageStruct) + pimage->buffer_size; //restore pointer

			if(!swReadFromPipe((unsigned char *)pimage->metadata, (u32)(pimage->metadata_size),
							   stdin, timeout_ms)) {
				swPurgePipe(stdin);
				return 0;
			}
		} else {
			pimage->metadata = NULL;
		}

#ifdef __SWPLUGIN_DEBUG__
		fprintf(stderr, SWPLUGIN_SIDE_PRINT ">>>>> SwPluginCore::%s:%d : Data_in read.\n", __func__, __LINE__);
		debugWriteImage("processfunction_in.ppm", pimage);
#endif
		}
		break;
	default:
		return 0;
		break;
	}

	// check if output is allocated
	switch (funcList[indexFunction].outputType) {
	case swImage: {
		// data is from this format : swImageStruct as binary field, then buffer
		// we must read image struct to get buffer size, then read size

		// allocate buffer for data_out
		swImageStruct * pimage = (swImageStruct *)data_out;
		if(!data_out || size_out != size_in)
		{
			if(data_out) { delete [] data_out; }

			data_out = new unsigned char [ size_in ];
			memset(data_out, 0, sizeof(unsigned char) * size_in); // clear for valgrind
			size_out = size_in;
			pimage = (swImageStruct *)data_out;

			fprintf(stderr, SWPLUGIN_SIDE_PRINT "SwPluginCore::%s:%d: Realloc data_out [ %d ] \n", __func__, __LINE__,
					size_out); fflush(stderr);

			// Copy header
			memcpy(data_out, data_in, sizeof(swImageStruct));

			// Restore buffers
			pimage->buffer = (unsigned char *)data_out + sizeof(swImageStruct);
			pimage->metadata = (unsigned char *)data_out + sizeof(swImageStruct) + pimage->buffer_size;

#ifdef __SWPLUGIN_DEBUG__
			fprintf(stderr, SWPLUGIN_SIDE_PRINT "SwPluginCore::%s:%d: data_out header : \n"
					"\tSize : %dx%d\n"
					"\tbuffer_size=%ld\n"
					"\tmetadata_size=%ld\n",
					__func__, __LINE__,
					(int)pimage->width, (int)pimage->height,
					(long)pimage->buffer_size,
					(long)pimage->metadata_size
					); fflush(stderr);
#endif
		}
		}
		break;
	default:
		return 0;
		break;
	}


	// STEP 2 - launch procedure ******************************************
	void (* pfunction)();
	pfunction = funcList[indexFunction].procedure;

#ifdef __SWPLUGIN_DEBUG__
	fprintf(stderr, SWPLUGIN_SIDE_PRINT "SwPluginCore::%s:%d : >>>>> Processing function %s at %p...\n",
			__func__, __LINE__,
			funcList[indexFunction].name,
			funcList[indexFunction].procedure);  fflush(stderr);
#endif

	// Process function
	struct timeval tv1, tv2;
	struct timezone tz;
	gettimeofday(&tv1, &tz);
	pfunction();
	gettimeofday(&tv2, &tz);
	swImageStruct * imout = (swImageStruct *)data_out;
	imout->deltaTus = 1000000*(tv2.tv_sec - tv1.tv_sec)
		+ (tv2.tv_usec - tv1.tv_usec);

#ifdef __SWPLUGIN_DEBUG__
	fprintf(stderr, SWPLUGIN_SIDE_PRINT ">>>>>>\tDone with processing function %s ...\n", funcList[indexFunction].name); fflush(stderr);
#endif

	// STEP 3 - reply with new data (data_out)
#ifdef __SWPLUGIN_DEBUG__
	fprintf(stderr, SWPLUGIN_SIDE_PRINT ">>>>> Sending result image...\n"); fflush(stderr);
	debugWriteImage("processfunction_out.ppm", (swImageStruct *)data_out);
#endif

	swSendImage(indexFunction, &frame, funcList[indexFunction].outputType, data_out, stdout);

	return 1;
}

#define TIMEOUT_STEP 10000
// utility to purge pipe when an invalid frame is received
void swPurgePipe(FILE *fR) {
		return;

		int c = (int)'A';
		while(c != feof(fR) && c != 'S') {
			c = fgetc(fR);
	/*		if( c != feof(fR))
				fprintf(stderr, SWPLUGIN_SIDE_PRINT "Purging char '%c'=0x%02x \n", (c>32?(char)c : '?'), (char)c);
		*/
		}
		clearerr(fR);
		ungetc(c, fR);
}


int swReadFromPipe(unsigned char * buffer, u32 size, FILE * pipe, int timeout_ms)
{
	int iter=0;
	int itermax = timeout_ms * 1000/TIMEOUT_STEP;
	u32 index = 0;
	u32 result = 0;
	while(iter < itermax && index<size) {
		iter++;

		result = fread(buffer + index, sizeof(unsigned char), (size_t)(size-index),
				 pipe);
		if(!result)
			usleep(TIMEOUT_STEP);
		else /*if(!ferror(pipe))*/ {
#ifdef __SWPLUGIN_DEBUG__
				fprintf(stderr, SWPLUGIN_SIDE_PRINT "\tread %lu/%d chars at index=%lu = 0x%02x\n", result, (size_t)(size-index), index, buffer[index]);
#endif
				index += result;
			}
	}
	if(index == size)
	{
#ifdef __SWPLUGIN_DEBUG__
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "\tRead %lu bytes\n", (ulong)index);
//		for(int k=0;k<index;k++) fprintf(stderr, SWPLUGIN_SIDE_PRINT "0x%02x ", *((unsigned char *)buffer + k));
#endif
		return 1;
	}

	fprintf(stderr, SWPLUGIN_SIDE_PRINT "!!!!!!! swReadFromPipe failed !!!! %lu < %lu (timeout=%d ms, iter= %d/%d)\n",
                (unsigned long)index, (unsigned long)size, (int)timeout_ms, (int)iter, (int)itermax);

	return 0;
}



int swWriteToPipe(unsigned char * buffer, u32 size, FILE * pipe, int timeout_ms)
{
	int iter=0;
	int itermax = timeout_ms * 1000/TIMEOUT_STEP;
	u32 index = 0;
	u32 result = 0;
	while(iter < itermax && index<size) {
		iter++;

		size_t tobewritten = (size_t)(size-index);
		if( tobewritten > BUFSIZ ) {
			tobewritten = BUFSIZ;
			iter--;
		}

		result = fwrite(buffer + index, sizeof(unsigned char), tobewritten,
				 pipe);
		fflush(pipe);

		if( tobewritten == BUFSIZ ) {
//			usleep( 5000 );
		}
		if(result==0) {
			usleep(TIMEOUT_STEP);
		} else /*if(!ferror(pipe))*/ {
#ifdef __SWPLUGIN_DEBUG__
			fprintf(stderr, SWPLUGIN_SIDE_PRINT "\t%s:%d\twrote %lu/%d chars at index=%lu = 0x%02x\n",
					__func__, __LINE__,
					 (ulong)result, (int)(size-index), (ulong)index, buffer[index]);
#endif
			if(result > 0) {
				index += result;
			} else {
				int errnum = errno;
				fprintf(stderr, SWPLUGIN_SIDE_PRINT "%s:%d : error = %d=%s\n", __func__, __LINE__,
						errnum, strerror(errnum));
			}
		}
	}


	if(index == size)
	{
#ifdef __SWPLUGIN_DEBUG__
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "\tWrote %lu bytes\n", (ulong)index);
//		for(int k=0;k<index;k++) fprintf(stderr, SWPLUGIN_SIDE_PRINT "0x%02x ", *((unsigned char *)buffer + k));
#endif
		return 1;
	}

	fprintf(stderr, SWPLUGIN_SIDE_PRINT  "!!!!!!! swWriteToPipe failed !!!! %lu < %lu (timeout=%d ms, iter= %d/%d) error=%d\n",
                (unsigned long)index, (unsigned long)size, (int)timeout_ms, (int)iter, (int)itermax, ferror(pipe));

	return 0;
}



int swReceiveImage(void * data_out, FILE * fR, int timeout_ms)
{

#ifdef __SWPLUGIN_DEBUG__

		fprintf(stderr, SWPLUGIN_SIDE_PRINT "<<<<<<< %s:%d ...\n", __func__, __LINE__);
#endif

	// read image
	char framebuffer[1024] = "";
	char * ret = NULL;
	int iter=0;
	int itermax = timeout_ms * 1000/TIMEOUT_STEP;
	while(!ret && iter < itermax) {
		iter++;
		ret = fgets(framebuffer, 1024, fR);
		if(!ret) {
			usleep(TIMEOUT_STEP);
		}
	}

	if(iter == itermax) {

		fprintf(stderr, SWPLUGIN_SIDE_PRINT "!!!!!!! %s:%d : timeout time elapsed. Cancel operation.\n", __func__, __LINE__);
		return 0;
	}

	framebuffer[1023] = '\0';
	char * header = strstr(framebuffer, SWFRAME_HEADER);
	// First arg =? SWFRAME_HEADER ??
	if( !header)
	{

		fprintf(stderr, SWPLUGIN_SIDE_PRINT "!!!!!!! %s:%d: invalid header in '%s'\n", __func__, __LINE__, framebuffer);

		// New reading
		ret = fgets(framebuffer, 1023, fR);
		if(ret>0) {

			fprintf(stderr, SWPLUGIN_SIDE_PRINT "%s:%d RETRY : '%s'\n", __func__, __LINE__, framebuffer);
			header = strstr(framebuffer, SWFRAME_HEADER);
			if(!header) {
				fprintf(stderr, SWPLUGIN_SIDE_PRINT "FAILED -> returning 0\n");
				swPurgePipe(fR);
				return 0;
			}

		} else {
			 fprintf(stderr, SWPLUGIN_SIDE_PRINT "FAILED (timeout) -> returning 0\n");
			 swPurgePipe(fR);
			 return 0;

		}
	}

	// well, header is ok
	char * command = nextTab(header);
#ifdef __SWPLUGIN_DEBUG__

	fprintf(stderr, SWPLUGIN_SIDE_PRINT "\t%s:%d: command='%s'\n", __func__, __LINE__, command);
#endif
	if(strncmp(command, SWFRAME_PROCESSRESULT, strlen(SWFRAME_PROCESSRESULT)))
	{
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "!!!!!!! %s:%d : is not image command\n", __func__, __LINE__);
		swPurgePipe(fR);
		return 0;
	}

	// read type
	char * arg = nextTab(command);
	char * arg2 = nextTab(arg);
	// jump over function index
	arg = arg2;
	arg2 = nextTab(arg);

	swType inputType = *(swType *)arg;
	if( inputType != swImage)
	{
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "!!!!!!! %s:%d: ERROR - Types do not match %d (read) != %d\n", __func__, __LINE__,
			(int)inputType, (int)swImage);
		return 0;
	}
#ifdef __SWPLUGIN_DEBUG__
	fprintf(stderr, SWPLUGIN_SIDE_PRINT "%s:%d: Input type = %d\n", __func__, __LINE__, (int)inputType); fflush(stderr);
#endif

	// ok, we've got header and
	// readimage  header
	swImageStruct * imOut = (swImageStruct *)data_out;
	unsigned char * buffer = (unsigned char *)imOut->buffer;
	unsigned char * metadata = (unsigned char *)imOut->metadata;
	if(! swReadFromPipe((unsigned char *)imOut,
		(u32) sizeof(swImageStruct),
		fR, timeout_ms )) {
		imOut->buffer = buffer;


		fprintf(stderr, SWPLUGIN_SIDE_PRINT "!!!!!!! %s:%d FAILED FOR swReceiveImage\n", __func__, __LINE__);

		swPurgePipe(fR);
		return 0;
	}
	// Restore pointers
	imOut->buffer = buffer;
	imOut->metadata = metadata;

	// debug
#ifdef __SWPLUGIN_DEBUG__

	fprintf(stderr, SWPLUGIN_SIDE_PRINT "SwPluginCore::%s:%d: received image %dx%d = buffer_size=%ld bytes metadata=%ld\n", __func__, __LINE__,
			(int)imOut->width, (int)imOut->height,
			(long)imOut->buffer_size, (long)imOut->metadata_size);
#endif

	// Read image buffer from pipe
	if(! swReadFromPipe((unsigned char *)imOut->buffer, (u32)imOut->buffer_size, fR, timeout_ms)) {

		return 0;
	}

	if(!imOut->metadata && imOut->metadata_size > 0) {

	}

	// Read metadata from pipe


#ifdef __SWPLUGIN_DEBUG__
	char file[64];
	if(fR == stdin)
		sprintf(file, "receiveimage_stdout.ppm");
	else
		sprintf(file, "receiveimage_parent.ppm");
	debugWriteImage(file, (swImageStruct *)data_out);
#endif

	return 1;
}










// directly send an image to stdout
int swSendImage(int iFunc, swFrame * frame, swType outputType, void * data_out, FILE * fW)
{
	if(!fW) return 0;

#ifdef __SWPLUGIN_DEBUG__
	fprintf(stderr, SWPLUGIN_SIDE_PRINT "%s:%d : sending func=%d\n", __func__ ,__LINE__, iFunc); fflush(stderr);
#endif


	switch (outputType) {
	case swImage: {
		// write
		swAddHeaderToFrame(frame);
		swAddSeparatorToFrame(frame);
		swAddStringToFrame(frame, SWFRAME_PROCESSRESULT);
		swAddSeparatorToFrame(frame);

		// add type
		char txt[64];
		sprintf(txt, "%d\t%c\n", iFunc, *(char *)(&outputType));
		swAddStringToFrame(frame, txt);

		// send buffer
		fwrite(frame->buffer, sizeof(unsigned char), frame->pos, fW);
		fflush(fW);

		// write image struct and buffer
		swImageStruct * pimage = (swImageStruct *)data_out;

		// send header
#ifdef __SWPLUGIN_DEBUG__
//		for(int k=0;k<sizeof(swImageStruct);k++) fprintf(stderr, SWPLUGIN_SIDE_PRINT "0x%02x ", *((unsigned char *)pimage + k));
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "&&&&&&&&&&&&&&&&&&\t%s:%d : send header :"
				 "\tSize : %dx%d"
				 "\tbuffer_size=%d metadata_size=%d metadata=%p\n",
				 __func__, __LINE__,
				 (int)pimage->width, (int)pimage->height,
				 (int)pimage->buffer_size,
				 (int)pimage->metadata_size, pimage->metadata
				 );
#endif
		fwrite(pimage, sizeof(swImageStruct), 1, fW);
		fflush(fW);


		// send image buffer
#ifdef __SWPLUGIN_DEBUG__
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "&&&&&&&&&&&&&&&&&&\t%s:%d: send buffer %d + metadata %d\n",
				__func__, __LINE__, (int)pimage->buffer_size, (int)pimage->metadata_size
				);
#endif
		if(!swWriteToPipe((unsigned char *)pimage->buffer,
						  pimage->buffer_size,
						  fW, 1000)) {
			fprintf(stderr, SWPLUGIN_SIDE_PRINT "%s:%d : write failed ! for buffer_size=%d\n",
					__func__, __LINE__, (int)pimage->buffer_size); fflush(stderr);
			return 0;
		}

		// Send metadata
		if( pimage->metadata_size > 0 && pimage->metadata) {
			if(!swWriteToPipe((unsigned char *)pimage->metadata,
							  pimage->metadata_size,
							  fW, 1000)) {
				fprintf(stderr, SWPLUGIN_SIDE_PRINT "%s:%d : write failed ! for metadata_size=%d\n",
						__func__, __LINE__, (int)pimage->metadata_size); fflush(stderr);
				return 0;
			}
		}
#ifdef __SWPLUGIN_DEBUG__
#ifdef SWPLUGIN_SIDE
fprintf(stderr, SWPLUGIN_SIDE_PRINT "PLUGIN_SIDE\t");
#endif
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "&&&&&&&&&&&&&&&&&& Done with swSendImage.\n"); fflush(stderr);

		char file[64];
		if(fW == stdout)
			sprintf(file, "sendimage_stdout.ppm");
		else
			sprintf(file, "sendimage_parent.ppm");
		debugWriteImage(file, (swImageStruct *)data_out);
#endif

		return 1;
		}
		break;
	default:
		return 0;
		break;
	}

	return 0;
}

int debugWriteImage(char * filename, swImageStruct * pimage)
{
	// debug
	FILE *f = fopen(filename, "w");
	if(!f) return 0;

	fprintf(f, "P6\n%d %d\n255\n", pimage->width, pimage->height);
	for(u32 i=0;i<pimage->buffer_size;i+=4) {
		fwrite((unsigned char *)pimage->buffer + i , sizeof(unsigned char), 3, f);
	}
	fclose(f);

	return 1;
}



int SwPluginCore::setFunctionParameters(char *frame, int //unused: len
										)
{
	//read function number
	int indexFunction = atoi(frame);
	if(indexFunction >= NbFunctions) {
		return 0;
	}

	fprintf(stderr, SWPLUGIN_SIDE_PRINT "SwPluginCore::%s:%d : Reading function # %d parameters...\n",
			__func__, __LINE__,
			indexFunction);

	// check function name
	char * arg = frame;
	char * arg2 = nextTab(frame);
	arg = arg2;
	arg2 = nextTab(arg);
	if(strncmp( arg, funcList[indexFunction].name, (int)(arg2-arg)-1) != 0)
	{
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "Function names do not match '%s' != '%s'\n", arg, funcList[indexFunction].name);
		return 0;
	}

	// check param number
	arg = arg2;
	arg2 = nextTab(arg);
	int nb = atoi(arg);
	if( nb != funcList[indexFunction].nb_params)
	{
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "Function parameters number do not match %d != %d\n", nb, funcList[indexFunction].nb_params);
		return 0;
	}


	// read parameters
	// must be name, then param 1 (name, type, value), param2...
	for(int i=0; i < funcList[indexFunction].nb_params ; i++)
	{
		swFuncParams * parlist = &(funcList[indexFunction].param_list[i]);
		// check name
		arg = arg2;
		arg2 = nextTab(arg);
		if(strncmp( arg, parlist->name, (int)(arg2-arg)-1) != 0)
		{
			fprintf(stderr, SWPLUGIN_SIDE_PRINT "Parameter do not match '%s' != '%s'\n", arg, parlist->name);
			return 0;
		}
		// check type
		arg = arg2;
		arg2 = nextTab(arg);
		if( *(swType *)arg != parlist->type)
		{
#ifdef SWPLUGIN_SIDE
fprintf(stderr, SWPLUGIN_SIDE_PRINT "PLUGIN_SIDE\t");
#endif
			fprintf(stderr, SWPLUGIN_SIDE_PRINT "Types do not match 0x%02x != 0x%02x in string command '%s'\n",
				*(swType *)arg, parlist->type, arg);
			return 0;
		}

		// read value
		arg = arg2;
		arg2 = nextTab(arg);
		swGetValueFromTypeAndString(parlist->type, arg, parlist->value);

		// debug
		char txt[2048];// may be a big list of strings
		swGetStringValueFromType(parlist->type, parlist->value, txt);
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "PLUGIN:\t\tRead parameter # %d : "
						"type=0x%02x '%s' = '%s' (after processing)\n",
						i, parlist->type, parlist->name, txt);
	}
	return 1;
}




/***************************************************************************
 * Function : formatFrame()
 * Description : format output frame
 * Author : CSE
 ***************************************************************************/
int SwPluginCore::formatFrame(char * /*unused frameDescriptor*/)
{
	// OBSOLETE
	return 1;
}

/***************************************************************************
 * Function : exportOutputToFile()
 * Description : export component output to file
 * Author : CSE
 ***************************************************************************/
int SwPluginCore::exportOutputToFile(char * /*unused filename*/)
{

	return 1;
}

/***************************************************************************
 * Function : exportOutputToStdout()
 * Description : export component output to standard output
 * Author : CSE
 ***************************************************************************/
int SwPluginCore::exportOutputToStdout()
{

	return 1;
}

/***************************************************************************
 * Function : exportParameters()
 * Description : export parameters to file
 * Author : CSE
 ***************************************************************************/
int SwPluginCore::exportParameters()
{
/*	FILE *f;
	char line[512] = "#";


	// Open configuration file in default directory or in current firectory
	if(( f = swFopen(SW_name_CFGFILE, "w")))
	{
		int len = 1;
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "WRITING " SW_name_CFGFILE " CONFIGURATON FILE...\n");

		// Write header
		fprintf(f, SW_name_CFGFILE_HEADER);

		// Write each parameter




		// Writing end of file
		fprintf(f, SW_CFGFILE_END);

		return 1;
	}
*/
	return 0;
}




/***************************************************************************
 * Function : readConfigurationFile()
 * Description : read configuration file
 * Author : CSE
 ***************************************************************************/
int SwPluginCore::readConfigurationFile()
{
/*	FILE *f;
	char line[512] = "#";


	// Open configuration file in default directory or in current firectory
	if(( f = swFopen(SW_name_CFGFILE, "r")))
	{
		int len = 1;
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "READING " SW_name_CFGFILE " CONFIGURATON FILE...\n");
		while( ! feof(f) && len>0)
		{
			// Reading each lines
			while( (line[0] == '#' || line[0] == '\n') && !feof(f))
			{
				fgets( line, 512 * sizeof(char), f );
			}
			len = strlen(line);
			if(len>0)
			{
				line[len-1] = '\0'; // clear the line feed

				// ok, we've got a line, lets split it at space or tab
				int j=0,i;
				for(i=0; i<len && (line[i] != ' ' && line[i] != '\t'); i++) { j++;}
				for(i=j; i<len && (line[i] == ' ' || line[i] == '\t'); i++) { line[i] = '\0'; }

				// get the command and the parameter
				char * pt = line + i;

				fprintf(stderr, SWPLUGIN_SIDE_PRINT "Command = '%s' - Param = '%s'\n", line, pt);

				// analysis of command and order

				// GENERIC SECTOIN
				if(strcmp(line, "maskImage") == 0)
				{
					fprintf(stderr, SWPLUGIN_SIDE_PRINT "\tReading detection mask image '%s'\n", pt);

				}

				// COMPONENT SPECIFIC SECTON
				if(strcmp(line, "areaMaskImage") == 0)
				{
					fprintf(stderr, SWPLUGIN_SIDE_PRINT "\tReading speed mask image '%s'\n", pt);
				}

				// Reinit line for comment
				line[0] = '#';
			}
		}
	}
	else
		return 0;
*/
	return 1;
}


/*****************************************************************************
						UTILITY FUNCTIONS
 *****************************************************************************/

 // ************************ FILE SECTION ************************
// find next tab in a char string and return next char after tab
char * nextTab( char * frame )
{
	char * cur = frame;

	while( *cur != '\t' && *cur != '\n' && *cur != '\0')
	{
		cur++;
	}
	cur++; //cur now points to the beginning of the data
	return cur;
}

// find next tab in a char string and return next char after tab
char * nextFileSeparator( char * frame )
{
	int i=0;
	char * cur = frame;

	while( *cur != '\t' && *cur != ' ' && *cur != '\n')
	{
		cur++;
		i++;
	}
	*cur = '\0';
	do {
		cur++; //cur now points to the beginning of the data
	} while( *cur ==' ' || *cur == '\t' );
	return cur;
}



FILE * swFopen(char * file, char *mode)
{
	FILE *f = NULL;
	char name[512];

	strcpy(name, getenv("HOME"));
	if(name[0] != '\0') {
		if(name[strlen(name) - 1] == '/')
			name[strlen(name) - 1] = '\0';
		sprintf(name, "%s/.sworkshop/%s", name, file);
		if( (f = fopen(name, mode)))
			return f;
	}

	f = fopen(file, mode);
	return f;
}



// ************************ FRAME FOR COMMUNICATOIN SECTION ************************
int swAllocateFrame(swFrame *current, int size)
{
	current->pos = 0;
	if( ! (current->buffer = new char [size]))
	{
	   current->maxlen = 0;
	   return 0;
	}
	current->maxlen = size-1;
	return 1;
}

int swAddHeaderToFrame(swFrame * current)
{
	current->pos = 0;
	if(current->maxlen < (long)strlen(SWFRAME_HEADER)) return 0;
	strcpy(current->buffer, SWFRAME_HEADER);
	current->pos = strlen(SWFRAME_HEADER);

	return 1;
}

int swAddStringToFrame(swFrame *current, char *txt)
{
	if( (current->pos + (int)strlen(txt)) >= current->maxlen) return 0;

	memcpy( current->buffer + current->pos, txt, strlen(txt));
	current->pos += strlen(txt);
	*(current->buffer + current->pos) = '\0';
	return 1;
}

int swAddBufferToFrame(swFrame *current, void * buffer, int size)
{
	if( current->pos + size >= current->maxlen) return 0;

	memcpy(current->buffer + current->pos, buffer, size);
	current->pos += size;
	*(current->buffer + current->pos) = '\0';
	return 1;
}


// Add an image to frame
int swAddImageToFrame(swFrame *current, swImageStruct * image)
{
	if( current->pos + 2 + (int) sizeof(swImageStruct) + (int)image->buffer_size >= current->maxlen) return 0;
	char txt[3];

	// add descriptor
	sprintf(txt, "%c\t", (char)swImage);
	swAddStringToFrame(current, txt);

	// write struct descriptor
	memcpy(current->buffer + current->pos, image, sizeof(swImageStruct));
	// (we wrote address of buffer, which won't be used)

	// write image buffer
	memcpy(current->buffer + current->pos + sizeof(swImageStruct),
		image->buffer, image->buffer_size);

	current->pos += sizeof(swImageStruct) + image->buffer_size;

	return 1;
}

// Add an 1D measure to frame
int swAddMeasure1DToFrame(swFrame *current, swMeasure1DStruct * measure)
{
	if( current->pos + 2 + (int) sizeof(swMeasure1DStruct) + (int)measure->buffer_size >= current->maxlen) return 0;
	char txt[3];

	// add descriptor
	sprintf(txt, "%c\t", (char)swMeasure1D);
	swAddStringToFrame(current, txt);

	// write struct descriptor
	memcpy(current->buffer + current->pos, measure, sizeof(swMeasure1DStruct));
	// (we wrote address of buffer, which won't be used)

	// write image buffer
	memcpy(current->buffer + current->pos + sizeof(swMeasure1DStruct),
		measure->buffer, measure->buffer_size);

	current->pos += sizeof(swMeasure1DStruct) + measure->buffer_size;

	return 1;
}


//



int swAddSeparatorToFrame(swFrame *current)
{
	if( current->pos + 1 >= current->maxlen) return 0;
	current->buffer[current->pos] = '\t';
	current->pos++;
	*(current->buffer + current->pos) = '\0';
	return 1;
}

int swCloseFrame(swFrame * current)
{
	return swAddStringToFrame(current, SWFRAME_END "\n");
}

int swSendFrame(swFrame *current)
{
	// for the moment, we just print its content
	if( fwrite(current->buffer, sizeof(unsigned char), current->pos, stdout) != (size_t)current->pos)
	{
		fprintf(stderr, SWPLUGIN_SIDE_PRINT "!!!!!!! Could not write %d bytes.", current->pos);
	}
	fflush(stdout);

	return 1;
}

int swFreeFrame(swFrame * current)
{
	if(current->buffer && current->maxlen>0)
		delete [] current->buffer;
	return 1;
}



///////////////////////////////////////////////////////////////////////////


int swGetTypeSize(unsigned char type)
{
	int dlen = 0;
	switch(type)
	{
	case swU8:
	case swS8:
		dlen = 1;
		break;
	case swU16:
	case swS16:
		dlen = 1;
		break;
	case swU32:
	case swS32:
		dlen = 1;
		break;
	case swFloat:
		dlen = sizeof( float );
		break;
	case swDouble:
		dlen = sizeof( double );
		break;
	default :
		dlen = 0;
		break;
	}
	return dlen;
}


int swGetByteSizeFromType(swType type)
{
	int dlen = 0;
	switch(type)
	{
	case swU8:
	case swS8:
		dlen = sizeof(char);
		break;
	case swU16:
	case swS16:
		dlen = sizeof(short);
		break;
	case swU32:
	case swS32:
		dlen = sizeof(long);
		break;
	case swFloat:
		dlen = sizeof( float );
		break;
	case swDouble:
		dlen = sizeof( double );
		break;
	default :
		dlen = 0;
		break;
	}
	return dlen;
}

int swAllocValueFromType(swType type, void ** value)
{
	if(*value) return 0;
	switch(type)
	{
	case swU8:
		*value = new unsigned char;
		break;
	case swS8:
		*value = new char;
		break;
	case swU16:
		*value = new unsigned short;
		break;
	case swS16:
		*value = new short;
		break;
	case swU32:
		*value = new u32;
		break;
	case swS32:
		*value = new char;
		break;
	case swFloat:
		*value = new float;
		break;
	case swDouble:
		*value = new double;
		break;
	case swStringList: {
		*value = new swStringListStruct;
		swStringListStruct *s= (swStringListStruct *)*value;
		s->list = NULL;
		s->nbitems = 0;
		}
		break;
	default:
		return 0;
		break;
	}
	return 1;
}

int swGetValueFromType(swType type, // type
	void * val, // pointer to value
	void * value // pointer to value output
	)
{
	if(!value)
		swAllocValueFromType(type, &value);
	switch(type) {
	default:
		memcpy(value, val, swGetByteSizeFromType(type));
		break;
	}
	return 1;
}

char *nextPipe(char *txt) {
	char *pt = txt;
	while(*pt != '|' && *pt != '\t' && *pt != '\n' && *pt != '\0')
	{
		pt++;
	}
	if(*pt == '\0')
		return NULL;
	pt++;
	return pt;
}


int swGetValueFromTypeAndString(swType type, // type
	char * val, // pointer to value
	void * value // pointer to value output
	)
{
	if(! value)
		return  0;

	switch(type)
	{
	case swU8: {
		int d;
		sscanf(val, "%d", &d);
		if(d<0 || d>255) return 0;
		*(unsigned char *)value = (unsigned char)d;
		}
		break;
	case swS8: {
		int d;
		sscanf(val, "%d", &d);
		if(d<-128 || d>127) return 0;
		*(char *)value = (char)d;
		}
		break;
	case swU16: {
		unsigned short d;
		sscanf(val, "%hu", &d);
		*(u16 *)value = (unsigned short)d;
		}
		break;
	case swS16: {
		int d;
		sscanf(val, "%d", &d);
		*(i16 *)value = d;
		}
		break;
	case swU32: {
		unsigned long d;
		sscanf(val, "%lu", &d);
		*(u32 *)value = d;
		}
		break;
	case swS32: {
		long d;
		sscanf(val, "%ld", &d);
		*(i32 *)value = d;
		}
		break;
	case swFloat: {
		float d;
		sscanf(val, "%f", &d);
		*(float *)value = d;
		}
		break;
	case swDouble: {
		double d;
		sscanf(val, "%lf", &d);
		*(double *)value = d;
		}
		break;
	case swStringList: { // separators are pipes '|'
		// read item count

		swStringListStruct * s=(swStringListStruct *)value;
		sscanf(val, "%d", &(s->nbitems));

		//alloc
		s->list = new char * [ s->nbitems ];

		// cur pos
		char * arg = nextPipe(val);
		sscanf(arg, "%d", &(s->curitem));

		char * p = nextPipe(arg);
		for(int i=0; p && i<s->nbitems; i++)
		{
			char * p2 = nextPipe(p);
			if(p) {
				int len;
				if(p2) len = (int)(p2-p)-1;
				else len = strlen(p);

				s->list[i] = new char [len+1];
				memcpy(s->list[i], p, len);
				*(s->list[i]+len) = '\0';
			}
			p = p2;
		}
		}
		break;
	default:
		return 0;
		break;
	}
	return 1;
}



int swGetStringValueFromType(swType type, // type
	void * value, // pointer to value
	char * txt)
{
	if(!value)
		return 0;

	switch(type)
	{
	case swU8:
		sprintf(txt, "%d", (int)*(unsigned char *)value);
		break;
	case swS8:
		sprintf(txt, "%d", (int)*(char *)value);
		break;
	case swU16:
		sprintf(txt, "%d", (int)(*(u16 *)value));
		break;
	case swS16:
			sprintf(txt, "%d", (int)(*(i16 *)value));
		break;
	case swU32: {
			sprintf(txt, "%lu", (unsigned long)(*(u32*)value));
		}break;
	case swS32:
		sprintf(txt, "%ld", (long)( *(i32 *)value));
		break;
	case swFloat:
		sprintf(txt, "%g", *(float *)value);
		break;
	case swDouble:
		sprintf(txt, "%g", *(double *)value);
		break;
	case swStringList: {
		swStringListStruct *s =(swStringListStruct *)value;
		sprintf(txt, "%d|%d", s->nbitems, s->curitem);
		for(int i=0; i< s->nbitems;i++)
			sprintf(txt, "%s|%s", txt, s->list[i]);
		sprintf(txt, "%s", txt);
		}
		break;

	default:
		return 0;
		break;
	}
	return 1;
}




int swAddScalarToDescriptor(swFrame *current, char *name, unsigned char type, char *value)
{
	if( ! swAddStringToFrame(current, name)) 	return 0;
	if( ! swAddSeparatorToFrame(current)) 		return 0;
	if( ! swAddStringToFrame(current, "S")) 	return 0;

	current->buffer[current->pos] = type;
	current->pos ++;
	if( ! swAddStringToFrame(current, value)) 	return 0;
	if( ! swAddSeparatorToFrame(current)) 		return 0;
	return 1;
}

int swAddBooleanToDescriptor(swFrame *current,
	char *name,
	unsigned char , //unused type,
	bool value)
{
	if( ! swAddStringToFrame(current, name)) 	return 0;
	if( ! swAddSeparatorToFrame(current)) 		return 0;
	if( ! swAddStringToFrame(current, "B")) 	return 0;
	if(value) {
		if( ! swAddStringToFrame(current, "T")) 	return 0;
	} else {
		if( ! swAddStringToFrame(current, "F")) 	return 0;
	}
	if( ! swAddSeparatorToFrame(current)) 		return 0;
	return 1;
}

int swAddListToDescriptor(swFrame *current, char *name,
	char ** itemnames,
	unsigned char , //unused type,
	char ** values, int nb_item)
{
	if( ! swAddStringToFrame(current, name)) 	return 0;
	if( ! swAddSeparatorToFrame(current)) 		return 0;
	if( ! swAddStringToFrame(current, "L")) 	return 0;

	for(int i=0; i<nb_item; i++)
	{
		if( ! swAddStringToFrame(current, itemnames[i])) 	return 0;
		if( ! swAddSeparatorToFrame(current)) 		return 0;
		if( ! swAddStringToFrame(current, values[i])) 	return 0;
		if( ! swAddSeparatorToFrame(current)) 		return 0;
	}

	return 1;
}



int swTypeMaxTextLength(swType type)
{
	int dlen = 32;
	switch(type)
	{
	case swU8:
	case swS8:
		dlen = 5; // max = "0xFF\0"
		break;
	case swU16:
	case swS16:
		dlen = 7; // max = "0xFFFF\0"
		break;
	case swU32:
	case swS32:
		dlen = 32;
		break;
	case swFloat:
		dlen = 32;
		break;
	case swDouble:
		dlen = 32;
		break;
	}
	return dlen;
}
