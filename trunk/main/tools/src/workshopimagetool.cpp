/***************************************************************************
	workshopimagetool.cpp  -  still image display/processing tool for Piaf
							 -------------------
	begin                : Wed Nov 13 10:07:22 CET 2002
	copyright            : (C) 2002 by Christophe Seyve
	email                : cseyve@free.fr
 ***************************************************************************/

#include "workshopimagetool.h"

#include <unistd.h>

// include files for Qt
#include <qprinter.h>
#include <qpainter.h>
#include <q3accel.h>
#include <qlayout.h>
#include <qbitmap.h>
#include <qcursor.h>

// test for new main windows
#include <q3mainwindow.h>
#include <q3grid.h>
#include <qtooltip.h>

//Added by qt3to4:
#include <QPixmap>
#include <QResizeEvent>
#include <Q3PopupMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QCloseEvent>
#include <QMenu>


// application specific includes
#include <QWorkspace>

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#include "workshop.h"

//#define __DEBUG_ZOOMING__

#define TOOLBAR_HEIGHT 32
#define WIMGT_MARGIN	2

WorkshopImageTool::WorkshopImageTool(WorkshopImage *iv,
									 QWidget *p_parent, const char* name,
									 Qt::WidgetAttribute wflags)
	 : WorkshopTool(p_parent, name, wflags)
{
	pWin->hide();

	OrigImgRGB = ProcImgRGB = NULL;

	//fprintf(stderr, "WImgTool::%s:%d : parent=%p\n", __func__, __LINE__, p_parent);
	pWorkspace = (QWorkspace *) NULL;
	pParent  = p_parent;
	init();

	setWorkshopImage(iv);

	// set image reference
	pWin->setMouseTracking(TRUE);

	setToolbar();

//	if(ImgRGB.depth()==8) {
		aMenuColor->show();
//	} else {
//		aMenuColor->hide();
//	}

	mToolMode = MODE_SMARTZOOM;
	mpViewWidget->setRefImage(ProcImgRGB);

	slotUpdateView();

	ShowSnapbutton = false;
	ShowRecbutton = false;

	// init tool properties
	initTool();


	if(1) {
		// Compute view scale
		int downscale = 1;
		while(imageSize.height/downscale > 480) {
			downscale++;
		}
		if(downscale > 1) {
			// Resize
			pWin->update();
			slotUpdateView();

			ZoomScale = 1 - downscale;
			fprintf(stderr, "ImgRGB= %d x %d !! "
					"\tView size : %lu x %lu "
					"===========> downscale = %d => ZoomScale=%d => pWin->resize(%dx%d)\n",
					ImgRGB.width(), ImgRGB.height(),
					viewSize.width, viewSize.height,
					downscale, ZoomScale,
					WIMGT_MARGIN+ (int)imageSize.width / downscale, TOOLBAR_HEIGHT+WIMGT_MARGIN + (int)imageSize.height/downscale
					);

			pWin->resize(WIMGT_MARGIN + imageSize.width / downscale, TOOLBAR_HEIGHT+WIMGT_MARGIN + imageSize.height/downscale);
			//fprintf(stderr, "WImgTool::%s:%d: pWin->update();...\n", __func__, __LINE__);
			pWin->update();
			//fprintf(stderr, "WImgTool::%s:%d: slotUpdateView()...\n", __func__, __LINE__);
			slotUpdateView();

			//fprintf(stderr, "WImgTool::%s:%d: downscale OK.\n", __func__, __LINE__);
		}
	}
	QPixmap winIcon = QPixmap(BASE_DIRECTORY "images/pixmaps/stillimage.png");
	display()->setIcon(winIcon);

	pWin->setCaption( iv->getLabel() );

}


WorkshopImageTool::WorkshopImageTool(WorkshopImage *iv, QWidget *p_parent,
									 bool showSnap,
									 bool showRec,
									 const char* name, Qt::WidgetAttribute wflags)
	: WorkshopTool(p_parent, name, wflags)
{
	//fprintf(stderr, "WImTool::%s:%d : parent=%p\n", __func__, __LINE__, p_parent);
	OrigImgRGB = ProcImgRGB = NULL;

	pWorkspace = NULL;
	pParent  = p_parent;
	pWin->hide();
	init();

	setWorkshopImage(iv);

	ShowSnapbutton = showSnap;
	ShowRecbutton = showRec;

	// set image reference
	pWin->setMouseTracking(TRUE);
	setToolbar();


	mpViewWidget->setRefImage(ProcImgRGB);

	slotUpdateView();


	// init tool properties
	initTool();
	if(1) {
		// Compute view scale
		int downscale = 1;
		while(imageSize.width/downscale > 640) {
			downscale++;
		}
		if(downscale > 1) {
			// Resize
			ZoomScale = 2 - downscale;
			viewSize.width = imageSize.width / downscale;
			viewSize.height = imageSize.height / downscale;

			while(viewSize.width % 4) { viewSize.width--; }

			fprintf(stderr, "WImgT::%s:%d : ImgRGB= %d x %d !! "
					"\tView size : %lu x %lu "
					"===========> downscale = %d "
					"=> ZoomScale=%d => pWin->resize(%dx%d)\n", __func__, __LINE__,
					ImgRGB.width(), ImgRGB.height(),
					viewSize.width, viewSize.height,
					downscale, ZoomScale,
					WIMGT_MARGIN+WIMGT_MARGIN + (int)viewSize.width,
					TOOLBAR_HEIGHT + (int)viewSize.height
					);

			//fprintf(stderr, "WImgTool::%s:%d: pWin->update();...\n", __func__, __LINE__);
			pWin->update();

			viewPixel = viewSize.width * viewSize.height;
			mpViewWidget->resize(viewSize.width, viewSize.height);
			mpViewWidget->updateGeometry();
			vBox->resize( viewSize.width+WIMGT_MARGIN+WIMGT_MARGIN, TOOLBAR_HEIGHT+WIMGT_MARGIN + viewSize.height);
			hBox->resize( viewSize.width+WIMGT_MARGIN, TOOLBAR_HEIGHT+WIMGT_MARGIN);
			hBox->updateGeometry();
			vBox->updateGeometry();

			pWin->resize( viewSize.width, TOOLBAR_HEIGHT + viewSize.height);
			pWin->updateGeometry();
			changeViewSize();

			//fprintf(stderr, "WImgTool::%s:%d: slotUpdateView()...\n", __func__, __LINE__);
			slotUpdateView();

			//fprintf(stderr, "WImgTool::%s:%d: downscale OK.\n", __func__, __LINE__);
		}
	}

	QPixmap winIcon = QPixmap(BASE_DIRECTORY "images/pixmaps/stillimage.png");
	display()->setIcon(winIcon);
	pWin->setCaption( iv->getLabel() );

}


void WorkshopImageTool::setWorkspace(QWorkspace * wsp) {
	pWorkspace = wsp;
	if(pWorkspace && !pParent) {
		((QWorkspace *)pWorkspace)->addWindow((QWidget *)pWin);
	}
	pWin->show();
}


