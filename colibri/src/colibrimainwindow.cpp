/***************************************************************************
 *  wafmeter - Woman Acceptance Factor measurement / Qt GUI class
 *
 *  2009-08-10 21:22:13
 *  Copyright  2007  Christophe Seyve
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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "colibrimainwindow.h"
#include "ui_colibrimainwindow.h"
#include "imgutils.h"

#include <QFileDialog>
#include <QPainter>
#include <QDateTime>
#include <QDesktopWidget>
#include <QMessageBox>

u8 mode_file = 0;

ColibriMainWindow::ColibriMainWindow(QWidget *parent)
	:
	QMainWindow(parent),
	ui(new Ui::ColibriMainWindow),
	mSettings(PIAFCOLIBRISETTINGS)
{
	ui->setupUi(this);

	m_pColibriThread = NULL;

	memset(&mSwImage, 0, sizeof(swImageStruct));

	QString filename = ":/qss/Colibri.qss";
	QFile file(filename);
	if(file.open(QFile::ReadOnly)) {
		QString styleSheet = QLatin1String(file.readAll());
		//setStyleSheet(styleSheet);
	}

	qtImage.load(":/icons/Colibri128.png");
	ui->imageLabel->setRefImage(&qtImage);

	connect(&m_timer, SIGNAL(timeout()), this, SLOT(on_m_timer_timeout()));

	loadSettings();
}

#define COLIBRISETTING_LASTPLUGINDIR	"LastPluginDir"
#define COLIBRISETTING_LASTIMAGEDIR		"LastImageDir"
#define COLIBRISETTING_LASTMOVIEDIR		"LastMovieDir"


void ColibriMainWindow::loadSettings() {
	// overwrite with batch settings
	QString entry = mSettings.readEntry(COLIBRISETTING_LASTPLUGINDIR);
	if(!entry.isNull())
	{
		mLastSettings.lastPluginsDir = entry;
	}

	entry = mSettings.readEntry(COLIBRISETTING_LASTIMAGEDIR);
	if(!entry.isNull())
	{
		mLastSettings.lastImagesDir = entry;
	}

	entry = mSettings.readEntry(COLIBRISETTING_LASTMOVIEDIR);
	if(!entry.isNull())
	{
		mLastSettings.lastMoviesDir = entry;
	}

}

void ColibriMainWindow::saveSettings()
{
	// overwrite with batch settings
	mSettings.writeEntry(COLIBRISETTING_LASTPLUGINDIR, mLastSettings.lastPluginsDir);
	mSettings.writeEntry(COLIBRISETTING_LASTIMAGEDIR, mLastSettings.lastImagesDir);
	mSettings.writeEntry(COLIBRISETTING_LASTMOVIEDIR, mLastSettings.lastMoviesDir);
}

ColibriMainWindow::~ColibriMainWindow()
{
	saveSettings();
	delete ui;
}

int ColibriMainWindow::setPluginSequence(QString sequencepath)
{
	if(sequencepath.isEmpty()) {
		QString fileName = QFileDialog::getOpenFileName(this, tr("Open plugin sequence file"),
														 mLastSettings.lastPluginsDir,
														 tr("Piaf sequence (*.flist)"));
		QFileInfo fi(fileName);
		if(fi.isFile() && fi.exists())
		{
			mLastSettings.lastPluginsDir = fi.absoluteDir().absolutePath();
			saveSettings();

			sequencepath = fileName;
		}
		else
		{
			return -1;
		}
	}

	if(mFilterManager.loadFilterList(sequencepath.toUtf8().data()) < 0) {
		QMessageBox::critical(NULL, tr("Invalid sequence"),
							  tr("File containing plugins sequence is invalid : ")
							  + sequencepath);
		ui->toolsWidget->setEnabled(false);
		return -1;
	}

	return 0;
}


void ColibriMainWindow::on_fileButton_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
													mLastSettings.lastImagesDir,
													tr("Images (*.png *.PNG "
													   "*.xpm "
													   "*.jpg *.jpeg *.JPG *.JPEG "
													   "*.bmp *.BMP)"));
	if(fileName.isEmpty()) return;

	QFileInfo fi(fileName);
	if(!fi.exists()) return;

	mLastSettings.lastImagesDir = fi.absolutePath();
	saveSettings();

	// load file
	IplImage * iplImage = cvLoadImage(fileName.toUtf8().data());
	computeImage(iplImage);

	displayImage(iplImage);

	tmReleaseImage(&iplImage);
}



typedef uint32_t u32;

static u32 * grayToBGR32 = NULL;

static void init_grayToBGR32()
{
	if(grayToBGR32) {
		return;
	}

	grayToBGR32 = new u32 [256];
	for(int c = 0; c<256; c++) {
		int Y = c;
		u32 B = Y;// FIXME
		u32 G = Y;
		u32 R = Y;
		grayToBGR32[c] = (R << 16) | (G<<8) | (B<<0);
	}

}

QImage iplImageToQImage(IplImage * iplImage) {
	if(!iplImage)
		return QImage();


	int depth = iplImage->nChannels;

	bool rgb24_to_bgr32 = false;
	if(depth == 3  ) {// RGB24 is obsolete on Qt => use 32bit instead
            depth = 4;
            rgb24_to_bgr32 = true;
	}

	u32 * grayToBGR32palette = grayToBGR32;
	bool gray_to_bgr32 = false;

	if(depth == 1) {// GRAY is obsolete on Qt => use 32bit instead
		depth = 4;
		gray_to_bgr32 = true;

		init_grayToBGR32();
		grayToBGR32palette = grayToBGR32;
	}

	int orig_width = iplImage->width;
//	if((orig_width % 2) == 1)
//		orig_width--;


        QImage qImage(orig_width, iplImage->height, depth == 1 ? QImage::Format_Mono : QImage::Format_ARGB32);
        memset(qImage.bits(), 255, orig_width*iplImage->height*depth); // to fill the alpha channel


	switch(iplImage->depth) {
	default:
		fprintf(stderr, "[TamanoirApp]::%s:%d : Unsupported depth = %d\n", __func__, __LINE__, iplImage->depth);
		break;

	case IPL_DEPTH_8U: {
		if(!rgb24_to_bgr32 && !gray_to_bgr32) {
			if(iplImage->nChannels != 4) {
				for(int r=0; r<iplImage->height; r++) {
					// NO need to swap R<->B
					memcpy(qImage.bits() + r*orig_width*depth,
						iplImage->imageData + r*iplImage->widthStep,
						orig_width*depth);
				}
			} else {
				for(int r=0; r<iplImage->height; r++) {
					// need to swap R<->B
					u8 * buf_out = (u8 *)(qImage.bits()) + r*orig_width*depth;
					u8 * buf_in = (u8 *)(iplImage->imageData) + r*iplImage->widthStep;
					memcpy(qImage.bits() + r*orig_width*depth,
						iplImage->imageData + r*iplImage->widthStep,
						orig_width*depth);
/*
					for(int pos4 = 0 ; pos4<orig_width*depth; pos4+=depth,
						buf_out+=4, buf_in+=depth
						 ) {
						buf_out[2] = buf_in[0];
						buf_out[0] = buf_in[2];
                                        }
*/				}
			}
		}
		else if(rgb24_to_bgr32) {
			// RGB24 to BGR32
			u8 * buffer3 = (u8 *)iplImage->imageData;
			u8 * buffer4 = (u8 *)qImage.bits();
			int orig_width4 = 4 * orig_width;

			for(int r=0; r<iplImage->height; r++)
			{
				int pos3 = r * iplImage->widthStep;
				int pos4 = r * orig_width4;
				for(int c=0; c<orig_width; c++, pos3+=3, pos4+=4)
				{
					//buffer4[pos4 + 2] = buffer3[pos3];
					//buffer4[pos4 + 1] = buffer3[pos3+1];
					//buffer4[pos4    ] = buffer3[pos3+2];
					buffer4[pos4   ] = buffer3[pos3];
					buffer4[pos4 + 1] = buffer3[pos3+1];
					buffer4[pos4 + 2] = buffer3[pos3+2];
				}
			}
		} else if(gray_to_bgr32) {
			for(int r=0; r<iplImage->height; r++)
			{
				u32 * buffer4 = (u32 *)qImage.bits() + r*qImage.width();
				u8 * bufferY = (u8 *)(iplImage->imageData + r*iplImage->widthStep);
				for(int c=0; c<orig_width; c++) {
					buffer4[c] = grayToBGR32palette[ (int)bufferY[c] ];
				}
			}
		}
		}break;
	case IPL_DEPTH_16S: {
		if(!rgb24_to_bgr32) {

			u8 * buffer4 = (u8 *)qImage.bits();
			short valmax = 0;

			for(int r=0; r<iplImage->height; r++)
			{
				short * buffershort = (short *)(iplImage->imageData + r*iplImage->widthStep);
				for(int c=0; c<iplImage->width; c++)
					if(buffershort[c]>valmax)
						valmax = buffershort[c];
			}

			if(valmax>0)
				for(int r=0; r<iplImage->height; r++)
				{
					short * buffer3 = (short *)(iplImage->imageData
									+ r * iplImage->widthStep);
					int pos3 = 0;
					int pos4 = r * orig_width;
					for(int c=0; c<orig_width; c++, pos3++, pos4++)
					{
						int val = abs((int)buffer3[pos3]) * 255 / valmax;
						if(val > 255) val = 255;
						buffer4[pos4] = (u8)val;
					}
				}
		}
		else {
			u8 * buffer4 = (u8 *)qImage.bits();
			if(depth == 3) {

				for(int r=0; r<iplImage->height; r++)
				{
					short * buffer3 = (short *)(iplImage->imageData + r * iplImage->widthStep);
					int pos3 = 0;
					int pos4 = r * orig_width*4;
					for(int c=0; c<orig_width; c++, pos3+=3, pos4+=4)
					{
						buffer4[pos4   ] = buffer3[pos3];
						buffer4[pos4 + 1] = buffer3[pos3+1];
						buffer4[pos4 + 2] = buffer3[pos3+2];
					}
				}
			} else if(depth == 1) {
				short valmax = 0;
				short * buffershort = (short *)(iplImage->imageData);
				for(int pos=0; pos< iplImage->widthStep*iplImage->height; pos++)
					if(buffershort[pos]>valmax)
						valmax = buffershort[pos];

				if(valmax>0) {
					for(int r=0; r<iplImage->height; r++)
					{
						short * buffer3 = (short *)(iplImage->imageData
											+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = (u8)val;
						}
					}
				}
			}
		}
		}break;
	case IPL_DEPTH_16U: {
		if(!rgb24_to_bgr32) {

			unsigned short valmax = 0;

			for(int r=0; r<iplImage->height; r++)
			{
				unsigned short * buffershort = (unsigned short *)(iplImage->imageData + r*iplImage->widthStep);
				for(int c=0; c<iplImage->width; c++)
					if(buffershort[c]>valmax)
						valmax = buffershort[c];
			}

			if(valmax>0) {
				if(!gray_to_bgr32) {
					u8 * buffer4 = (u8 *)qImage.bits();
					for(int r=0; r<iplImage->height; r++)
					{
						unsigned short * buffer3 = (unsigned short *)(iplImage->imageData
										+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = (u8)val;
						}
					}
				} else {
					u32 * buffer4 = (u32 *)qImage.bits();
					for(int r=0; r<iplImage->height; r++)
					{
						unsigned short * buffer3 = (unsigned short *)(iplImage->imageData
										+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = grayToBGR32palette[ val ];
						}
					}
				}
			}
		}
		else {
			fprintf(stderr, "[TamanoirApp]::%s:%d : U16  depth = %d -> BGR32\n", __func__, __LINE__, iplImage->depth);
			u8 * buffer4 = (u8 *)qImage.bits();
			if(depth == 3) {

				for(int r=0; r<iplImage->height; r++)
				{
					short * buffer3 = (short *)(iplImage->imageData + r * iplImage->widthStep);
					int pos3 = 0;
					int pos4 = r * orig_width*4;
					for(int c=0; c<orig_width; c++, pos3+=3, pos4+=4)
					{
						buffer4[pos4   ] = buffer3[pos3]/256;
						buffer4[pos4 + 1] = buffer3[pos3+1]/256;
						buffer4[pos4 + 2] = buffer3[pos3+2]/256;
					}
				}
			} else if(depth == 1) {
				short valmax = 0;
				short * buffershort = (short *)(iplImage->imageData);
				for(int pos=0; pos< iplImage->widthStep*iplImage->height; pos++)
					if(buffershort[pos]>valmax)
						valmax = buffershort[pos];

				if(valmax>0)
					for(int r=0; r<iplImage->height; r++)
					{
						short * buffer3 = (short *)(iplImage->imageData
											+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = (u8)val;
						}
					}
			}
		}
		}break;
	}

	if(qImage.depth() == 8) {
		qImage.setNumColors(256);

		for(int c=0; c<256; c++) {
			/* False colors
			int R=c, G=c, B=c;
			if(c<128) B = (255-2*c); else B = 0;
			if(c>128) R = 2*(c-128); else R = 0;
			G = abs(128-c)*2;

			qImage.setColor(c, qRgb(R,G,B));
			*/
			qImage.setColor(c, qRgb(c,c,c));
		}
	}
	return qImage;
}


