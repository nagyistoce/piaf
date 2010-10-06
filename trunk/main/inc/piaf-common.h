/***************************************************************************
				piaf-common.h  - Piaf common header
	common macros and definitions for Piaf GUI
							 -------------------
	begin                : Thu July 5 2010
	copyright            : (C) 2010 Christophe Seyve (CSE)
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

#ifndef PIAFCOMMON_H
#define PIAFCOMMON_H



#define EXIT_ON_ERROR fprintf(stderr,"%s (%s:%d) : EXIT - FATAL ERROR !!!!!!!!!!!!!\n",__func__,__FILE__,__LINE__);fflush(stderr);exit(0);
#define DEBUG_ALL(s) fprintf(stderr, "DEBUG_MSG %s (%s l.%d) '%s'\n",__func__,__FILE__,__LINE__, (s));
#define PRINT_FIXME fprintf(stderr, "%s : %s:%d FIXME\n", __FILE__, __func__, __LINE__);

// MEMORY ALLOCATION MACROS
#define CPP_DELETE(buf) {if((buf)) delete (buf); (buf)=NULL;}
#define CPP_DELETE_ARRAY(buf) {if((buf)) delete [] (buf); (buf)=NULL;}
#define CPP_DELETE_DOUBLE_ARRAY(buf,nb) {for(int i=0;i<(nb);i++) if(buf[i]) delete [] buf[i]; \
		if((buf)) delete [] (buf); (buf)=NULL;}

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif


#endif // PIAFCOMMON_H