void WorkshopImageTool::init()
{
	imageSize.width  = 0;
	imageSize.height = 0;
	filterManager = NULL;
	m_colorMode = COLORMODE_GREY;

	aMenuColor = NULL;
	mpViewWidget=NULL;
	selectPressed = false;
	mToolMode = 0;
	ZoomScale = 1;
	xZoomOrigine = yZoomOrigine = 0;
	xZoomCenter = yZoomCenter = 0;

	viewSize.width=  viewSize.height = 0;
	mpegEncoder = NULL;
	record = false;
	recordNum = 0;

	char home[128] = "/home";
	strcpy(home, getenv("HOME"));

	movieDir = g_movieDirName;
	imageDir = g_imageDirName;
}

void WorkshopImageTool::setWorkshopImage(WorkshopImage * iv)
{
	if(OrigImgRGB && (iv->width() != OrigImgRGB->width || iv->height() != OrigImgRGB->height
			|| iv->depth() != 8*OrigImgRGB->nChannels) )
	{
		swReleaseImage(&OrigImgRGB);
		swReleaseImage(&ProcImgRGB);
	}

	if(!OrigImgRGB)	{
		OrigImgRGB = swCreateImage(cvSize(iv->width(), iv->height() ), IPL_DEPTH_8U, iv->depth()/8);
	}
	memcpy(OrigImgRGB->imageData, iv->bits(), OrigImgRGB->widthStep * OrigImgRGB->height);
	if(!ProcImgRGB)	{
		ProcImgRGB = cvCloneImage(OrigImgRGB);
	}
	else {
		cvCopy(OrigImgRGB, ProcImgRGB);
	}
	m_pWorkshopImage = iv;

	tBoxSize oldviewSize = viewSize;


	// set view size from image size
	tBoxSize oldSize = imageSize;
	imageSize.width  = iv->width();
	imageSize.height = iv->height();


	bool first = false;
	if( oldSize.width != imageSize.width
		|| oldSize.height != imageSize.height
//		|| OrigImgRGB->nChannels*8!=ImgRGB.depth()
		) {

//		if(ProcImgRGB.isNull()) {
//			first = true;
//		}

		if(aMenuColor) {
			aMenuColor->show();
		}

//		ProcImgRGB = OrigImgRGB;
//		if(OrigImgRGB->nChannels*8!=ImgRGB.depth()) { // updae ImgRGB
//			changeViewSize();
//		}
	}

	//ProcImgRGB = OrigImgRGB;

	// IF SIZE CHANGED, LET'S CALCULATE ANOTHER VIEW SIZE
	if(	oldSize.width != imageSize.width
		|| oldSize.height != imageSize.height
		|| true
		) {
		if(first)
			fprintf(stderr, "!!!!!!!!!!!!! WorkshopImageTool::setWorkshopImage : FIRST CALL WINDOW %lu x %lu\n", viewSize.width, viewSize.height);

		// Zoom initialisation
		if(first) {
			viewSize.width = imageSize.width;
			while(viewSize.width % 4) { viewSize.width--; }

			viewSize.height = imageSize.height;
		}
		viewPixel = viewSize.width * viewSize.height;

		if((int)viewSize.width != ImgRGB.width()
			|| (int)viewSize.height != ImgRGB.height()) {
			changeViewSize();
		}


	} // if size changed

#ifdef DEBUG_ZOOMING
	fprintf(stderr, "ImgRGB.height()= %d  ImgRGB.width()= %d !!\n"
			"\tView size : %lu x %lu\n",
			ImgRGB.height(),ImgRGB.width(),
			viewSize.width, viewSize.height
			);
#endif

	if(!first ) {
		if(	viewSize.width != oldviewSize.width
				|| 	viewSize.height != oldviewSize.height) {
			mpViewWidget->resize(viewSize.width, viewSize.height);
			mpViewWidget->updateGeometry();
			vBox->resize( viewSize.width, TOOLBAR_HEIGHT + viewSize.height);
			hBox->resize( viewSize.width, TOOLBAR_HEIGHT);
			hBox->updateGeometry();
			vBox->updateGeometry();
			pWin->resize( viewSize.width, TOOLBAR_HEIGHT + viewSize.height);
			pWin->updateGeometry();
		}
		pWin->update();
		slotUpdateView();
	}

	//
	if(filterManager || record) {
		slotUpdateImage();
	} else {
//		paintEvent(NULL);
	}

}

void WorkshopImageTool::initTool()
{
	// enable handling measures
	disableDisplay(CLASS_VIDEO);
	disableCreate(CLASS_VIDEO);
	// disable handling image and video
	enableDisplay(CLASS_IMAGE);
	enableCreate(CLASS_IMAGE);
	disableDisplay(CLASS_MEASURE);
	disableCreate(CLASS_MEASURE);
}

WorkshopImageTool::~WorkshopImageTool()
{
//	fprintf(stderr, "WorkshopImageTool::~WorkshopImageTool()...\n");
	if(filterManager) {
		delete filterManager;
	}
	// video encoder
	if(mpegEncoder) {
		delete mpegEncoder;
	}

	swReleaseImage(&OrigImgRGB);
	swReleaseImage(&ProcImgRGB);

//	fprintf(stderr, "%s::%s:%d : deleted.", __FILE__, __func__, __LINE__);

}


