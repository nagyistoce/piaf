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
#include "piaf-common.h"

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
#include "workshopimagetool.h"

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif


#define IMAGEDIR BASE_DIRECTORY "images/pixmaps/"

//#define __DEBUG_ZOOMING__

WorkshopVideoCaptureView::WorkshopVideoCaptureView(
		VirtualDeviceAcquisition * va,
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

		if(detailsView) {
			detailsView->setWorkspace((QWorkspace *)pWorkspace);
		}
	}
}

void WorkshopVideoCaptureView::init()
{
	// thread for background grabbing with synchronized unlocking
	m_pWorkshopVideoCaptureThread = new WorkshopVideoCaptureThread(doc);

	connect(m_pWorkshopVideoCaptureThread, SIGNAL(documentChanged()),
			this, SLOT(slotDocumentChanged()));

	m_isLocked = false;
	playGrayscale = false;

	deviceParamsWidget = NULL;
	deviceSettingsWidget = NULL;

	setToolbar();

	// color
	video_picture pic;
	doc->getpicture(&pic);
	brightness = pic.brightness;
	contrast = pic.contrast;
	color = pic.colour;
	white = pic.whiteness;
	hue = pic.hue;
	exposure = 0;

	pWin->setCaption(tr("Video Acquisition"));
	connect(pWin, SIGNAL(signalResizeEvent(QResizeEvent *)),
		this, SLOT(resizeEvent(QResizeEvent *)));
	pWin->show();

	// start thread
	m_pWorkshopVideoCaptureThread->start();
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
	vBox->setMinimumSize(320, 240 + TOOLBAR_HEIGHT);
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

	pixIcon = QPixmap("IconDeviceParams.png");
	actDevParams = new QAction(pixIcon, tr("Image settings"), QString(""), this, "device");
	actDevParams->addTo(deviceMenu);
	connect(actDevParams, SIGNAL(activated()), this, SLOT(slotDeviceParams()));
	deviceMenu->connectItem( 0, actDevParams, SLOT(activated()) );
	bDevice->setPopup(deviceMenu);





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


	// fix height
	hBox->setMinimumWidth(5*23 + 210);
	pWin->setMinimumWidth(5*23 + 210);
	hBox->setFixedHeight(TOOLBAR_HEIGHT);
	hBox->setSpacing(0);
	hBox->setMargin(0);

	hBox->updateGeometry();


	// DISPLAY IMAGE
	// image
	detailsImage = new WorkshopImage( "full size image" );
	detailsImage->QImage::create(320, 240, 32);

//	detailsView = new WorkshopImageTool( detailsImage, containerWorkshopImageTool,
	detailsView = new WorkshopImageTool( detailsImage, vBox,
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

	vBox->updateGeometry();

	pWin->updateGeometry();
	pWin->showNormal();
}

void WorkshopVideoCaptureView::slotResizeTool(QResizeEvent *e)
{
	int h = e->size().height() - 4;
	int w = e->size().width() - 4;

	vBox->resize(w, h);

	detailsView->display()->resize(w-4, h-36);
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
//	fprintf(stderr, "WorkshopVideoCaptureView::%s:%d !\n", __func__, __LINE__);
	refreshDisplay();
}

void WorkshopVideoCaptureView::refreshDisplay()
{
	// update view
	if(!freeze) {
		if(m_isLocked) {
			fprintf(stderr, "VidCaptView::%s:%d : locked. return now\n", __func__, __LINE__);
			return;
		}

		m_isLocked = true;
		CvSize theSize = doc->getImageSize();

		if( (detailsImage->width()!=(long)theSize.width)
			|| (detailsImage->height()!=(long)theSize.height)
			|| (detailsImage->depth()!= (playGrayscale?8:32))
			)
		{
			detailsImage->create(theSize.width, theSize.height, (playGrayscale?8:32));
		}

//		fprintf(stderr, "VidCaptView::%s:%d : detailsImage=%dx%dx%d "
//				"/ input=%dx%dx%d / playGray=%c\n",
//				__func__, __LINE__,
//				detailsImage->width(), detailsImage->height(), detailsImage->depth(),
//				theSize.width, theSize.height,
//				(playGrayscale?'T':'F')
//				);

		memcpy(detailsImage->bits(),
			   doc->getCurrentImageRGB(),
			   theSize.width*theSize.height*(playGrayscale?1:4));

		detailsView->setWorkshopImage(detailsImage);
		m_isLocked = false;
	} else {
		m_isLocked = false;
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
	if(deviceSettingsWidget) {
		deviceSettingsWidget->show();
		return;
	}
	CvSize imageSize = doc->getImageSize();

	deviceSettingsWidget = new QWidget(pWorkspace,"device size",0);

	deviceSettingsWidget->setCaption( tr("Image size"));

	deviceSettingsWidget->setMinimumSize(302, 48);
	deviceSettingsWidget->setGeometry(0, 0, 302, 48);

	Q3GridLayout * grid = new Q3GridLayout(deviceSettingsWidget, 2, 0, 0);

	///add label
	QLabel * label1 = new QLabel(tr("Size :"), deviceSettingsWidget, 0, 0);
	label1->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
	label1->setMinimumSize( 150, 24);
	int gridpos = 0;
	grid->addWidget(label1, gridpos, 0);

	QComboBox * combo1 = new QComboBox(deviceSettingsWidget);

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
		QLabel * label2 = new QLabel(tr("Channel :"), deviceSettingsWidget, 0, 0);
		label2->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
		label2->setMinimumSize( 150, 24);
		grid->addWidget(label2, gridpos, 0);
		QComboBox * combo2 = new QComboBox(deviceSettingsWidget);
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
		pWorkspace->addWindow(deviceSettingsWidget);
	}

	deviceSettingsWidget->show();
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
void WorkshopVideoCaptureView::slotSetExposure(int exp)
{
	exposure = exp*1000;
	doc->setexposure( exposure );
}

void WorkshopVideoCaptureView::slotSetGain( int g)
{
	gain = g;
	doc->setgain( gain );

}

void WorkshopVideoCaptureView::slotDeviceParams()
{
	//modif!!
	if(deviceParamsWidget){
		deviceParamsWidget->show();
		return;
	}

	// Create device parameters window
	deviceParamsWidget = new QWidget(pWorkspace,"device parameters",0);

	deviceParamsWidget->setMinimumSize(200, 100);
	deviceParamsWidget->setGeometry(0,0,200,100);

	Q3GridLayout * grid = new Q3GridLayout(deviceParamsWidget, 2, 0, 0);

	QLabel * label1 = new QLabel(tr("Brightness:"), deviceParamsWidget, 0, 0);
	label1->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
	int gridpos = 0;
	grid->addWidget(label1, gridpos, 0);

	QSlider * slider1 = new QSlider(0, 255, // min and max values
				1, 0, // increment, start
				Qt::Horizontal, deviceParamsWidget
				);
	grid->addWidget(slider1, gridpos++, 1);
	slider1->setMinimumSize( 100, 20);
	slider1->setTickmarks( QSlider::NoMarks );
	slider1->setValue(brightness);
	slider1->setGeometry( 100, 0, 100, 20);
	slider1->updateGeometry();
	slider1->update();
	connect( slider1, SIGNAL( valueChanged( int ) ), this, SLOT( slotSetBrightness( int ) ) );

	QLabel * label2 = new QLabel(tr("Contrast:"), deviceParamsWidget, 0, 0);
	grid->addWidget(label2, gridpos, 0);
	label2->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
	QSlider * slider2 = new QSlider(0, 255, // min and max values
			   1, 0, // increment, start
				Qt::Horizontal, deviceParamsWidget
	   );
	grid->addWidget(slider2, gridpos++, 1);
	slider2->setMinimumSize( 100, 20);
	slider2->setTickmarks( QSlider::NoMarks );
	slider2->setValue(contrast);
	slider2->setGeometry( 100, 0, 100, 20);
	slider2->updateGeometry();
	slider2->update();
	connect( slider2, SIGNAL( valueChanged( int ) ), this, SLOT( slotSetContrast( int ) ) );

	QLabel * label3 = new QLabel(tr("Hue:"), deviceParamsWidget, 0, 0);
	grid->addWidget(label3, gridpos, 0);
	label3->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
	QSlider * slider3 = new QSlider(0, 255, // min and max values
			   1, 0, // increment, start
				Qt::Horizontal, deviceParamsWidget
	   );
	grid->addWidget(slider3, gridpos++, 1);
	slider3->setMinimumSize( 100, 20);
	slider3->setTickmarks( QSlider::NoMarks );
	slider3->setValue(hue);
	slider3->setGeometry( 100, 0, 100, 20);
	slider3->updateGeometry();
	slider3->update();
	connect( slider3, SIGNAL( valueChanged( int ) ), this, SLOT( slotSetHue( int ) ) );

	QLabel * label4 = new QLabel(tr("Saturation:"), deviceParamsWidget, 0, 0);
	grid->addWidget(label4, gridpos, 0);
	label4->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
	QSlider * slider4 = new QSlider(0, 255, // min and max values
			   1, 0, // increment, start
				Qt::Horizontal, deviceParamsWidget
	   );
	grid->addWidget(slider4, gridpos++, 1);
	slider4->setMinimumSize( 100, 20);
	slider4->setTickmarks( QSlider::NoMarks );
	slider4->setValue(color);
	slider4->setGeometry( 100, 0, 100, 20);
	slider4->updateGeometry();
	slider4->update();
	connect( slider4, SIGNAL( valueChanged( int ) ), this, SLOT( slotSetColor( int ) ) );

	// WHITENESS
	QLabel * label5 = new QLabel(tr("Whiteness:"), deviceParamsWidget, 0, 0);
	grid->addWidget(label5, gridpos, 0);

	label5->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
	QSlider * slider5 = new QSlider(0, 255, // min and max values
			   1, 0, // increment, start
				Qt::Horizontal, deviceParamsWidget
	   );
	grid->addWidget(slider5, gridpos++, 1);
	slider5->setMinimumSize( 100, 20);
	slider5->setTickmarks( QSlider::NoMarks );
	slider5->setValue(white);
	slider5->setGeometry( 100, 0, 100, 20);
	slider5->updateGeometry();
	slider5->update();
	connect( slider5, SIGNAL( valueChanged( int ) ), this, SLOT( slotSetWhite( int ) ) );




	// EXPOSURE
	QLabel * label6 = new QLabel(tr("Exposure:"), deviceParamsWidget, 0, 0);
	grid->addWidget(label6, gridpos, 0);

	label6->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
	QSlider * slider6 = new QSlider(0, 255, // min and max values
			   1, 0, // increment, start
			   Qt::Horizontal, deviceParamsWidget
			   );
	grid->addWidget(slider6, gridpos++, 1);
	slider6->setMinimumSize( 100, 20);
	slider6->setTickmarks( QSlider::NoMarks );
	slider6->setValue(white);
	slider6->setGeometry( 100, 0, 100, 20);
	slider6->updateGeometry();
	slider6->update();
	connect( slider6, SIGNAL( valueChanged( int ) ), this, SLOT( slotSetExposure( int ) ) );




	// GAIN
	QLabel * label7 = new QLabel(tr("Gain:"), deviceParamsWidget, 0, 0);
	grid->addWidget(label7, gridpos, 0);

	label7->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
	QSlider * slider7 = new QSlider(0, 255, // min and max values
			   1, 0, // increment, start
			   Qt::Horizontal, deviceParamsWidget
			   );
	grid->addWidget(slider7, gridpos++, 1);
	slider7->setMinimumSize( 100, 20);
	slider7->setTickmarks( QSlider::NoMarks );
	slider7->setValue(white);
	slider7->setGeometry( 100, 0, 100, 20);
	slider7->updateGeometry();
	slider7->update();
	connect( slider7, SIGNAL( valueChanged( int ) ), this, SLOT( slotSetGain( int ) ) );







	// TITLE
	deviceParamsWidget->setCaption( tr("Image settings") );
	//	mw->setCentralWidget( slider1 );
//	grid->updateGeometry();
	deviceParamsWidget->updateGeometry();

	if(pWorkspace) {
		pWorkspace->addWindow(deviceParamsWidget);
	}
	deviceParamsWidget->show();
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


// ------------------ GLOBAL EVENTS -----------------------
void WorkshopVideoCaptureView::closeEvent(QCloseEvent*)
{
  // LEAVE THIS EMPTY: THE EVENT FILTER IN THE WorkshopVideoCaptureApp CLASS TAKES CARE FOR CLOSING
  // QWidget closeEvent must be prevented.

}



/*******************************************************************************



*******************************************************************************/



WorkshopVideoCaptureThread::WorkshopVideoCaptureThread(VideoCaptureDoc* pDoc)
{
	m_pDoc = pDoc;
}

WorkshopVideoCaptureThread::~WorkshopVideoCaptureThread()
{
	m_run = false;
	while(m_running)
	{
		usleep(10000);
	}
}

void WorkshopVideoCaptureThread::run()
{
	m_run = true;
	m_running = true;

	while(m_run)
	{
		// wait for grab
		if(m_pDoc->waitForImage() >= 0) {
			//fprintf(stderr, "WorkshopVideoCaptureThread::%s:%d amit documentChanged()\n",
			//		__func__, __LINE__);

			emit documentChanged();
		}
	}
}

