/***************************************************************************
	  videoplayertool.cpp  -  MJPEG video player for Piaf project
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

#include "videoplayertool.h"
#include <sys/time.h>
#include <time.h>
#include <QWorkspace>
#include <QToolTip>

#include <qmessagebox.h>
//Added by qt3to4:
#include <QResizeEvent>
#include <QPixmap>

#include "moviebookmarkform.h"
#include "ffmpeg_file_acquisition.h"

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

/******************************************************************************
								VIDEO PLAYER
*******************************************************************************/
VideoPlayerTool::VideoPlayerTool(QWidget *p_parent, const char *p_name,
								 Qt::WidgetAttribute wflags )
	: WorkshopTool(p_parent, p_name, wflags )//| Qt::WNoAutoErase)
{
	pWorkspace = p_parent;
	VideoFile[0] = '\0';

	fprintf(stderr, "VideoPlayerTool::%s:%d : parent=%p, pWin=%p\n",
			__func__, __LINE__, p_parent, pWin);

	((QWorkspace *)pWorkspace)->addWindow((QWidget *)display());

	initPlayer();
	m_pWorkshopMovie = NULL;

	// change icon
	QPixmap winIcon = QPixmap(":/images/pixmaps/movie.png");
	display()->setIcon(winIcon);

	pWin->hide();
}

VideoPlayerTool::~VideoPlayerTool()
{
	if(m_fileVA)
		delete m_fileVA;
	if(detailsView)
		delete detailsView;
	if(m_editBookmarksForm)
		delete m_editBookmarksForm;

//	fprintf(stderr, "%s::%s:%d : deleted.\n", __FILE__, __func__, __LINE__);
}

void VideoPlayerTool::run()
{
	 for(;;) {
		mutex.lock();
		waitCondition.wait(&mutex, play_period_ms);
		mutex.unlock();
		playTimer->stop();

		//
		while(playContinuous) {

			slotStepMovie();
		}

		QPixmap icon;
		if(icon.load(":/images/pixmaps/VcrPlay.png"))
		{
			playMovie->setPixmap(icon);
		}
	}
}


int VideoPlayerTool::setWorkshopMovie(WorkshopMovie * wm)
{
	if(!wm) return 0;

	m_pWorkshopMovie = wm;
	// first load file
	int ret = setFile((char *)wm->getFilePath(), 0);

	QList<t_movie_pos> bmklist = m_pWorkshopMovie->getListOfBookmarks();
	QList<t_movie_pos>::iterator it;
	for(it = bmklist.begin(); it!=bmklist.end(); it++) {
		appendBookmark( (*it) );
	}

	return ret;
}


int VideoPlayerTool::setFile(char * file, int period = 0)
{
	if(m_fileVA) {
		delete m_fileVA;
		m_fileVA = NULL;
	}

	fprintf(stderr, "[VideoPlayerT]::%s:%d : Set movie file '%s'\n",
			__func__, __LINE__,	file);

	QFileInfo fi(file);
	pWin->setCaption(fi.baseName(TRUE));

	detailsView->setTitle(fi.baseName(TRUE));

	m_fileVA = (FileVideoAcquisition *)new FFmpegFileVideoAcquisition(file);
	if(!m_fileVA)
	{
		fprintf(stderr, "[VideoPlayerT] : setFile : cannot open file '%s'\n", file);

		QMessageBox::critical(pWorkspace, tr("Error"),
				tr("File '")+ QString(file) + tr("' not  found."),
				QMessageBox::Abort, QMessageBox::NoButton);

		return 0;
	}

	playFileSize = m_fileVA->getFileSize();
	fprintf(stderr, "[VideoPlayerT]::%s:%d : Movie file size : %llu\n",
			__func__, __LINE__, (unsigned long long)playFileSize);

	playScrollBar->blockSignals(TRUE);
	playScrollBar->setValue(0);
	playScrollBar->blockSignals(FALSE);

	if(!period)
		playFPS = 0.f;
	else
		playFPS = 1000.f / (float)period;

	if(playFPS <= 0) {
		playFPS = 25;
	}
	play_period_ms = (unsigned long)(1000.f / (playSpeed * playFPS));

	strcpy(VideoFile, file);

	fprintf(stderr, "[VideoPlayerT]::setFile('%s') %d x %d @ fps=%g\n", VideoFile,
			m_fileVA->getImageSize().width, m_fileVA->getImageSize().height,
			playFPS);

	slotRewindMovie();

	CvSize acqSize = m_fileVA->getImageSize();
	if(acqSize.width != 0) {
		int img_scale = 1;
		while(acqSize.width / img_scale > 640) {
			img_scale ++;
		}

		pWin->resize(acqSize.width/img_scale+4, acqSize.height/img_scale + 72);
		imageTool()->setZoom(  1 - img_scale, acqSize.width/2, acqSize.height/2);
	}

	pWin->show();
	return 1;
}

