/***************************************************************************
	workshop.cpp  -  main IHM for Piaf
							 -------------------
	begin                : ven nov 29 15:53:48 UTC 2002
	copyright            : (C) 2002 by Olivier Vin & Christophe Seyve
	email                : olivier.vine@sisell.com
							cseyve@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <math.h>
// Qt includes
#include <q3vbox.h>
#include <q3accel.h>
#include <q3dockwindow.h>
#include <qlabel.h>
#include <q3listview.h>
#include <qtimer.h>
//Added by qt3to4:
#include <Q3PtrList>
#include <QPixmap>
#include <Q3Frame>
#include <Q3PopupMenu>
#include <QEvent>
#include <Q3ActionGroup>
#include <QCloseEvent>

#include <QFileDialog>

#define WORKSHOP_CPP

// application specific includes
#include "workshop.h"

// Piaf Workshop Components...
#include "workshopcomponent.h"
#include "workshopmeasure.h"
#include "workshopimage.h"
#include "workshopmovie.h"

// Piaf Workshop Viewers...
#ifdef WITH_QWT
#include "previewplot2d.h"
#endif
#include "previewimage.h"
// Piaf Workshop Acquisitions
#include "videocapture.h"
#include "opencvvideoacquisition.h"
#ifdef HAS_FREENECT
#include "freenectvideoacquisition.h"
#endif
#include "workshopvideocapture.h"

// SISELL Workshop Tools...
#include "workshoptool.h"

#ifdef WITH_QWT
#include "workshopplot2d.h"
#endif

#include "workshopimagetool.h"
#include "imagetoavidialog.h"

// forms
#ifdef WITH_QWT
#include "randsignalform.h"
#endif

#include "workshoplist.h"
#include "objectsexplorer.h"

#include "videoplayertool.h"



// Piaf Dialogs
#include "piafconfigurationdialog.h"
#include "pluginlistdialog.h"

bool g_has_measures = false;
#define PIAFSESSION_FILE	".piaf-session"
#define PIAFRC_FILE	".piafrc"


WorkshopApp::WorkshopApp()
{
	setCaption( tr("PIAF 2-SVN") );

	QPixmap winIcon(BASE_DIRECTORY "images/pixmaps/piaf32.png");
	setWindowIcon(winIcon);
	setIcon(winIcon);

	printer = new QPrinter;
	untitledSnapShotCount=0;

	///////////////////////////////////////////////////////////////////
	// call inits to invoke all other construction parts
	initVars();
	initView();
	initActions();
	initMenuBar();
	initToolBar();
	initStatusBar();
	//resize( 800, 600 );

	viewToolBar->setOn(true);
	viewStatusBar->setOn(true);

	// creating list of available components
	pWComponentList = new Q3PtrList<WorkshopComponent>;

	// creating list of vailable tools
	pWToolList = new Q3PtrList<WorkshopTool>;
	pWToolList->setAutoDelete(true);

	// Initialise pointers
	pRandSignalForm = NULL;

	// Initialise Objects Explorer
	pObjectsExplorer = new ObjectsExplorer(Q3DockWindow::InDock, (QWidget *)this, "objects explorer");
	moveDockWindow(pObjectsExplorer, Qt::DockLeft);
	connect(pObjectsExplorer->view(), 	SIGNAL(doubleClicked(Q3ListViewItem *)),
			this,             			SLOT(slotOnDoubleClickOnObject(Q3ListViewItem *)));
	connect(pObjectsExplorer->view(), 	SIGNAL(rightButtonClicked (Q3ListViewItem *, const QPoint &, int )),
			this,             			SLOT(slotOnRightClickOnObject(Q3ListViewItem *, const QPoint &, int)));
	connect(pObjectsExplorer->view(), 	SIGNAL(selectionChanged(Q3ListViewItem *)),
			this,             			SLOT(slotOnNewObjectSelected(Q3ListViewItem *)));

	// DEBUG
	// Initialise Plot2D preview
#ifdef WITH_QWT

	pPVPlot2D = new PreviewPlot2D(pWorkspace, "Signal Preview", Qt::WA_DeleteOnClose);
	pPVPlot2D->display()->show();
	((QWorkspace *)pWorkspace)->addWindow((QWidget *)pPVPlot2D->display());
#endif

	// create preview display for images
	pPImage = new PreviewImage(pWorkspace, tr("Image Preview"));
	pPImage->display()->show();

	// Load last session
	loadOnStart();
}


WorkshopApp::~WorkshopApp()
{
	saveSettings();
	delete printer;
}


void WorkshopApp::initVars()
{
	defaultFPS = 25.f;
	untitledMovieCount = 0;


	char home[128] = "/home";
	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
	}

	g_movieDirName = QString(home) + tr("/Movies");
	g_imageDirName = QString(home) + tr("/Images");
	g_measureDirName = QString(home) + tr("/Measures");

	QDir dir;
	dir.mkdir(g_movieDirName);
	dir.mkdir(g_imageDirName);
	dir.mkdir(g_measureDirName);

	// use defaut for seeking files
	m_lastMovieDirName = g_movieDirName;
	m_lastMeasureDirName = g_measureDirName;
	m_lastImageDirName = g_imageDirName;

	// Read configuration directories
	QString filename = QString(home) + "/" PIAFRC_FILE;
	QFile file;
	file.setName(filename);

	saveSettingsImmediatly = true;

	if(file.exists()) {
		if(file.open(QIODevice::ReadOnly)) {
			char str[512];
			while(file.readLine(str, 512) >= 0) {
				// split
				QStringList lst( QStringList::split( "\t", str ) );
				QStringList::Iterator itCmd = lst.begin();
				if(itCmd != lst.end()) {
					QString cmd = *itCmd;
					if(cmd.contains("\n"))
						cmd.truncate(cmd.length()-1);

					if(!cmd.isNull()) {
						itCmd++;

						QString val;
						if(itCmd != lst.end()) {
							val = *itCmd;
							if(!val.isNull()) {
								if(val.contains("\n"))
									val.truncate(val.length()-1);
								if(val.length()>0) {
									if( cmd.contains("MeasureDir" ))
										g_measureDirName = val;
									if( cmd.contains("ImageDir" ))
										g_imageDirName = val;
									if( cmd.contains("MovieDir" ))
										g_movieDirName = val;

									if( cmd.contains("LastMeasurePath" ))
										m_lastMeasureDirName = val;
									if( cmd.contains("LastImagePath" ))
										m_lastImageDirName = val;
									if( cmd.contains("LastMoviePath" ))
										m_lastMovieDirName = val;
									if( cmd.contains("Geometry" )) {
										char geometry[128];
										strcpy(geometry, val.toAscii().data());
										int rectx, recty, rectw, recth;
										if(sscanf(geometry, "%d,%d+%dx%d", &rectx, &recty, &rectw, &recth)==4) {
											setGeometry(rectx, recty, rectw, recth);
										}
									}
									if( cmd.contains("saveSettings")) {
										saveSettingsImmediatly = val.contains("true", FALSE);
									}
								}
							}
						}
					}
				}
			}
		}
	} else {
		fprintf(stderr, "[Workshop] %s:%d : Cannot read file $HOME/.piafrc='%s' \n",
				__func__, __LINE__,
				filename.toAscii().data());
	}
}

void WorkshopApp::loadOnStart() {
	//if(!saveAtExit) return; // do not restore settings

	char home[128] = "/home";
	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
	}

	// Read configuration directories
	QFile file;
	file.setName(QString(home) + "/" PIAFSESSION_FILE);
	if(file.exists()) {
		if(file.open(QIODevice::ReadOnly)) {
			char str[512];
			while(file.readLine(str, 512) >= 0) {
				// split
				QStringList lst( QStringList::split( "\t", str ) );
				QStringList::Iterator itCmd = lst.begin();
				QString cmd = *itCmd;
				if(cmd.contains("\n"))
					cmd.truncate(cmd.length()-1);

				if(!cmd.isNull()) {
					itCmd++;
					QString val = *itCmd;
					if(!val.isNull()) {
						QFileInfo fi;
						if(val.contains("\n"))
							val.truncate(val.length()-1);
						if(!val.isNull()) {

							if( cmd.contains("Measure" ) && !val.isEmpty()) {
								fi.setFile(cmd);
								if(fi.exists() && fi.isFile()) {
									WorkshopMeasure * pWm = new WorkshopMeasure(REAL_VALUE);
									pWm->openFile(val);
									pWm->setPathName(fi.dirPath());
									pWm->setFileName(fi.fileName());
									pWm->setModified(false);

									addNewMeasure(pWm);
								}
							}

							if( cmd.contains("Image" ) && !val.isEmpty()) {
								fi.setFile(val);
								if(fi.exists() && fi.isFile()) {
									WorkshopImage * pWi = new WorkshopImage(fi.baseName(), REAL_VALUE);
									pWi->openFile(val);
									pWi->setPathName(fi.dirPath());
									pWi->setFileName(fi.fileName());
									pWi->setModified(false);
									addNewImage(pWi);
								}
							}
							if( cmd.contains("Movie" ) && !val.isEmpty()) {
								fi.setFile(val);
								if(fi.exists() && fi.isFile()) {
									WorkshopMovie * pWmov = new WorkshopMovie((char *)val.latin1(), fi.baseName(), REAL_VALUE);
									pWmov->openFile(val);
									pWmov->setPathName(fi.dirPath());
									pWmov->setFileName(fi.fileName());
									pWmov->setModified(false);

									itCmd++;

									if(itCmd != lst.end()) {
										// if there are bookmarks, add them now
										QString bkmks = *itCmd;
										if(!bkmks.isNull()) {
											// PARSE
											QStringList positions( QStringList::split( ",", bkmks ) );
											QStringList::Iterator itPos = positions.begin();
											QList<t_movie_pos> bkList;
											for( ; itPos != positions.end(); itPos++) {
												QString posStr = *itPos;
												if(!posStr.isNull()) {
													char moviepos[32];
													strcpy(moviepos, posStr.toAscii().data());

													//
													t_movie_pos pos;
													// scan string
													unsigned long long absPos = 0; int nFrames = 1;
													int retscan = sscanf(moviepos, "%llu+%d", &absPos, &nFrames);
													if(retscan == 2) {
														pos.nbFramesSinceKeyFrame = nFrames;
														pos.prevKeyFramePosition = pos.prevAbsPosition = absPos;
													} else {
														// by default, no key frame
														pos.nbFramesSinceKeyFrame = 1;
														pos.prevKeyFramePosition = pos.prevAbsPosition = absPos;
													}

													if(pos.prevAbsPosition>0 || pos.nbFramesSinceKeyFrame>1 ) // if 0, that's just a rewind
													{
														fprintf(stderr, "\tread bookmark=%llu+%d frames\n",
																pos.prevKeyFramePosition, pos.nbFramesSinceKeyFrame);
														bkList.append(pos);
													}
												}
											}
											pWmov->setListOfBookmarks(bkList);
										}
									}
									addNewMovie(pWmov);
								}
							}

						}
					}
				}
			}
		}
	} else {
		fprintf(stderr, "Cannot read file $HOME/" PIAFSESSION_FILE "\n");
	}
}

void saveItem(FILE * f, ExplorerItem *item) {
	//unused: int iType = item->getItemType();

	//if(iType == ROOT_ITEM)
	{
		//unused: int iClass = item->getItemClass();
		//if(iClass == ROOT_COMPONENT_ITEM)
		{
			ExplorerItem *itemC = item; //(ExplorerItem *)itC.current();
			WorkshopMeasure * pWmea;
			WorkshopImage * pWim;
			WorkshopMovie * pWmov;
			switch(itemC->getItemClass()) {
			case MEASURE_ITEM:
				pWmea = (WorkshopMeasure *)itemC->getItemPtr();
				fprintf(f, "Measure:\t%s/%s\n", pWmea->getPathName().latin1(), pWmea->getFileName().latin1());
				break;
			case IMAGE_ITEM:
				pWim = (WorkshopImage *)itemC->getItemPtr();
				fprintf(f, "Image:\t%s/%s\n", pWim->getPathName().latin1(), pWim->getFileName().latin1());
				break;
			case VIDEO_ITEM:
				pWmov = (WorkshopMovie *)itemC->getItemPtr();
				fprintf(f, "Movie:\t%s/%s", pWmov->getPathName().latin1(), pWmov->getFileName().latin1());
				if(pWmov->hasBookmarks()) {
					fprintf(f, "\t");
					QList<t_movie_pos> l_listBookmarks = pWmov->getListOfBookmarks();
					QList<t_movie_pos>::iterator it ;
					for(it = l_listBookmarks.begin(); it != l_listBookmarks.end(); ++it) {
						// save number
						t_movie_pos pos = (*it);
						fprintf(f, "%llu+%d,", pos.prevKeyFramePosition, pos.nbFramesSinceKeyFrame);
					}
				}
				fprintf(f, "\n");
				break;
			default:
				break;
			}

		}
	}
}



void WorkshopApp::saveSettings() {
	if(!saveSettingsImmediatly)
		return;

	char home[128] = "/home";
	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
	}

	// Write configuration file
	char fsession[1024];
	strcpy(fsession, home);
	strcat(fsession, "/" PIAFSESSION_FILE);

	fprintf(stderr, "WorkshopApp::%s:%d : saving settings in %s\n",
			__func__, __LINE__, fsession);

	FILE * f = fopen(fsession, "w");
	if(f) {
		// Search Component item
		Q3ListViewItemIterator it( pObjectsExplorer->view() );
		while ( it.current() ) {
			ExplorerItem *item = (ExplorerItem *)it.current();
			saveItem(f, item);
			++it;
		}
		fclose(f);

	} else {
		fprintf(stderr, "Workshop::%s:%d: Cannot write session file '%s'\n",
				__func__, __LINE__, fsession);
	}

	// ---- save configuration ----
	strcpy(fsession, home);
	strcat(fsession, "/" PIAFRC_FILE);
	f = fopen(fsession, "w");
	if(f) {
		// Search main options
		fprintf(f, "MeasureDir:\t%s\n", g_measureDirName.toUtf8().data() );
		fprintf(f, "ImageDir:\t%s\n", g_imageDirName.toUtf8().data() );
		fprintf(f, "MovieDir:\t%s\n", g_movieDirName.toUtf8().data() );
		fprintf(f, "\n");
		fprintf(f, "LastMeasurePath:\t%s\n", m_lastMeasureDirName.toUtf8().data() );
		fprintf(f, "LastImagePath:\t%s\n", m_lastImageDirName.toUtf8().data() );
		fprintf(f, "LastMoviePath:\t%s\n", m_lastMovieDirName.toUtf8().data() );
		fprintf(f, "\n");
		fprintf(f, "Geometry:\t%d,%d+%dx%d\n", rect().x(), rect().y(), rect().width(), rect().height() );
		fprintf(f, "\n");
		fprintf(f, "saveSettings:\t%s\n", saveSettingsImmediatly ? "true":"false");
		fclose(f);

	} else {
		fprintf(stderr, "Workshop::%s:%d: Cannot write config file '%s'\n",
				__func__, __LINE__, fsession);
	}

}

void WorkshopApp::initActions()
{
	/*QIcon openIcon, saveIcon, newIcon;
	newIcon = QIcon(filenew);
	openIcon = QIcon(fileopen);
	saveIcon = QIcon(filesave);

	fileNew = new QAction(newIcon, tr("New File"),
						  Q3Accel::stringToKey(tr("Ctrl+N")), (QObject *)this, "new action");
	fileNew->setStatusTip(tr("Creates a new document"));
	fileNew->setWhatsThis(tr("New File\n\nCreates a new document"));
	connect(fileNew, SIGNAL(activated()), this, SLOT(slotFileNew()));

	fileOpen = new QAction(openIcon, tr("Open File"), tr("&Open..."), (QObject *)this, "open action");
	fileOpen->setStatusTip(tr("Opens an existing document"));
	fileOpen->setWhatsThis(tr("Open File\n\nOpens an existing document"));
	connect(fileOpen, SIGNAL(activated()), this, SLOT(slotFileOpen()));

	fileSave = new QAction(saveIcon, tr("Save File"),
						   Q3Accel::stringToKey(tr("Ctrl+S")), this, "save action");
	fileSave->setStatusTip(tr("Saves the actual document"));
	fileSave->setWhatsThis(tr("Save File.\n\nSaves the actual document"));
	connect(fileSave, SIGNAL(activated()), this, SLOT(slotFileSave()));

	fileSaveAs = new QAction(tr("Save File As"), tr("Save &as..."), this, "save as action");
	fileSaveAs->setStatusTip(tr("Saves the actual document under a new filename"));
	fileSaveAs->setWhatsThis(tr("Save As\n\nSaves the actual document under a new filename"));
	connect(fileSaveAs, SIGNAL(activated()), this, SLOT(slotFileSave()));

	fileClose = new QAction(tr("Close File"),  Q3Accel::stringToKey(tr("Ctrl+W")), this, "close action");
	fileClose->setStatusTip(tr("Closes the actual document"));
	fileClose->setWhatsThis(tr("Close File\n\nCloses the actual document"));
	connect(fileClose, SIGNAL(activated()), this, SLOT(slotFileClose()));

	filePrint = new QAction(tr("Print File"), Q3Accel::stringToKey(tr("Ctrl+P")), this, "print action");
	filePrint->setStatusTip(tr("Prints out the actual document"));
	filePrint->setWhatsThis(tr("Print File\n\nPrints out the actual document"));
	connect(filePrint, SIGNAL(activated()), this, SLOT(slotFilePrint()));
*/
	QPixmap pixIcon;
	fileQuit = new QAction(tr("Exit"), Q3Accel::stringToKey(tr("Ctrl+Q")), this, "quit action");
	fileQuit->setStatusTip(tr("Quits the application"));
	fileQuit->setWhatsThis(tr("Exit\n\nQuits the application"));
	fileQuit->setIconVisibleInMenu(true);
	if(pixIcon.load(BASE_DIRECTORY "images/16x16/system-shutdown.png" )) {
		fileQuit->setIcon(pixIcon);}
	connect(fileQuit, SIGNAL(activated()), this, SLOT(slotFileQuit()));


	if(g_has_measures) {
		// TOOLS actions
		measureGen = new QAction(tr("Signal Generator"), tr("Signal &Generator"), this, "measure action");
		measureGen->setStatusTip(tr("Launch the signal generator tool"));
		measureGen->setWhatsThis(tr("Signal Generator\n\nGenerate a custom signal."));
		connect(measureGen, SIGNAL(activated()), this, SLOT(slotSignalGenerator()));
	}
	else
		measureGen = NULL;

	videoAcquire = new QAction(tr("Acquisition"), tr("Ac&quisition"), this, "video action");
	videoAcquire->setStatusTip(tr("Start a new video acquisition"));
	videoAcquire->setWhatsThis(tr("Video device acquisition\n\n.Start a new video acquisition"));
	if(pixIcon.load(BASE_DIRECTORY "images/pixmaps/camera-web16.png" )) {
		videoAcquire->setIcon(pixIcon);}
	connect(videoAcquire, SIGNAL(activated()), this, SLOT(slotOnNewVideoAcq()));

	snapshot = new QAction(tr("Snapshot"), tr("S&napshot"), this, "snap action");
	snapshot->setStatusTip(tr("Create an image from a video snapshot"));
	snapshot->setWhatsThis(tr("Video snapshot\n\n.Create an image from a video snapshot"));
	//connect(snapshot, SIGNAL(activated()), this, SLOT(slotSnapshot()));



	// Edit actions
	editCut = new QAction(tr("Cut"), Q3Accel::stringToKey(tr("Ctrl+X")), this, "edit");
	editCut->setStatusTip(tr("Cuts the selected section and puts it to the clipboard"));
	editCut->setWhatsThis(tr("Cut\n\nCuts the selected section and puts it to the clipboard"));
	connect(editCut, SIGNAL(activated()), this, SLOT(slotEditCut()));

	editCopy = new QAction(tr("Copy"), Q3Accel::stringToKey(tr("Ctrl+C")), this, "copy");
	editCopy->setStatusTip(tr("Copies the selected section to the clipboard"));
	editCopy->setWhatsThis(tr("Copy\n\nCopies the selected section to the clipboard"));
	connect(editCopy, SIGNAL(activated()), this, SLOT(slotEditCopy()));

	editUndo = new QAction(tr("Undo"), Q3Accel::stringToKey(tr("Ctrl+Z")), this, "undo");
	editUndo->setStatusTip(tr("Reverts the last editing action"));
	editUndo->setWhatsThis(tr("Undo\n\nReverts the last editing action"));
	connect(editUndo, SIGNAL(activated()), this, SLOT(slotEditUndo()));

	editPaste = new QAction(tr("Paste"), Q3Accel::stringToKey(tr("Ctrl+V")), this, "paste");
	editPaste->setStatusTip(tr("Pastes the clipboard contents to actual position"));
	editPaste->setWhatsThis(tr("Paste\n\nPastes the clipboard contents to actual position"));
	connect(editPaste, SIGNAL(activated()), this, SLOT(slotEditPaste()));


	viewToolBar = new QAction(tr("Toolbar"), tr("Tool&bar"), this, "view");
	viewToolBar->setStatusTip(tr("Enables/disables the toolbar"));
	viewToolBar->setCheckable(true);
	viewToolBar->setWhatsThis(tr("Toolbar\n\nEnables/disables the toolbar"));
	connect(viewToolBar, SIGNAL(toggled(bool)), this, SLOT(slotViewToolBar(bool)));

	viewStatusBar = new QAction(tr("Statusbar"), tr("&Statusbar"), this, "status");
	viewStatusBar->setCheckable(true);
	viewStatusBar->setStatusTip(tr("Enables/disables the statusbar"));
	viewStatusBar->setWhatsThis(tr("Statusbar\n\nEnables/disables the statusbar"));
	connect(viewStatusBar, SIGNAL(toggled(bool)), this, SLOT(slotViewStatusBar(bool)));

	windowCascade = new QAction(tr("Cascade"), tr("&Cascade"), this, "cascade");
	windowCascade->setStatusTip(tr("Cascades all windows"));
	windowCascade->setWhatsThis(tr("Cascade\n\nCascades all windows"));
	connect(windowCascade, SIGNAL(activated()), pWorkspace, SLOT(cascade()));

	windowTile = new QAction(tr("Tile"), tr("&Tile"), this, "tile");
	windowTile->setStatusTip(tr("Tiles all windows"));
	windowTile->setWhatsThis(tr("Tile\n\nTiles all windows"));
	connect(windowTile, SIGNAL(activated()), pWorkspace, SLOT(tile()));

	windowAction = new QActionGroup(this);
	windowAction->add(windowCascade);
	windowAction->add(windowTile);

	helpAboutApp = new QAction(tr("About"), tr("&About..."), this, "about");
	helpAboutApp->setStatusTip(tr("About the application"));
	helpAboutApp->setWhatsThis(tr("About\n\nAbout the application"));
	connect(helpAboutApp, SIGNAL(activated()), this, SLOT(slotHelpAbout()));


	// Images to AVI converter
	convertAction  = new QAction(tr("Convert img to AVI"), tr(""), this, "convert");
	convertAction->setStatusTip(tr("Convert image files to AVI movie"));
	convertAction->setWhatsThis(tr("Convert dialog\n"
								   "Convert a list of image files into a MJPEG AVI movie file."));
	if(pixIcon.load(BASE_DIRECTORY "images/pixmaps/movie16.png" )) {
		convertAction->setIcon(pixIcon);}
	connect(convertAction, SIGNAL(activated()), this, SLOT(slotConvertor()));


	// Configuration action
	configAction = new QAction(tr("Configuration"), tr("&Configuration"), this, "config");
	configAction->setStatusTip(tr("Launch the configuration dialog"));
	configAction->setWhatsThis(tr("Configuration dialog\nDefines working directories and options."));
	connect(configAction, SIGNAL(activated()), this, SLOT(slotConfigurator()));

	// Plugin action
	pluginAction = new QAction(tr("Plugins"), tr("&Plugins"), this, "plugin");
	pluginAction->setStatusTip(tr("Launch the plugin list dialog"));
	pluginAction->setWhatsThis(tr("Plugin list dialog\nEdition of plugin list."));
	connect(pluginAction, SIGNAL(activated()), this, SLOT(slotPluginDialog()));

}