void WorkshopImageTool::setToolbar()
{
	//------------ HMI -------------
	vBox = new Q3VBox(pWin);

	// vBox resizing
//	vBox->setMinimumSize(viewSize.width, viewSize.height + TOOLBAR_HEIGHT);
	hBox = new Q3HBox(vBox);
//	hBox->setMinimumSize(viewSize.width, TOOLBAR_HEIGHT);
	hBox->setFixedHeight(TOOLBAR_HEIGHT);
	hBox->updateGeometry();

	//hBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed, false);
	//hBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed, false);

	QIcon pixIcon;

	// ------------- VIEW OPTIONS
	//hBox->insertSeparator(-1);

	chdir(BASE_DIRECTORY "images/pixmaps");
	QDir dir( BASE_DIRECTORY "images/pixmaps");
	aMenuView = new QPushButton(hBox);
	{
		QPixmap pixMap;
		if(pixMap.load("IconMenuView.png"))
			aMenuView->setPixmap(pixMap);
		else
			aMenuView->setText(tr("View menu"));
	}
	aMenuView->setFlat(true);
	aMenuView->setToggleButton(true);
	QToolTip::add(aMenuView, tr("View zoom/move menu"));

	viewMenu = new QMenu(aMenuView);
	aMenuView->setPopup(viewMenu);

	// view selection mode menu

	// zoom +
	pixIcon = QIcon("IconZoomInView.png" );
	actZoomInView = viewMenu->addAction(QIcon("IconZoomInView.png" ), tr("Zoom in"));
	actZoomInView->setShortcut(QKeySequence(("Ctrl++")));
	actZoomInView->setIconVisibleInMenu(true);
	connect(actZoomInView, SIGNAL(activated()), this, SLOT(slotZoomInMode()));
	//viewMenu->connectItem( 0, actZoomInView, SLOT(activated()) );




	pixIcon = QIcon("IconZoomOutView.png" );
	actZoomOutView = viewMenu->addAction(pixIcon, tr("Zoom out"));
	actZoomOutView->setShortcut(QKeySequence(("Ctrl+-")));
	actZoomOutView->setIconVisibleInMenu(true);
	connect(actZoomOutView, SIGNAL(activated()), this, SLOT(slotZoomOutMode()));

	pixIcon = QIcon("IconMoveView.png" );
	actMoveView = viewMenu->addAction(pixIcon, tr("Move"));
	actMoveView->setIconVisibleInMenu(true);
	actMoveView->setShortcut(QKeySequence(("Ctrl+M")));
	connect(actMoveView, SIGNAL(activated()), this, SLOT(slotMoveMode()));

	// insert actions with immediate zooming
	viewMenu->addSeparator();
	// zoom +
	pixIcon = QIcon("IconZoomInView.png" );
	QAction * actZoomInOnce = viewMenu->addAction(QIcon("IconZoomInView.png" ), tr("Zoom in"));
	//actZoomInOnce->setShortcut(QKeySequence(("+")));
	actZoomInOnce->setIconVisibleInMenu(true);
	connect(actZoomInOnce, SIGNAL(activated()), this, SLOT(slotZoomInOnce()));

	pixIcon = QIcon("IconZoomFitView.png" );
	actZoomFitView = viewMenu->addAction(pixIcon, tr("Zoom 1:1"));
	//actZoomFitView->setShortcut(QKeySequence(("1")));
	actZoomFitView->setIconVisibleInMenu(true);
	connect(actZoomFitView, SIGNAL(activated()), this, SLOT(slotZoomFit()));

	pixIcon = QIcon("IconZoomOutView.png" );
	QAction * actZoomOutOnce = viewMenu->addAction(pixIcon, tr("Zoom out once"));
	//actZoomOutOnce->setShortcut(QKeySequence(("-")));
	actZoomOutOnce->setIconVisibleInMenu(true);
	connect(actZoomOutOnce, SIGNAL(activated()), this, SLOT(slotZoomOutOnce()));






	// --- Color mode menu for grayscaled images ---
	aMenuColor = new QPushButton(hBox);
	{
		QPixmap pixMap;
		if(pixMap.load("IconColorGrey.png"))
			aMenuColor->setPixmap(pixMap);
		else
			aMenuColor->setText(tr("Color"));
	}
	aMenuColor->setFlat(true);
	aMenuColor->setToggleButton(true);
	aMenuColor->show();

	colorMenu = new QMenu(hBox);
	aMenuColor->setPopup(colorMenu);

	// buttons
	pixIcon = QIcon("IconColorGrey.png" );
	actColorGrey = colorMenu->addAction(pixIcon, tr("Normal"));
	actColorGrey->setIconVisibleInMenu(true);
	actColorGrey->setShortcut(QKeySequence(("Ctrl+N")));

	connect(actColorGrey, SIGNAL(activated()), this, SLOT(slotColorGreyMode()));
//	colorMenu->connectItem( 0, actColorGrey, SLOT(activated()) );

	pixIcon = QIcon("IconColorGreyInverted.png" );
	actColorGreyInverted = colorMenu->addAction(pixIcon, tr("Grey inverted"));
	actColorGreyInverted->setIconVisibleInMenu(true);
	actColorGreyInverted->setShortcut(QKeySequence(("Ctrl+I")));
	connect(actColorGreyInverted, SIGNAL(activated()), this, SLOT(slotColorGreyInvertMode()));

	pixIcon = QIcon("IconColorThermicBlackToRed.png" );
	actColorThermicBlack2Red = colorMenu->addAction(pixIcon, tr("Thermic Black->Red"));
	actColorThermicBlack2Red->setIconVisibleInMenu(true);
	connect(actColorThermicBlack2Red, SIGNAL(activated()), this, SLOT(slotColorThermicBlackToRedMode()));

	pixIcon = QIcon("IconColorThermicBlueToRed.png" );
	actColorThermicBlue2Red = colorMenu->addAction(pixIcon, tr("Thermic Blue->Red"));
	actColorThermicBlue2Red->setIconVisibleInMenu(true);
	actColorThermicBlue2Red->setIcon(pixIcon);
	connect(actColorThermicBlue2Red, SIGNAL(activated()), this, SLOT(slotColorThermicBlueToRedMode()));

	pixIcon = QIcon("IconColorGreyIndexed.png" );
	actColorIndexed = colorMenu->addAction(pixIcon, tr("Indexed"));
	actColorIndexed->setIconVisibleInMenu(true);

	connect(actColorIndexed, SIGNAL(activated()), this, SLOT(slotColorIndexedMode()));

	// --- Color/grayscale pickers
	pixIcon = QIcon(":/images/22x22/color-picker.png" );
	QPushButton * colorPickerButton = new QPushButton(pixIcon, "", hBox);
	colorPickerButton->setFlat(true);
	connect(colorPickerButton, SIGNAL(clicked()), this, SLOT(slotColorPickerMode()));


	// --- MASK / REGION OF INTEREST

	// ------------- MASK SECTION
	aMenuMask = new QPushButton(hBox);
	{
		QPixmap pixMap;
		if(pixMap.load("IconSelAreaMenu.xpm"))
			aMenuMask->setPixmap(pixMap);
		else
			aMenuMask->setText(tr("Mask"));
	}
	aMenuMask->setFlat(true);
	aMenuMask->setToggleButton(true);


	maskMenu = new QMenu(hBox);
	aMenuMask->setPopup(maskMenu);

	// buttons
//	actXXX->setShortcut(QKeySequence(("Ctrl+G")));

	pixIcon = QPixmap("IconSelArea.xpm");
	actSelMask = maskMenu->addAction(pixIcon, tr("Select area"));
	actSelMask->setIconVisibleInMenu(true);
	connect(actSelMask, SIGNAL(activated()), this, SLOT(slotSelMask()));

	pixIcon = QPixmap("IconAddArea.xpm");
	actAddMask = maskMenu->addAction(pixIcon, tr("Add area"));
	actAddMask->setIconVisibleInMenu(true);
	connect(actAddMask, SIGNAL(activated()), this, SLOT(slotAddMask()));

	pixIcon = QPixmap("IconDelArea.xpm");
	actDelSelectedMask = maskMenu->addAction(pixIcon, tr("Del selected area"));
	actDelSelectedMask->setIconVisibleInMenu(true);
	connect(actDelSelectedMask, SIGNAL(activated()), this, SLOT(slotDelSelectedMask()));

	pixIcon = QPixmap("IconAllArea.xpm");
	actAllMask = maskMenu->addAction(pixIcon, tr("Clear mask"));
	actAllMask->setIconVisibleInMenu(true);
	connect(actAllMask, SIGNAL(activated()), this, SLOT(slotClearMask()));



	// --- SNAPSHOT/ EXPORT BUTTON
	if(ShowSnapbutton) {
		bExport = new QPushButton(hBox);
		{
			QPixmap pixMap;
			if(pixMap.load("snapshot.png"))
				bExport->setPixmap(pixMap);
			else
				bExport->setText(tr("Snapshot"));
		}
		bExport->setFlat(true);
		bExport->setToggleButton(false);
		connect(bExport, SIGNAL(clicked()), this, SLOT(slotSaveImage()));
		QToolTip::add(bExport, tr("Save snapshot in explorer (Images)"));
	}

	// --- record butotn
	if(ShowRecbutton) {
		bRecord = new QPushButton(hBox);
		{
			QPixmap pixMap;
			if(pixMap.load("IconRecordPause.png"))
				bRecord->setPixmap(pixMap);
			else
				bRecord->setText(tr("Record"));
		}
		bRecord->setFlat(true);
		bRecord->setToggleButton(false);
		connect(bRecord, SIGNAL(pressed()), this, SLOT(slotRecord()));
		QToolTip::add(bRecord, tr("Record plugins output as movie (Movies)"));
	}

	// FILTERS
	QPushButton * filtersButton = new QPushButton(hBox);
	{
		QPixmap pixMap;
		if(pixMap.load("IconFilters.png")) {
			filtersButton->setPixmap(pixMap);
		} else {
			filtersButton->setText(tr("Filters"));
		}
	}
	filtersButton->setFlat(true);
	filtersButton->setToggleButton(false);
	connect(filtersButton, SIGNAL(pressed()), this, SLOT(slotFilters()));

	QToolTip::add(filtersButton, tr("Open filters dialog (plugins)"));


	// fix height
	//hBox->setMinimumSize(viewSize.width, TOOLBAR_HEIGHT);
	hBox->setFixedHeight(TOOLBAR_HEIGHT);
	hBox->setSpacing(0);
	hBox->setMargin(0);
	hBox->updateGeometry();

	mpViewWidget = new ImageWidget(vBox,NULL , Qt::WRepaintNoErase);
#ifdef DEBUG_ZOOMING
	fprintf(stderr, "ViewWidget->setMinimumSize( %lu, %ld)\n", viewSize.width, viewSize.height );
#endif
	//ViewWidget->setGeometry( 0,0,viewSize.width, viewSize.height );
	mpViewWidget->updateGeometry();
	mpViewWidget->setEditMode(EDITMODE_ZOOM); // smart zooming mode by default

	connect(mpViewWidget, SIGNAL(signalPicker(QRgb,int,QPoint)),
			this, SLOT(slot_mpViewWidget_signalPicker(QRgb, int, QPoint)));

	vBox->updateGeometry();

	mpViewWidget->setMouseTracking(true);

	connect(mpViewWidget, SIGNAL(mouseMoved( QMouseEvent*)),
			SLOT(mouseMoveEvent(  QMouseEvent*)));
	connect(mpViewWidget, SIGNAL(mousePressed( QMouseEvent *)),
			SLOT(mousePressEvent(  QMouseEvent*)));
	connect(mpViewWidget, SIGNAL(mouseReleased( QMouseEvent *)),
			SLOT(mouseReleaseEvent( QMouseEvent*)));

//	pWin->setMinimumSize(viewSize.width, viewSize.height + TOOLBAR_HEIGHT);
	// resize window
	pWin->resize(viewSize.width, viewSize.height + TOOLBAR_HEIGHT);
	connect(pWin, SIGNAL(signalResizeEvent(QResizeEvent *)), this, SLOT(resizeEvent(QResizeEvent *)));

	pWin->updateGeometry();
	pWin->showNormal();
}


