/***************************************************************************
	workshopvideocapture.cpp  -  live video capture/display/processing tool for Piaf
							 -------------------
	begin                : Wed Nov 13 10:07:22 CET 2002
	copyright            : (C) 2002 by Christophe Seyve
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

// include files for Qt
#include <qprinter.h>
#include <qpainter.h>
#include <q3accel.h>
#include <qlayout.h>
#include <qbitmap.h>
#include <qcursor.h>
#include <qstring.h>

// test for new main windows
#include <q3mainwindow.h>
#include <q3grid.h>
//Added by qt3to4:
#include <QLabel>
#include <Q3GridLayout>
#include <QPixmap>
#include <QResizeEvent>
#include <Q3PopupMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QCloseEvent>
#include <QWorkspace>
#include <QToolTip>


// application specific includes
#include "workshopvideocapture.h"

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif


#define IMAGEDIR BASE_DIRECTORY "images/pixmaps/"

//#define __DEBUG_ZOOMING__

WorkshopVideoCaptureView::WorkshopVideoCaptureView(
		SwVideoAcquisition * va,
		QWidget *parent, const char* name, Qt::WidgetAttribute wflags)
			: WorkshopTool(parent, name, wflags)
{
	VideoCaptureDoc * pDoc;

	if(va) {
		pDoc = new VideoCaptureDoc(va);
	} else {
		pDoc = new VideoCaptureDoc();
	}

	if(!pDoc) {
		fprintf(stderr, "WorkshopVideoCaptureView : cannot create VideoCaptureDoc for input SwVideoAcquisition.\n");
		return;
	}
	doc = pDoc;
	((QWorkspace *)parent)->addWindow((QWidget *)pWin);

	pWorkspace = NULL;

	init();

	QPixmap winIcon = QPixmap(BASE_DIRECTORY "images/pixmaps/camera-web.png");
	display()->setIcon(winIcon);
}

WorkshopVideoCaptureView::WorkshopVideoCaptureView(VideoCaptureDoc * pDoc,
	   QWidget *parent, const char* name, Qt::WidgetAttribute wflags)
 : WorkshopTool(parent, name, wflags)
{
	doc = pDoc;
	pWorkspace = NULL;

	init();

	QPixmap winIcon = QPixmap(BASE_DIRECTORY "images/pixmaps/camera-web.png");
	display()->setIcon(winIcon);
}

void WorkshopVideoCaptureView::setWorkspace(QWorkspace * wksp) {
	pWorkspace = wksp;
	if(pWorkspace) {
		pWorkspace->addWindow(display());
	}
}

void WorkshopVideoCaptureView::init()
{
	connect(doc, SIGNAL(documentChanged()), this, SLOT(slotDocumentChanged()));
	mask = NULL;
	drawWidget = NULL;
	m_isLocked = false;

	// modif!!
	dpw=NULL;
	dsw=NULL;
	selectPressed = false;
	ToolMode = 0;

	// Zoom initialisation
	ZoomScale = 1;
	// set view size from image size
	imageSize.width  = 0;
	imageSize.height = 0;

	originalImage = NULL;
	viewPixel=0;
	setViewSize();
	setToolbar();


	pWin->setMouseTracking(TRUE);
	showMask = false;
	selectedMask = NULL;

	// Zoom initialisation
	ZoomScale = 1;

	// color
	video_picture pic;
	doc->getpicture(&pic);
	brightness = pic.brightness;
	contrast = pic.contrast;
	color = pic.colour;
	white = pic.whiteness;
	hue = pic.hue;

	filterManager = NULL;
	mpegEncoder = NULL;
	record = false;
	recordNum = 0;


	pWin->setCaption(tr("Video Acquisition"));
	connect(pWin, SIGNAL(signalResizeEvent(QResizeEvent *)),
		this, SLOT(resizeEvent(QResizeEvent *)));
	pWin->show();
}

void WorkshopVideoCaptureView::initTool()
{
	// enable handling measures
	enableDisplay(CLASS_VIDEO);
	enableCreate(CLASS_VIDEO);
	// disable handling image and video
	disableDisplay(CLASS_IMAGE);
	disableCreate(CLASS_IMAGE);
	disableDisplay(CLASS_MEASURE);
	disableCreate(CLASS_MEASURE);
}

WorkshopVideoCaptureView::~WorkshopVideoCaptureView()
{
	// delete filter manager
	if(filterManager)
		delete filterManager;

	// video encoder
	if(mpegEncoder)
		delete mpegEncoder;

	if(dsw)
		delete dsw;
	if(dpw)
		delete dpw;

}

VideoCaptureDoc *WorkshopVideoCaptureView::getDocument() const
{
	return doc;
}

#define TOOLBAR_HEIGHT 40





void WorkshopVideoCaptureView::setToolbar()
{
	chdir(BASE_DIRECTORY "images/pixmaps");

	//------------ HMI -------------
	vBox = new Q3VBox(pWin);


	// vBox resizing
	vBox->setMinimumSize(viewSize.width, viewSize.height + TOOLBAR_HEIGHT);
	hBox = new Q3HBox(vBox);
   // hBox->setMinimumSize(viewSize.width, 22);


	//hBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed, false);
	hBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed, false);

	QPixmap pixIcon;

	// ------------- DEVICE SECTION
	bDevice = new QPushButton(hBox);
	if(pixIcon.load("IconDeviceMenu.xpm"))
		bDevice->setPixmap(pixIcon);
	else
		bDevice->setText(tr("Device"));
	bDevice->setFlat(true);
	bDevice->setToggleButton(true);

	deviceMenu = new Q3PopupMenu(bDevice, "popupMenu_Device");

	pixIcon = QPixmap("IconDeviceSize.xpm");
	actDevEdit = new QAction(pixIcon, tr("Acquisition size"), QString(""), this, "acq");
	actDevEdit->addTo(deviceMenu);
	connect(actDevEdit, SIGNAL(activated()), this, SLOT(slotDeviceSize()));
	deviceMenu->connectItem( 0, actDevEdit, SLOT(activated()) );

	pixIcon = QPixmap("IconDeviceParams.xpm");
	actDevParams = new QAction(pixIcon, tr("Image settings"), QString(""), this, "device");
	actDevParams->addTo(deviceMenu);
	connect(actDevParams, SIGNAL(activated()), this, SLOT(slotDeviceParams()));
	deviceMenu->connectItem( 0, actDevParams, SLOT(activated()) );
	bDevice->setPopup(deviceMenu);

	// ------------- MASK SECTION
	aMenuMask = new QPushButton(hBox);
	if(pixIcon.load("IconSelAreaMenu.xpm"))
		aMenuMask->setPixmap(pixIcon);
	else
		aMenuMask->setText(tr("Select area"));
	aMenuMask->setFlat(true);
	aMenuMask->setToggleButton(true);

	maskMenu = new Q3PopupMenu(aMenuMask, "popupMenu_Mask");
	pixIcon = QPixmap("IconSelArea.xpm");
	actSelMask = new QAction(pixIcon, tr("Select area"), QString(""), this, "sel");
	actSelMask->addTo(maskMenu);
	connect(actSelMask, SIGNAL(activated()), this, SLOT(slotSelMask()));
	maskMenu->connectItem( 0, actSelMask, SLOT(activated()) );

	aMenuMask->setPopup(maskMenu);

	pixIcon = QPixmap("IconAddArea.xpm");
	actAddMask = new QAction(pixIcon, tr("Add area"), QString(""),this, "add");
	actAddMask->addTo(maskMenu);
	connect(actAddMask, SIGNAL(activated()), this, SLOT(slotAddMask()));
	maskMenu->connectItem( 0, actAddMask, SLOT(activated()) );

	pixIcon = QPixmap("IconDelArea.xpm");
	actDelSelectedMask = new QAction(pixIcon, tr("Add area"), QString(""), this, "del");
	actDelSelectedMask->addTo(maskMenu);
	connect(actDelSelectedMask, SIGNAL(activated()), this, SLOT(slotDelSelectedMask()));
	maskMenu->connectItem( 0, actDelSelectedMask, SLOT(activated()) );

	pixIcon = QPixmap("IconAllArea.xpm");
	actAllMask = new QAction(pixIcon, tr("Add area"), Q3Accel::stringToKey(tr("Ctrl+A")), this, "all");
	actAllMask->addTo(maskMenu);
	connect(actAllMask, SIGNAL(activated()), this, SLOT(slotClearMask()));
	maskMenu->connectItem( 0, actAllMask, SLOT(activated()) );



	// ------------- VIEW OPTIONS
	//hBox->insertSeparator(-1);
	aMenuView = new QPushButton( hBox);
	if(pixIcon.load("IconMenuView.png"))
		aMenuView->setPixmap(pixIcon);
	else
		aMenuView->setText(tr("View menu"));
	aMenuView->setFlat(true);
	aMenuView->setToggleButton(true);
	QToolTip::add(aMenuView, tr("Display view menu: zooming, moving..."));

	// view selection mode menu
	viewMenu = new Q3PopupMenu(aMenuView, "popupMenu");
	pixIcon = QPixmap("IconZoomInView.png");
	actZoomInView = new QAction(pixIcon, tr("Zoom in"),  Q3Accel::stringToKey(tr("Ctrl++")), this, "in");
	actZoomInView->addTo(viewMenu);
	connect(actZoomInView, SIGNAL(activated()), this, SLOT(slotZoomInMode()));
	viewMenu->connectItem( 0, actZoomInView, SLOT(activated()) );
	aMenuView->setPopup(viewMenu);

	pixIcon = QPixmap("IconZoomFitView.png");
	actZoomFitView = new QAction(pixIcon, tr("Zoom 1:1"), Q3Accel::stringToKey(tr("Ctrl+1")), this, "fit");
	actZoomFitView->addTo(viewMenu);
	connect(actZoomFitView, SIGNAL(activated()), this, SLOT(slotZoomFit()));

	pixIcon = QPixmap("IconZoomOutView.png");
	actZoomOutView = new QAction(pixIcon, tr("Zoom out"), Q3Accel::stringToKey(tr("Ctrl+-")), this, "out");
	actZoomOutView->addTo(viewMenu);
	connect(actZoomOutView, SIGNAL(activated()), this, SLOT(slotZoomOutMode()));

	pixIcon = QPixmap("IconMoveView.png");
	actMoveView = new QAction(pixIcon, tr("Move"), Q3Accel::stringToKey(tr("Ctrl+M")), this, "move");
	actMoveView->addTo(viewMenu);
	connect(actMoveView, SIGNAL(activated()), this, SLOT(slotMoveMode()));

	// --- play/pause
	bPlayPause = new QPushButton(hBox);
	if(pixIcon.load("IconPause.xpm"))
		bPlayPause->setPixmap(pixIcon);
	else
		bPlayPause->setText(tr("Pause"));
	bPlayPause->setFlat(true);
	bPlayPause->setToggleButton(false);
	connect(bPlayPause, SIGNAL(pressed()), this, SLOT(slotPlayPause()));
	freeze = false;

	// --- record
	bRecord = new QPushButton(hBox);
	if(pixIcon.load("IconRecordPause.png"))
		bRecord->setPixmap(pixIcon);
	else
		bRecord->setText(tr("Record"));
	bRecord->setFlat(true);
	bRecord->setToggleButton(false);
	connect(bRecord, SIGNAL(pressed()), this, SLOT(slotRecord()));

	// --- export
	bExport = new QPushButton(hBox);
	if(pixIcon.load("snapshot.png"))
		bExport->setPixmap(pixIcon);
	else
		bExport->setText(tr("Snapshot"));
	bExport->setFlat(true);
	bExport->setToggleButton(false);
	////////////////////////////////
	connect(bExport, SIGNAL(clicked()), this, SLOT(slotSaveImage()));
	////////////////////////////////

	// FILTERS
	filtersButton = new QPushButton(hBox);
	if(pixIcon.load("IconFilters.png")) {
		filtersButton->setPixmap(pixIcon);
	} else {
		filtersButton->setText(tr("Filters"));
	}
	filtersButton->setFlat(true);
	filtersButton->setToggleButton(false);
	connect(filtersButton, SIGNAL(pressed()), this, SLOT(slotFilters()));

	// fix height
	hBox->setMinimumWidth(5*23 + 210);
	pWin->setMinimumWidth(5*23 + 210);
	hBox->setFixedHeight(TOOLBAR_HEIGHT);
	hBox->setSpacing(0);
	hBox->setMargin(0);

	hBox->updateGeometry();

	drawWidget = new ImageWidget(vBox, NULL, Qt::WRepaintNoErase);
	drawWidget->setMinimumSize( viewSize.width, viewSize.height );
	drawWidget->updateGeometry();
	drawWidget->update();
	if(drawWidget) {
		drawWidget->setRefImage(&ImgRGB);
	}

	pWin->setGeometry(0,0, viewSize.width, viewSize.height + TOOLBAR_HEIGHT);
	// resize window
	pWin->resize(viewSize.width, viewSize.height + TOOLBAR_HEIGHT);

	drawWidget->setMouseTracking(true);

	connect(drawWidget, SIGNAL(mouseMoved( QMouseEvent*)),
			SLOT(mouseMoveEvent(  QMouseEvent*)));
	connect(drawWidget, SIGNAL(mousePressed( QMouseEvent *)),
			SLOT(mousePressEvent(  QMouseEvent*)));
	connect(drawWidget, SIGNAL(mouseReleased( QMouseEvent *)),
			SLOT(mouseReleaseEvent( QMouseEvent*)));


	vBox->updateGeometry();

	pWin->updateGeometry();
	pWin->showNormal();
}

void WorkshopVideoCaptureView::changeViewSize()
{
	// realloc
	if(ZoomScale>0) {
		viewSize.width  -= (viewSize.width % ZoomScale);
		viewSize.height -= (viewSize.height % ZoomScale);
	}
	viewPixel = viewSize.width*viewSize.height;

#ifdef __DEBUG_ZOOMING__
	printf("View Size : %ld x %ld -> %d bytes\n",
		viewSize.width, viewSize.height, viewPixel);
#endif

	ImgRGB.create(viewSize.width, viewSize.height, 32);
	memset(ImgRGB.bits(), 0, viewPixel*4);
	if(drawWidget) {
		drawWidget->setRefImage(&ImgRGB);
	}

	CalculateZoomWindow();
}

void WorkshopVideoCaptureView::setViewSize()
{
	// get image size
	tBoxSize newSize = doc->getImageSize();

	if(newSize.width != imageSize.width || newSize.height != imageSize.height)
	{
		imageSize.width = newSize.width;
		imageSize.height = newSize.height;
		imagePixel = imageSize.width*imageSize.height;
		if(originalImage) delete [] originalImage;

		originalImage = new unsigned char [imagePixel*4];

		// clear old image
		if(viewPixel!=0)
			memset(ImgRGB.bits(), 0, viewPixel*4);
		viewSize.width = newSize.width;
		viewSize.height = newSize.height;
		changeViewSize();
	}


	// construct image
#ifdef __DEBUG_ZOOMING__
	printf("setViewSize (0,0,%ld, %ld)\n", viewSize.width, viewSize.height);
#endif
}






/* -------------------------   SLOTS   -------------------------
*/