void VideoPlayerTool::initPlayer()
{
	fprintf(stderr, "[VideoPlayerT] %s:%d : initializing video codecs... FFMPEG version '"
			LIBAVCODEC_IDENT
			"'\n", __func__, __LINE__);

	m_fileVA = NULL;
	m_editBookmarksForm = NULL;
	playTimer = NULL;
	playSpeed = 1.f;
	playContinuous = false;

	QFileInfo fi(VideoFile);
	pWin->setCaption(tr("Movie:") + fi.fileName());

	// buttons
	chdir(":/images/pixmaps");

	playerVBox = new Q3VBox(pWin);
	playerVBox->resize(320, 266);

	playHBox = new Q3HBox(playerVBox);
	playHBox->setSpacing(1);

	rewindMovie = new QPushButton( playHBox);
	{
		QPixmap pixIcon;
		if(pixIcon.load(":/images/pixmaps/VcrRewind.png"))
			rewindMovie->setPixmap(pixIcon);
		else
			rewindMovie->setText(tr("Rewind"));
	}
	rewindMovie->setFlat(true);
	rewindMovie->setToggleButton(false);



	connect(rewindMovie, SIGNAL(clicked()), this, SLOT(slotRewindMovie()));


	QPushButton * stepBackMovie = new QPushButton( playHBox);
	{
		QPixmap pixIcon;
		if(pixIcon.load(":/images/pixmaps/VcrStepBackward.png"))
			stepBackMovie->setPixmap(pixIcon);
		else
			stepBackMovie->setText(tr("Back"));
	}
	stepBackMovie->setFlat(true);
	stepBackMovie->setToggleButton(false);
	connect(stepBackMovie, SIGNAL(clicked()), this, SLOT(slotStepBackwardMovie()));

	playMovie = new QPushButton( playHBox);
	{
		QPixmap pixIcon;
		if(pixIcon.load(":/images/pixmaps/VcrPlay.png"))
			playMovie->setPixmap(pixIcon);
		else
			playMovie->setText(tr("Play"));
	}
	playMovie->setFlat(true);
	playMovie->setToggleButton(false);
	connect(playMovie, SIGNAL(clicked()), this, SLOT(slotPlayPauseMovie()));

	stepMovie = new QPushButton( playHBox);
	{
		QPixmap pixIcon;
		if(pixIcon.load(":/images/pixmaps/VcrStepForward.png"))
			stepMovie->setPixmap(pixIcon);
		else
			stepMovie->setText(tr("Step"));
	}

	stepMovie->setFlat(true);
	stepMovie->setToggleButton(false);
	connect(stepMovie, SIGNAL(clicked()), this, SLOT(slotStepMovie()));

	playCombo = new QComboBox(playHBox);
	playCombo->insertItem("x 1/4", -1);
	playCombo->insertItem("x 1/2", -1);
	playCombo->insertItem("x 1", -1);
	playCombo->insertItem("x 2", -1);
	playCombo->insertItem("x 4", -1);
	playCombo->setCurrentItem(2);
	connect( playCombo, SIGNAL( activated( const QString &) ), this, SLOT( slotSpeedMovie( const QString & ) ) );

	// play as grayscale
	grayButton = new QPushButton( playHBox);
	{
		QPixmap pixIcon;
		if(pixIcon.load(":/images/pixmaps/:images/22x22/view-color.png"))
			grayButton->setPixmap(pixIcon);
		else
			grayButton->setText(tr("Rewind"));
	}
	grayButton->setFlat(true);
	grayButton->setToggleButton(true);
	QToolTip::add(grayButton, tr("Toggle grayscale/color"));
	playGrayscale = false;
	connect(grayButton, SIGNAL(toggled(bool)), this, SLOT(on_grayButton_toggled(bool)));


	// Scrollbar
	playScrollBar = new QSlider(Qt::Horizontal, playHBox);
	//playScrollBar->setOrientation( Qt::Horizontal );
	playScrollBar->setMinimum(0);
	playScrollBar->setPageStep(1);
	//playScrollBar->setRange(0, 100);
	playScrollBar->setMaximum(100);
	playScrollBar->setValue( 0 );
	playScrollBar->setTracking(false);

	connect(playScrollBar, SIGNAL(sliderReleased()), this, SLOT(slotReleaseScrollbar()));
	connect(playScrollBar, SIGNAL(valueChanged(int)), this, SLOT(slotChangedScrollbar(int)));




	// bookmarks button and menu
	buttonBookmarks = new QPushButton( playHBox);
	{
		QPixmap pixMap;
		if(pixMap.load(":/images/pixmaps/IconBookmark.png"))
			buttonBookmarks->setPixmap(pixMap);
		else
			buttonBookmarks->setText(tr("Bkmk"));
	}
	buttonBookmarks->setFlat(true);
	buttonBookmarks->setToggleButton(true);
	buttonBookmarks->setToolTip(tr("Bookmarks"));

	menuBookmarks = new QMenu(playHBox);
	buttonBookmarks->setPopup(menuBookmarks);

	// Append add and edit buttons
	QIcon pixIcon("IconBookmark.png");
	actAddBookmark = menuBookmarks->addAction(pixIcon, tr("Add"));
	actAddBookmark->setToolTip(tr("Add a bookmark at this position in movie"));
	actAddBookmark->setIconVisibleInMenu(true);

	QIcon editIcon("IconBookmarkEdit.png");
	actEditBookmark = menuBookmarks->addAction(editIcon, tr("Edit"));
	actEditBookmark->setIconVisibleInMenu(true);
	actEditBookmark->setToolTip(tr("Edit bookmarks list"));

	QIcon playIcon("IconBookmarkPlay.png");
	actPlayToBookmark = menuBookmarks->addAction(playIcon, tr("Play until bmrk"));
	actPlayToBookmark->setIconVisibleInMenu(true);
	actPlayToBookmark->setToolTip(tr("Play video until next bookmark"));

	menuBookmarks->addSeparator();

	connect(menuBookmarks, SIGNAL(triggered(QAction *)), this, SLOT(on_menuBookmarks_triggered(QAction *)));

	play_period_ms = 40; // 25 fps

	// image
	detailsImage = new WorkshopImage( "full size image" );
	detailsImage->QImage::create(320, 240, 32);

//	detailsView = new WorkshopImageTool( detailsImage, containerWorkshopImageTool,
	detailsView = new WorkshopImageTool( detailsImage, playerVBox,
										 true, // show snap button
										 true, // show record button
										 "FullsizeImageViewer", Qt::WA_DeleteOnClose //WDestructiveClose
										 );

	// This won't force the image to be on workspace because it already has a parent
	detailsView->setWorkspace((QWorkspace *)pWorkspace);
	detailsView->display()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	detailsView->display()->update();

	connect(pWin, SIGNAL(signalResizeEvent(QResizeEvent *)), this, SLOT(slotResizeTool(QResizeEvent *)));
	pWin->resize(380,320);
}


