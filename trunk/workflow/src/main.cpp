/***************************************************************************
 *  main.cpp - Main for piaf workflow
 *
 *  Wed Jun 11 08:47:41 2009
 *  Copyright  2009  Christophe Seyve
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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <QtGui/QApplication>
#include "emamainwindow.h"
#include <QSplashScreen>
#include <QTranslator>

extern QSplashScreen * g_splash;

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QPixmap pixmap(":/icons/ema-splash.png");
	g_splash = new QSplashScreen(pixmap, Qt::WindowStaysOnTopHint);
	g_splash->show();

	QTranslator tor;
	QLocale localLang;
	QDir dir(QApplication::applicationDirPath());
	QString g_application_path = QApplication::applicationDirPath();
	QString translationFile = QString("piafworkflow_") +
							  localLang.languageToString(localLang.language()) +
							  QString(".qm");
	fprintf(stderr, "Translation file='%s'\n", translationFile.ascii());
	tor.load( translationFile,
			  dir.absolutePath() );

	a.installTranslator( &tor );


	a.processEvents();

	g_splash->showMessage(QObject::tr("Starting GUI..."), Qt::AlignBottom | Qt::AlignHCenter);

	EmaMainWindow w;
	a.setMainWidget(&w);
	w.show();
	g_splash->finish(&w);
	return a.exec();
}