/** slotDocumentChanged
	VideoCaptureDoc emitted documentChanged signal to tell that a new image is available.
*/
void WorkshopVideoCaptureView::slotDocumentChanged()
{
	if(freeze) {
		return;
	}

	refreshDisplay();
}

void WorkshopVideoCaptureView::refreshDisplay()
{
	// changing view
	u32 * imgRgb = (u32 *) ImgRGB.bits();
	mask = doc->getMaskBuffer();

	showMask = ( mask != NULL);
	if(!freeze) {
		if(m_isLocked) {
			fprintf(stderr, "VidCaptView::%s:%d : locked. return now\n", __func__, __LINE__);
			return;
		}

		m_isLocked = true;

		// check for resize
		if(ImgRGB.width()!=(int)viewSize.width || ImgRGB.height()!=(int)viewSize.height) {
			changeViewSize();
			imgRgb = (u32 *) ImgRGB.bits();
			if(drawWidget) {
				drawWidget->setMinimumSize(viewSize.width, viewSize.height);
				drawWidget->updateGeometry();
			}
			if(vBox) {
				vBox->setMinimumSize(viewSize.width, viewSize.height+TOOLBAR_HEIGHT);
				vBox->updateGeometry();
			}
		}




		memcpy(originalImage,
			   doc->getCurrentImageRGB(),
			   imageSize.width*imageSize.height*4);

		//
		/******************* FILTER PROCESSING *********************/
		if(filterManager) {
			swImageStruct image;
			memset(&image, 0, sizeof(swImageStruct));

			image.width = (int)imageSize.width;
			image.height = (int)imageSize.height;
			image.depth = 4; // rgb32
			image.buffer_size = image.width * image.height * 4; // RGB 32

			image.buffer = originalImage; // Buffer

			filterManager->processImage(&image);
		}


		// record video on demand
		if(record && mpegEncoder) {
			unsigned char * p32 = originalImage;
			mpegEncoder->encodeFrameRGB32( p32 );
		}

		// Display image (after zooming ...)
		if(ZoomScale == 1) {
			int vwidth4  = (int)ImgRGB.width()*4;
			int vheight = (int)ImgRGB.height();
			int iwidth4  = (int)imageSize.width*4;
			int iheight = (int)imageSize.height;

			int minwidth4 = min( vwidth4, (iwidth4-4*xZoomOrigine) );
			int minheight = min(vheight, (iheight-yZoomOrigine));

			if(vwidth4 == iwidth4 && vheight == iheight
				&& xZoomOrigine==0 && yZoomOrigine==0)
				memcpy(imgRgb,  originalImage, viewPixel*4);
			else {
				unsigned char * orig = originalImage
					+  yZoomOrigine*iwidth4 + 4*xZoomOrigine;
				for(int r = 0; r<minheight; r++)
				{
					memcpy((unsigned char *)imgRgb + r*vwidth4,
							 orig,
							 minwidth4 );
					orig += iwidth4;
				}
			}
			if(showMask) {
				int iwidth  = (int)imageSize.width;
				int vwidth  = (int)viewSize.width;
				int minwidth = min( vwidth, (iwidth-xZoomOrigine) );

				for(int r = 0; r<minheight; r++) {
					int vpos = r*vwidth;
					int ipos = (r+yZoomOrigine)*iwidth+xZoomOrigine;
					for(int c=0; c<minwidth; c++)
					{
						if(!mask[ipos]) {
							imgRgb[vpos] =( ((imgRgb[vpos] >> 1) & 0x7f7f7f) | 0x808080);
						}
						vpos++;
						ipos++;
					}
				}
			}
		}
		else // over or under sampling
		  if (ZoomScale > 1) {
			u32 * orig = (u32 *)originalImage;
			int r,c;
			int origx = xZoomOrigine;
			int origy = yZoomOrigine;
			int vwidth  = (int)ImgRGB.width();
			int vheight = (int)ImgRGB.height();
			int iwidth  = (int)imageSize.width;
			int iheight = (int)imageSize.height;
			int minwidth = min(vwidth, (iwidth-origx)*ZoomScale);
			int minheight = min(vheight, (iheight-origy)*ZoomScale);
			int nr;

			imgRgb = (u32 *)ImgRGB.bits();
			for(r = 0, nr=0; r<minheight; r+=ZoomScale, nr++)
			{
				int pos = r * vwidth ;
				int posi = ((nr+origy)*iwidth + origx);
				int kmax = min(ZoomScale, minheight - r);
				int nc=0;
				for(c=0; c<minwidth; c+=ZoomScale)
				{
					u32 pix = orig[posi];
					if(showMask && !mask[posi]) {
						pix = (((pix >> 1) & 0x7f7f7f) | 0x808080);
					}
					int lmax = min(ZoomScale, minwidth  - c);
					for(int k=0; k<kmax;k++) {
						int dk = k * vwidth;
						for(int l=0; l<lmax;l++)
						{
							int pos4 = (pos + dk + l);
							imgRgb[pos4] = pix;
						}
					}
					nc++;
					posi ++;
					pos  += ZoomScale;
				}
			}
		}
		else // zoomscale <= 0 : 0 = 1/2 , -1 = 1/3
		{
			int sample = 2-ZoomScale;

			u32 * orig = (u32 *)originalImage;
			int origx = xZoomOrigine;
			int origy = yZoomOrigine;
			int vwidth  = (int)viewSize.width;
			int vheight = (int)viewSize.height;
			int iwidth  = (int)imageSize.width;
			int minwidth = min(vwidth, iwidth/sample);
			int minheight = min(vheight, (int)imageSize.height/sample);

			int nr = 0;
			for(int r=0; nr<minheight; r+=sample, nr++)
			{
				int pos = nr * vwidth ;
				int posi = ((r+origy)*iwidth + origx);
				int nc=0;
				for(int c=0; nc<minwidth; c+=sample, nc++) {
					u32 pix = orig[posi];
					if(showMask && !mask[posi]) {
						pix = (((pix >> 1) & 0x7f7f7f) | 0x808080);
					}

					// under sample
					imgRgb[pos] = pix;
					pos ++;
					posi+=sample;
				}
			}
		}

		m_isLocked = false;
	} else {
		m_isLocked = false;
	}

	drawWidget->repaint();
}

