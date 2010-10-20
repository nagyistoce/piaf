/***************************************************************************
      videoplayertool.h  -  MJPEG video player for Sisell applications
                             -------------------
    begin                : April 2003
    copyright            : (C) 2003 by Christophe SEYVE 
    email                : christophe.seyve@sisell.com
 ***************************************************************************/

#ifndef VIDEOPLAYERTOOL_H
#define VIDEOPLAYERTOOL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Qt's librairies
#include <qwidget.h>
#include <qworkspace.h>
#include <qobject.h>
#include <q3listview.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <q3mainwindow.h>
#include <q3iconview.h>
#include <q3vbox.h>
#include <qtimer.h>
#include <qlabel.h>
#include <qtabwidget.h>
#include <q3ptrlist.h>
#include <q3datetimeedit.h>
#include <qcombobox.h>
#include <qscrollbar.h>
#include <qthread.h>
#include <qmutex.h>
#include <qwaitcondition.h>
//Added by qt3to4:
#include <QResizeEvent>


#include "FileVideoAcquisition.h"

#include "workshopmovie.h"
#include "workshoptool.h"
#include "workshopimagetool.h"

class WorkshopMovie;
/** \brief Workshop videos display and processing tool

	Enables to display (zoom/move) image, navigate (VCR style), and advanced 
	options such as snapshot and plugin manager handling.

	\author Christophe SEYVE \mail christophe.seyve@sisell.com
*/

class VideoPlayerTool : public WorkshopTool
{
    Q_OBJECT
    
public:
	VideoPlayerTool(QWidget *parent, 
		const char *name, Qt::WidgetAttribute wflags);
  	~VideoPlayerTool();
	int setFile(char * file, int period);
	WorkshopImageTool * imageTool() { return detailsView; };
	int setWorkshopMovie(WorkshopMovie * wm);
	int setDefaultFPS(float fps) { defaultFPS = fps; return 1;};
	/** Set default directories for images and movies */
	void setDefaultDirectories(const QString & image_dir, const QString & movie_dir) {
		if(detailsView) 
			detailsView->setDefaultDirectories(image_dir, movie_dir);
		
		imageDir = image_dir;
		movieDir = movie_dir;
	};
	virtual void run();
private: 
	QMutex mutex;
	QWaitCondition waitCondition;
	bool playContinuous;
	// movie controls
	char VideoFile[512];
	
	// default image directory
	QString imageDir; 
	// default movie directory
	QString movieDir;
	
	QTimer * playTimer;
	QPushButton * playMovie;
	QPushButton * stepMovie;
	QPushButton * rewindMovie;
	QPushButton * rewindStartMovie;
	QPushButton * grayButton;

	bool playGrayscale;

	Q3HBox * playHBox;
	QLabel * playLabel;

	/// Large number for size in bytes
	unsigned long long playFileSize;
	int playFrame;
	float playFPS;
	float defaultFPS;
	float playSpeed;
	
	unsigned long play_period_ms;
	
	long prevPos;
	unsigned long playSize;
	QComboBox * playCombo;
//	QScrollBar * playScrollBar;
	QSlider * playScrollBar;

	void initPlayer();
	void nextFrame();
	void display_frame();

	// libavcodec structs
	FileVideoAcquisition * m_fileVA;

    // -- end of libavcodec structs
	WorkshopImage * detailsImage;
	WorkshopImageTool * detailsView;
	Q3VBox * playerVBox;
	
private slots:
	// movie player
	void slotPlayMovie();
	void slotPlayPauseMovie();
	void slotStepMovie();
	void slotStepBackwardMovie();
	void slotRewindMovie();
	void slotRewindStartMovie();
	void slotSpeedMovie(const QString & str);
	void slotResizeTool(QResizeEvent *e);
	void on_grayButton_toggled(bool);
    void slotReleaseScrollbar();
    void slotChangedScrollbar(int);
};

#endif
