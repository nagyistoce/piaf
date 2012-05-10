#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include "SwPluginCore++.h"

int
SwPluginCore2::treatFrame(char * framebuffer, int framesize)
{
	char *header = strstr(framebuffer, SWFRAME_HEADER);
	if(!header)
	{
		fprintf(stderr, "!!!!!!! SwPluginCore: Invalid header in treatFrame()\n");
		fflush(stderr);
		swPurgePipe(stdin);
		return 0;
	}

	char *command = header + strlen(SWFRAME_HEADER) + 1;
	char *arg = nextTab( command );

	// category ??
	if(strncmp(command, SWFRAME_ASKCATEGORY, strlen(SWFRAME_ASKCATEGORY)) == 0)
	{
		sendCategory();
		return 1;
	}

	// Function list ??
	if(strncmp(command, SWFRAME_ASKFUNCLIST, strlen(SWFRAME_ASKFUNCLIST)) == 0)
	{
		sendFunctions();
		return 1;
	}
	//	quit command
	if(strncmp(command, SWFRAME_QUIT, strlen(SWFRAME_QUIT)) == 0)
	{
		fprintf(stderr, "Quitting...\n");
		fflush(stderr);
		quit = true;
		return 1;
	}

	//	Function descriptor ??
	if(strncmp(command, SWFRAME_ASKFUNCTIONDESC, strlen(SWFRAME_ASKFUNCTIONDESC)) == 0)
	{
       	int nb = atoi(arg);
        if(nb > NbFunctions)
			return 0;
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

char *
SwPluginCore2::sendFunctions()
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
		swAddStringToFrame(&frame, plugs[i]->_pluginName);
		swAddSeparatorToFrame(&frame);
	}

	// close and send
	swCloseFrame(&frame);
	swSendFrame(&frame);

	// return nb of functions
	return NULL;
}