//////////////////////////////////////
void WorkshopVideoCaptureView::slotSaveImage()
{
   QImage * newImage = new QImage(ImgRGB);

   emit ImageSaved(newImage);
}
/////////////////////////////////////


void WorkshopVideoCaptureView::paintEvent(QPaintEvent * /*e*/)
{
	QPainter p(drawWidget);
	p.drawImage(0     , 0      , ImgRGB);
	if(ToolMode == MODE_ADD_RECT && selectPressed) {
		p.setPen(QColor(0,0,255));
		int Vx, Vy, w, h;
		Absolute2View(myStartX, myStartY, &Vx, &Vy);
		if( ZoomScale > 0)
		{
			w = ZoomScale * (myStopX-myStartX);
			h = ZoomScale * (myStopY-myStartY);
		} else {
			int scale = 2 - ZoomScale;
			w = (myStopX-myStartX) / scale;
			h = (myStopY-myStartY) / scale;
		}
		p.drawRect(Vx, Vy,
				w,
				h);
	}
	if(ToolMode == MODE_SELECT && selectedMask)
	{
		p.setPen(QColor(0,255,0));
		int Vx, Vy, w, h;
		Absolute2View(selectedMask->x(), selectedMask->y(), &Vx, &Vy);
		if( ZoomScale > 0)
		{
			w = ZoomScale * selectedMask->width();
			h = ZoomScale * selectedMask->height();
		} else {
			int scale = 2 - ZoomScale;
			w = selectedMask->width() / scale;
			h = selectedMask->height() / scale;
		}
		p.drawRect(Vx, Vy,
				w,
				h);
	}

}