void WorkshopImageTool::changeViewSize()
{
	// realloc
	if(ZoomScale>0) {
		viewSize.width  -= (viewSize.width % ZoomScale);
		while(viewSize.width % 4) { viewSize.width--; }

		viewSize.height -= (viewSize.height % ZoomScale);
	}

	viewPixel = viewSize.width*viewSize.height;
#ifdef __DEBUG_ZOOMING__
	printf("WorkshopImageTool::changeViewSize : %lu x %lu -> %d bytes\n",
		   viewSize.width, viewSize.height, viewPixel);
#endif

	CalculateZoomWindow();
}

int WorkshopImageTool::loadFilterManager()
{
	if(!filterManager)
	{
		filterManager = new SwFilterManager( NULL , "filterManager",0);

		filterManager->setWorkspace(pWorkspace);
		filterManager->setCaption(tr("Filters:") + display()->caption());
		connect(filterManager, SIGNAL(selectedFilterChanged()), this, SLOT(slotUpdateImage()));
	}
	else
	{
		printf("filtermanager already loaded !!\n");
		filterManager->showWindow();
	}

	return 1;
}

void WorkshopImageTool::slotFilters()
{
	loadFilterManager();
}

void WorkshopImageTool::slotUpdateImage()
{
//	memcpy(originalImage, OrigImgRGB.bits(),
//		   OrigImgRGB.width()*OrigImgRGB.height()*(OrigImgRGB->nChannels*8/8) );

	/******************* FILTER PROCESSING *********************/
	if(filterManager) {
		PIAF_MSG(SWLOG_DEBUG, "Process image IplImage *=%p = %dx%dx%dx%d",
				 OrigImgRGB,
				 OrigImgRGB->width, OrigImgRGB->height, OrigImgRGB->depth, OrigImgRGB->nChannels);

		int retproc = filterManager->processImage(OrigImgRGB, &ProcImgRGB);
		if(retproc >= 0)
		{
			PIAF_MSG(SWLOG_INFO, "=> ret=%d out IplImage *=%p = %dx%dx%dx%d",
					 retproc, ProcImgRGB,
					 (!ProcImgRGB) ? -1: ProcImgRGB->width,
					 (!ProcImgRGB) ? -1: ProcImgRGB->height,
					 (!ProcImgRGB) ? -1: ProcImgRGB->depth,
					 (!ProcImgRGB) ? -1: ProcImgRGB->nChannels);
		}
	}

/*
	fprintf(stderr, "WImgTool::%s:%d : Encoding image ? record=%c mpegEncoder=%p\n",
			__func__, __LINE__,
			record?'T':'F',
			mpegEncoder
			);
*/
	// record video on demand
	if(record && mpegEncoder) {
		unsigned char * p32 = (unsigned char *)ProcImgRGB->imageData;

		if(mpegEncoder->encodeFrameRGB32( p32 ) == 0) {
			fprintf(stderr, "WImgTool::%s:%d : Error while encoding !\n", __func__, __LINE__);
		}
	}

	slotUpdateView();
}

void WorkshopImageTool::slotUpdateView()
{
#ifdef DEBUG_ZOOMING
	fprintf(stderr, "WorkshopImageTool::slotUpdateView()\n");
#endif


	float realScale = ZoomScale;
	if(ZoomScale < 1)
	{
		realScale = 1.f / (2.f - ZoomScale);
	}

	ImgRGB = iplImageToQImage( ProcImgRGB );

	/* OBSOLETE : now the zooming is done in the display widget
	ImgRGB = ProcImgRGB.copy(xZoomOrigine, yZoomOrigine,
							 viewSize.width / realScale,
							 viewSize.height / realScale
							 ).scaled(viewSize.width, viewSize.height);
*/
	if(!mpViewWidget) { return; }
	// update fake colors mode
	mpViewWidget->setColorMode(m_colorMode);

	if(mToolMode == MODE_ADD_RECT && selectPressed)
	{
		fprintf(stderr, "WImgTool::%s:%d : adding rect : start=%d,%d stop=%d,%d\n",
				__func__, __LINE__,
				myStartX, myStartY,
				myStopX, myStopY);

		setOverlayRect(QRect(myStartX, myStartY, (myStopX-myStartX), (myStopY-myStartY)),
					   QColor(0,0,255));
	}

	if(mToolMode == MODE_SELECT && selectedMask)
	{
		setOverlayRect(QRect(selectedMask->x(), selectedMask->y(),
							 selectedMask->width(), selectedMask->height()),
					   QColor(0,255,0));
	}

//	ViewWidget->setZoomParams(xZoomOrigine, yZoomOrigine, ZoomScale);

	mpViewWidget->update();

#ifdef DEBUG_ZOOMING
	fprintf(stderr, "\tslotUpdateView : done with ViewWidget->update();\n");
#endif
}


