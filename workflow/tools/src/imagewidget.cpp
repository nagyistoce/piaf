/***************************************************************************
	  imagewidget.cpp  -  image display widget for Piaf
							 -------------------
	begin                : ven nov 29 15:53:48 UTC 2002
	copyright            : (C) 2002-2011 by Christophe Seyve
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
#include <math.h>

#include "imagewidget.h"
#include "imgutils.h"

#include "virtualdeviceacquisition.h" // for iplImageToQImage

#include <stdio.h>
#include <QPainter>
#include <QShortcut>


#include <qcolor.h>

//Added by qt3to4:
#include <QPixmap>
#include <QPaintEvent>
#include <QBitmap>


unsigned char RthermicBlack2Red(int p) {
	int dp = p-170;
	if(dp<0) dp = -dp;
	unsigned char R = (p<170 ? 0 : (unsigned char)(dp*3));
	return R;
}
unsigned char GthermicBlack2Red(int p) {
	int dp = p-170;
	if(dp<0) dp = -dp;
	unsigned char G = (p<85 ? 0 : (unsigned char)(255 - dp*3));
	return G;
}

unsigned char BthermicBlack2Red(int p) {
	int dp = p-85;
	if(dp<0) dp = -dp;
	unsigned char B = (p>170 ? 0 : (unsigned char)(255-dp*3));
	return B;
}

unsigned char RthermicBlue2Red(int p) {
	if(p<128) return 0;

	int dp = p-128;
	if(dp<0) dp = -dp;
	unsigned char R = (p<128 ? 0 : (unsigned char)(dp*2));
	return R;
}

unsigned char GthermicBlue2Red(int p) {
	int dp = p-127;
	if(dp<0) dp = -dp;
	unsigned char G = (unsigned char)(255 - dp*2);
	return G;
}

unsigned char BthermicBlue2Red(int p) {
	if(p>127) return 0;

	unsigned char B = (unsigned char)(255 - p*2);
	return B;
}

bool g_debug_ImageWidget = false;

IplImage * gLUT_BlueToRed = NULL;
IplImage * gLUT_BlackToRed = NULL;
IplImage * gLUT_Inverted = NULL;
IplImage * gLUT_Indexed = NULL;
IplImage * gLUT_Gray = NULL;

void initColorLUTs()
{
	if(gLUT_BlueToRed) { return; }

	gLUT_BlueToRed = tmCreateImage(cvSize(256,1), IPL_DEPTH_8U, 4);
	gLUT_BlackToRed = tmCreateImage(cvSize(256,1), IPL_DEPTH_8U, 4);
	gLUT_Indexed = tmCreateImage(cvSize(256,1), IPL_DEPTH_8U, 4);
	gLUT_Inverted = tmCreateImage(cvSize(256,1), IPL_DEPTH_8U, 4);
	gLUT_Gray = tmCreateImage(cvSize(256,1), IPL_DEPTH_8U, 4);
	int hue = 17;
	QColor color;
	for(int col = 0; col<256; col++, hue+=97)
	{
		u8 R, G, B, A=255;
		// Indexed
		color.setHsv(hue%360, 255, 127);
		R = color.red();
		G = color.green();
		B = color.blue();

#define CONCAT_BGRA (A << 24 | R << 16 | G << 8 | B)
		*((u32 *)gLUT_Indexed->imageData + col) = CONCAT_BGRA;

		// Blue to red
		R = RthermicBlue2Red(col);
		G = GthermicBlue2Red(col);
		B = BthermicBlue2Red(col);
		*((u32 *)gLUT_BlueToRed->imageData + col) = CONCAT_BGRA;

		// Black to red
		R = RthermicBlack2Red(col);
		G = GthermicBlack2Red(col);
		B = BthermicBlack2Red(col);
		*((u32 *)gLUT_BlackToRed->imageData + col) = CONCAT_BGRA;

		// Gray
		R = G = B = col;
		*((u32 *)gLUT_Gray->imageData + col) = CONCAT_BGRA;

		// Inverted
		R = G = B = 255 - col;
		*((u32 *)gLUT_Inverted->imageData + col) = CONCAT_BGRA;
	}

//	// Save for debug
//	cvSaveImage(TMP_DIRECTORY "LUT-gray.png", gLUT_Gray);
	cvSaveImage(TMP_DIRECTORY "LUT-indexed.png", gLUT_Indexed);
//	cvSaveImage(TMP_DIRECTORY "LUT-inverted.png", gLUT_Inverted);
//	cvSaveImage(TMP_DIRECTORY "LUT-blue2red.png", gLUT_BlueToRed);
//	cvSaveImage(TMP_DIRECTORY "LUT-black2red.png", gLUT_BlackToRed);
}


ImageWidget::ImageWidget(QWidget *parent, const char *name, Qt::WFlags f)
	: QWidget(parent)
{
	initColorLUTs();


	m_greyImage = m_displayImageBGRA = NULL;

	mZoomFit = true; // fit to size by default
	mZoomFitFactor = -1.; // don't know yet the zoom factor
	xMouseMoveStart = -1;
	xOrigine = yOrigine = 0;
	xZoomCenter = yZoomCenter = 0;

	//setFocusPolicy (Qt::StrongFocus);
	m_displayImageBGRA=NULL;
	m_pOriginalImage = NULL;

	m_colorMode = 0;
	mShift = mCtrl = false;

	mEditMode = EDITMODE_NONE;
	mGridSteps = 0; // disabled by default

	QShortcut * shortcut_in = new QShortcut(QKeySequence(tr("+", "Zoom in")),
											this);
	connect(shortcut_in, SIGNAL(activated()), this, SLOT(slot_shortcut_in_activated()));
	QShortcut * shortcut_out = new QShortcut(QKeySequence(tr("-", "Zoom out")),
											this);
	connect(shortcut_out, SIGNAL(activated()), this, SLOT(slot_shortcut_out_activated()));
	QShortcut * shortcut_fit = new QShortcut(QKeySequence(tr("x", "Zoom fit")),
											this);
	connect(shortcut_fit, SIGNAL(activated()), this, SLOT(slot_shortcut_fit_activated()));
	QShortcut * shortcut_1 = new QShortcut(QKeySequence(tr("1", "Zoom 1:1")),
											this);
	connect(shortcut_1, SIGNAL(activated()), this, SLOT(slot_shortcut_1_activated()));
}
void ImageWidget::purge()
{
	tmReleaseImage(&m_greyImage);
	tmReleaseImage(&m_displayImageBGRA);
}

/* display a grid over the image */
void ImageWidget::showGrid(int steps)
{
	if(mGridSteps != steps)
	{
		mGridSteps = steps;

		update();
	}
}