void VideoPlayerTool::slotResizeTool(QResizeEvent *e)
{
	int h = e->size().height() - 4;
	int w = e->size().width() - 4;

	playerVBox->resize(w, h);

	detailsView->display()->resize(w-4, h-36);
}

void VideoPlayerTool::appendBookmark(t_movie_pos pos) {
	// create bookmark
	video_bookmark_t new_bookmark;
	new_bookmark.index = m_listBookmarks.count()+1;
	new_bookmark.movie_pos = pos;

	// Add action
	//QImage thumbImage = Original

	bool icon_visible = false;
	QIcon pixIcon(":/images/pixmaps/IconBookmark.png");
	if(m_fileVA) {
		if(m_fileVA->getPrevAbsolutePosition() == pos.prevAbsPosition) {
			// this position is the same than added position, so we can get the current image
			QImage pImage = detailsView->imageView()->getQImage()->copy() ;
			if(!pImage.isNull()) {
				//
				QPixmap pixmap = QPixmap::fromImage( pImage.scaledToHeight(22) );
				pixIcon = QIcon(pixmap);
				icon_visible = true;
			}
		}
	}

	QString str;

	new_bookmark.percent = (int)round((double)new_bookmark.movie_pos.prevAbsPosition / (double)playFileSize * 100.);
	fprintf(stderr, "[VidPlay]::%s:%d : append prevPos=%llu / %llu = %d %%\n",
			__func__, __LINE__, new_bookmark.movie_pos.prevAbsPosition, playFileSize,
			new_bookmark.percent
			);
	str.sprintf("%d: %d %%", new_bookmark.index, new_bookmark.percent);

	new_bookmark.pAction = menuBookmarks->addAction(pixIcon, tr("Mark ") + str);
	new_bookmark.pAction->setIconVisibleInMenu(icon_visible);

	m_listBookmarks.append(new_bookmark);
}