void ColibriMainWindow::computeImage(IplImage * iplImage) {
	if(!iplImage) return;

	// TODO : check if size changed
	if(mSwImage.width != iplImage->width || mSwImage.height != iplImage->height
	   || mSwImage.depth != iplImage->nChannels) {
		// unload previously loaded filters
		mFilterManager.slotUnloadAll();
		// reload same file
		mFilterManager.loadFilterList(mFilterManager.getPluginSequenceFile());
		memset(&mSwImage, 0, sizeof(swImageStruct));
	}

	// compute plugin sequence
	mSwImage.width = iplImage->width;
	mSwImage.height = iplImage->height;
	mSwImage.depth = iplImage->nChannels;
	mSwImage.buffer_size = mSwImage.width * mSwImage.height * mSwImage.depth;

	mSwImage.buffer = iplImage->imageData; // Buffer

//	fprintf(stderr, "[Colibri] %s:%d : process sequence '%s' on image (%dx%dx%d)\n",
//			__func__, __LINE__,
//			mFilterManager.getPluginSequenceFile(),
//			mSwImage.width, mSwImage.height, mSwImage.depth
//			);

	// Process this image with filters
	mFilterManager.processImage(&mSwImage);

	// Display image as output
	//displayImage(iplImage);
}

void ColibriMainWindow::on_deskButton_clicked() {
    QPoint curpos = QApplication::activeWindow()->pos();
    QRect currect = QApplication::activeWindow()->rect();
    QPoint imgPos = QApplication::activeWindow()->geometry().topLeft();
    QRect imgRect = ui->imageLabel->rect();

    fprintf(stderr, "%s:%d: curpos=%dx%d +%dx%d imageLabel=%d,%d rect=%d,%d+%dx%d\n",
            __func__, __LINE__,
            curpos.x(), curpos.y(),
            currect.width(), currect.height(),
            imgPos.x(), imgPos.y(),
            imgRect.x(), imgRect.y(),
            imgRect.width(), imgRect.height()
            );

    m_grabRect = QRect(imgPos.x()+1, imgPos.y()+1,
                       imgRect.width(), imgRect.height());
    hide();
	QTimer::singleShot(200, this, SLOT(on_grabTimer_timeout()));
}