void ImageWidget::setRefImage(IplImage *pIm)
{
	if(!pIm)
	{
		// Clear images
		m_pOriginalImage = NULL;

		tmReleaseImage(&m_greyImage);
		tmReleaseImage(&m_displayImageBGRA);

		update();
		return;
	}

	if(m_displayImageBGRA && (m_displayImageBGRA->width != pIm->width
			|| m_displayImageBGRA->height != pIm->height) ) {
		tmReleaseImage(&m_displayImageBGRA);
	}
	if(!m_displayImageBGRA)
	{
		m_displayImageBGRA = tmCreateImage(cvGetSize(pIm), IPL_DEPTH_8U, 4);
	}
	if(m_greyImage && (m_greyImage->width != pIm->width
			|| m_greyImage->height != pIm->height) ) {
		tmReleaseImage(&m_greyImage);
	}
	if(!m_greyImage)
	{
		m_greyImage = tmCreateImage(cvGetSize(pIm), IPL_DEPTH_8U, 1);
	}

	m_pOriginalImage = pIm;

	setColorMode(m_colorMode);
}

/* Switch to smart zooming mode */
void ImageWidget::switchToSmartZoomMode(bool on)
{
	mZoomFit = on;
	mZoomFitFactor = -1.f;
	xMouseMoveStart = -1;
	xOrigine = yOrigine = 0;

	clipSmartZooming();
}
void ImageWidget::setZoomCenter(int xC, int yC, float scale) {

	if(scale != 0)
	{
		xOrigine = (int)roundf(xC - 0.5f * size().width() / scale);
		yOrigine = (int)roundf(yC - 0.5f * size().height() / scale);
	}
	else
	{
		xOrigine = 0;
		yOrigine = 0;
	}
	float diff = (mZoomFitFactor - scale);
	if(diff < 0.f) { diff = -diff; }

	mZoomFitFactor = scale;

	fprintf(stderr, "ImageWidget::%s:%d : zoom on center=%d,%d, sc=%g / disp=%dx%d"
			" => origin=%d,%d\n",
			__func__, __LINE__, xC, yC, scale,
			size().width(), size().height(),
			xOrigine, yOrigine
			);
	clipSmartZooming();
	update();

	if(diff > 0.001f) { // zoom increment is big enough to signal it to navigation widget
		emit signalZoomChanged(mZoomFitFactor);
	}
}