void VideoPlayerTool::on_menuBookmarks_triggered(QAction * pAction) {
	if(!m_fileVA) return;

	// Add a bookmark
	if(actAddBookmark == pAction) {
		fprintf(stderr, "[VidPlay]::%s:%d : create bookmark for : pos=%llu\n",
				__func__, __LINE__, m_fileVA->getPrevAbsolutePosition());
		appendBookmark(m_fileVA->getMoviePosition());

		if(m_pWorkshopMovie) {
			QList<t_movie_pos> bmklist = m_pWorkshopMovie->getListOfBookmarks();
			bmklist.append(m_fileVA->getMoviePosition());
			m_pWorkshopMovie->setListOfBookmarks(bmklist);

			emit signalSaveSettings();
		}

	} else if(actEditBookmark == pAction) {// Edit a bookmark

		m_editBookmarksForm = new MovieBookmarkForm(NULL);
		if(pWorkspace) {
			((QWorkspace *)pWorkspace)->addWindow((QWidget *)m_editBookmarksForm);

		}
		m_editBookmarksForm->setBookmarkList(m_listBookmarks);
		connect(m_editBookmarksForm, SIGNAL(signalNewBookmarkList(QList<video_bookmark_t>)),
				this, SLOT(slotNewBookmarkList(QList<video_bookmark_t>)));

		// Set bookmarks
		m_editBookmarksForm->show();

	} else if (actPlayToBookmark == pAction) {
		// Play to next bookmark
		if(!m_fileVA) {
			slotRewindStartMovie();
		}

		//find the next bookmark
		unsigned long long curpos = m_fileVA->getPrevAbsolutePosition();
		m_nextBookmarkPos = curpos;
		QList<video_bookmark_t>::iterator i;
		// find the next bookmark
		for (i = m_listBookmarks.begin(); i != m_listBookmarks.end(); ++i)
		{
			if (curpos == m_nextBookmarkPos // first iteration
				&& i->movie_pos.prevAbsPosition > curpos // bookmark # i is after current position
				) { // we are at
				m_nextBookmarkPos =  i->movie_pos.prevAbsPosition;

				fprintf(stderr, "[VidPlayer]::%s:%d : curpos=%llu => next = %llu\n", __func__,__LINE__, curpos, m_nextBookmarkPos);
				break; //
			} // else, since the bookmarks aren't ordered, find if there is another one before m_nextBookmarkPos
			else if (i->movie_pos.prevAbsPosition > curpos // bookmark # i is after current position
					   && i->movie_pos.prevAbsPosition < m_nextBookmarkPos // and before the curent next bookmark
					   ) {
				m_nextBookmarkPos =  i->movie_pos.prevAbsPosition;
				fprintf(stderr, "[VidPlayer]::%s:%d : curpos=%llu => next = %llu\n", __func__,__LINE__, curpos, m_nextBookmarkPos);
			}
		}

		if (curpos >= m_nextBookmarkPos) {
			return;
		}

		connect(playTimer, SIGNAL(timeout()), this, SLOT(slotBookmarkReached()));
		slotPlayPauseMovie();

	} else {
		// We activated one bookmark, go to this position
		QList<video_bookmark_t>::iterator i;
		for (i = m_listBookmarks.begin(); i != m_listBookmarks.end(); ++i) {
			video_bookmark_t sel_bookmark = *i;
			if(sel_bookmark.pAction == pAction) {
				// select an existing bookmark
				fprintf(stderr, "[VidPlay]::%s:%d : selected bookmark idx=%d => pos=%llu\n",
						__func__, __LINE__, sel_bookmark.index,
						sel_bookmark.movie_pos.prevAbsPosition);

				if(sel_bookmark.movie_pos.nbFramesSinceKeyFrame <= 1) {
					m_fileVA->setAbsolutePosition(sel_bookmark.movie_pos.prevAbsPosition);

					// display
					slotStepMovie();

				} // else go some frames before
				else {
					m_fileVA->setAbsolutePosition(sel_bookmark.movie_pos.prevKeyFramePosition);
					fprintf(stderr, "[VidPlay]::%s:%d : skip %d frames to reach the bookmark frame\n",
							__func__, __LINE__, sel_bookmark.movie_pos.nbFramesSinceKeyFrame-1);
					// display
					for(int nb = 0; nb<sel_bookmark.movie_pos.nbFramesSinceKeyFrame-1; nb++) {
						m_fileVA->GetNextFrame();
					}

					// Display & process last one
					fprintf(stderr, "[VidPlay]::%s:%d : Display & process last one\n",
							__func__, __LINE__);
					slotStepMovie();
					fprintf(stderr, "[VidPlay]::%s:%d : jump done & processed\n",
							__func__, __LINE__);

				}


				//detailsView->setWorkshopImage(detailsImage);

				return;
			}
		}
	}
}
void VideoPlayerTool::slotBookmarkReached()
{
	if ( m_fileVA->getPrevAbsolutePosition() >= m_nextBookmarkPos)
	{
		slotPlayPauseMovie();
	}
}