/////////////////////////////////////
void WorkshopImageTool::slotSaveImage()
{
	savedImage = ImgRGB;
	fprintf(stderr, "WorkshopImageTool::%s:%d: %d x %d x%d => emit ImageSaved(&savedImage) !\n",
			__func__, __LINE__,
			savedImage.width(), savedImage.height(), savedImage.depth());

	emit ImageSaved(&savedImage);
}
/////////////////////////////////////




void WorkshopImageTool::setViewSize()
{

	// construct image
	//	setMinimumSize(viewSize.width, viewSize.height + TOOLBAR_HEIGHT);
#ifdef __DEBUG_ZOOMING__
	printf("setViewSize (0,0,%ld, %ld)\n", viewSize.width, viewSize.height);
#endif

	if(mpViewWidget) {
#ifdef __DEBUG_ZOOMING__
		fprintf(stderr, "WImgT::%s:%d resize(%ld, %ld)\n",
				__func__, __LINE__,
				viewSize.width, viewSize.height);
#endif
		mpViewWidget->resize(viewSize.width, viewSize.height);
		vBox->resize( viewSize.width+WIMGT_MARGIN, TOOLBAR_HEIGHT +WIMGT_MARGIN+ viewSize.height);
	}

}





/* -------------------------   SLOTS   -------------------------
*/

void WorkshopImageTool::setOverlayRect(QRect overlayRect, QColor col)
{
	int Vx, Vy, w, h;
	Absolute2View(overlayRect.x(), overlayRect.y(), &Vx, &Vy);
	if( ZoomScale > 0)
	{
		w = ZoomScale * overlayRect.width();
		h = ZoomScale * overlayRect.height();
	} else {
		int scale = 2 - ZoomScale;
		w = overlayRect.width() / scale;
		h = overlayRect.height() / scale;
	}

	mpViewWidget->setOverlayRect(QRect(Vx, Vy, w, h), col);
}



/**************************************************************************

	View modes

*************************************************************************/
void WorkshopImageTool::slotZoomInMode(){
	chdir(BASE_DIRECTORY "images/pixmaps");

	mToolMode = MODE_ZOOM_IN;
	mpViewWidget->setEditMode(EDITMODE_NONE);
	QCursor zoomCursor = QCursor( QBitmap( "CursorZoomIn.bmp", 0),
								  QBitmap( "CursorZoomInMask.bmp", 0) );
	mpViewWidget->setCursor(zoomCursor);

	updateToolbar();
}

/* Change the zoom factor */
void WorkshopImageTool::setZoom(int zoomscale, int xcenter, int ycenter)
{
	if(xcenter<0 || ycenter < 0)
	{
		xcenter = 0;
		ycenter = 0;
	}
	xZoomCenter = xcenter;
	xZoomCenter = ycenter;
	ZoomScale = zoomscale;
	CalculateZoomWindow();
	slotUpdateView();
}

void WorkshopImageTool::slotZoomOutMode(){
	chdir(BASE_DIRECTORY "images/pixmaps");
	mpViewWidget->setEditMode(EDITMODE_NONE);
	mToolMode = MODE_ZOOM_OUT;
	QCursor zoomCursor = QCursor( QBitmap( "CursorZoomOut.bmp", 0),
								  QBitmap( "CursorZoomOutMask.bmp", 0));
	mpViewWidget->setCursor(zoomCursor);

	updateToolbar();
}


void WorkshopImageTool::slotZoomFit()
{
	ZoomScale = 1;
	xZoomOrigine = 0;
	yZoomOrigine = 0;
	xZoomCenter = (int)viewSize.width / 2;
	yZoomCenter = (int)viewSize.height / 2;

	mpViewWidget->setZoomParams(0,0,1);
}

void WorkshopImageTool::slotZoomInOnce(){
	ZoomScale++;
//	xZoomOrigine = 0;
//	yZoomOrigine = 0;
	int x = (int)viewSize.width / 2;
	int y  = (int)viewSize.height / 2;

	mpViewWidget->zoomOnDisplayPixel(x, y, +1);
}

void WorkshopImageTool::slotZoomOutOnce(){
	ZoomScale--;
//	xZoomOrigine = 0;
//	yZoomOrigine = 0;
	int x = (int)viewSize.width / 2;
	int y  = (int)viewSize.height / 2;

	mpViewWidget->zoomOnDisplayPixel(x, y, -1);
}

void WorkshopImageTool::slotMoveMode(){
	mToolMode = MODE_MOVE;
	chdir(BASE_DIRECTORY "images/pixmaps");

	QCursor zoomCursor = QCursor( QBitmap( "CursorMove.bmp", 0),
								  QBitmap( "CursorMoveMask.bmp", 0));
	mpViewWidget->setCursor(zoomCursor);
	mpViewWidget->setEditMode(EDITMODE_ZOOM);
	updateToolbar();
}














void WorkshopImageTool::slotMenuView()
{
	viewMenu->exec( aMenuView->mapToGlobal(QPoint(0,0)), 0);
	updateToolbar();
}

void WorkshopImageTool::updateToolbar() {
	chdir(BASE_DIRECTORY "images/pixmaps");
	switch(mToolMode) {
	case MODE_ZOOM_IN:
		aMenuView->setPixmap(QPixmap("IconZoomInView.png"));
		break;
	case MODE_ZOOM_OUT:
		aMenuView->setPixmap(QPixmap("IconZoomOutView.png"));
		break;
	case MODE_MOVE:
		aMenuView->setPixmap(QPixmap("IconMoveView.png"));
		break;

	default:
		break;
	}
	switch(mToolMode) {
	case MODE_MOVE:
	case MODE_ZOOM_OUT:
	case MODE_ZOOM_IN:
		aMenuView->setOn(true);
		//	aMenuMask->setDown(false);
		break;

	default:
		aMenuView->setOn(false);
		//		aMenuMask->setDown(false);

		break;
	}
}

void WorkshopImageTool::slotColorGreyMode() {
	m_colorMode = COLORMODE_GREY;
	updateColorMenu();
}

void WorkshopImageTool::slotColorGreyInvertMode() {
	m_colorMode = COLORMODE_GREY_INVERTED;
	updateColorMenu();
}

void WorkshopImageTool::slotColorThermicBlackToRedMode() {
	m_colorMode = COLORMODE_THERMIC_BLACK2RED;
	updateColorMenu();
}

void WorkshopImageTool::slotColorThermicBlueToRedMode() {
	m_colorMode = COLORMODE_THERMIC_BLUE2RED;
	updateColorMenu();
}

void WorkshopImageTool::slotColorIndexedMode() {
	m_colorMode = COLORMODE_INDEXED;
	updateColorMenu();
}

void WorkshopImageTool::slotColorPickerMode()
{
	fprintf(stderr, "WImgTool::%s:%d : load cursor\n", __func__, __LINE__);

	chdir(BASE_DIRECTORY "images/pixmaps");
	mToolMode = MODE_PICKER;
	QCursor zoomCursor = QCursor( QBitmap( "CursorPicker.bmp", 0),
								  QBitmap( "CursorPickerMask.bmp", 0),
								  1, 20);
	mpViewWidget->setCursor(zoomCursor);
	mpViewWidget->setEditMode(EDITMODE_PICKER);
	updateToolbar();
}