void ColibriMainWindow::on_grabTimer_timeout() {

    QPixmap desktopPixmap = QPixmap::grabWindow(
            //QApplication::activeWindow()->winId());
            QApplication::desktop()->winId());
   fprintf(stderr, "%s:%d: desktopImage=%dx%dx%d\n",
            __func__, __LINE__,
            desktopPixmap.width(), desktopPixmap.height(), desktopPixmap.depth());
   //desktopPixmap.save("/tmp/desktop.png");
    //QApplication::activeWindow()->show();

    // Crop where the window is

	fprintf(stderr, "%s:%d: curpos=%dx%d+%dx%d\n",
            __func__, __LINE__,
            m_grabRect.x(), m_grabRect.y(),
            m_grabRect.width(), m_grabRect.height());

    QImage desktopImage;
    desktopImage = ( desktopPixmap.copy(m_grabRect).toImage() );

	if(desktopImage.isNull()) {
		fprintf(stderr, "%s:%d: desktopImage is null !\n",
					__func__, __LINE__);

		return;
	}
	fprintf(stderr, "%s:%d: desktopImage=%dx%dx%d\n",
            __func__, __LINE__,
            desktopImage.width(), desktopImage.height(), desktopImage.depth());


	IplImage * iplImage = cvCreateImage(cvSize(desktopImage.width(), desktopImage.height()),
                                            IPL_DEPTH_8U, desktopImage.depth()/8);
	memcpy(iplImage->imageData, (char *)desktopImage.bits(), iplImage->widthStep*iplImage->height);

//	// We have to set the alpha flag
//	u8 * alpha = (u8 *) iplImage->imageData;

//	for(int pos = 3; pos <iplImage->widthStep*iplImage->height; pos+=4)
//	{
//		alpha[pos] = 255;
//	}
//	cvSaveImage("/dev/shm/unprocessed.png", iplImage);
	computeImage(iplImage);
//	cvSaveImage("/dev/shm/processed.png", iplImage);
	displayImage(iplImage);

    show();

	tmReleaseImage(&iplImage);
}