int
SwPluginCore2::sendFunctionDescriptor(int funcnum)
{
	if(funcnum>=NbFunctions)
		return 0;

	swAddHeaderToFrame(&frame);
	swAddSeparatorToFrame(&frame);
	swAddStringToFrame(&frame, SWFRAME_SETFUNCTIONDESC);
	swAddSeparatorToFrame(&frame);

	// send func number, name, number of parameters
	char txt[2048];
	sprintf(txt, "%d\t%s\t%d\t",
	        funcnum,
	        plugs[funcnum]->_pluginName,
	        plugs[funcnum]->_pluginVars.size()
	       );
	swAddStringToFrame(&frame, txt);

	// send each parameter
	for(int j=0; j< plugs[funcnum]->_pluginVars.size(); j++)
	{
		// format : param name \t type as uchar \t value as string
		sprintf(txt, "%s\t%c\t",
		        plugs[funcnum]->_pluginVars[j].name,
		        (char)plugs[funcnum]->_pluginVars[j].type);

		switch(plugs[funcnum]->_pluginVars[j].type)
		{
		case swU8:
			sprintf(txt, "%s%u\t", txt,
			        (int)*(unsigned char *)plugs[funcnum]->_pluginVars[j].var);
			break;
		case swS8:
			sprintf(txt, "%s%d\t", txt,
			        (int)*(char *)plugs[funcnum]->_pluginVars[j].var);
			break;
		case swU16:
			sprintf(txt, "%s%u\t", txt,
			        (unsigned short)*(u16 *)plugs[funcnum]->_pluginVars[j].var);
			break;
		case swS16:
			sprintf(txt, "%s%d\t", txt,
			        (short)*(i16 *)plugs[funcnum]->_pluginVars[j].var);
			break;
		case swU32:
			sprintf(txt, "%s%lu\t", txt,
			       (unsigned long) *(u32 *)plugs[funcnum]->_pluginVars[j].var);
			break;
		case swS32:
			sprintf(txt, "%s%ld\t", txt,
			        (long)*(i32 *)plugs[funcnum]->_pluginVars[j].var);
			break;
			// floats
		case swFloat:
			sprintf(txt, "%s%g\t", txt,
			        *(float *)plugs[funcnum]->_pluginVars[j].var);
			break;
		case swDouble:
			sprintf(txt, "%s%g\t", txt,
			        *(double *)plugs[funcnum]->_pluginVars[j].var);
			break;
		case swStringList:
			{
				swStringListStruct *s =(swStringListStruct *)plugs[funcnum]->_pluginVars[j].var;
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

int SwPluginCore2::processFunction(char *framebuffer, int )//unused len)
{
	// read function number, data type, data buffer
	int indexFunction = atoi(framebuffer);
	if(indexFunction >= NbFunctions)
		return 0;

	// read data type
	char * arg = framebuffer;
	char * arg2 = nextTab(arg);
	arg = arg2;
	arg2 = nextTab(arg);

	swType inputType = *(swType *)arg;
	if( inputType != swImage)
	{
		fprintf(stderr, ">>>>> SwPluginCore::processFunction: ERROR - Types do not match %d != %d\n",
		        (int)inputType, (int)funcList[indexFunction].inputType);
		return 0;
	}

	// read data
	swImageStruct tmpStruct;
	int timeout_ms = 100;
	if(! swReadFromPipe((unsigned char *)&tmpStruct, (unsigned long)sizeof(swImageStruct), stdin, timeout_ms))
	{
		swPurgePipe(stdin);
		return 0;
	}

	// allocate buffer for
	int size = sizeof(swImageStruct) + tmpStruct.buffer_size;
	swImageStruct * pimage_in = (swImageStruct *)data_in;
	if(!data_in || size_in != size)
	{
		if(data_in)
			delete [] data_in;
		data_in = new unsigned char [ size ];
		size_in = size;
		pimage_in = (swImageStruct *)data_in;
		fprintf(stderr, "MMMMMMM Realloc data_in [ %d ] \n", size);
		fflush(stderr);
	}

	memcpy(pimage_in, &tmpStruct, sizeof(swImageStruct));
	pimage_in->buffer = (unsigned char *)data_in + sizeof(swImageStruct); //restore pointer

	// read info from stdin to buffer

	if(!swReadFromPipe((unsigned char *)pimage_in->buffer, (unsigned long)pimage_in->buffer_size, stdin, timeout_ms))
	{
		swPurgePipe(stdin);
		return 0;
	}

	// check if output is allocated

	// data is from this format : swImageStruct as binary field, then buffer
	// we must read image struct to get buffer size, then read size

	// allocate buffer for data_out
	swImageStruct * pimage_out = (swImageStruct *)data_out;
	if(!data_out || size_out != size_in)
	{
		if(data_out)
			delete [] data_out;
		data_out = new unsigned char [ size_in ];
		size_out = size_in;
		pimage_out = (swImageStruct *)data_out;

		fprintf(stderr, "MMMMMMM Realloc data_out [ %d ] \n", size_out);
		fflush(stderr);

		memcpy(data_out, data_in, sizeof(swImageStruct));
		pimage_out->buffer = (unsigned char *)data_out + sizeof(swImageStruct);
	}

	plugs[indexFunction]->imIn = (swImageStruct*)data_in;
	plugs[indexFunction]->imOut = (swImageStruct*)data_out;

	// STEP 2 - launch procedure ******************************************
	// Process function
	struct timeval tv1, tv2;
	struct timezone tz;
	gettimeofday(&tv1, &tz);
	plugs[indexFunction]->proc();
	gettimeofday(&tv2, &tz);
	swImageStruct * imout = (swImageStruct *)data_out;
	imout->deltaTus = 1000000*(tv2.tv_sec - tv1.tv_sec)
	                  + (tv2.tv_usec - tv1.tv_usec);
	// STEP 3 - reply with new data (data_out)
	swSendImage(indexFunction, &frame, swImage, data_out, stdout);

	return 1;
}

int SwPluginCore2::setFunctionParameters(char *frame, int )//len)
{
	//read function number
	int indexFunction = atoi(frame);
	if(indexFunction >= NbFunctions)
		return 0;

	fprintf(stderr, "PLUGIN : Reading function # %d parameters...\n",
	        indexFunction);

	// check function name
	char * arg = frame;
	char * arg2 = nextTab(frame);
	arg = arg2;
	arg2 = nextTab(arg);
	if(strncmp( arg, plugs[indexFunction]->_pluginName, (int)(arg2-arg)-1) != 0)
	{
		fprintf(stderr, "Function names do not match '%s' != '%s'\n", arg, funcList[indexFunction].name);
		return 0;
	}

	// check param number
	arg = arg2;
	arg2 = nextTab(arg);
	int nb = atoi(arg);
	if( nb != plugs[indexFunction]->_pluginVars.size())
	{
		fprintf(stderr, "Function parameters number do not match %d != %d\n", nb, funcList[indexFunction].nb_params);
		return 0;
	}


	// read parameters
	// must be name, then param 1 (name, type, value), param2...
	for(int i=0; i < plugs[indexFunction]->_pluginVars.size() ; i++)
	{
		//swFuncParams * parlist = &(funcList[indexFunction].param_list[i]);
		PiafVar* parlist = &plugs[indexFunction]->_pluginVars[i];
		// check name
		arg = arg2;
		arg2 = nextTab(arg);
		if(strncmp( arg, parlist->name, (int)(arg2-arg)-1) != 0)
		{
			fprintf(stderr, "Parameter do not match '%s' != '%s'\n", arg, parlist->name);
			return 0;
		}
		// check type
		arg = arg2;
		arg2 = nextTab(arg);
		if( *(swType *)arg != parlist->type)
		{
			fprintf(stderr, "Types do not match 0x%02x != 0x%02x in string command '%s'\n",
			        *(swType *)arg, parlist->type, arg);
			return 0;
		}

		// read value
		arg = arg2;
		arg2 = nextTab(arg);
		swGetValueFromTypeAndString(parlist->type, arg, parlist->var);

		// debug
		char txt[64];
		swGetStringValueFromType(parlist->type, parlist->var, txt);
		fprintf(stderr, "PLUGIN:\t\tRead parameter # %d : "
		        "type=0x%02x '%s' = '%s' (after processing)\n",
		        i, parlist->type, parlist->name, txt);
	}
	plugs[indexFunction]->onParameterChanged();
	return 1;
}