void WorkshopImageTool::updateColorMenu() {
	mpViewWidget->setColorMode(m_colorMode);

	chdir(BASE_DIRECTORY "images/pixmaps");
	switch(m_colorMode) {
	default:
	case COLORMODE_GREY:
		aMenuColor->setPixmap(QPixmap("IconColorGrey.png"));
		break;
	case COLORMODE_INDEXED:
		aMenuColor->setPixmap(QPixmap("IconColorGreyIndexed.png"));
		break;
	case COLORMODE_GREY_INVERTED:
		aMenuColor->setPixmap(QPixmap("IconColorGreyInverted.png"));
		break;
	case COLORMODE_THERMIC_BLACK2RED:
		aMenuColor->setPixmap(QPixmap("IconColorThermicBlackToRed.png"));
		break;
	case COLORMODE_THERMIC_BLUE2RED:
		aMenuColor->setPixmap(QPixmap("IconColorThermicBlueToRed.png"));
		break;
	}
}

/*****************************************************************

  IMAGE MASK

  *****************************************************************/

/* Mask  areas butotn group callback
*************************************************************************
*/
void WorkshopImageTool::slotMaskTools(int id)
{

	switch(id)
	{
	default:
	case 0 :  // select
//		deviceMenu->exec( bDevice->mapToGlobal(QPoint(0,0)), 0);

		aMenuView->setDown(false);
		aMenuMask->setDown(false);
		break;
	case 1: // add
		slotMenuMask();
		aMenuView->setDown(false);
//		bDevice->setDown(false);
		break;
	case 2 : // Zoom in
		slotMenuView();
		aMenuMask->setDown(false);
//		bDevice->setDown(false);
		break;
	}
}
/*
void WorkshopImageTool::updateToolbar() {
	switch(ToolMode) {
	case MODE_ZOOM_IN:
		aMenuView->setPixmap(QPixmap("IconZoomInView.png"));
		break;
	case MODE_ZOOM_OUT:
		aMenuView->setPixmap(QPixmap("IconZoomOutView.png"));
		break;
	case MODE_MOVE:
		aMenuView->setPixmap(QPixmap("IconMoveView.png"));
		break;
	case MODE_SELECT:
		aMenuMask->setPixmap(QPixmap("IconSelAreaMenu.xpm"));
		break;
	case MODE_ADD_RECT:
		aMenuMask->setPixmap(QPixmap("IconAddAreaMenu.xpm"));
		break;
	default:
		break;
	}
	switch(ToolMode) {
	case MODE_MOVE:
	case MODE_ZOOM_OUT:
	case MODE_ZOOM_IN:
		aMenuView->setDown(true);
		aMenuMask->setDown(false);
		bDevice->setDown(false);
		break;
	case MODE_SELECT:
	case MODE_ADD_RECT:
		aMenuView->setDown(false);
		aMenuMask->setDown(true);
		bDevice->setDown(false);
		break;
	default:
		aMenuView->setDown(false);
		aMenuMask->setDown(false);
		bDevice->setDown(false);

		break;
	}
}
*/

/* Mask modes
*************************************************************************
*/
void WorkshopImageTool::slotSelMask(){
	 mToolMode = MODE_SELECT;

	QCursor zoomCursor;
	mpViewWidget->setCursor(Qt::PointingHandCursor );

	updateToolbar();
}
void WorkshopImageTool::slotAddMask(){
	 mToolMode = MODE_ADD_RECT;
	QCursor zoomCursor = QCursor( QBitmap(IMAGEDIR "CursorAddArea.bmp", 0),
		QBitmap(IMAGEDIR "CursorAddAreaMask.bmp", 0));
	mpViewWidget->setCursor(zoomCursor);
	updateToolbar();
}
void WorkshopImageTool::slotClearMask(){

	showMask = false;
	//doc->clearMask();
	// FIXME
}

void WorkshopImageTool::slotDelSelectedMask(){
	if(mToolMode == MODE_SELECT)
		if(selectedMask) {
			printf("Delete area.\n");
//FIXME			doc->removeMask(selectedMask);
			selectedMask = NULL;
		}
}
void WorkshopImageTool::slotMenuMask()
{
	maskMenu->exec( aMenuMask->mapToGlobal(QPoint(0,0)), 0);
}


/***************************************************************

				VIDEO RECORDING SECTION

****************************************************************/
void WorkshopImageTool::startRecording()
{
	if(!mpegEncoder)
	{
		//mpegEncoder = new MovieEncoder(CODEC_ID_MPEG1VIDEO, (int)imageSize.width, (int)imageSize.height, 10);
		mpegEncoder = new MovieEncoder((int)imageSize.width,
										(int)imageSize.height, 25);
		mpegEncoder->setQuality(75);
	}

	char txt[1024];
	bool exists = false;
	QString movieStr = m_title;
	do {
		recordNum++;
		exists = false;

		sprintf(txt, "%s/%s-%03d.avi",
				movieDir.toUtf8().data(),
				movieStr.toUtf8().data(),
				recordNum);
		FILE * f = fopen(txt, "r");
		if(f) {
			exists = true;
			fclose(f);
		}
	} while(exists);

	strcpy(movieFile, txt);
	fprintf(stderr, "WImgTool::%s:%d: Starting encoding file '%s'\n",
			__func__, __LINE__, movieFile);

	if(mpegEncoder->startEncoder( txt )) {
		record = true;
	} else {
		record = false;
	}
	fprintf(stderr, "WImgTool::%s:%d: Starting encoding file '%s' => record=%c\n",
			__func__, __LINE__, movieFile,
			record?'T':'F');
}

void WorkshopImageTool::stopRecording()
{
	if(!record) return;

	fprintf(stderr, "[WImgTool]::%s:%d : stop recording.\n", __func__, __LINE__);
	record = false;
	mpegEncoder->stopEncoder();

	emit VideoSaved(movieFile);
}

bool WorkshopImageTool::isRecording()
{
	if(!mpegEncoder)
		return false;
	return record;
}



void WorkshopImageTool::slotRecord()
{
	chdir(IMAGEDIR);
	if(isRecording()) //
	{
		QPixmap icon;
		if(icon.load("IconRecordPause.png"))
			bRecord->setPixmap(icon);
		else
			bRecord->setText(tr("Record"));
		stopRecording();
	} else {
		fprintf(stderr, "[WImgTool]::%s:%d : start recording...\n", __func__, __LINE__);
		startRecording();
		if(isRecording()) {
			QPixmap icon;
			if(icon.load("IconRecord.png"))
				bRecord->setPixmap(icon);
			else
				bRecord->setText(tr("Recording"));
		} else {
			fprintf(stderr, "[WImgTool]::%s:%d : ERROR : could not start recording\n", __func__, __LINE__);
		}
	}
}


// ------------------ GLOBAL EVENTS -----------------------
void WorkshopImageTool::closeEvent(QCloseEvent*)
{
	// LEAVE THIS EMPTY: THE EVENT FILTER IN THE WorkshopVideoCaptureApp CLASS TAKES CARE FOR CLOSING
	// QWidget closeEvent must be prevented.

}


void WorkshopImageTool::resizeEvent( QResizeEvent * e)
{

#ifdef __DEBUG_ZOOMING__
	printf("Resize event : %d x %d\n",
		   e->size().width(), e->size().height());
#endif

	// Change view size => change zoom origine
	QPoint Wpos = mpViewWidget->pos() + vBox->pos();
	viewSize.width  = e->size().width()  - Wpos.x();
	viewSize.height = e->size().height() - Wpos.y();

	// update window for zooming
	setViewSize();
	changeViewSize();
	CalculateZoomWindow();

	slotUpdateView();
}