void WorkshopApp::initMenuBar()
{
	///////////////////////////////////////////////////////////////////
	// MENUBAR

	///////////////////////////////////////////////////////////////////
	// menuBar entry pFileMenu
	pFileMenu=new Q3PopupMenu();

	/* stupid items, since they have no use *
	fileNew->addTo(pFileMenu);
	fileOpen->addTo(pFileMenu);
	fileClose->addTo(pFileMenu);
	pFileMenu->insertSeparator();
	fileSave->addTo(pFileMenu);
	fileSaveAs->addTo(pFileMenu);
	pFileMenu->insertSeparator();
	filePrint->addTo(pFileMenu);
	pFileMenu->insertSeparator();
	* stupid items, since they have no use */
	fileQuit->addTo(pFileMenu);

	///////////////////////////////////////////////////////////////////
	// menuBar entry editMenu
	/* stupid items, since they have no use *
	pEditMenu=new Q3PopupMenu();
	editUndo->addTo(pEditMenu);
	pEditMenu->insertSeparator();
	editCut->addTo(pEditMenu);
	editCopy->addTo(pEditMenu);
	editPaste->addTo(pEditMenu);
	* stupid items, since they have no use */
	pEditMenu = NULL;

	///////////////////////////////////////////////////////////////////
	// menuBar entry viewMenu
	pViewMenu=new Q3PopupMenu();
	pViewMenu->setCheckable(true);
	viewToolBar->addTo(pViewMenu);
	viewStatusBar->addTo(pViewMenu);

	// TOOLS menu
	pImagesTools=new Q3PopupMenu();
	snapshot->addTo(pImagesTools);
	pMeasuresTools=new Q3PopupMenu();
	measureGen->addTo(pMeasuresTools);
	pVideosTools=new Q3PopupMenu();
	videoAcquire->addTo(pVideosTools);

	pToolsMenu = new Q3PopupMenu();

	pToolsMenu->insertItem(tr("Images"), pImagesTools);
	if(g_has_measures) {
		pToolsMenu->insertItem(tr("Measures"), pMeasuresTools);
	}
	pToolsMenu->insertItem(tr("Videos"), pVideosTools);
	pToolsMenu->insertSeparator();

	configAction->addTo(pToolsMenu);
	convertAction->addTo(pVideosTools);
	pluginAction->addTo(pToolsMenu);

	///////////////////////////////////////////////////////////////////
	// menuBar entry windowMenu
	pWindowMenu = new Q3PopupMenu(this);
	pWindowMenu->setCheckable(true);
	connect(pWindowMenu, SIGNAL(aboutToShow()), this, SLOT(windowMenuAboutToShow()));

	///////////////////////////////////////////////////////////////////
	// menuBar entry helpMenu
	pHelpMenu=new Q3PopupMenu();
	helpAboutApp->addTo(pHelpMenu);
	pHelpMenu->insertSeparator();
	pHelpMenu->insertItem(tr("What's &This"), this, SLOT(whatsThis()));//, SHIFT+Key_F1);


	menuBar()->insertItem(tr("&File"), pFileMenu);
	if(pEditMenu)	menuBar()->insertItem(tr("&Edit"), pEditMenu);
	if(pViewMenu) menuBar()->insertItem(tr("&View"), pViewMenu);
	menuBar()->insertItem(tr("&Tools"), pToolsMenu);
	menuBar()->insertItem(tr("&Window"), pWindowMenu);
	menuBar()->insertItem(tr("&Help"), pHelpMenu);

}