void ImageWidget::setZoomParams(int xO, int yO, float scale)
{
	xOrigine = xO;
	yOrigine = yO;

	float diff = (mZoomFitFactor - scale);
	if(diff < 0.f) { diff = -diff; }
	mZoomFitFactor = scale;

	fprintf(stderr, "ImageWidget::%s:%d : zoom on origin=%d,%d, sc=%g\n",
			__func__, __LINE__, xO, yO, scale);
	clipSmartZooming();
	update();
	if(diff > 0.001f) {
		emit signalZoomChanged(mZoomFitFactor);
	}
}

int ImageWidget::setEditMode(int mode)
{
	if(mode > EDITMODE_MAX) {
		return mEditMode;
	}

	if(mEditMode != mode)
	{
		xMouseMoveStart = -1; // clear mouse state = forget history of previous action: zoom, move...
	}
	mEditMode = mode;

	switch(mEditMode) {
	default:
		setCursor(Qt::ArrowCursor);
		break;

	case EDITMODE_ZOOM:
		setCursor(Qt::OpenHandCursor);//Qt::CrossCursor);
		break;

	case EDITMODE_PICKER: {
		QCursor zoomCursor = QCursor( QBitmap( ":images/pixmaps/CursorPicker.bmp", 0),
									  QBitmap( ":images/pixmaps/CursorPickerMask.bmp", 0),
									  1, 20);
		setCursor(zoomCursor);
		}break;

	case EDITMODE_ROIS:
		setCursor(Qt::CrossCursor);
		break;
	}

	// add a fake QRect
//	QRect * rect = new QRect(10,10, 100, 100);
//	mROIList.append(rect);
//	mSelectedROI = rect;
	mSelectedROI = NULL;


	update();

	return mEditMode;
}

void ImageWidget::setColorMode(int mode)
{	
	if(mode >= COLORMODE_GREY && mode <= COLORMODE_MAX )
	{
		m_colorMode = mode;
	} else {
		m_colorMode = COLORMODE_GREY;
	}

	if(m_colorMode == COLORMODE_GREY)
	{
		// convert from input image
		tmConvert(m_pOriginalImage, m_displayImageBGRA);
	}
	else
	{
		// convert to grey 8bit, then use LUT
		tmConvert(m_pOriginalImage, m_greyImage);

		m_pLUT = NULL;
		switch(m_colorMode)
		{
		default:
			PIAF_MSG(SWLOG_ERROR, "Invalid color mode %d", m_colorMode);
			break;
		case COLORMODE_INDEXED:
			m_pLUT = gLUT_Indexed;
			break;
		case COLORMODE_GREY_INVERTED:
			m_pLUT = gLUT_Inverted;
			break;
		case COLORMODE_THERMIC_BLACK2RED:
			m_pLUT = gLUT_BlackToRed;
			break;
		case COLORMODE_THERMIC_BLUE2RED:
			m_pLUT = gLUT_BlueToRed;
			break;
		}


		if(m_pLUT)
		{
			try {
				// convert using LUT
				cvCvtColor(m_greyImage, m_displayImageBGRA, CV_GRAY2BGRA);
				cvLUT(m_displayImageBGRA, m_displayImageBGRA, m_pLUT);
			}
			catch(cv::Exception e)
			{
				cvZero( m_displayImageBGRA );

				PIAF_MSG(SWLOG_ERROR, "Invalid call to cvLUT: grey=%dx%dx%dbx%d "
						 "=> display=%dx%dx%dbx%d / m_pLUT=%p=%dx%dx%d",
						 m_greyImage->width, m_greyImage->height,
							m_greyImage->depth, m_greyImage->nChannels,
						 m_displayImageBGRA->width, m_displayImageBGRA->height,
							m_displayImageBGRA->depth, m_displayImageBGRA->nChannels,
						 m_pLUT, m_pLUT->width, m_pLUT->height, m_pLUT->nChannels
						 );
			}
		}
	}

	m_displayImage = iplImageToQImage(m_displayImageBGRA);
	repaint();
}

#define KEY_ZOOM_INC	0.2f

void ImageWidget::focusInEvent ( QFocusEvent * event )
{
	fprintf(stderr, "ImageWidget::%s:%d event=%p\n", __func__, __LINE__, event);
	grabKeyboard();
}

void ImageWidget::focusOutEvent ( QFocusEvent * event )
{
	fprintf(stderr, "ImageWidget::%s:%d event=%p\n", __func__, __LINE__, event);
	releaseKeyboard();
}

void ImageWidget::keyReleaseEvent ( QKeyEvent * event )
{
	mShift = mCtrl = false;
}

void ImageWidget::slot_shortcut_in_activated()
{
	zoomOnDisplayPixel(width()/2, height()/2, KEY_ZOOM_INC);
}

void ImageWidget::slot_shortcut_out_activated()
{
	zoomOnDisplayPixel(width()/2, height()/2, -KEY_ZOOM_INC);
}