void ColibriMainWindow::on_snapButton_clicked() {
    if(resultImage.isNull()) return;

    // Assemblate file name
    QDir snapDir(QDir::homePath());
    snapDir.cd(tr("Desktop"));


	float waf_factor = 0.5f;
    QString wafStr;
	wafStr.sprintf("%02d", (int)roundf(waf_factor*100.f));
    QString dateStr = QDateTime::currentDateTime().toString(tr("yyyy-MM-dd_hhmmss"));

	QString filepath = snapDir.absoluteFilePath("Colibri_" + dateStr + "waf=" + wafStr + "perc.png");

    resultImage.save( filepath,
                      "PNG");
}

void ColibriMainWindow::displayImage(IplImage * iplImage)
{
	// Display
	if(!iplImage) return;

	// load image to display
	qtImage = iplImageToQImage(iplImage);
	ui->imageLabel->setRefImage(&qtImage);
}

CvCapture* capture = 0;

void ColibriMainWindow::on_camButton_toggled(bool checked)
{
	if(!checked) {
		if(m_timer.isActive())
			m_timer.stop();
		// Stop thread
		if(m_pColibriThread) {
			m_pColibriThread->stop();
			m_pColibriThread->setCapture(NULL);
		}
		cvReleaseCapture(&capture);
		capture = NULL;

		return;
	}

	if(!capture) {
		int retry = 0;
		do {
			capture = cvCreateCameraCapture (retry);
			fprintf(stderr, "[Colibri] %s:%d : capture #%d =%p\n", __func__, __LINE__,
					retry, capture);
			retry++;
		} while(!capture && retry < 5);
	}
	if(!capture) {
		QMessageBox::critical(NULL, tr("No webcam"),
							  tr("Unable to find webcam. Please try disconnect/reconnect."));
		return ; }

	startBackgroundThread();
}