void WorkshopApp::initToolBar()
{
/*  ///////////////////////////////////////////////////////////////////
  // TOOLBAR
  fileToolbar = new Q3ToolBar(this, "file operations");
  fileNew->addTo(fileToolbar);
  fileOpen->addTo(fileToolbar);
  fileSave->addTo(fileToolbar);
  fileToolbar->addSeparator();
  Q3WhatsThis::whatsThisButton(fileToolbar);
  fileToolbar->hide();
  */
	fileToolbar = NULL;
}

void WorkshopApp::initStatusBar()
{
	///////////////////////////////////////////////////////////////////
	//STATUSBAR
	statusBar()->message(tr("Ready."));
}

void WorkshopApp::initView()
{
	////////////////////////////////////////////////////////////////////
	// set the main widget here
	Q3HBox* view_back = new Q3HBox( this );
	view_back->setFrameStyle( Q3Frame::StyledPanel | Q3Frame::Sunken );

	pWorkspace = new QWorkspace( view_back );
	setCentralWidget(view_back);
}


void WorkshopApp::createPlot2D(WorkshopMeasure* wm)
{
	WorkshopPlot2D* wp;
	QString basename="2D Plotting window";

	// Creating a unique name
	filterToolName(basename);
	printf("nom : %s\n", basename.latin1());

#ifdef WITH_QWT
	if(wm)
		wp = new WorkshopPlot2D(wm, pWorkspace,basename,Qt::WA_DeleteOnClose);
	else
		wp = new WorkshopPlot2D(pWorkspace,basename,Qt::WA_DeleteOnClose);

	// install event filter on close event
	wp->display()->installEventFilter(this);

	// register new tool...
	pWToolList->append((WorkshopTool *)wp);
	// connect signals
	connect(wp, SIGNAL(newMeasure(WorkshopMeasure *)), this, SLOT(slotNewVirtualMeasure(WorkshopMeasure *)));
	displayToolsList();


	if ( pWorkspace->windowList().isEmpty() ) // show the very first window in maximized mode
		wp->display()->showMaximized();
	else
		wp->display()->show();
#endif
}