void ImageWidget::slot_shortcut_fit_activated()
{
	mZoomFitFactor = -1;
	update();
}

void ImageWidget::slot_shortcut_1_activated()
{
	setZoomParams(xZoomCenter, yZoomCenter, 1);
	update();
}

void ImageWidget::keyPressEvent ( QKeyEvent * event )
{
	fprintf(stderr, "ImageWidget %p::%s:%d : received event=%p\n",
			this, __func__, __LINE__, event);

	if(event)
	{
		QString key = event->text();
		//if(g_debug_Imagewidget)
		{
			fprintf(stderr, "ImageWidget::%s:%d : received event=%p='%s'\n", __func__, __LINE__,
				event,
				event->text().toUtf8().data());
		}

		if(key.compare("+")==0)
		{
			fprintf(stderr, "ImageWidget::%s:%d : zoom + %g !\n",
					__func__, __LINE__, KEY_ZOOM_INC);
			//
			zoomOnDisplayPixel(width()/2, height()/2, KEY_ZOOM_INC);
		}
		else if(key.compare("-")==0)
		{
			fprintf(stderr, "ImageWidget::%s:%d : zoom - %g !\n",
					__func__, __LINE__, KEY_ZOOM_INC);
			//
			zoomOnDisplayPixel(width()/2, height()/2, -KEY_ZOOM_INC);
		}
		else if(key.compare("x")==0)
		{
			fprintf(stderr, "ImageWidget::%s:%d : zoom fit !\n",
					__func__, __LINE__);
			mZoomFitFactor = -1.;
			update();
		}
		else if(key.compare("1")==0)
		{
			fprintf(stderr, "ImageWidget::%s:%d : zoom 1:1 !\n",
					__func__, __LINE__);
			//mZoomFitFactor = 1.;
			setZoomParams(xZoomCenter, yZoomCenter, 1);
			update();
		}
		if(event->key() == Qt::Key_Shift)
		{
//			fprintf(stderr, "ThumbImageFrame::%s:%d : Shift !\n",
//					__func__, __LINE__);
			mShift = true;
		}

		if(event->key() == Qt::Key_Control)
		{
//			fprintf(stderr, "ThumbImageFrame::%s:%d : Ctrl !\n",
//					__func__, __LINE__);
			mCtrl = true;
		}

		if(event->key() == Qt::Key_Delete)
		{
			// only works with ROIs
			if(mEditMode == EDITMODE_ROIS &&  mSelectedROI) {
				mROIList.removeOne(mSelectedROI);
				mSelectedROI = NULL;
				update();
			}
		}
	}
}

void ImageWidget::mousePressEvent(QMouseEvent *e) {
	if(e->button() == Qt::LeftButton) {
		switch(mEditMode)
		{
		default:
		case EDITMODE_NONE:
			emit mousePressed(e); // Forward signal
			break;
		case EDITMODE_ZOOM:
			if(mZoomFit) {
				xMouseMoveStart = e->pos().x();
				yMouseMoveStart = e->pos().y();
				xOriginMoveStart = xOrigine;
				yOriginMoveStart = yOrigine;
			}
			break;
		case EDITMODE_ROIS: {
			xMouseMoveStart = e->pos().x();
			yMouseMoveStart = e->pos().y();
			xOriginMoveStart = xOrigine;
			yOriginMoveStart = yOrigine;

			QRect inImage = displayToImage(QRect(xMouseMoveStart, yMouseMoveStart,
												 e->pos().x()-xMouseMoveStart,
												 e->pos().y()-yMouseMoveStart));
			// Check if we selected an existing rect or add a new one
			QList<QRect *>::iterator it;
			mSelectedROI = NULL;
			int dmin = 5;
			for(it = mROIList.begin(); it!=mROIList.end(); ++it)
			{
				QRect * prect = (*it);
				if(prect)
				{
					QRect onDisplay = imageToDisplay(*prect);

					// check if point is on the border
					if(e->pos().x() >= onDisplay.x()-dmin
							&& e->pos().x() <= onDisplay.x()+onDisplay.width()+dmin)
					{
						// mouse X is on region

						// now check if it's near the top or bottom border of the rect
						int dymin = tmmin( (int)abs(e->pos().y() - onDisplay.y()),
											  (int)abs(e->pos().y() - (onDisplay.y()+onDisplay.height()))
											  );
						if(dymin < dmin)
						{
							dmin = dymin;
							mSelectedROI = prect;
						}
					}
					// same test with y instead of x
					if(e->pos().y() >= onDisplay.y()-dmin && e->pos().y() <= onDisplay.y()+onDisplay.height()+dmin)
					{
						// mouse Y is on region

						// now check if it's near the top or bottom border of the rect
						int dxmin = tmmin( abs(e->pos().x() - onDisplay.x()),
										   abs(e->pos().x() - (onDisplay.x()+onDisplay.width()))
										   );
						if(dxmin < dmin)
						{
							dmin = dxmin;
							mSelectedROI = prect;
						}
					}

				}
			}
			if(!mSelectedROI) {
				QRect * prect = new QRect(inImage);
				mROIList.append(prect);
				mSelectedROI = prect;
			}
			else
			{
				update();
			}
			}break;
		case EDITMODE_PICKER: {
			QPoint pt = displayToImage(e->pos());
/// @todo Fixme : use input image and not display image

			QRgb colorRGB = m_displayImage.pixel(pt);
			int colorGrey = qGray(colorRGB);

			fprintf(stderr, "ImgWidget::%s:%d : FIXME "
					"mouse=(%d,%d) => in orig(%d,%d) RGB=%d,%d,%d gray=%d",
					__func__, __LINE__,
					e->pos().x(), e->pos().y(),
					pt.x(), pt.y(),
					qRed(colorRGB), qGreen(colorRGB), qBlue(colorRGB),
					colorGrey);

			emit signalPicker(colorRGB, colorGrey, pt);
			}
			break;
		}
	}
}