void ColibriMainWindow::on_movieButton_toggled(bool checked)
{
	if(!checked) {
		if(m_timer.isActive()) {
			m_timer.stop();
		}

		// Stop thread
		if(m_pColibriThread) {
			m_pColibriThread->stop();
			m_pColibriThread->setCapture(NULL);
		}

		cvReleaseCapture(&capture);
		capture = NULL;

		return;
	}

	if(!capture) {
		QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
							 mLastSettings.lastMoviesDir,
							 tr("Movies (*.avi *.mp* *.mov)"));
		if(fileName.isEmpty()) return;

		QFileInfo fi(fileName);
		if(!fi.exists()) return;

		mLastSettings.lastMoviesDir = fi.absolutePath();
		saveSettings();


		capture = cvCaptureFromFile( fi.absoluteFilePath().toUtf8().data() );

		fprintf(stderr, "%s:%d : capture=%p\n", __func__, __LINE__, capture);
		startBackgroundThread();
	}
}

void ColibriMainWindow::startBackgroundThread() {

	if(!capture) { return ; }

	if(!m_pColibriThread) {
		m_pColibriThread = new ColibriThread(this);
	}
	m_pColibriThread->setCapture(capture);
	m_pColibriThread->start();

	if(!m_timer.isActive()) {
		//  m_timer.start(40); // 25 fps
		m_timer.start(100); // 10 fps
	}

}