// --------------------- MOUSE EVENTS SECTION ----------------------------
void WorkshopImageTool::mousePressEvent(QMouseEvent* e)
{
	if( e->button() == Qt::LeftButton ) {
		onLButtonDown(e->state(),e->pos());
	} else if(e->button() == Qt::RightButton ) {
		onRButtonDown(e->state(),e->pos());
	}
}

void WorkshopImageTool::mouseReleaseEvent(QMouseEvent* e)
{
	if ( e->button() == Qt::LeftButton ) {
		onLButtonUp(e->state(),e->pos());
	} else if(e->button() == Qt::RightButton ) {
		onRButtonUp(e->state(),e->pos());
	}
}

void WorkshopImageTool::mouseMoveEvent(QMouseEvent* e)
{
	onMouseMove(e->state(),e->pos());
}



/* Calculate zoom window to be displayed
*/

void WorkshopImageTool::CalculateZoomWindow()
{
	int vwidth  = (int)viewSize.width;
	int vheight = (int)viewSize.height;
	int iwidth  = (int)imageSize.width;
	int iheight = (int)imageSize.height;

#ifdef __DEBUG_ZOOMING__
	printf("CalculateZoomWindow Center In : (%d,%d)\n",
		   xZoomCenter, yZoomCenter);
#endif

	// left/top limit
	if(ZoomScale >= 1) {
		// LEFT-RIGHT LIMITATION
		xZoomOrigine = xZoomCenter - (vwidth/ ZoomScale)/2;
		if(xZoomOrigine < 0) {
#ifdef __DEBUG_ZOOMING__
			printf("\tleft limit replace\n");
#endif
			xZoomOrigine = 0;
		}
		else {
			// right/bottom limit
			//			int xZoomMax = xZoomCenter + (iwidth/ ZoomScale)/2;
			if((iwidth - xZoomOrigine) < (vwidth /ZoomScale)
				){
#ifdef __DEBUG_ZOOMING__
				//	printf("\tright limit replace - cond %d,%d\n",
				//		xZoomMax > iwidth, (iwidth - xZoomOrigine) < (vwidth /ZoomScale));
#endif
				xZoomOrigine = max(0,
								   -(vwidth - iwidth * ZoomScale)/ZoomScale);
			}
		}

		yZoomOrigine = yZoomCenter - (vheight/ ZoomScale)/2;
		if(yZoomOrigine < 0){
#ifdef __DEBUG_ZOOMING__
			printf("\ttop limit replace\n");
#endif
			yZoomOrigine = 0;
		} else {

			//			int yZoomMax = yZoomCenter + (iheight/ ZoomScale)/2;
			if( (iheight - yZoomOrigine) < (vheight /ZoomScale)
				) {
#ifdef __DEBUG_ZOOMING__
				printf("\tbottom limit replace\n");
#endif
				yZoomOrigine = max(0,
								   -(vheight - iheight * ZoomScale)/ZoomScale);
			}
		}
		// update center for move purpose
		xZoomCenter = xZoomOrigine + (vwidth /ZoomScale)/2;
		yZoomCenter = yZoomOrigine + (vheight/ZoomScale)/2;

	}
	else {
		int scale = 2-ZoomScale;
		xZoomOrigine = xZoomCenter - (vwidth*scale)/2;
		if(xZoomOrigine < 0) {
#ifdef __DEBUG_ZOOMING__
			printf("\tleft limit replace\n");
#endif
			xZoomOrigine = 0;
		}
		else {
			// right/bottom limit
			//			int xZoomMax = xZoomCenter + (iwidth* scale)/2;
			if((iwidth - xZoomOrigine) < (vwidth* scale)
				){
#ifdef __DEBUG_ZOOMING__
				//			printf("\tright limit replace - cond %d,%d\n",
				//			xZoomMax > iwidth, (iwidth - xZoomOrigine) < (vwidth * scale));
#endif
				xZoomOrigine = max(0,
								   -(vwidth - iwidth /scale)* scale);
			}
		}

		yZoomOrigine = yZoomCenter - (vheight* scale)/2;
		if(yZoomOrigine < 0){
#ifdef __DEBUG_ZOOMING__
			printf("\ttop limit replace\n");
#endif
			yZoomOrigine = 0;
		} else {

			//			int yZoomMax = yZoomCenter + (iheight* scale)/2;
			if( (iheight - yZoomOrigine) < (vheight * scale)
				) {
#ifdef __DEBUG_ZOOMING__
				printf("\tbottom limit replace\n");
#endif
				yZoomOrigine = max(0,
								   -(vheight - iheight / scale)* scale);
			}
		}

		// update center for move purpose
		xZoomCenter = xZoomOrigine + (vwidth * scale)/2;
		yZoomCenter = yZoomOrigine + (vheight * scale)/2;
	}
#ifdef __DEBUG_ZOOMING__
	printf("CalculateZoomWindow Center Out : (%d,%d)\n",
		   xZoomCenter, yZoomCenter);
	printf("CalculateZoomWindow Origine Out (scale : %d) : (%d,%d)\n",
		   ZoomScale, xZoomOrigine, yZoomOrigine);
#endif
}

void WorkshopImageTool::refreshDisplay()
{
	mpViewWidget->update();
}
void WorkshopImageTool::slot_mpViewWidget_signalPicker(QRgb colorRGB,
													 int colorGrey,
													 QPoint point)
{
	QPoint pickPt = mpViewWidget->originalToDisplay(point.x(), point.y());

	// Pick color in ImgRGB
	emit signalColorPicker(pickPt, colorRGB);

	QString colorStr;
	colorStr.sprintf("(%d,%d) RGB=%d,%d,%d gray=%d",
					 point.x(), point.y(),
					 qRed(colorRGB), qGreen(colorRGB), qBlue(colorRGB),
					 colorGrey);
//	fprintf(stderr, "WImTool::%s:%d : %s\n",
//			__func__, __LINE__, colorStr.toAscii().data());

	QToolTip::showText(
				pWorkspace->mapToGlobal(//point
									//pWorkspace->mapToParent(pickPt)
									pickPt
//									+ QPoint(20,20)
									- pWorkspace->mapToParent(QPoint(0,0))
									)
			,
			//	  ViewWidget->rect().width(), ViewWidget->rect().height()),
			colorStr,
			mpViewWidget
			);

}