/* Device slots
*************************************************************************
*/
typedef struct _swVSize {
	int width;
	int height;
	char * format;
	} swVSize;

swVSize vsizes[] = {
	{128, 96, "128x96 subGCIF"},
	{160, 120, "160x120 QSIF"},
	{176, 144, "176x144 QCIF"},
	{320, 240, "320x240 SIF"},
	{352, 288, "352x288 CIF"},
	{640, 480, "640x480 VGA"},
	{768, 576, "768x576 PAL"}
	};
int vsizes_max = 7;

void WorkshopVideoCaptureView::slotDeviceSize()
{
	// select acquisition size
	if(dsw) {
		dsw->show();
		return;
	}

	dsw = new QWidget(pWorkspace,"device size",0);

	dsw->setCaption( tr("Image size"));

	dsw->setMinimumSize(302, 48);
	dsw->setGeometry(0, 0, 302, 48);

	Q3GridLayout * grid = new Q3GridLayout(dsw, 2, 0, 0);

	///add label
	QLabel * label1 = new QLabel(tr("Size :"), dsw, 0, 0);
	label1->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
	label1->setMinimumSize( 150, 24);
	int gridpos = 0;
	grid->addWidget(label1, gridpos, 0);

	QComboBox * combo1 = new QComboBox(dsw);

	grid->addWidget(combo1, gridpos++, 1);
	video_capability vc;
	doc->getcapability(&vc);

	combo1->setMinimumSize( 150, 24);
	int cur=0;
	for(int i=0; i<vsizes_max; i++)
	{
		if(vc.minwidth <= vsizes[i].width && vsizes[i].width <= vc.maxwidth
			&& vc.minheight <= vsizes[i].height && vsizes[i].height <= vc.maxheight)
		{
			combo1->insertItem(vsizes[i].format, -1);
			if( vsizes[i].width == (int)imageSize.width)
				combo1->setCurrentItem(cur);
			cur++;
		}
	}
	connect( combo1, SIGNAL( activated( const QString &) ), this, SLOT( slotSetSize( const QString & ) ) );

	// channel selection
	if(vc.channels>1)
	{
		QLabel * label2 = new QLabel(tr("Channel :"), dsw, 0, 0);
		label2->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
		label2->setMinimumSize( 150, 24);
		grid->addWidget(label2, gridpos, 0);
		QComboBox * combo2 = new QComboBox(dsw);
		label2->setMinimumSize( 150, 24);

		grid->addWidget(combo2, gridpos++, 1);
		for(int i=0; i<vc.channels;i++) {
			char txt[16];
			sprintf(txt, "%d", i);
			combo2->insertItem( txt, -1);
		}

		connect( combo2, SIGNAL( activated( const QString &) ), this, SLOT( slotSetChannel( const QString & ) ) );
	}

	if(pWorkspace) {
		pWorkspace->addWindow(dsw);
	}

	dsw->show();
}