void WorkshopApp::createVideoCaptureView(VideoCaptureDoc * va)
{
	WorkshopVideoCaptureView * vcv;
	QString basename="Video Capture window";

	// Creating a unique name
	filterToolName(basename);
	printf("nom : %s\n", basename.latin1());

	if(va)
		vcv = new WorkshopVideoCaptureView(va, NULL,basename,Qt::WA_DeleteOnClose);
	else
		vcv = new WorkshopVideoCaptureView((VideoCaptureDoc *)NULL, NULL,basename,Qt::WA_DeleteOnClose);

	vcv->setWorkspace(pWorkspace);

	connect(vcv, SIGNAL(ImageSaved(QImage *)), this, SLOT(slotSnapShot(QImage *)));
	connect(vcv, SIGNAL(VideoSaved(char *)), this, SLOT(slotMovieCapture(char *)));

	// install event filter on close event
	vcv->display()->installEventFilter(this);

	// register new tool...
	pWToolList->append((WorkshopTool *)vcv);
	displayToolsList();

	// connect signals
//	connect(vcv, SIGNAL(newMeasure(WorkshopMeasure *)), this, SLOT(slotNewVirtualMeasure(WorkshopMeasure *)));

	if ( pWorkspace->windowList().isEmpty() ) // show the very first window in maximized mode
		vcv->display()->showMaximized();
	else
		vcv->display()->show();
}


//thomas.
void WorkshopApp::createImageView()
{
	WorkshopImageTool * wit;
	QString basename="Image window";
	WorkshopImage *wi;

	// Creating a unique name
	filterToolName(basename);
	printf("nom : %s\n", basename.latin1());


	ExplorerItem *item = (ExplorerItem *)pObjectsExplorer->view()->currentItem();
	wi = (WorkshopImage *)item->getItemPtr();

	wit = new WorkshopImageTool( (WorkshopImage *)wi,
								 NULL,
								 true /* show snapshot button */,
								 false,
								 (const char *)basename.ascii(),
								 Qt::WA_DeleteOnClose
								 );

	wit->setWorkspace(pWorkspace);

	connect(wit,SIGNAL(ImageSaved(QImage *)),this,SLOT(slotSnapShot(QImage *)));
	wit->setDefaultDirectories(g_imageDirName, g_movieDirName);

	// install event filter on close event
	wit->display()->installEventFilter(this);

	// register new tool...
	pWToolList->append((WorkshopTool *)wit);


	displayToolsList();

	// connect signals
//	connect(vcv, SIGNAL(newMeasure(WorkshopMeasure *)), this, SLOT(slotNewVirtualMeasure(WorkshopMeasure *)));

	if ( pWorkspace->windowList().isEmpty() ) // show the very first window in maximized mode
		wit->display()->showMaximized();
	else
		wit->display()->show();
}




void WorkshopApp::displayToolsList()
{
	WorkshopTool *wt;
	if(!pWToolList->isEmpty())
	{
		for(wt = pWToolList->first(); wt;  wt = pWToolList->next())
		{
			printf("%s\n", wt->name());
		}
	}
	else
		printf("No tool available...\n");
}

// --------------------------------------------------------------------------------
// Methode : filterToolName
// Auteur  : Olivier Vine
// Date    : 26/12/2002
// Description 	: Cette methode est appelee pour eviter les doublons dans les noms
//				: des outils utilises.
// --------------------------------------------------------------------------------
void WorkshopApp::filterToolName(QString &basename)
{
	WorkshopTool *wt;
	bool test=true;
	QString tstString= basename;
	int inc = 0;
	if(!pWToolList->isEmpty())
	{
		while(test)
		{
			test=FALSE;
			if(inc==0)
				tstString = basename;
			else
				tstString = basename + QString(" (%1)").arg(inc);
			for(wt = pWToolList->first(); wt;  wt = pWToolList->next())
			{
				if(tstString==wt->name())
				{
					inc++;
					test=TRUE;
				}
			}
		}
	}
	basename=tstString;
}

// --------------------------------------------------------------------------------
// Methode : filterComponentName
// Auteur  : Olivier Vine
// Date    : 26/12/2002
// Description 	: Cette methode est appelee pour eviter les doublons dans les noms
//				: des composants geres.
// --------------------------------------------------------------------------------
void WorkshopApp::filterComponentName(QString &basename)
{
	WorkshopComponent *wc;
	bool test=true;
	QString tstString= basename;
	int inc = 0;
	if(!pWComponentList->isEmpty())
	{
		while(test)
		{
			test=FALSE;
			if(inc==0)
				tstString = basename;
			else
				tstString = basename + QString(" (%1)").arg(inc);
			for(wc = pWComponentList->first(); wc;  wc = pWComponentList->next())
			{
				if(tstString==wc->getLabel())
				{
					inc++;
					test=TRUE;
				}
			}
		}
	}
	basename=tstString;
}

bool WorkshopApp::queryExit()
{
  int exit=QMessageBox::information(this, tr("Quit..."),
									tr("Do your really want to quit?"),
									QMessageBox::Ok, QMessageBox::Cancel);

  if (exit==1)
  {

  }
  else
  {

  };

  return (exit==1);
}

bool WorkshopApp::eventFilter(QObject* object, QEvent* event)
{
	bool closeAccept=TRUE;

	if((event->type() == QEvent::Close)&&((WorkshopApp*)object!=this))
	{
		QCloseEvent* e=(QCloseEvent*)event;
		printf("close event !\n");

		// finding the tool
		WorkshopTool *wt;
		Q3MainWindow *wc = (Q3MainWindow *)object;
		if(!pWToolList->isEmpty())
		{
			for(wt = pWToolList->first(); wt;  wt = pWToolList->next())
			{
				if(wt->display() == wc)
				{
					wc->close();
					pWToolList->remove(wt);
					displayToolsList();
				}
			}
		}

		if(closeAccept)
			e->accept();
		else
			e->ignore();
	} else {
		if((event->type() == QEvent::Close)&&((WorkshopApp*)object==this)) {
			saveSettings();
		}

	}


	return QWidget::eventFilter( object, event );    // standard event processing
}

// --------------------------------------------------------------------------------
// Methode : createNewMeasure
// Auteur  : Olivier Vine
// Date    : 05/12/2002
// Description 	: Cette methode est appelee pour intégrer une mesure dans l'application.
// --------------------------------------------------------------------------------
WorkshopMeasure *WorkshopApp::createNewMeasure(WorkshopMeasure *src, int type)
{

	WorkshopMeasure *pWm = new WorkshopMeasure(QString::null, type);

	QString newlabel;
	QString newSlabel;
	newlabel = src->getLabel();
	// testing if the name is unique
	// otherwise adding a "(x)" suffixe
	filterComponentName(newlabel);
	pWm->setLabel(newlabel);
	newSlabel = src->getShortLabel();
	pWm->setShortLabel(newSlabel);
	pWm->setDate(src->getDate());
	if(src->isValid())
		pWm->validate();
	else
		pWm->unvalidate();

	int msize=src->size();
	pWm->resize(msize);

	for(int i=0; i<msize; i++)
		pWm->at(i) = src->at(i);
	pWm->setPathName(src->getPathName());
	pWm->setFileName(newlabel);
	pWm->setModified(true);

	return pWm;
}

// --------------------------------------------------------------------------------
// Methode : addNewMeasure
// Auteur  : Olivier Vine
// Date    : 05/12/2002
// Description 	: Cette fonction doit etre appelee a chaque fois qu'un outil cree
//				: une nouvelle mesure afin de l'insérer dans l'application.
// --------------------------------------------------------------------------------
void WorkshopApp::addNewMeasure(WorkshopMeasure *pWm)
{
	pWComponentList->append((WorkshopComponent *)pWm);
	pObjectsExplorer->addWorkshopComponent((WorkshopComponent *)pWm);

	// adding the measure to all tools waiting for it..
	// sending the signal newMeasureAdded
	WorkshopTool *wt;
	if(!pWToolList->isEmpty())
	{
		for(wt = pWToolList->first(); wt;  wt = pWToolList->next())
		{
			if(wt->displayNewMeasure())
				wt->addNewMeasure(pWm);
		}
	}
}