void VideoPlayerTool::slotNewBookmarkList(QList<video_bookmark_t> list) {
	// clear actions
	QList<video_bookmark_t>::iterator it;
	for(it = m_listBookmarks.begin(); it != m_listBookmarks.end(); it++) {
		// delete action
		menuBookmarks->removeAction((*it).pAction);
	}
	m_listBookmarks.clear();

	QList<t_movie_pos> bmklist;
	for(it = list.begin(); it != list.end(); it++) {
		appendBookmark((*it).movie_pos);
		bmklist.append((*it).movie_pos);
	}

	// Save settings
	if(m_pWorkshopMovie) {
		m_pWorkshopMovie->setListOfBookmarks(bmklist);

		emit signalSaveSettings();
	}
}

void VideoPlayerTool::slotReleaseScrollbar() {
	if(!m_fileVA) return;

//	slotChangedScrollbar( playScrollBar->value() );
}

void VideoPlayerTool::slotChangedScrollbar(int val) {
	if(!m_fileVA) return;

	// get current indexFrame
	unsigned long long pos = (unsigned long long)((double)val/ 100. * (double)playFileSize);

	fprintf(stderr, "[VideoPlayerT]::%s:%d : changed position to %d %% (=%d ?) => byte=%llu / %llu\n",
			__func__, __LINE__, val,
			playScrollBar->value(),
			pos,
			m_fileVA->getFileSize());
	m_fileVA->setAbsolutePosition(pos);

	// display
	slotStepMovie();

	detailsView->setWorkshopImage(detailsImage);
}