void ImageWidget::mouseReleaseEvent(QMouseEvent *e){
	if((e->button() & Qt::LeftButton)) {
		switch(mEditMode)
		{
		default:
			xMouseMoveStart = -1;
			emit mouseReleased(e);
			break;
		case EDITMODE_ZOOM:
			xMouseMoveStart = -1;
//			if(mZoomFit) {
//				xMouseMoveStart = e->pos().x();
//				yMouseMoveStart = e->pos().y();
//				xOriginMoveStart = xOrigine;
//				yOriginMoveStart = yOrigine;
//			}
			break;
		case EDITMODE_ROIS: // nothing to do, rect is defined in move event
			break;
		case EDITMODE_PICKER:
			break;
		}
	}
}

void ImageWidget::mouseMoveEvent(QMouseEvent *e)
{

	switch(mEditMode)
	{
	default:
	case EDITMODE_NONE:
		if(!mZoomFit) { return; }
		emit mouseMoved(e);
		//if(xMouseMoveStart<0) { return; } // not clicked
		break;
	case EDITMODE_ZOOM: {
		if(!mZoomFit) { return; }
		if( xMouseMoveStart<0 ) { return; } // not clicked
		// dx,dy is the move in pixel on display
		int dx = e->pos().x() - xMouseMoveStart ;
		int dy = e->pos().y() - yMouseMoveStart ;

		xOrigine = xOriginMoveStart - dx / mZoomFitFactor;
		yOrigine = yOriginMoveStart - dy / mZoomFitFactor;

		clipSmartZooming();
		update();

		}break;
	case EDITMODE_ROIS:
		if(!mSelectedROI){
		} else {
			*mSelectedROI = displayToImage(QRect(xMouseMoveStart, yMouseMoveStart,
												 e->pos().x()-xMouseMoveStart,
												 e->pos().y()-yMouseMoveStart));
		}
		update();
		break;
	case EDITMODE_PICKER: {
		QPoint pt = displayToImage(e->pos());
		QRgb colorRGB = m_displayImage.pixel(pt);
		int colorGrey = qGray(colorRGB);
		fprintf(stderr, "ImgWidget::%s:%d : FIXME "
				"mouse=(%d,%d) => in orig(%d,%d) RGB=%d,%d,%d gray=%d",
				__func__, __LINE__,
				e->pos().x(), e->pos().y(),
				pt.x(), pt.y(),
				qRed(colorRGB), qGreen(colorRGB), qBlue(colorRGB),
				colorGrey);

		emit signalPicker(colorRGB, colorGrey, pt);
		}
		break;

	}

}



void ImageWidget::clipSmartZooming()
{
	if(!m_displayImageBGRA) return;

	int width = size().width() / mZoomFitFactor;
	int height = size().height() / mZoomFitFactor;

	if(xOrigine + width > m_displayImageBGRA->width) {
		xOrigine = m_displayImageBGRA->width - width;
	}
	if(yOrigine + height > m_displayImageBGRA->height) {
		yOrigine = m_displayImageBGRA->height - height;
	}

	// do xO, yO at end to align on left and top
	if(xOrigine < 0) { xOrigine = 0; }
	if(yOrigine < 0) { yOrigine = 0; }
	// update xZoomCenter
	xZoomCenter = xOrigine + width/2 ;
	yZoomCenter = yOrigine + height/2 ;

	QRect cropRect = displayToImage(QRect(0,0, size().width(), size().height()));

	fprintf(stderr, "ImageWidget::%s:%d : emit signalZoomRect(cropRect=%d,%d+%dx%d);\n",
			__func__, __LINE__,
			cropRect.x(), cropRect.y(), cropRect.width(), cropRect.height());
	emit signalZoomRect(cropRect);

	mCropRect = cropRect;
}