void WorkshopApp::addNewImage(WorkshopImage *pWi)
{
	pWComponentList->append((WorkshopComponent *)pWi);
	pObjectsExplorer->addWorkshopComponent((WorkshopComponent *)pWi);
}

void WorkshopApp::addNewMovie(WorkshopMovie *pWi)
{
	pWComponentList->append((WorkshopComponent *)pWi);
	pObjectsExplorer->addWorkshopComponent((WorkshopComponent *)pWi);
}

void WorkshopApp::deleteImage(WorkshopImage * /*unused pWi*/)
{
	// FIXME :

}



/////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATION
/////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------------
// Methode : slotNewVirtualMeasure
// Auteur  : Olivier Vine
// Date    : 05/12/2002
// Description 	: Received when a new virtual measure has been generated by a tool
// --------------------------------------------------------------------------------
void WorkshopApp::slotNewVirtualMeasure(WorkshopMeasure *src)
{
	WorkshopMeasure *pwm = createNewMeasure(src, VIRTUAL_VALUE);
	addNewMeasure(pwm);
	// Save settings
	saveSettings();
}


// received when a double-click has occured in the object browser
void WorkshopApp::slotOnDoubleClickOnObject(Q3ListViewItem *item)
{
	ExplorerItem *pItem = (ExplorerItem *)item;
	switch(pItem->getItemType()) {
	case ROOT_ITEM:
		{
			switch(pItem->getItemClass()) {
			case ROOT_VIDEO_ACQ_ITEM:
				slotOnNewVideoAcq();
				break;
			default:
				slotFileOpen();
				break;
			}
		}
		break;
	default:
		{
			switch(pItem->getItemClass()) {
			case MEASURE_ITEM:
				slotCreatePlot2D();
				break;
			case VIDEO_ACQ_ITEM:
				slotCreateVideoCaptureView();
				break;
			case VIDEO_ITEM: {
					WorkshopMovie * wm = (WorkshopMovie *)pItem->getItemPtr();
					fprintf(stderr, "WorkshopApp::%s:%d : Double click on movie ! "
							"pWorkspace=%p WorkshopMovie * wm=%p\n",
							__func__, __LINE__, pWorkspace, wm);

					VideoPlayerTool * vpt = new VideoPlayerTool(pWorkspace, "video tool",
																Qt::WA_DeleteOnClose);
					vpt->setWorkshopMovie(wm);
					vpt->setDefaultFPS(defaultFPS);
					vpt->setDefaultDirectories(g_imageDirName, g_movieDirName);
					//XXX			vpt->start();

					connect(vpt,SIGNAL(signalSaveSettings()),this,SLOT(saveSettings()));
					connect(vpt->imageTool(),SIGNAL(ImageSaved(QImage *)), this, SLOT(slotSnapShot(QImage *)));
					connect(vpt->imageTool(),SIGNAL(VideoSaved(char *)), this, SLOT(slotMovieCapture(char *)));

					//			pVideoPlayerToolList->append(vpt);

					// install event filter on close event
					vpt->display()->installEventFilter(this);

					// register new tool...
					pWToolList->append((WorkshopTool *)vpt);
					displayToolsList();

				}
				break;
			default:
				slotCreateImageView();
				break;
			}
		}
		break;
	}
}

void WorkshopApp::createSubmenu(Q3ListViewItem *item, Q3PopupMenu *menu)
{
	ExplorerItem *pItem = (ExplorerItem *)item;

	if(pItem->getItemClass() == MEASURE_ITEM)
	{
		Q3PopupMenu *submenuStatic = new Q3PopupMenu( this );
		submenuStatic->insertItem( "&Table", this, SLOT(slotCreateTable()) );
		submenuStatic->insertItem( "&Plot 2D", this, SLOT(slotCreatePlot2D()) );
		menu->insertItem( "&View", submenuStatic);
		// dynamic zone : searching the viewers not already displaying the measure
		WorkshopMeasure *wm = (WorkshopMeasure *)pItem->getItemPtr();

		if(wm)
		{
			QString label;
			WorkshopTool *wt;
			int id;
			// Dynamic menu "Add to"
			Q3PopupMenu *submenuAdd = new Q3PopupMenu( this );
			Q_CHECK_PTR( submenuAdd );
			// Dynamic menu "Remove from"
			Q3PopupMenu *submenuDel = new Q3PopupMenu( this );
			Q_CHECK_PTR( submenuDel );
			if(!pWToolList->isEmpty())
			{
				for(wt = pWToolList->first(); wt;  wt = pWToolList->next())
				{
					if((!wt->haveMeasure(wm)) && (wt->canDisplay(CLASS_MEASURE)))
					{
						label.sprintf("%s\n", wt->name());
						id = submenuAdd->insertItem( label, this, SLOT(slotNewMeasureOnTool(int)) );
						submenuAdd->setItemParameter(id, (int)wt->getID());
					}
					if(wt->haveMeasure(wm))
					{
						label.sprintf("%s\n", wt->name());
						id = submenuDel->insertItem( label, this, SLOT(slotDelMeasureOnTool(int)) );
						submenuDel->setItemParameter(id, (int)wt->getID());
					}

				}
			}
			menu->insertItem("Add to", submenuAdd);
			menu->insertItem("Remove from", submenuDel);
		}
	}
}

void WorkshopApp::slotOnRightClickOnObject(Q3ListViewItem *item,
	const QPoint & ,
	int )
{
	Q3PopupMenu* contextMenu;
	QString 	keyName;

	ExplorerItem *pItem = (ExplorerItem *)item;
	switch(pItem->getItemType())
	{
	case ROOT_ITEM: {
		int cClass=pItem->getItemClass();
		fprintf(stderr, "RightClick on component class root class=%d\n", cClass);

		contextMenu = new Q3PopupMenu( this );
		Q_CHECK_PTR( contextMenu );
		QLabel *caption = new QLabel( this );

		switch(cClass)
		{
		case ROOT_MEASURE_ITEM:
			fprintf(stderr, "RightClick on measures\n");
			caption->setText(tr("Measures"));
			caption->setAlignment( Qt::AlignCenter );
			contextMenu->insertItem( caption->text(), 0, 0 ); // FIXME : 0,0
			contextMenu->insertItem( tr("&Clear"), this, SLOT(slotComponentClear()));//, CTRL+Key_W );
			contextMenu->insertItem( tr("&Open"), this, SLOT(slotFileOpen()));//, CTRL+Key_S );

			createSubmenu(pItem, contextMenu);
			break;
		case ROOT_IMAGE_ITEM:
			caption->setText(tr("Images"));
			caption->setAlignment( Qt::AlignCenter );
			contextMenu->insertItem( caption->text() , 0, 0 ); // FIXME : 0,0
			contextMenu->insertItem( tr("&Clear"), this, SLOT(slotComponentClear()));//, CTRL+Key_W );
			contextMenu->insertItem( tr("&Open"), this, SLOT(slotFileOpen()));//, CTRL+Key_S );

			createSubmenu(pItem, contextMenu);
			break;
		case ROOT_VIDEO_ITEM:
			caption->setText(tr("Movies"));
			caption->setAlignment( Qt::AlignCenter );
			contextMenu->insertItem( caption->text() , 0, 0 ); // FIXME : 0,0
			contextMenu->insertItem( tr("&Clear"), this, SLOT(slotComponentClear()));//, CTRL+Key_W );
			contextMenu->insertItem( tr("&Open"), this, SLOT(slotFileOpen()));//, CTRL+Key_S );

			createSubmenu(pItem, contextMenu);
			break;
		default:
			break;
		}
		contextMenu->exec( QCursor::pos() );
		delete contextMenu;
		}
		break;
	default: {
		int cClass=pItem->getItemClass();

		contextMenu = new Q3PopupMenu( this );
		Q_CHECK_PTR( contextMenu );
		QLabel *caption = new QLabel( this );

		switch(cClass)
		{
			default:
				break;
			case MEASURE_ITEM:

				caption->setText(tr("Measure"));
				caption->setAlignment( Qt::AlignCenter );
				contextMenu->insertItem( caption->text() );
				contextMenu->insertItem( tr("&Close"), this, SLOT(slotFileClose()));//, CTRL+Key_W );
				contextMenu->insertItem( tr("&Save"), this, SLOT(slotFileSave()));//, CTRL+Key_S );
				contextMenu->insertItem( tr("Save &As"), this, SLOT(slotFileSaveAs()));

				createSubmenu(pItem, contextMenu);
				break;

			case IMAGE_ITEM:

				caption->setText(tr("image"));
				caption->setAlignment( Qt::AlignCenter );
				contextMenu->insertItem( caption->text() );
				contextMenu->insertItem( tr("&Close"), this, SLOT(slotFileClose()));//, CTRL+Key_W );
				contextMenu->insertItem( tr("&Save"), this, SLOT(slotFileSave()));//, CTRL+Key_S );
				contextMenu->insertItem( tr("Save &As"), this, SLOT(slotFileSaveAs()));
				break;

			case VIDEO_ITEM:
				caption->setText(tr("Movie"));
				caption->setAlignment( Qt::AlignCenter );
				contextMenu->insertItem( caption->text() );
				contextMenu->insertItem( tr("&Close"), this, SLOT(slotFileClose()));//, CTRL+Key_W );
				contextMenu->insertItem( tr("&Save"), this, SLOT(slotFileSave()));//, CTRL+Key_S );
				contextMenu->insertItem( tr("Save &As"), this, SLOT(slotFileSaveAs()));


				break;
		}

		contextMenu->exec( QCursor::pos() );
		delete contextMenu;
		}
		break;
	}

	/*
	if(!item)
	{
		// generate global menu/actions on object explorer
		return;
	}

	if(!item->parent())
	{
		keyName=item->text(0);
		if(keyName==COMP_VIDEO_LABEL)
			printf("menu pour les videos\n");
		if(keyName==COMP_MEASURE_LABEL)
			printf("menu pour les mesures\n");
		if(keyName==COMP_IMAGE_LABEL)
			printf("menu pour les images\n");
	}
	else
	{
		contextMenu = new QPopupMenu( this );
		Q_CHECK_PTR( contextMenu );
		//QLabel *caption = new QLabel( "<font color=darkblue><u><b>""Measure</b></u></font>", this );
		// DEBUG !
		QLabel *caption = new QLabel( "Measure", this );
		caption->setAlignment( Qt::AlignCenter );
		contextMenu->insertItem( caption );
		contextMenu->insertItem( "&Close", this, SLOT(slotFileClose()), CTRL+Key_W );
		contextMenu->insertItem( "&Save", this, SLOT(slotFileSave()), CTRL+Key_S );
		contextMenu->insertItem( "Save &As", this, SLOT(slotFileSaveAs()));

		createSubmenu(item, contextMenu);

		contextMenu->exec( QCursor::pos() );
		delete contextMenu;
	}
	*/
}


