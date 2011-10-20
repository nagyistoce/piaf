/***************************************************************************
						  main.cpp  -  description
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

// signal handling
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>


#include <qapplication.h>
#include <qfont.h>
#include <qstring.h>
#include <qtextcodec.h>
#include <qtranslator.h>

#include "workshop.h"
#include "SwFilters.h"


int main(int argc, char *argv[])
{
/*	// signals handling
	for(int i=0; i<NSIG; i++)
		signal(i, signalhandler);
*/
	struct sigaction act;
#ifdef SA_RESTART
	act.sa_flags = SA_RESTART;
#else
	act.sa_flags = 0;
#endif

	act.sa_handler = sigchld;
	if(sigaction(SIGCHLD, &act, NULL) == -1)
		perror("sigaction(SIGCLHD)");
	act.sa_handler = sigpipe;
	if(sigaction(SIGPIPE, &act, NULL) == -1)
		perror("sigaction(SIGPIPE)");
	act.sa_handler = sigusr1;
	if(sigaction(SIGUSR1, &act, NULL) == -1)
		perror("sigaction(SIGUSR1)");

	QApplication a(argc, argv);
	//a.setFont(QFont("helvetica", 12));
	QTranslator tor( 0 );

	// set the location where your .qm files are in load() below as the last parameter instead of "."
	// for development, use "/" to use the english original as
	// .qm files are stored in the base project directory.
	QLocale localLang;

#ifdef xxLINUX
	g_application_path = QString("/usr/share/piaf");
	QDir dir("/usr/share/piaf");
#else
	QDir dir(QApplication::applicationDirPath());
	QString g_application_path = QApplication::applicationDirPath();
#endif
	QString translationFile = QString("piaf_") +
							  localLang.languageToString(localLang.language()) +
							  QString(".qm");
	fprintf(stderr, "Translation file='%s'\n", translationFile.ascii());
	tor.load( translationFile,
			  dir.absolutePath() );

	a.installTranslator( &tor );
	WorkshopApp * workshop = new WorkshopApp();
	a.setMainWidget(workshop);


	workshop->show();

	/*
	if(argc>1)
	  workshop->openDocumentFile(argv[1]);
	else
	  workshop->openDocumentFile();
	*/
	return a.exec();
}