QPoint ImageWidget::displayToOriginal(int x, int y)
{
	float curZoom = mZoomFitFactor;
	int xOrig = xOrigine + x / curZoom;
	int yOrig = yOrigine + y / curZoom;
	return QPoint(xOrig, yOrig);
}
QPoint ImageWidget::originalToDisplay(int x, int y)
{
	float curZoom = mZoomFitFactor;
	int xDisplay = x * curZoom - xOrigine;
	int yDisplay = y * curZoom - yOrigine;
	return QPoint(xDisplay, yDisplay);
}

/* Change zoom centered to a point on display with a zoom increment */
void ImageWidget::zoomOnDisplayPixel( int xCenterOnDisp, int yCenterOnDisp,
						  float zoominc )
{
	if(!m_displayImageBGRA) return;

	fprintf(stderr, "ImageWidget %p::%s:%d : mZoomFitFactor=%g image=%dx%d => center=%d,%d inc=%g\n",
			this, __func__,__LINE__,
			mZoomFitFactor,
			m_displayImageBGRA->width, m_displayImageBGRA->height,
			xCenterOnDisp, yCenterOnDisp, zoominc);

	if((mZoomFitFactor<=0. || mZoomFitFactor > 10000.f ) /* infinity */
		&& m_displayImageBGRA->width>0 && m_displayImageBGRA->height>0) {
		int wdisp = size().width()-2;
		int hdisp = size().height()-2;

		mZoomFitFactor = tmmin( (float)wdisp / (float)m_displayImageBGRA->width,
								   (float)hdisp / (float)m_displayImageBGRA->height );
	}

	float curZoom = mZoomFitFactor;

	mZoomFitFactor += zoominc;

	int disp_w =  size().width();
	int disp_h =  size().height();
	float zoommin = tmmin( (float)disp_w / (float)m_displayImageBGRA->width,
							  (float)disp_h / (float)m_displayImageBGRA->height );

	if(mZoomFitFactor < zoommin) {
		fprintf(stderr, "ImageWidget::%s:%d : mZoomFitFactor=%g < zoommin=%g\n",
				__func__, __LINE__,
				mZoomFitFactor, zoommin);

//		fprintf(stderr, "ImageWidget::%s:%d : mZoomFitFactor=%g < zoommin=%g"
//				"\nDETAILS: orig=%d,%d / %dx%d x,y=%d,%d zoom=%g => new=%g"
//				" => ZoomCenter=%d,%d disp=%dx%d => xOrig=%dx%d\n",
//				__func__, __LINE__,
//				mZoomFitFactor, zoommin,
//				xOrigine, yOrigine, dImage->width(), dImage->height(),
//				x,y, curZoom, mZoomFitFactor,
//				xZoomCenter, yZoomCenter,
//				disp_w, disp_h,
//				xZoomCenter - disp_w*0.5f/mZoomFitFactor,
//				yZoomCenter - disp_h*0.5f/mZoomFitFactor
//				);

		// recompute minimal zoom
		xOrigine = yOrigine = 0;
		mZoomFitFactor = -1.f;
		emit signalZoomChanged(mZoomFitFactor);

		update();
		return ;
	}

	// Compute zoom center on original image = origin + display pt / previous zoom scale
	// use float for more precision and no derivation up-left
	float xZCf = (float)xOrigine + (float)xCenterOnDisp / curZoom;
	float yZCf = (float)yOrigine + (float)yCenterOnDisp / curZoom;

	xZoomCenter = (int)roundf(xZCf);
	yZoomCenter = (int)roundf(yZCf);

	xOrigine = (int)roundf(xZCf - disp_w*0.5f/mZoomFitFactor);
	yOrigine = (int)roundf(yZCf - disp_h*0.5f/mZoomFitFactor);
	fprintf(stderr, "ImageWidget::%s:%d : emit signalZoomChanged(%g)\n",
			__func__, __LINE__, mZoomFitFactor);
	emit signalZoomChanged(mZoomFitFactor);

	clipSmartZooming();

	update();
}