// slot for selecting image acquisition size
void WorkshopVideoCaptureView::slotSetSize( const QString &str)
{
	printf("Change image acquisition size : '%s'\n", str.latin1());
	char txt[128];
	strcpy(txt, str.latin1());
	// reads
	char c;
	int width, height;
	if(sscanf(txt, "%d%c%d", &width, &c, &height))
	{
		// print new size
		printf("\tchanged to %dx%d\n", width, height);

		tBoxSize newSize;
		newSize.width = width;
		newSize.height = height;

		doc->changeAcqParams(newSize, 0);

		// check if view size is large enough
		if( viewSize.width < newSize.width || viewSize.height < newSize.height)
		{
			viewSize.width  = newSize.width;
			viewSize.height = newSize.height;
		}
		//
		setViewSize();
	}
}


// slot for selecting the acquisition channel
void WorkshopVideoCaptureView::slotSetChannel( const QString &str)
{

	printf("Change acquisition channel size : '%s'\n", str.latin1());
	bool ok=FALSE;

	int chnl = str.toInt( &ok, 10 );

	if(ok)
	{
		printf("ACQUISITION CHANNEL= %d.\n",chnl);
		doc->setChannel(chnl);
	}
	else
	{
		printf("ERROR : bad channel number -> slotSetChannel\n");
	}
}



void WorkshopVideoCaptureView::slotSetContrast(int cont)
{
	contrast = cont;
	doc->setpicture( brightness, hue, color, contrast, white );
}
void WorkshopVideoCaptureView::slotSetColor(int col)
{
	color = col;
	doc->setpicture( brightness, hue, color, contrast, white );
}
void WorkshopVideoCaptureView::slotSetHue(int h)
{
	hue = h;
	doc->setpicture( brightness, hue, color, contrast, white );
}
void WorkshopVideoCaptureView::slotSetBrightness(int br)
{
	brightness = br;
	doc->setpicture( brightness, hue, color, contrast, white );
}
void WorkshopVideoCaptureView::slotSetWhite(int w)
{
	white = w;
	doc->setpicture( brightness, hue, color, contrast, white );
}