void VideoPlayerTool::slotRewindMovie()
{
	fprintf(stderr, "[VideoPlayerT]:%s:%d : rewind file '%s'\n", __func__, __LINE__,
			VideoFile);
	// read first frame to see if format is supported
	if(m_fileVA)
		m_fileVA->rewindMovie();
	else
		setFile(VideoFile);

	if(m_fileVA) {

		playScrollBar->blockSignals(TRUE);
		playScrollBar->setValue(0);
		playScrollBar->blockSignals(FALSE);

		slotStepMovie();

	} // if playFile
	else {
		fprintf(stderr, "[RT]: rewind File '%s' not found\n", VideoFile);
		return;
	}


	if(!playTimer) {
		playTimer = new QTimer(this);
		connect(playTimer, SIGNAL(timeout()), this, SLOT(slotStepMovie()));
	}

	playTimer->changeInterval((int)(1000.f / (playSpeed * playFPS)));
	playTimer->stop();

	QPixmap icon;
	if(icon.load(":/images/pixmaps/VcrPlay.png"))
		playMovie->setPixmap(icon);
	//
	playHBox->show();
}



void VideoPlayerTool::slotStepBackwardMovie()
{
	if(!m_fileVA || playFrame <=0)
		return;

	m_fileVA->setAbsolutePosition(m_fileVA->getPrevAbsolutePosition());

	display_frame();
	// display
//	detailsView->setWorkshopImage(detailsImage);
}

void VideoPlayerTool::slotPlayPauseMovie()
{

	if(!m_fileVA)
		slotRewindStartMovie();

	playContinuous = false;

	if(!playTimer)
		return;

	QPixmap icon;
	if(playTimer && m_fileVA) {
		if(playTimer->isActive()) {
		  disconnect(playTimer, SIGNAL(timeout()), this, SLOT(slotBookmarkReached()));			
		  playTimer->stop();
			if(icon.load(":/images/pixmaps/VcrPlay.png"))
				playMovie->setPixmap(icon);
		} else {
			playTimer->start((int)(1000.f / (playSpeed * playFPS)));
			if(icon.load(":/images/pixmaps/VcrPause.png"))
				playMovie->setPixmap(icon);
			
		}
	}
}

void VideoPlayerTool::slotSpeedMovie( const QString & str)
{
	char tmp[128];
	strcpy(tmp, str.latin1());

	char * slash = strstr(tmp, "/");
	if(slash) {
		slash++;
		playSpeed = 1.f/atof(slash);
	}
	else {
		slash = tmp+2; // "x "
		playSpeed = atof(slash);
	}
	if(playFPS <= 0) {
		playFPS = 25;
	}
	play_period_ms = (unsigned long)(1000.f / (playSpeed * playFPS));

printf("\n|> |> |> |> |> |> |> |> Changing play speed to %g / %lu ms |> |> |> \n", playSpeed, play_period_ms);
	if(playTimer) {
		bool running = playTimer->isActive();


		playTimer->changeInterval( play_period_ms );
		if(!running)
			playTimer->stop();
	}
}

void VideoPlayerTool::slotPlayMovie()
{

}

void VideoPlayerTool::nextFrame()
{
	if(!m_fileVA) return;
	bool got_picture = m_fileVA->GetNextFrame();

	if (got_picture) {
		playFrame++;
	} else {
		if(m_fileVA->endOfFile()) // stop timer
			if(playTimer)
				if(playTimer->isActive())
					playTimer->stop();
	}
}

void VideoPlayerTool::slotRewindStartMovie()
{
	// rewind to real start of file, then go to start frame
	slotRewindMovie();
	if(!m_fileVA) {
		fprintf(stderr, "[VideoPlayerTool]::%s:%d : file not found\n", __func__, __LINE__);
		return;
	}
}