void ImageWidget::wheelEvent ( QWheelEvent * e )
{
	if(!e) return;
	if(!m_displayImageBGRA) return;
	if(!mZoomFit) return;
	if(m_displayImageBGRA->width <= 0
			|| m_displayImageBGRA->height <= 0
			) return;


	if(mZoomFitFactor<0.) {
		int wdisp = size().width()-2;
		int hdisp = size().height()-2;

		mZoomFitFactor = tmmin( (float)wdisp / (float)m_displayImageBGRA->width,
								   (float)hdisp / (float)m_displayImageBGRA->height );
	}

	float curZoom = mZoomFitFactor;
	int numDegrees = e->delta() / 8;
	int numSteps = numDegrees / 15;

	int disp_w =  size().width();
	int disp_h =  size().height();
	float zoommin = tmmin( (float)disp_w / (float)m_displayImageBGRA->width,
							  (float)disp_h / (float)m_displayImageBGRA->height );



	if(numSteps > 0) {
		mZoomFitFactor *= 1.2;
	} else {
		mZoomFitFactor *= 0.8;
	}

	if(mZoomFitFactor < zoommin) {

		// recompute minimal zoom
		xOrigine = yOrigine = 0;
		mZoomFitFactor = -1.f;
		update();
		return ;
	}
	int x = e->pos().x(), y =  e->pos().y();
	xZoomCenter = xOrigine + x / curZoom;
	yZoomCenter = yOrigine + y / curZoom;

//	fprintf(stderr, "%s:%d : orig=%d,%d / %dx%d x,y=%d,%d zoom=%g => new=%g"
//			" => ZoomCenter=%d,%d disp=%dx%d => xOrig=%gx%g\n", __func__, __LINE__,
//			xOrigine, yOrigine, dImage->width(), dImage->height(),
//			x,y, curZoom, mZoomFitFactor,
//			xZoomCenter, yZoomCenter,
//			disp_w, disp_h,
//			xZoomCenter - disp_w*0.5f/mZoomFitFactor,
//			yZoomCenter - disp_h*0.5f/mZoomFitFactor
//			);

	xOrigine = xZoomCenter - disp_w*0.5f/mZoomFitFactor;
	yOrigine = yZoomCenter - disp_h*0.5f/mZoomFitFactor;
	clipSmartZooming();
	update();

	emit signalWheelEvent( e );
}

void ImageWidget::paintEvent( QPaintEvent * )
{
	if(!m_displayImageBGRA) {
		return;
	}

	QRect cr = rect();
	if(g_debug_ImageWidget) {
		fprintf(stderr, "ImageWidget %p::%s:%d : dImage=%dx%d size=%dx%d => cr=%dx%d fit=%c\n",
				this, __func__, __LINE__,
				m_displayImageBGRA->width, m_displayImageBGRA->height,
				size().width(), size().height(),
				cr.width(), cr.height(), mZoomFit ? 'T':'F');
	}

	QPainter p(this);

	if(mZoomFit)
	{
		int wdisp = size().width();
		int hdisp = size().height();
		if(mZoomFitFactor<0.) {
			mZoomFitFactor = tmmin( (float)wdisp / (float)m_displayImageBGRA->width,
									   (float)hdisp / (float)m_displayImageBGRA->height );

			m_displayImage = iplImageToQImage(m_displayImageBGRA).scaled(size(), Qt::KeepAspectRatio,
											Qt::SmoothTransformation ).copy(cr);
		}
		else
		{
			QImage cropImage = iplImageToQImage(m_displayImageBGRA).copy(
						xOrigine, yOrigine,
						wdisp/mZoomFitFactor, hdisp/mZoomFitFactor);
			if(cropImage.isNull())
			{
				fprintf(stderr, "ImageWidget::%s:%d : pb with crop (%d,%d+%d/%gx%d/%g)!\n",
						__func__, __LINE__,
						xOrigine, yOrigine,
						wdisp, mZoomFitFactor, hdisp, mZoomFitFactor
						);
			}
			else {
				m_displayImage = cropImage.scaled( wdisp, hdisp,
												   Qt::KeepAspectRatio,
												   (mZoomFitFactor > 1? Qt::FastTransformation: Qt::SmoothTransformation) ).copy(cr);
			}
		}
		int xoff = (size().width() - m_displayImage.width())/2;
		int yoff = (size().height() - m_displayImage.height())/2;

		p.drawImage(xoff, yoff, m_displayImage);
	}
	else
	{
		if(cr != this->rect()) {
			p.drawImage(0,0, m_displayImage.copy(cr));
		} else {
			p.drawImage(0,0, m_displayImage);
		}
	}

	if(mGridSteps > 0 && m_displayImageBGRA)
	{
		p.setPen(QPen(QColor(qRgb(0,0,192))));

		for(int step = 0; step < mGridSteps; step++)
		{
			int x = (int)((float)m_displayImageBGRA->width * (float)step
						  / (float)mGridSteps + 0.5f);
			int y = (int)((float)m_displayImageBGRA->height * (float)step
						  / (float)mGridSteps + 0.5f);

			QPoint P1H = imageToDisplay(QPoint(x, 0));
			QPoint P2H = imageToDisplay(QPoint(x, m_displayImageBGRA->height));
			QPoint P1V = imageToDisplay(QPoint(0, y));
			QPoint P2V = imageToDisplay(QPoint(m_displayImageBGRA->width, y));

			p.drawLine(P1H, P2H); // horizontal line
			p.drawLine(P1V, P2V); // vertical line
		}
	}

	//#define EDITMODE_ZOOM	0
	//#define EDITMODE_PICKER	1
	//#define EDITMODE_ROIS	2
	if(mEditMode == EDITMODE_ROIS)
	{
		QList<QRect *>::iterator it;
		if(!mROIList.isEmpty()) {
			p.setPen(QPen(QColor(qRgb(164,192,164))));

			for(it = mROIList.begin(); it!=mROIList.end(); ++it)
			{
				QRect * prect = (*it);
				if(prect)
				{
					// Draw rect
					QRect drawRect = imageToDisplay(*prect);
					p.drawRect(drawRect);
				}
			}
		}

		if(mSelectedROI) {
			QRect drawRect = imageToDisplay(*mSelectedROI);
			p.setPen(QPen(QColor(qRgb(0,255,0))));
			p.drawRect(drawRect);
		}
	}


	if(m_overlayRect.width()>0) {
		p.setPen(m_overlayColor);
		p.drawRect(m_overlayRect.x()-cr.x(), m_overlayRect.y()-cr.y(),
				   m_overlayRect.width(), m_overlayRect.height());
	}

	p.end();
}