void WorkshopVideoCaptureView::slotDeviceParams()
{
	//modif!!
	if(dpw){
		dpw->show();
		return;
	}

	// Create device parameters window
	dpw = new QWidget(pWorkspace,"device parameters",0);

	dpw->setMinimumSize(200, 100);
	dpw->setGeometry(0,0,200,100);

	Q3GridLayout * grid = new Q3GridLayout(dpw, 2, 0, 0);

	QLabel * label1 = new QLabel(tr("Brightness :"), dpw, 0, 0);
	label1->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
	int gridpos = 0;
	grid->addWidget(label1, gridpos, 0);

	QSlider * slider1 = new QSlider(0, 65535, // min and max values
				1, 0, // increment, start
				Qt::Horizontal, dpw
		);
	grid->addWidget(slider1, gridpos++, 1);
	slider1->setMinimumSize( 100, 20);
	slider1->setTickmarks( QSlider::NoMarks );
	slider1->setValue(brightness);
	slider1->setGeometry( 100, 0, 100, 20);
	slider1->updateGeometry();
	slider1->update();
	connect( slider1, SIGNAL( valueChanged( int ) ), this, SLOT( slotSetBrightness( int ) ) );

	QLabel * label2 = new QLabel(tr("Contrast :"), dpw, 0, 0);
	grid->addWidget(label2, gridpos, 0);
	label2->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
	QSlider * slider2 = new QSlider(0, 65535, // min and max values
			   1, 0, // increment, start
				Qt::Horizontal, dpw
	   );
	grid->addWidget(slider2, gridpos++, 1);
	slider2->setMinimumSize( 100, 20);
	slider2->setTickmarks( QSlider::NoMarks );
	slider2->setValue(contrast);
	slider2->setGeometry( 100, 0, 100, 20);
	slider2->updateGeometry();
	slider2->update();
	connect( slider2, SIGNAL( valueChanged( int ) ), this, SLOT( slotSetContrast( int ) ) );

	QLabel * label3 = new QLabel(tr("Hue :"), dpw, 0, 0);
	grid->addWidget(label3, gridpos, 0);
	label3->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
	QSlider * slider3 = new QSlider(0, 65535, // min and max values
			   1, 0, // increment, start
				Qt::Horizontal, dpw
	   );
	grid->addWidget(slider3, gridpos++, 1);
	slider3->setMinimumSize( 100, 20);
	slider3->setTickmarks( QSlider::NoMarks );
	slider3->setValue(hue);
	slider3->setGeometry( 100, 0, 100, 20);
	slider3->updateGeometry();
	slider3->update();
	connect( slider3, SIGNAL( valueChanged( int ) ), this, SLOT( slotSetHue( int ) ) );

	QLabel * label4 = new QLabel(tr("Color :"), dpw, 0, 0);
	grid->addWidget(label4, gridpos, 0);
	label4->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
	QSlider * slider4 = new QSlider(0, 65535, // min and max values
			   1, 0, // increment, start
				Qt::Horizontal, dpw
	   );
	grid->addWidget(slider4, gridpos++, 1);
	slider4->setMinimumSize( 100, 20);
	slider4->setTickmarks( QSlider::NoMarks );
	slider4->setValue(color);
	slider4->setGeometry( 100, 0, 100, 20);
	slider4->updateGeometry();
	slider4->update();
	connect( slider4, SIGNAL( valueChanged( int ) ), this, SLOT( slotSetColor( int ) ) );

	QLabel * label5 = new QLabel(tr("Whiteness :"), dpw, 0, 0);
	grid->addWidget(label5, gridpos, 0);

	label5->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
	QSlider * slider5 = new QSlider(0, 65535, // min and max values
			   1, 0, // increment, start
				Qt::Horizontal, dpw
	   );
	grid->addWidget(slider5, gridpos++, 1);
	slider5->setMinimumSize( 100, 20);
	slider5->setTickmarks( QSlider::NoMarks );
	slider5->setValue(white);
	slider5->setGeometry( 100, 0, 100, 20);
	slider5->updateGeometry();
	slider5->update();
	connect( slider5, SIGNAL( valueChanged( int ) ), this, SLOT( slotSetWhite( int ) ) );

	dpw->setCaption( tr("Image settings") );
	//	mw->setCentralWidget( slider1 );
//	grid->updateGeometry();
	dpw->updateGeometry();

	if(pWorkspace) {
		pWorkspace->addWindow(dpw);
	}
	dpw->show();
}



/* Mask  areas butotn group callback
*************************************************************************
*/
void WorkshopVideoCaptureView::slotMaskTools(int id){

	switch(id)
	{
	default:
	case 0 :  // select
		deviceMenu->exec( bDevice->mapToGlobal(QPoint(0,0)), 0);

		aMenuView->setDown(false);
		aMenuMask->setDown(false);
		break;
	case 1: // add
		slotMenuMask();
		aMenuView->setDown(false);
		bDevice->setDown(false);
		break;
	case 2 : // Zoom in
		slotMenuView();
		aMenuMask->setDown(false);
		bDevice->setDown(false);
		break;
	}
}

/* View modes
*************************************************************************
*/
void WorkshopVideoCaptureView::slotZoomInMode(){
	ToolMode = MODE_ZOOM_IN;
	QCursor zoomCursor = QCursor( QBitmap(IMAGEDIR "CursorZoomIn.bmp", 0),
		QBitmap(IMAGEDIR "CursorZoomInMask.bmp", 0));
	drawWidget->setCursor(zoomCursor);
	updateToolbar();
}

void WorkshopVideoCaptureView::slotZoomOutMode(){
	ToolMode = MODE_ZOOM_OUT;
	QCursor zoomCursor = QCursor( QBitmap(IMAGEDIR "CursorZoomOut.bmp", 0),
		QBitmap(IMAGEDIR "CursorZoomOutMask.bmp", 0));
	drawWidget->setCursor(zoomCursor);

	updateToolbar();
}