void WorkshopImageTool::onLButtonDown(Qt::ButtonState , const QPoint point)
{
	if(!mpViewWidget) return;

	switch(mToolMode)
	{

	default:
		break;
	case MODE_PICKER: {
//		QPoint pickPt = mpViewWidget->displayToOriginal(point.x(), point.y());

//		// Pick color in ImgRGB
//		QRgb pickRGB = ImgRGB.pixel(point);
//		emit signalColorPicker(pickPt, pickRGB);

//		QString colorStr;
//		colorStr.sprintf("RGB=%d,%d,%d gray=%d",
//						 qRed(pickRGB), qGreen(pickRGB), qBlue(pickRGB),
//						 qGray(pickRGB));
//		QToolTip::showText(
//				pWorkspace->mapToGlobal(point
//				+ pWorkspace->mapToParent(QPoint(0,0)) )
//				,
//				//	  ViewWidget->rect().width(), ViewWidget->rect().height()),
//				colorStr,
//				mpViewWidget
//				);
		}break;

		/// Zoom in, so calculate zoom origine and width
	case MODE_ZOOM_IN:
//		WindowView2Absolute(point.x(), point.y(), &xZoomCenter, &yZoomCenter);
//		ZoomScale++;
//		CalculateZoomWindow();
//		memset(ImgRGB.bits(), 0, viewPixel*(ImgRGB.depth()/8));

		mpViewWidget->zoomOnDisplayPixel( point.x(), point.y(), +1 );
		break;

	case MODE_ZOOM_OUT:
//		WindowView2Absolute(point.x(), point.y(), &xZoomCenter, &yZoomCenter);
//		ZoomScale--;
//		memset(ImgRGB.bits(), 0, viewPixel*(ImgRGB.depth()/8));
//		CalculateZoomWindow();

		mpViewWidget->zoomOnDisplayPixel( point.x(), point.y(), -1 );
		break;

	case MODE_MOVE:
		// Beware : here, we count in zoomed pixels !! (else we'll lose some slow moves)
		myStopX = point.x();
		myStopY = point.y();
		myStartX = xZoomCenter;
		myStartY = yZoomCenter;
		moveDx = moveDy = 0;
		fprintf(stderr, "WImgTool::%s:%d : activate select pressed\n",
				__func__, __LINE__);

		selectPressed = true;
		fprintf(stderr, "WImTool::%s:%d : Move view : start at (%d,%d)\n",
				__func__, __LINE__,
				myStartX, myStartY);
		break;

	case MODE_SELECT: {
		int selX, selY;
		WindowView2Absolute(point.x(), point.y(), &selX, &selY);

//FIXME		selectedMask = doc->getMaskFromPos(selX, selY);
		refreshDisplay();

		}
		break;

	case MODE_ADD_RECT:
		WindowView2Absolute(point.x(), point.y(), &myStartX, &myStartY);
		myStopX = myStartX; // for drawing first rectangle
		myStopY = myStartY;
		selectPressed = true;
		refreshDisplay();
		break;

	}

	slotUpdateView();
}


// convert window view coord (from vBox) to absolute
void WorkshopImageTool::WindowView2Absolute(int Vx, int Vy, int * Ax, int * Ay)
{
	// modif!!
	//QPoint Wpos = drawWidget->pos() + vBox->pos();
	QPoint Wpos(0,0);

	if(ZoomScale>0) {
		*Ax = xZoomOrigine + (Vx - Wpos.x())/ZoomScale;
		*Ay = yZoomOrigine + (Vy - Wpos.y())/ZoomScale;
	}
	else {
		int scale = 2-ZoomScale;
		*Ax = xZoomOrigine + (Vx - Wpos.x())*scale;
		*Ay = yZoomOrigine + (Vy - Wpos.y())*scale;
	}

}

void WorkshopImageTool::Absolute2View(int Ax, int Ay, int * Vx, int * Vy)
{
	if(ZoomScale > 0) {
		*Vx = (Ax - xZoomOrigine) * ZoomScale;
		*Vy = (Ay - yZoomOrigine) * ZoomScale;
	}
	else
	{
		int scale = 2 - ZoomScale;
		*Vx = (Ax - xZoomOrigine) /scale;
		*Vy = (Ay - yZoomOrigine) /scale;
	}
}

void WorkshopImageTool::onLButtonUp(Qt::ButtonState , const QPoint point)
{
	switch(mToolMode)
	{
	case MODE_ADD_RECT: {

		WindowView2Absolute(point.x(), point.y(), &myStopX, &myStopY);

		fprintf(stderr, "WImgTool::%s:%d : clear select pressed\n",
				__func__, __LINE__);
		selectPressed = false;

		// set detection mask to detection object and to view object
		int tmp;
		if(myStartX > myStopX) {
			tmp = myStopX;
			myStopX = myStartX;
			myStartX = tmp;
		}
		if(myStartY > myStopY) {
			tmp = myStopY;
			myStopY = myStartY;
			myStartY = tmp;
		}
		if(myStopX >= (int)imageSize.width) myStopX = (int)imageSize.width-1;
		if(myStopY >= (int)imageSize.height) myStopY = (int)imageSize.height-1;

		if(myStartX <0) myStartX = 0;
		if(myStartY <0) myStartY = 0;

		int w = myStopX - myStartX;
		int h = myStopY - myStartY;

		fprintf(stderr, "WImgTool::%s:%d : added rect = %d,%d+%dx%d\n",
				__func__, __LINE__,
				myStartX, myStartY, w, h);

//FIXME		m_pWorkshopImage->addMask(myStartX, myStartY, w, h);

		}
		break;
	default:

		fprintf(stderr, "WImgTool::%s:%d : clear select pressed\n",
				__func__, __LINE__);
		selectPressed = false;
		break;
	}

}

void WorkshopImageTool::onRButtonDown(Qt::ButtonState , const QPoint )
{

}
void WorkshopImageTool::onRButtonUp(Qt::ButtonState , const QPoint )
{

}

void WorkshopImageTool::onMouseMove(Qt::ButtonState nFlags, const QPoint point)
{
//	fprintf(stderr, "%s:%d nFlags=%d Left=%c toolMode=%d pressed=%c\n",
//			__func__, __LINE__, (int)nFlags,
//			( nFlags & Qt::LeftButton )?'T':'F',
//			mToolMode,
//			selectPressed ? 'T':'F');
	if( ( nFlags & Qt::LeftButton ) )
	{
		if(mToolMode == MODE_PICKER) {
			onLButtonDown(Qt::NoButton, point);
		}

		if(mToolMode == MODE_MOVE && selectPressed) {
			moveDx -= (point.x() - myStopX);
			moveDy -= (point.y() - myStopY);

			myStopX = point.x();
			myStopY = point.y();
			if(ZoomScale > 0) {
				xZoomCenter = myStartX + (moveDx / ZoomScale);
				yZoomCenter = myStartY + (moveDy / ZoomScale);
			} else {
				int scale = 2 - ZoomScale;
				xZoomCenter = myStartX + (moveDx * scale);
				yZoomCenter = myStartY + (moveDy * scale);
			}
			fprintf(stderr, "%s:%d MOVING: nFlags=%d Left=%c toolMode=%d pressed=%c"
					" => center =%d,%d\n",
					__func__, __LINE__, (int)nFlags,
					( nFlags & Qt::LeftButton )?'T':'F',
					mToolMode,
					selectPressed ? 'T':'F',
					xZoomCenter, yZoomCenter
					);
			CalculateZoomWindow();
			fprintf(stderr, "%s:%d MOVING: nFlags=%d Left=%c toolMode=%d pressed=%c"
					" => center =%d,%d\n",
					__func__, __LINE__, (int)nFlags,
					( nFlags & Qt::LeftButton )?'T':'F',
					mToolMode,
					selectPressed ? 'T':'F',
					xZoomCenter, yZoomCenter
					);
		}

		if(mToolMode == MODE_ADD_RECT && selectPressed)
		{
			WindowView2Absolute(point.x(), point.y(),
								&myStopX, &myStopY);
			fprintf(stderr, "WImgTool::%s:%d : adding rect : stop=%d,%d\n",
					__func__, __LINE__, myStopX, myStopY);
		}

		slotUpdateView();
	}
}
