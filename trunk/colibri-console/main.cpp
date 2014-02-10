/***************************************************************************
 *  colibri-console
 *	main.cpp - Console app processing a sequece on a given device
 *
 *  2013-12-18 21:55
 *  Copyright  2013  Christophe Seyve
 *  Email cseyve@free.fr
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <QtCore/QCoreApplication>
#include "colibrithread.h"
#include "piaf-common.h"


ColibriThread g_colibriThread;

int g_debug_piaf = SWLOG_DEBUG;


/* handle the signals so it can work with :
- plugins which crashed at loading
- Ctrl+C to stop the player
*/
void signalhandler(int sig)
{
	fprintf(stderr, "================== RECEIVED SIGNAL %d = '%s' From process %d ==============\n",
			sig, sys_siglist[sig], getpid());
	signal(sig, signalhandler);
	if(sig != SIGUSR1 && sig != SIGPROF && sig != SIGPIPE) {
		exit(0);
	}
}


int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		fprintf(stderr, "Usage: ./colibri-console "
				"--seq path_to_sequence.flist "
				"[--device /dev/video0]\n");
		return -1;
	}

	for(int i=0; i<NSIG; i++) {
		signal(i, signalhandler);
	}

	QCoreApplication a(argc, argv);
	char device[128]="/dev/video0";
	int device_mode = V4L2_DEVICE;
	char sequencepath[1024]="";
	for(int arg=0; arg<argc; arg++)
	{
		COLIBRI_MSG("Arg[%d]='%s'", arg, argv[arg]);
		if(strcasecmp(argv[arg], "--device")==0)
		{
			if(arg < argc-1)
			{
				strcpy(device, argv[argc+1]);
				COLIBRI_MSG("Reading device name in '%s'",
							argv[arg+1]);
			}
			else
			{
				COLIBRI_MSG("ERROR: missing argument");
			}
		}
		if(strcasecmp(argv[arg], "--seq")==0)
		{
			if(arg+1 < argc)
			{
				strcpy(sequencepath, argv[arg+1]);
				COLIBRI_MSG("Reading sequence path in '%s'",
							sequencepath);
			}
			else
			{
				COLIBRI_MSG("ERROR: missing argument");
			}
		}

	}
	if(strlen(sequencepath) == 0)
	{
		// use the argv[1]
		if(argc>=2 && argv[1])
		{
			strcpy(sequencepath, argv[1]);
		}
	}
	COLIBRI_MSG("Starting acquisition on device '%s' sequence='%s'...",
				device, sequencepath);
	
	g_colibriThread.setSequenceFile(sequencepath);

	int retry = -1;
	CvCapture * capture = NULL;
	do {
		
		capture = cvCreateCameraCapture(retry);
		retry++;
	} while(retry < 5 && !capture);

	g_colibriThread.setCapture(capture);

	COLIBRI_MSG("Starting thread ...");
	g_colibriThread.start();

//	computeImage(IplImage * iplImage) {
//		if(!iplImage) return;

//	#ifndef WIN32
//	//	fprintf(stderr, "[Colibri] %s:%d : process sequence '%s' on image (%dx%dx%d)\n",
//	//			__func__, __LINE__,
//	//			mFilterManager.getPluginSequenceFile(),
//	//			iplImage->width, iplImage->height, iplImage->depth
//	//			);

//		// Process this image with filters
//		int retproc = mFilterManager.processImage(iplImage, &mOutputImage);
//	#else
//		/// \todo port to Win32
//	#endif
//		// Display image as output
//		if(retproc > 0 && mOutputImage)
//		{
//			displayImage(mOutputImage);
//		}
//		else
//		{
//			displayImage(iplImage);
//		}
//	}

	return a.exec();
}