void WorkshopVideoCaptureView::slotZoomFit(){
#ifdef __DEBUG_ZOOMING__
	printf("Zoom fit\n");
#endif
	ZoomScale = 1;
	xZoomOrigine = 0;
	yZoomOrigine = 0;
	xZoomCenter = (int)viewSize.width / 2;
	yZoomCenter = (int)viewSize.height / 2;
	CalculateZoomWindow();
}
void WorkshopVideoCaptureView::slotMoveMode(){
	ToolMode = MODE_MOVE;

	QCursor zoomCursor = QCursor( QBitmap(IMAGEDIR "CursorMove.bmp", 0),
		QBitmap(IMAGEDIR "CursorMoveMask.bmp", 0));
	drawWidget->setCursor(zoomCursor);
	updateToolbar();
}

void WorkshopVideoCaptureView::slotMenuView()
{
	viewMenu->exec( aMenuView->mapToGlobal(QPoint(0,0)), 0);
	updateToolbar();
}
void WorkshopVideoCaptureView::updateToolbar() {
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


/* Mask modes
*************************************************************************
*/
void WorkshopVideoCaptureView::slotSelMask(){
	 ToolMode = MODE_SELECT;

	QCursor zoomCursor;
	drawWidget->setCursor(Qt::PointingHandCursor );

	updateToolbar();
}
void WorkshopVideoCaptureView::slotAddMask(){
	 ToolMode = MODE_ADD_RECT;
	QCursor zoomCursor = QCursor( QBitmap(IMAGEDIR "CursorAddArea.bmp", 0),
		QBitmap(IMAGEDIR "CursorAddAreaMask.bmp", 0));
	drawWidget->setCursor(zoomCursor);
	updateToolbar();
}
void WorkshopVideoCaptureView::slotClearMask(){

	showMask = false;
	doc->clearMask();
}

void WorkshopVideoCaptureView::slotDelSelectedMask(){
	if(ToolMode == MODE_SELECT)
		if(selectedMask) {
			printf("Delete area.\n");
			doc->removeMask(selectedMask);
			selectedMask = NULL;
		}
}
void WorkshopVideoCaptureView::slotMenuMask()
{
	maskMenu->exec( aMenuMask->mapToGlobal(QPoint(0,0)), 0);
}

/* PLAY/PAUSE
*************************************************************************
*/
void WorkshopVideoCaptureView::slotPlayPause()
{
	if(freeze) //
	{
		freeze = false;
		QPixmap icon("IconPause.xpm");
		bPlayPause->setPixmap(icon);
	} else {
		freeze = true;
		QPixmap icon("IconPlay.xpm");
		bPlayPause->setPixmap(icon);
	}
}
/* RECORD/STOP RECORD
*************************************************************************
*/

int WorkshopVideoCaptureView::loadFilterManager()
{
	if(!filterManager) {
		filterManager = new SwFilterManager(doc, NULL, "filterManager", 0);
		filterManager->setWorkspace(pWorkspace);
	} else {
		filterManager->showWindow();
	}

	return 1;
}


/***************************************************************

				VIDEO RECORDING SECTION

****************************************************************/
void WorkshopVideoCaptureView::startRecording()
{
	if(!mpegEncoder) {
		//mpegEncoder = new SwMpegEncoder(CODEC_ID_MPEG1VIDEO, (int)imageSize.width, (int)imageSize.height, 10);
		mpegEncoder = new OpenCVEncoder((int)imageSize.width, (int)imageSize.height, 10);
		mpegEncoder->setQuality(1);
	}
	char home[512] = "";
	strcpy(home, getenv("HOME"));

	char txt[512];
	bool exists = false;
	do {
		recordNum++;
		exists = false;
		sprintf(txt, "%s/Movies/Movie%03d.avi", home, recordNum);
		FILE * f = fopen(txt, "r");
		if(f) {
			exists = true;
			fclose(f);
		}
	} while(exists);
	strcpy(movieFile, txt);
	fprintf(stderr, "Starting encoding file '%s'\n", movieFile);

	if(mpegEncoder->startEncoder(txt))
		record = true;
	else
		record = false;
}

void WorkshopVideoCaptureView::stopRecording()
{
	record = false;
	mpegEncoder->stopEncoder();
	emit VideoSaved(movieFile);

}

bool WorkshopVideoCaptureView::isRecording()
{
	if(!mpegEncoder)
		return false;
	return record;
}



void WorkshopVideoCaptureView::slotRecord()
{
	chdir(IMAGEDIR);
	if(isRecording()) //
	{
		QPixmap icon;
		if(icon.load("IconRecordPause.png"))
			bRecord->setPixmap(icon);
		stopRecording();
	} else {
		startRecording();
		if(isRecording()) {
			QPixmap icon;
			if(icon.load("IconRecord.png"))
				bRecord->setPixmap(icon);
		}
	}
}

// ------------------ GLOBAL EVENTS -----------------------
void WorkshopVideoCaptureView::closeEvent(QCloseEvent*)
{
  // LEAVE THIS EMPTY: THE EVENT FILTER IN THE WorkshopVideoCaptureApp CLASS TAKES CARE FOR CLOSING
  // QWidget closeEvent must be prevented.

}


void WorkshopVideoCaptureView::resizeEvent( QResizeEvent * e)
{

//#ifdef __DEBUG_ZOOMING__
	printf("Resize event : %d x %d\n",
		e->size().width(), e->size().height());
//#endif

	// Change view size => change zoom origine
	QPoint Wpos = drawWidget->pos() + vBox->pos();
	viewSize.width  = e->size().width()  - Wpos.x();
	viewSize.height = e->size().height() - Wpos.y();

	drawWidget->resize(viewSize.width, viewSize.height);
	vBox->resize(viewSize.width, viewSize.height+26);
	// update window for zooming
	CalculateZoomWindow();
}


// --------------------- MOUSE EVENTS SECTION ----------------------------
void WorkshopVideoCaptureView::mousePressEvent(QMouseEvent* e)
{
	if( e->button() == Qt::LeftButton ) {
		onLButtonDown(e->state(),e->pos());
	} else if(e->button() == Qt::RightButton ) {
		onRButtonDown(e->state(),e->pos());
	}
}

void WorkshopVideoCaptureView::mouseReleaseEvent(QMouseEvent* e)
{
	if ( e->button() == Qt::LeftButton ) {
		onLButtonUp(e->state(),e->pos());
	} else if(e->button() == Qt::RightButton ) {
		onRButtonUp(e->state(),e->pos());
	}
}

void WorkshopVideoCaptureView::mouseMoveEvent(QMouseEvent* e)
{
	onMouseMove(e->state(),e->pos());
}



/* Calculate zoom window to be displayed
*/

void WorkshopVideoCaptureView::CalculateZoomWindow()
{
	int vwidth  = (int)viewSize.width;
	int vheight = (int)viewSize.height;
	int iwidth  = (int)imageSize.width;
	int iheight = (int)imageSize.height;

	// left/top limit
	if(ZoomScale >= 1) {
		// LEFT-RIGHT LIMITATION
		xZoomOrigine = xZoomCenter - (vwidth/ ZoomScale)/2;
		if(xZoomOrigine < 0) {
			xZoomOrigine = 0;
		}
		else {
			// right/bottom limit
			if((iwidth - xZoomOrigine) < (vwidth /ZoomScale)
					){
				xZoomOrigine = max(0,
					-(vwidth - iwidth * ZoomScale)/ZoomScale);
			}
		}

		yZoomOrigine = yZoomCenter - (vheight/ ZoomScale)/2;
		if(yZoomOrigine < 0){
			yZoomOrigine = 0;
		} else {

//			int yZoomMax = yZoomCenter + (iheight/ ZoomScale)/2;
			if( (iheight - yZoomOrigine) < (vheight /ZoomScale)
					) {
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
			xZoomOrigine = 0;
		}
		else {
			// right/bottom limit
			if((iwidth - xZoomOrigine) < (vwidth* scale)
					){
				xZoomOrigine = max(0,
					-(vwidth - iwidth /scale)* scale);
			}
		}

		yZoomOrigine = yZoomCenter - (vheight* scale)/2;
		if(yZoomOrigine < 0){
			yZoomOrigine = 0;
		} else {

			if( (iheight - yZoomOrigine) < (vheight * scale)
					) {
				yZoomOrigine = max(0,
					-(vheight - iheight / scale)* scale);
			}
		}

		// update center for move purpose
		xZoomCenter = xZoomOrigine + (vwidth * scale)/2;
		yZoomCenter = yZoomOrigine + (vheight * scale)/2;
	}
}

void WorkshopVideoCaptureView::onLButtonDown(Qt::ButtonState ,//unused nFlags,
	const QPoint point)
{
	switch(ToolMode)
	{
	default:
		break;
	/// Zoom in, so calculate zoom origine and width
	case MODE_ZOOM_IN:
		WindowView2Absolute(point.x(), point.y(), &xZoomCenter, &yZoomCenter);
		ZoomScale++;
		CalculateZoomWindow();
		memset(ImgRGB.bits(), 0, viewPixel*4);
		refreshDisplay();

		break;
	case MODE_ZOOM_OUT:
		WindowView2Absolute(point.x(), point.y(), &xZoomCenter, &yZoomCenter);
		ZoomScale--;
		memset(ImgRGB.bits(), 0, viewPixel*4);
		CalculateZoomWindow();
		refreshDisplay();
		break;
	case MODE_SELECT: {
		int selX, selY;
		WindowView2Absolute(point.x(), point.y(), &selX, &selY);

		selectedMask = doc->getMaskFromPos(selX, selY);
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

	case MODE_MOVE:
		// Beware : here, we count in zoomed pixels !! (else we'll lose some slow moves)
		myStopX = point.x();
		myStopY = point.y();
		myStartX = xZoomCenter;
		myStartY = yZoomCenter;
		moveDx = moveDy = 0;
		selectPressed = true;
		refreshDisplay();
		break;
	}
}


// convert window view coord (from vBox) to absolute
void WorkshopVideoCaptureView::WindowView2Absolute(int Vx, int Vy, int * Ax, int * Ay)
{
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

void WorkshopVideoCaptureView::Absolute2View(int Ax, int Ay, int * Vx, int * Vy)
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

void WorkshopVideoCaptureView::onLButtonUp(Qt::ButtonState , const QPoint point)
{
	switch(ToolMode)
	{
	case MODE_ADD_RECT: {

		WindowView2Absolute(point.x(), point.y(), &myStopX, &myStopY);

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
		doc->addMask(myStartX, myStartY, w, h);
		}
		break;
	default:
		selectPressed = false;
		break;
	}
}

void WorkshopVideoCaptureView::onRButtonDown(Qt::ButtonState /*nFlags*/, const QPoint /*point*/)
{

}
void WorkshopVideoCaptureView::onRButtonUp(Qt::ButtonState /*nFlags*/, const QPoint /*point*/)
{

}

void WorkshopVideoCaptureView::onMouseMove(Qt::ButtonState nFlags, const QPoint point)
{
	if ( nFlags & Qt::LeftButton ) {

		if(ToolMode == MODE_MOVE && selectPressed) {
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
			CalculateZoomWindow();
			refreshDisplay();
		}
		if(ToolMode == MODE_ADD_RECT && selectPressed)
		{
			WindowView2Absolute(point.x(), point.y(),
								&myStopX, &myStopY);
		}
	}
}


/* slot for filter window creation
*/
void WorkshopVideoCaptureView::slotFilters()
{
	printf("slotFilters\n");
	//
	loadFilterManager();
}
