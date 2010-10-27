/***************************************************************************
      videoplayertool.h  -  MJPEG video player for Sisell applications
                             -------------------
    begin                : April 2003
    copyright            : (C) 2003 by Christophe SEYVE 
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

typedef struct {
	QAction * pAction;
	int index;
	t_movie_pos movie_pos; /*! Position in file */
	int percent;	/*! Percentage of the movie size */
} video_bookmark_t;

/** \brief Workshop videos display and processing tool

	Enables to display (zoom/move) image, navigate (VCR style), and advanced 
	options such as snapshot and plugin manager handling.

	\author Christophe SEYVE \mail cseyve@free.fr
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
	
	WorkshopMovie * m_pWorkshopMovie;

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

	QPushButton * buttonBookmarks;
	QMenu * menuBookmarks;
	QAction * actAddBookmark;
	QAction * actEditBookmark;
	QAction * actPlayToBookmark;
	void appendBookmark(t_movie_pos pos);
	bool playGrayscale;
	unsigned long long m_nextBookmarkPos;
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
	
	// Bookmarks
	QList<video_bookmark_t> m_listBookmarks;

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

	// bookmarks
	void on_menuBookmarks_triggered(QAction *);
	void slotNewBookmarkList(QList<video_bookmark_t>);
	void slotBookmarkReached();
};

#endif