// --------------------------------------------------------------------------------
// Methode : slotOnNewObjectSelected
// Auteur  : Olivier Vine
// Date    : 19/12/2002
// Description 	: Received when a new item is selected in the object explorer.
// --------------------------------------------------------------------------------
void WorkshopApp::slotOnNewObjectSelected(Q3ListViewItem *item)
{
	WorkshopMeasure *wm;
	WorkshopImage *wi;
	VideoCaptureDoc *sva;

	ExplorerItem *pItem = (ExplorerItem *)item;
	int iClass = pItem->getItemClass();
	fprintf(stderr, "[%s] WorkshopApp::%s:%d : class is '%d' / "
			"VIDEO_ITEM=%d VIDEO_ACQ_ITEM=%d IMAGE_ITEM=%d\n",
			__FILE__, __func__, __LINE__,
			iClass,
			VIDEO_ITEM, VIDEO_ACQ_ITEM, IMAGE_ITEM);

	switch(iClass)
	{
		case MEASURE_ITEM:
			wm = (WorkshopMeasure *)pItem->getItemPtr();
#ifdef WITH_QWT
			if(pPVPlot2D)
				pPVPlot2D->addNewMeasure(wm);
#endif
			break;

		case IMAGE_ITEM:
			wi = (WorkshopImage *)pItem->getItemPtr();
			if(pPImage)
			{
				if(pPImage->displayVideo())
					pPImage->unconnectVideo();
				pPImage->addNewImage(wi);
				if(pPImage->display()->isHidden())
					pPImage->display()->show();
				// Save settings
				saveSettings();

			}
			break;

		case VIDEO_ACQ_ITEM:
			sva = (VideoCaptureDoc *)pItem->getItemPtr();
			fprintf(stderr, "WorkshopApp::%s:%d : Select a video acq device sva=%p\n",
					__func__, __LINE__, sva);
			if(pPImage)
			{
				if(pPImage->displayVideo()) {
					pPImage->unconnectVideo(); }
				pPImage->connect2video(sva);

				if(pPImage->display() &&
				   pPImage->display()->isHidden()) {
					pPImage->display()->show();
				}
			}
			break;
		case VIDEO_ITEM:
			fprintf(stderr, "WorkshopApp::%s:%d : slotOnNewObjectSelected : Select a video\n",
					__func__, __LINE__);
			if(pPImage)
			{
				if(pPImage->displayVideo()) {
					pPImage->unconnectVideo(); }

				if(pPImage->display() &&
				   pPImage->display()->isHidden()) {
					pPImage->display()->show();
				}
				// Save settings
				saveSettings();
			}
			break;
		default:
			if(pPImage)
			{
				if(pPImage->displayVideo()) {
					pPImage->unconnectVideo();
				}
			}
			break;
	}
}


// --------------------------------------------------------------------------------
// Methode : slotNewMeasureOnTool
// Auteur  : Olivier Vine
// Date    : 19/12/2002
// Description 	: Received when a new measure is to be added to a tool
// --------------------------------------------------------------------------------
void WorkshopApp::slotNewMeasureOnTool(int param)
{
	WorkshopTool *wti = NULL;

	 if(!pWToolList->isEmpty())
		 {
		for(WorkshopTool * wt = pWToolList->first(); wt;  wt = pWToolList->next())
		{
					if(wt->getID() == param)
						{
				wti = wt;
			}

		}
	}
	if(!wti) return;

	ExplorerItem *item = (ExplorerItem *)pObjectsExplorer->view()->currentItem();
	if(!item)
	{
		statusBar()->message(tr("Error : no item selected in Explorer !"));
		return;
	}

	if(item->getItemClass() == MEASURE_ITEM)
	{
		WorkshopMeasure *pwm = (WorkshopMeasure *)item->getItemPtr();

		if(pwm) {
			wti->addNewMeasure(pwm);
			saveSettings();
		}
	}
}

// --------------------------------------------------------------------------------
// Methode : slotDelMeasureOnTool
// Auteur  : Olivier Vine
// Date    : 19/12/2002
// Description 	: Received when a new measure is to be removed from a tool
// --------------------------------------------------------------------------------
void WorkshopApp::slotDelMeasureOnTool(int param)
{
	 WorkshopTool *wti = NULL;

		 if(!pWToolList->isEmpty())
		 {
				for(WorkshopTool *wt = pWToolList->first(); wt;  wt = pWToolList->next())
				{
						if( wt->getID() == param)
						{
								wti = wt;
						}

				}
		}
		if(!wti) return;

	ExplorerItem *item = (ExplorerItem *)pObjectsExplorer->view()->currentItem();
	if(!item)
	{
		statusBar()->message(tr("Error : no item selected in Explorer !"));
		return;
	}

	if(item->getItemClass() == MEASURE_ITEM) {
		WorkshopMeasure *pwm = (WorkshopMeasure *)item->getItemPtr();

		if(pwm)
			wti->removeMeasure(pwm);
	}
}


void WorkshopApp::slotSnapShot(QImage * image)
{
	statusBar()->message(tr("Snapshot!"));
	//printf("Snapshot!!\n");
	WorkshopImage *pWi;
	QString snapShotName;

	QFileInfo fi;
	do {
		snapShotName = QString("snapshot%1").arg(long(untitledSnapShotCount));
		fi.setFile(g_imageDirName + "/" + snapShotName + ".png");

		untitledSnapShotCount++;
	} while(fi.exists());

	QDir dir(g_imageDirName);
	if(image->save( dir.absoluteFilePath(snapShotName + ".png"), "PNG")) {
		fprintf(stderr, "WorkshopApp::%s:%d : saved %dx%dx%d as '%s'+'%s'.png\n",
			__func__, __LINE__,
			image->width(), image->height(), image->depth(),
			g_imageDirName.toUtf8().data(),
			snapShotName.toUtf8().data()
			);
	} else {
		fprintf(stderr, "WorkshopApp::%s:%d : ERROR saving %dx%dx%d as '%s'+'%s'.png\n",
			__func__, __LINE__,
			image->width(), image->height(), image->depth(),
			g_imageDirName.toUtf8().data(),
			snapShotName.toUtf8().data()
			);
	}

	pWi = new WorkshopImage(*image, snapShotName, REAL_VALUE);
	addNewImage(pWi);

	saveSettings();

	statusBar()->message(tr("Snapshot added: " ) + snapShotName+ ".png");
}



void WorkshopApp::slotMovieCapture(char * movie)
{
	statusBar()->message(tr("Video Captured."));
	printf("!!!!!!!!!!!!!!! \n\t\t\tslotMovieCapture \n\t\t\t\t\t!!!!!!!!!!!!!!!!!!!!\n");
	WorkshopMovie *pWi;
	QString snapShotName;
	char * filename = strstr(movie, "/");
	if(filename) {
		char * next;
		while((next = strstr(filename+1, "/"))) {
			filename = next;
		}
		filename++;
	}
	else
		filename = movie;

	snapShotName = QString(filename);
	untitledMovieCount++;

	pWi = new WorkshopMovie(movie, snapShotName, REAL_VALUE);

	addNewMovie(pWi);
	saveSettings();
	statusBar()->message(tr("Movie added."));
}





// start the signal generation tool
void WorkshopApp::slotSignalGenerator()
{
#ifdef WITH_QWT
	if(!pRandSignalForm)
	{
		pRandSignalForm = new RandSignalForm(this);
		connect(pRandSignalForm, SIGNAL(newMeasure(WorkshopMeasure *)), this, SLOT(slotNewVirtualMeasure(WorkshopMeasure *)));
	}
	if(pRandSignalForm->isHidden())
		pRandSignalForm->show();
#endif
}