void VideoPlayerTool::slotStepMovie()
{
	if(!m_fileVA) {
		slotRewindStartMovie();
		if(!m_fileVA)
			return;
	}

	if(playFileSize <= 0) {
		fprintf(stderr, "[VideoPlayerT]:%s:%d : ERROR: file size=%llu\n", __func__, __LINE__,
					playFileSize);
		return;
	}

	// Get step time
	struct timeval tv1, tv2;
	struct timezone tz;
	gettimeofday(&tv1, &tz);

	bool got_picture = m_fileVA->GetNextFrame();

	// set Slider value
	unsigned long long pos = m_fileVA->getAbsolutePosition();
	int poscent = (int)(pos * 100 / playFileSize);

	if(poscent != playScrollBar->value()) {
		playScrollBar->blockSignals(TRUE);

	//	fprintf(stderr, "[VideoPlayerT]:%s:%d : set file to pos=%d %%\n", __func__, __LINE__,
	//			poscent);

		playScrollBar->setValue(poscent);
		playScrollBar->blockSignals(FALSE);
	}

	if (got_picture) {
		display_frame();

		playFrame++;
	}
	else {

		fprintf(stderr, "%s %s:%d : cannot read frame\n", __FILE__, __func__, __LINE__);

		if (playSize <= 0) { // stop timer
			if(playTimer) {
				if(playTimer->isActive()) {
					playTimer->stop();
				}
			}
		}

		playContinuous = false;
		QPixmap icon;

		if(icon.load(":/images/pixmaps/VcrPlay.png")) {
			playMovie->setPixmap(icon);
		}
	}


	if(m_fileVA->endOfFile()) {
		fprintf(stderr, "%s %s:%d : EOF\n", __FILE__, __func__, __LINE__);
		if(playTimer) { playTimer->stop(); }
		playContinuous = false;

		QPixmap icon;
		if(icon.load(":/images/pixmaps/VcrPlay.png")) {
			playMovie->setPixmap(icon);
		}
	}

	gettimeofday(&tv2, &tz);
}

void VideoPlayerTool::on_grayButton_toggled(bool gray) {
	playGrayscale = gray;
	if(playGrayscale) {
		QPixmap pixIcon;
		if(pixIcon.load(":/images/pixmaps/:images/22x22/view-gray.png"))
			grayButton->setPixmap(pixIcon);
		else
			grayButton->setText(tr("Gray"));
	} else {
		QPixmap pixIcon;
		if(pixIcon.load(":/images/pixmaps/:images/22x22/view-color.png"))
			grayButton->setPixmap(pixIcon);
		else
			grayButton->setText(tr("Color"));
	}
	display_frame();
}

void VideoPlayerTool::display_frame()
{
	CvSize theSize = m_fileVA->getImageSize();
	long buffersize;

	if( (detailsImage->width()!=(long)theSize.width)
		|| (detailsImage->height()!=(long)theSize.height)
		|| (detailsImage->depth()!= (playGrayscale?8:32)))
	{
		detailsImage->create(theSize.width, theSize.height, (playGrayscale?8:32));
	}

	IplImage * lastImage = NULL;
	//if(!playGrayscale)
	{
		//buffersize = theSize.width*theSize.height*4;

		//m_fileVA->readImageRGB32NoAcq(detailsImage->bits(), &buffersize);
		lastImage = m_fileVA->readImageRGB32();

	}
//  else {
//		buffersize = theSize.width*theSize.height;
//		m_fileVA->readImageYNoAcq(detailsImage->bits(), &buffersize);
//		lastImage = m_fileVA->readImageY();
//	}

	QImage qImage = iplImageToQImage(lastImage, true);

	memcpy(detailsImage->bits(), qImage.bits(),
		   qImage.width()*qImage.height()*qImage.depth()/8);
	detailsView->setWorkshopImage(detailsImage);


/*	fprintf(stderr, "%s %s:%d : read frame=> %ld Bytes (FIXME SAVE PGM)\n",
			__FILE__, __func__, __LINE__,
			buffersize);
	// FIXME :
	FILE * ftmp = fopen("/dev/shm/videoplayertool.pgm", "wb");
	if(ftmp){
		fprintf(ftmp, "P5\n%d %d\n255\n", 4*theSize.width, theSize.height);
		fwrite(detailsImage->bits(), 1, buffersize, ftmp);
		fclose(ftmp);
	}
*/
}

