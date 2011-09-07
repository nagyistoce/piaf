
#include <QtGui/QApplication>
#include "emamainwindow.h"
#include <QSplashScreen>
extern QSplashScreen * g_splash;

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QPixmap pixmap(":/icons/icons/ema-splash.png");
	g_splash = new QSplashScreen(pixmap);
	g_splash->show();
	a.processEvents();

	g_splash->showMessage(QObject::tr("Starting GUI..."), Qt::AlignBottom | Qt::AlignHCenter);

	EmaMainWindow w;
	w.show();
//	g_splash->finish(&w);
	return a.exec();
}