// start the video acquisition tool
void WorkshopApp::slotOnNewVideoAcq()
{
	tBoxSize size;

	size.width  = 320;
	size.height = 240;
	char txt[128]="", name[128]="";

	statusBar()->message(tr("Creating New Video Acquisition...."));

	// check if there are Kinects connected
#ifdef HAS_FREENECT
	FreenectVideoAcquisition * freenectDevice = new FreenectVideoAcquisition(0);
	if(freenectDevice->isDeviceReady()) {
		// append to Piaf
		statusBar()->message(tr("(VideoAcquisition) : Freenect device Init OK"));

		strcat(txt, "Kinect");

		// acquisition init
		if(freenectDevice->startAcquisition()<0)
		{
			statusBar()->message(tr("Error: cannot initialize acquisition !"));
			return;
		}
		else
		{
			if(freenectDevice->isAcquisitionRunning())
			{
				statusBar()->message(tr("Initialization OK"));

				VideoCaptureDoc * pVCD = new VideoCaptureDoc(freenectDevice);

				pObjectsExplorer->addVideoAcquisition(pVCD, txt);
			}
			else {
				statusBar()->showMessage(tr("Initialization FAILURE !"));
			}
		}

	}
	else {
		delete freenectDevice;
	}

#endif // HAS_FREENECT



	// list all video devices from /proc/video/dev/video*
	bool found;
	int dev=0, failed =0;
	do {
		found = false;

		sprintf(txt, "/sys/class/video4linux/video%d/name", dev);
		bool kern2_6 = true;
		FILE * fv = fopen(txt, "r");
		if(!fv) {
			sprintf(txt, "/proc/video/dev/video%d", dev);
			fv = fopen(txt, "r");
			kern2_6 = false;
		}
		char *ptname = name;

		if(fv) {
			fprintf(stderr, "Workshop::%s:%d : opened dev '%s'\n",
					__func__, __LINE__, txt);
			found = true;
//			sprintf(txt, "/dev/video%d", dev);
			fgets(name, 127, fv);
			name[strlen(name) -1]='\0';

			// split for seeking name
			if(!kern2_6) {
				for(ptname = strstr(name, ":")+1; (*ptname == ' ' || *ptname == '\t') && (ptname < (name+127)); ptname++) {}
			} else {
				ptname = name;
			}

		} else {
			sprintf(name, "Device # %d", dev);
			sprintf(txt, "%d - %s", dev, ptname);
		}

		//SwVideoAcquisition *myVAcq = new SwVideoAcquisition(dev);
		OpenCVVideoAcquisition *myVAcq = new OpenCVVideoAcquisition(dev);

		if(!myVAcq->isDeviceReady())
		{
			failed++;
			statusBar()->message(tr("(VideoAcquisition) : Video device init failed"));
		}
		else {
			statusBar()->message(tr("(VideoAcquisition) : Video device Init OK"));
			found = true;

			// acquisition init
			if(myVAcq->startAcquisition()<0)
			{
				statusBar()->message(tr("Error: canot initialize acquisition !"));
				return;
			}
			else
			{
				if(myVAcq->isAcquisitionRunning())
				{
					statusBar()->message(tr("Initialization OK"));

					VideoCaptureDoc * pVCD = new VideoCaptureDoc(myVAcq);

					pObjectsExplorer->addVideoAcquisition(pVCD, txt);
				}
				else {
					statusBar()->showMessage(tr("Initialization FAILURE !"));
				}
			}
		}

		dev++;

	} while(found || dev<5);

}



// -----------------------------------
// VIEWERS slots
// ------------------------------------
void WorkshopApp::slotCreateTable()
{
}

void WorkshopApp::slotCreatePlot2D()
{
	statusBar()->message(tr("Creating new Plot 2D..."));

	ExplorerItem *item = (ExplorerItem *)pObjectsExplorer->view()->currentItem();
	if(!item)
	{
		statusBar()->message(tr("Error : no item selected in Explorer !"));
		return;
	}

	if(item->getItemClass() == MEASURE_ITEM)
	{
		WorkshopMeasure *pwm = (WorkshopMeasure *)item->getItemPtr();

		if(pwm)
			createPlot2D(pwm);
	}
	else
		statusBar()->message(tr("Error : you must select a measure in Explorer !"));

	statusBar()->message(tr("Ready."));
}

void WorkshopApp::slotCreateVideoCaptureView()
{
	statusBar()->message(tr("Creating new Video Capture Tool window..."));

	ExplorerItem *item = (ExplorerItem *)pObjectsExplorer->view()->currentItem();
	if(!item)
	{
		statusBar()->message(tr("Error : no item selected in Explorer !"));
		return;
	}

	if(item->getItemClass() == VIDEO_ACQ_ITEM)
	{
		VideoCaptureDoc *va = (VideoCaptureDoc *)item->getItemPtr();

		if(va)
			createVideoCaptureView(va);
	}
	else
		statusBar()->message(tr("Error : you must select a measure in Explorer !"));

	statusBar()->message(tr("Ready."));
}


void WorkshopApp::slotCreateImageView()
{
	statusBar()->message(tr("Creating image view window..."));

	ExplorerItem *item = (ExplorerItem *)pObjectsExplorer->view()->currentItem();
	if(!item)
	{
		statusBar()->message(tr("Error : no item selected in Explorer !"));
		return;
	}

	//thomas.
	if(item->getItemClass() == IMAGE_ITEM)
		createImageView();

	else
		statusBar()->message(tr("Error : you must select a image in Explorer !"));

	statusBar()->message(tr("Ready."));
}




void WorkshopApp::slotFileNew()
{
  statusBar()->message(tr("Creating new file..."));

  statusBar()->message(tr("Ready."));
}

void WorkshopApp::slotComponentClear() {
	ExplorerItem *item = (ExplorerItem *)pObjectsExplorer->view()->currentItem();
	if(!item)
	{
		statusBar()->message(tr("Error : no item selected in Explorer !"));
		return;
	}
	int type = item->getItemClass();
	// get $HOME environnement variable
	switch(type)
	{
	case ROOT_MEASURE_ITEM:
	case ROOT_IMAGE_ITEM:
	case ROOT_VIDEO_ITEM:
		// clear listview branch
		while( item->firstChild()) {
			item->removeItem(item->firstChild());
		}
		break;
	default:
		break;
	}
}

void WorkshopApp::slotFileOpen()
{
	int type;
	QString fileName;
	QFileInfo fi;
	WorkshopMeasure *pWm;
	WorkshopMovie *pWmov;
	WorkshopImage   *pWi;
	ExplorerItem *item = (ExplorerItem *)pObjectsExplorer->view()->currentItem();
	if(!item)
	{
		statusBar()->message(tr("Error : no item selected in Explorer !"));
		return;
	}

	type = item->getItemClass();
	// get $HOME environnement variable
	switch(type)
	{
		case ROOT_MEASURE_ITEM:
		case MEASURE_ITEM:
			statusBar()->message(tr("Opening measure file..."));
			fileName = Q3FileDialog::getOpenFileName(g_measureDirName, "Measures (*.meas)", this, "open file dialog", "Choose a measure to open" );
			fi.setFile(fileName);
			if(!fi.exists() || !fi.isFile())
				break;
			pWm = new WorkshopMeasure(REAL_VALUE);
			pWm->openFile(fileName);
			pWm->setPathName(fi.dirPath());
			pWm->setFileName(fi.fileName());
			printf("filename = %s.\n",pWm->getFileName().latin1());
			printf("Pathename = %s.\n",pWm->getPathName().latin1());
			printf("Labelname = %s.\n",pWm->getLabel());

			m_lastMeasureDirName = fi.dirPath();
			pWm->setModified(false);

			addNewMeasure(pWm);
			break;
		case ROOT_IMAGE_ITEM:
		case IMAGE_ITEM: {

			statusBar()->message(tr("Opening image file..."));

			QStringList files = QFileDialog::getOpenFileNames(
									 NULL,
									 tr("Select one or more image files to open"),
									 m_lastImageDirName,
									 tr("Images (*.png *.xpm *.jpg *.jpeg *.bmp *.tif*)"));
/*
			fileName = Q3FileDialog::getOpenFileName(imageDirName,
													 "Images (*.png *.jpg *.jpeg *.bmp *.tif*)", this,
													 "open image dialog", "Choose an image to open" );
													 */
			QStringList list = files;
			QStringList::Iterator it = list.begin();
			while(it != list.end()) {
				fileName = (*it);
				++it;

				// Append file
				fi.setFile(fileName);
				if(fi.exists() && fi.isFile()) {
					pWi = new WorkshopImage(fi.baseName(), REAL_VALUE);
					pWi->openFile(fileName);
					pWi->setPathName(fi.dirPath());
					pWi->setFileName(fi.fileName());
					pWi->setModified(false);
					m_lastImageDirName = fi.dirPath();
					addNewImage(pWi);
				}
			}
			}break;
		case ROOT_VIDEO_ITEM:
		case VIDEO_ITEM: {
			statusBar()->message(tr("Opening movie file..."));
			QStringList files = QFileDialog::getOpenFileNames(
									 NULL,
									 tr("Select one or more movie files to open"),
									 m_lastMovieDirName,
									 tr("Movies (*.mpg *.avi *.wmv *.mov)"));
			QStringList list = files;
			QStringList::Iterator it = list.begin();
			while(it != list.end()) {
				fileName = (*it);
				++it;

//			fileName = Q3FileDialog::getOpenFileName(movieDirName, tr("Movies (*.mpg *.avi *.wmv *.mov)"),
//													 this, "open movie dialog",
//													 tr("Choose a movie file") );
				fi.setFile(fileName);
				if(!fi.exists() || !fi.isFile())
					break;
				pWmov = new WorkshopMovie((char *)fileName.latin1(), fi.baseName(), REAL_VALUE);
				pWmov->openFile(fileName);
				pWmov->setPathName(fi.dirPath());

				m_lastMovieDirName = fi.dirPath();
				pWmov->setFileName(fi.fileName());
				pWmov->setModified(false);

				addNewMovie(pWmov);
			}
			}break;
		default:
			statusBar()->message(tr("Nothing to Open..."));
	}
	statusBar()->message(tr("Ready."));
}