/** @brief Convert a rect from image reference to display */
QRect ImageWidget::imageToDisplay(QRect img_rect)
{
	QRect display_rect;
	display_rect = QRect((img_rect.x()- xOrigine) * mZoomFitFactor ,
						 (img_rect.y()- yOrigine) * mZoomFitFactor ,
						 img_rect.width() * mZoomFitFactor ,
						 img_rect.height() * mZoomFitFactor
						 );
//	fprintf(stderr, "ImgWidget::%s:%d : zs=%g img=%d,%d+%dx%d => %d,%d+%dx%d\n",
//			__func__, __LINE__,
//			mZoomFitFactor,
//			img_rect.x(),img_rect.y(), img_rect.width(), img_rect.height(),
//			display_rect.x(),display_rect.y(), display_rect.width(), display_rect.height()
//			);
	return display_rect;
}

/** @brief Convert a rect from display reference to image */
QRect ImageWidget::displayToImage(QRect display_rect)
{
	QRect img_rect;
	img_rect = QRect(xOrigine + (float)display_rect.x()/mZoomFitFactor,
					 yOrigine + (float)display_rect.y()/mZoomFitFactor,
					 (float)display_rect.width() /mZoomFitFactor,
					 (float)display_rect.height() /mZoomFitFactor
					 );
	if(g_debug_ImageWidget) {
		fprintf(stderr, "ImgWidget::%s:%d : sc=%g O=%d,%d disp=%d,%d+%dx%d => img=%d,%d+%dx%d\n",
				__func__, __LINE__,
				mZoomFitFactor, xOrigine, yOrigine,
				display_rect.x(),display_rect.y(), display_rect.width(), display_rect.height(),
				img_rect.x(),img_rect.y(), img_rect.width(), img_rect.height()
				);
	}

	return img_rect;
}

/** @brief Convert a point from image reference to display */
QPoint ImageWidget::imageToDisplay(QPoint img_pt)
{
	QPoint display_pt;
	display_pt = QPoint((img_pt.x()- xOrigine) * mZoomFitFactor ,
						(img_pt.y()- yOrigine) * mZoomFitFactor
						 );
//	fprintf(stderr, "ImgWidget::%s:%d : zs=%g img=%d,%d => disp=%d,%d\n",
//			__func__, __LINE__,
//			mZoomFitFactor,
//			img_pt.x(),img_pt.y(),
//			display_pt.x(), display_pt.y()
//			);
	return display_pt;
}

/** @brief Convert a point from display reference to image */
QPoint ImageWidget::displayToImage(QPoint display_pt)
{
	QPoint img_pt;
	img_pt = QPoint(xOrigine + (float)display_pt.x()/mZoomFitFactor,
					 yOrigine + (float)display_pt.y()/mZoomFitFactor);
//	fprintf(stderr, "ImgWidget::%s:%d : disp=%d,%d => img=%d,%d\n",
//			__func__, __LINE__,
//			display_pt.x(), display_pt.y(),
//			img_pt.x(), img_pt.y()
//			);
	return img_pt;
}