void ColibriMainWindow::on_m_timer_timeout() {
	if(!capture) {
		m_timer.stop();
		return;
	}
	if(!m_pColibriThread) {
		m_timer.stop();
		return;
	}


	// Grab image
	int cur_iteration = m_pColibriThread->getIteration();
	if(m_lastIteration != cur_iteration) {
		m_lastIteration = cur_iteration;
		// Display image
		m_pColibriThread->lockDisplay();
		displayImage(m_pColibriThread->getOutputImage());
		m_pColibriThread->unlockDisplay();
	} else {
		//
		fprintf(stderr, "%s:%d no fast enough\n", __func__, __LINE__);
	}
}


void ColibriMainWindow::on_fullScreenButton_clicked()
{
	on_actionFull_screen_activated();
}

void ColibriMainWindow::on_actionFull_screen_activated()
{
	if(!isFullScreen())
	{
		showFullScreen();
	}
	else {
		showNormal();
	}
}



/**************************************************************************

					BACKGROUND ANALYSIS THREAD

**************************************************************************/
ColibriThread::ColibriThread(ColibriMainWindow * proc) {
	m_isRunning = m_run = false;
	m_inputImage = m_outputImage = NULL;
	m_iteration = 0;
	m_pProcessor = proc;
}

ColibriThread::~ColibriThread() {
	stop();
	tmReleaseImage(&m_inputImage);
	tmReleaseImage(&m_outputImage);
}

void ColibriThread::setCapture(CvCapture * capture) {
	m_capture = capture;
}

/* Tell the thread to stop */
void ColibriThread::stop() {
	m_run = false;
	while(m_isRunning) {
		sleep(1);
	}
}

/* Thread loop */
void ColibriThread::run()
{
	m_isRunning = m_run = true;

	while(m_run) {
		if(m_capture ) {
			IplImage * frame = cvQueryFrame( m_capture );

			if( !frame) { //!cvGrabFrame( m_capture ) ) {
				fprintf(stderr, "%s:%d : capture=%p FAILED\n", __func__, __LINE__, capture);
				sleep(1);
				sleep(2);

			} else {
				// fprintf(stderr, "%s:%d : capture=%p ok, iteration=%d\n",
				//		__func__, __LINE__, m_capture, m_iteration);
				//IplImage * frame = cvRetrieveFrame( m_capture );
				if(!frame) {
					fprintf(stderr, "%s:%d : cvRetrieveFrame(capture=%p) FAILED\n", __func__, __LINE__, capture);
					sleep(1);
				} else {
					// Check if size changed
					if(m_outputImage &&
					   (m_outputImage->widthStep*m_outputImage->height != frame->widthStep*frame->height))
					{
						tmReleaseImage(&m_inputImage);
						tmReleaseImage(&m_outputImage);
					}
					if(!m_inputImage) { m_inputImage = cvCloneImage(frame); }
					else { cvCopy(frame, m_inputImage); }
					// then process current image
					m_pProcessor->computeImage(m_inputImage);

					// copy image
					mDisplayMutex.lock();
					if(!m_outputImage) { m_outputImage = cvCloneImage(m_inputImage); }
					else { cvCopy(m_inputImage, m_outputImage); }
					mDisplayMutex.unlock();


					m_iteration++;
				}
			}
		} else {
			sleep(1);
		}
	}

	m_isRunning = false;
}




void ColibriMainWindow::on_gridButton_toggled(bool checked)
{
	ui->imageLabel->showGrid( (checked ? 4 : 0) );
}