void WorkshopApp::slotFileSave()
{
	int type;
	QString fileName;
	WorkshopMeasure *pWm;
	//////////////////////
	WorkshopImage *pWi;
	/////////////////////

	ExplorerItem *item = (ExplorerItem *)pObjectsExplorer->view()->currentItem();
	if(!item)
	{
		statusBar()->message(tr("Error : no item selected in Explorer !"));
		return;
	}

	type = item->getItemClass();
	switch(type)
	{
		case MEASURE_ITEM:
			pWm = (WorkshopMeasure *)item->getItemPtr();
			if(pWm)
			{
				statusBar()->message(tr("Saving a measure..."));
				pWm->save();
			}
			else
				statusBar()->message(tr("Error while trying to save a measure..."));

			break;
		///////////////////////////////////
		case IMAGE_ITEM:
			pWi= (WorkshopImage *)item->getItemPtr();
			if(pWi)
			{
				statusBar()->message(tr("Saving an image..."));
				pWi->save();
			}
			else
				statusBar()->message(tr("Error while trying to save an image..."));
			break;
		///////////////////////////////

	}

	statusBar()->message(tr("Ready."));
}


void WorkshopApp::slotFileSaveAs()
{
	int type;
	QString fileName;
	WorkshopMeasure *pWm;
	WorkshopImage *pWi;
	WorkshopMovie *pWmov;

	ExplorerItem *item = (ExplorerItem *)pObjectsExplorer->view()->currentItem();
	if(!item)
	{
		statusBar()->message(tr("Error : no item selected in Explorer !"));
		return;
	}

	type = item->getItemClass();
	switch(type)
	{
		case MEASURE_ITEM:
			pWm = (WorkshopMeasure *)item->getItemPtr();
			if(pWm)
			{
				statusBar()->message(tr("Saving a measure under new file name..."));
				pWm->saveAs();
			}
			else
				statusBar()->message(tr("Error while trying to save a measure..."));

			break;
		/////////////////////////////
		case IMAGE_ITEM:
			pWi = (WorkshopImage *)item->getItemPtr();
			if(pWi)
			{
				statusBar()->message(tr("Saving an image under new file name..."));
				pWi->saveAs();

				item->setText(0, pWi->getLabelString());
			}
			else
				statusBar()->message(tr("Error while trying to save an image..."));
			break;
		case VIDEO_ITEM:
			pWmov = (WorkshopMovie *)item->getItemPtr();
			if(pWmov)
			{
				statusBar()->message(tr("Saving a movie under new file name..."));
				pWmov->saveAs();

				item->setText(0, pWmov->getLabelString());
			}
			else
				statusBar()->message(tr("Error while trying to save a movie..."));
		/////////////////////////////
	}

	statusBar()->message(tr("Ready."));
}

void WorkshopApp::slotFileClose()
{
  statusBar()->message(tr("Closing file..."));

  ExplorerItem *item = (ExplorerItem *)pObjectsExplorer->view()->currentItem();
	//pWToolList

  if(!item)
	{
		statusBar()->message(tr("Error : no item selected in Explorer !"));
		return;
	}

	//WorkshopTool *wt;
	//if(!pWToolList->isEmpty())
	//		{}


 int type = item->getItemClass();
	switch(type)
	{
		default:

		case MEASURE_ITEM:
			delete item;
			statusBar()->message(tr("remove measure !"));
			break;

		case IMAGE_ITEM:
			statusBar()->message(tr("remove image !"));
			delete item;
			break;

		case VIDEO_ITEM:
			statusBar()->message(tr("remove video !"));
			delete item;
			break;
		}
	// SISELL TODO
		/*
  MultiDocView* m = (MultiDocView*)pWorkspace->activeWindow();
  if( m )
  {
	WorkshopDoc* doc=m->getDocument();
	doc->closeDocument();
  }
	*/
  statusBar()->message(tr("Ready."));
}

void WorkshopApp::slotFilePrint()
{
	statusBar()->message(tr("Printing..."));

	statusBar()->message(tr("Ready."));
}

void WorkshopApp::slotFileQuit()
{
	saveSettings();
	statusBar()->message(tr("Exiting application..."));
  ///////////////////////////////////////////////////////////////////
  // exits the Application
//  if(doc->isModified())
//  {
//    if(queryExit())
//    {
//      qApp->quit();
//    }
//    else
//    {
//
//    };
//  }
//  else
//  {
	qApp->quit();
//  };

  statusBar()->message(tr("Ready."));
}

void WorkshopApp::slotEditUndo()
{
  statusBar()->message(tr("Reverting last action..."));

  MultiDocView* m = (MultiDocView*) pWorkspace->activeWindow();
  if ( m ) {
//   m->undo();
  }
  statusBar()->message(tr("Ready."));
}

void WorkshopApp::slotEditCut()
{
  statusBar()->message(tr("Cutting selection..."));

  MultiDocView* m = (MultiDocView*) pWorkspace->activeWindow();
  if ( m )
//  m->cut();

  statusBar()->message(tr("Ready."));
}

void WorkshopApp::slotEditCopy()
{
  statusBar()->message(tr("Copying selection to clipboard..."));

  MultiDocView* m = (MultiDocView*) pWorkspace->activeWindow();
  if ( m )
//  m->copy();

  statusBar()->message(tr("Ready."));
}

void WorkshopApp::slotEditPaste()
{
  statusBar()->message(tr("Inserting clipboard contents..."));

  MultiDocView* m = (MultiDocView*) pWorkspace->activeWindow();
  if ( m )
//   m->paste();

  statusBar()->message(tr("Ready."));
}


void WorkshopApp::slotViewToolBar(bool toggle)
{
	if(!fileToolbar) return;
	statusBar()->message(tr("Toggle toolbar..."));
	///////////////////////////////////////////////////////////////////
	// turn Toolbar on or off
	if (toggle == false)
	{
		fileToolbar->hide();
	}
	else
	{
		fileToolbar->show();
	}
}

void WorkshopApp::slotViewStatusBar(bool toggle)
{
  statusBar()->message(tr("Toggle statusbar..."));
  ///////////////////////////////////////////////////////////////////
  //turn Statusbar on or off

  if (toggle == false)
  {
	statusBar()->hide();
  }
  else
  {
	statusBar()->show();
  }

  statusBar()->message(tr("Ready."));
}

void WorkshopApp::slotConvertor() {
	statusBar()->message(tr("Launching conversion dialog..."));
	ImageToAVIDialog * pconvertDialog = new ImageToAVIDialog();

	connect(pconvertDialog, SIGNAL(signalNewMovie(QString)),
			this, SLOT(on_pconvertDialog_signalNewMovie(QString)));

	pconvertDialog->show();
}

void WorkshopApp::on_pconvertDialog_signalNewMovie(QString moviePath) {
	// check if path exixts
	QFileInfo fi;
	fi.setFile(moviePath);
	if(fi.exists() && fi.isFile()) {
		WorkshopMovie * pWmov = new WorkshopMovie((char *)moviePath.toUtf8().data(),
												  fi.baseName(), REAL_VALUE);
		pWmov->openFile(moviePath);
		pWmov->setPathName(fi.dirPath());
		pWmov->setFileName(fi.fileName());
		pWmov->setModified(false);
		addNewMovie(pWmov);
	}
}


void WorkshopApp::slotConfigurator() {
	statusBar()->message(tr("Launching configuration dialog..."));

	PiafConfigurationDialog * configDialog = new PiafConfigurationDialog(NULL, NULL, false, 0);
	configDialog->show();

}

void WorkshopApp::slotPluginDialog() {
	statusBar()->message(tr("Launching plugin list dialog..."));

	PluginListDialog * pluginDialog = new PluginListDialog(NULL);
	pluginDialog->show();
}

void WorkshopApp::slotHelpAbout()
{
	QString comment;
	comment.sprintf("Piaf\n"
					"\n"
					"See http://piaf.googlecode.com/"
					"Version %d\n(c) 2002-2010 by SISELL", VERSION);
	QMessageBox::about(this, tr("About Piaf..."),
					   tr(comment));
}

void WorkshopApp::slotStatusHelpMsg(const QString &text)
{
  ///////////////////////////////////////////////////////////////////
  // change status message of whole statusbar temporary (text, msec)
  statusBar()->message(text, 2000);
}

void WorkshopApp::windowMenuAboutToShow()
{
  pWindowMenu->clear();
	windowCascade->addTo(pWindowMenu);
	windowTile->addTo(pWindowMenu);

  if ( pWorkspace->windowList().isEmpty() )
  {
	windowAction->setEnabled(false);
  }
  else
  {
	windowAction->setEnabled(true);
  }

  pWindowMenu->insertSeparator();

  QWidgetList windows = pWorkspace->windowList();
  for ( int i = 0; i < int(windows.count()); ++i )
  {
	int id = pWindowMenu->insertItem(QString("&%1 ").arg(i+1)+windows.at(i)->caption(), this, SLOT( windowMenuActivated( int ) ) );
	pWindowMenu->setItemParameter( id, i );
	pWindowMenu->setItemChecked( id, pWorkspace->activeWindow() == windows.at(i) );
  }
}

void WorkshopApp::windowMenuActivated( int id )
{
  QWidget* w = pWorkspace->windowList().at( id );
  if ( w )
	w->setFocus();
}
