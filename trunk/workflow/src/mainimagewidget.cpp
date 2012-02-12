/***************************************************************************
 *      Main image display in middle of GUI
 *
 *  Sun Aug 16 19:32:41 2011
 *  Copyright  2011  Christophe Seyve
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

#include "mainimagewidget.h"
#include "ui_mainimagewidget.h"
#include "piaf-common.h"

#include "PiafFilter.h"

#include <QMouseEvent>
#include <QMenu>

int g_EMAMainImgW_debug_mode = EMALOG_DEBUG;

#define EMAMIW_printf(a,...)  { \
		if(g_EMAMainImgW_debug_mode>=(a)) { \
			fprintf(stderr,"EmaMainImageW::%s:%d : ",__func__,__LINE__); \
			fprintf(stderr,__VA_ARGS__); \
			fprintf(stderr,"\n"); \
		} \
	}


MainImageWidget::MainImageWidget(QWidget *parent) :
	QWidget(parent),
	m_ui(new Ui::MainImageWidget)
{
	mpFilterSequencer = NULL;
	m_ui->setupUi(this);
	m_ui->globalImageLabel->switchToSmartZoomMode(true);
	//m_ui->globalImageLabel->grabKeyboard ();
	m_ui->globalImageLabel->setEditMode(EDITMODE_ZOOM);

	QMenu * viewMenu = new QMenu(m_ui->viewModeButton);
	m_ui->viewModeButton->setMenu(viewMenu);
	QIcon pixIcon;

	// Normal view
	QAction * actNormalView = viewMenu->addAction(QIcon(":icons/22x22/IconColorNormal.png"),
										tr("Default colors"));
	actNormalView->setShortcut(QKeySequence(("Ctrl+N")));
	actNormalView->setIconVisibleInMenu(true);
	connect(actNormalView, SIGNAL(activated()), this, SLOT(slot_actNormalView_activated()));

	// Inverted view
	QAction * actInvertedView = viewMenu->addAction(QIcon(":icons/22x22/IconColorGreyInverted.png"),
										tr("Inverted colors"));
	actInvertedView->setShortcut(QKeySequence(("Ctrl+I")));
	actInvertedView->setIconVisibleInMenu(true);
	connect(actInvertedView, SIGNAL(activated()), this, SLOT(slot_actInvertedView_activated()));

	// Indexed view
	QAction * actIndexedView = viewMenu->addAction(QIcon(":icons/22x22/IconColorGreyIndexed.png"),
										tr("Indexed colors"));
	actIndexedView->setShortcut(QKeySequence(("Ctrl+8")));
	actIndexedView->setIconVisibleInMenu(true);
	connect(actIndexedView, SIGNAL(activated()), this, SLOT(slot_actIndexedView_activated()));

	// Thermal black->red view
	QAction * actBlack2RedView = viewMenu->addAction(QIcon(":icons/22x22/IconColorThermicBlackToRed.png"),
										tr("Thermal black->red colors"));
	actBlack2RedView->setShortcut(QKeySequence(("Ctrl+B")));
	actBlack2RedView->setIconVisibleInMenu(true);
	connect(actBlack2RedView, SIGNAL(activated()), this, SLOT(slot_actBlack2RedView_activated()));

	// Thermal blue->red view
	QAction * actBlue2RedView = viewMenu->addAction(QIcon(":icons/22x22/IconColorThermicBlueToRed.png"),
										tr("Thermal blue->red colors"));
	actBlue2RedView->setShortcut(QKeySequence(("Ctrl+T")));
	actBlue2RedView->setIconVisibleInMenu(true);
	connect(actBlue2RedView, SIGNAL(activated()), this, SLOT(slot_actBlue2RedView_activated()));

}


MainImageWidget::~MainImageWidget()
{
	delete m_ui;
}

void MainImageWidget::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		m_ui->retranslateUi(this);
		break;
	default:
		break;
	}
}
void MainImageWidget::setFilterSequencer(FilterSequencer * pFS)
{
	if(mpFilterSequencer)
	{
		mpFilterSequencer->disconnect();
	}
	mpFilterSequencer = pFS;

	// update !
	connect(mpFilterSequencer, SIGNAL(selectedFilterChanged()), this, SLOT(slotUpdateImage()));

}


void MainImageWidget::slotUpdateImage()
{
	PIAF_MSG(SWLOG_TRACE, "update filterseq=%p !!",
			 mpFilterSequencer);

	if(mpFilterSequencer)
	{
		swImageStruct image;
		memset(&image, 0, sizeof(swImageStruct));
		image.width = (int)m_fullImage.width();
		image.height = (int)m_fullImage.height();
		image.depth = m_fullImage.depth() / 8;
		image.buffer_size = image.width * image.height * image.depth;

		m_displayImage = m_fullImage.copy();

		image.buffer = m_displayImage.bits(); // use diplay image as processing uffer

		int ret = mpFilterSequencer->processImage(&image);
		PIAF_MSG(SWLOG_TRACE, "sequence returned %d", ret);
	}

	// refresh display
	m_ui->globalImageLabel->setRefImage(&m_displayImage);

}

int MainImageWidget::setImage(QImage imageIn,
								   t_image_info_struct * pinfo )
{

//	fprintf(stderr, "NavImageWidget::%s:%d (%dx%dx%d, pinfo=%p)\n",
//			__func__, __LINE__,
//			imageIn.width(), imageIn.height(), imageIn.depth(),
//			pinfo
//			);

	m_fullImage = imageIn.copy();
	m_displayImage = m_fullImage.copy();

	// hide movie navigation bar
//	m_ui->movieToolbarWidget->hide();

	if(!m_fullImage.isNull()) {
		QString strInfo;
		if(!pinfo) {
			strInfo.sprintf( "%d x %d x %d\n",
					//pinfo->filepath.toUtf8().data(),
					m_fullImage.width(),
					m_fullImage.height(),
					m_fullImage.depth()/8);
		} else {
//			sprintf(info, "%s\n%d x %d x %d\n%d ISO\n%s",
//					imagePath.toUtf8().data(),
//					m_fullImage.width(),
//					m_fullImage.height(),
//					m_fullImage.depth()/8,
//					pinfo->ISO,
//					pinfo->datetime.toAscii().data()
//					);
			strInfo.sprintf("%s\n%d x %d x %d\n%d ISO",
					pinfo->filepath.toUtf8().data(),
					m_fullImage.width(),
					m_fullImage.height(),
					m_fullImage.depth()/8,
					pinfo->exif.ISO
					);
		}

		m_ui->globalImageLabel->setToolTip(strInfo);
	}

	slotUpdateImage();

	return 0;
}


void  MainImageWidget::zoomOn(int x, int y, float scale) {
	if(m_fullImage.isNull()) { return; }


	m_ui->globalImageLabel->setZoomCenter(x,y, scale);

//	int wdisp = m_ui->globalImageLabel->width()-2;
//	int hdisp = m_ui->globalImageLabel->height()-2;

//	int x_crop = (scale>0? x - wdisp/(scale*2) : 0);
//	if(x_crop<0) x_crop = 0;
//	else if(x_crop>m_fullImage.width() - wdisp) x_crop = m_fullImage.width() - wdisp;
//	int y_crop = (scale>0? y - hdisp/(scale*2) : 0);
//	if(y_crop<0) y_crop = 0;
//	else if(y_crop>m_fullImage.height() - hdisp) y_crop = m_fullImage.height() - hdisp;

//	cropAbsolute(x_crop, y_crop, scale);
}

void MainImageWidget::on_globalImageLabel_signalZoomRect(QRect cropRect)
{
//	fprintf(stderr, "MainImageWidget::%s:%d : received signalZoomRect(cropRect=%d,%d+%dx%d);\n",
//			__func__, __LINE__,
//			cropRect.x(), cropRect.y(), cropRect.width(), cropRect.height());
	emit signalZoomRect(cropRect);
}
void MainImageWidget::on_globalImageLabel_signalZoomChanged(float scale)
{
	emit signalZoomChanged(scale);
}

void MainImageWidget::cropAbsolute(int x_crop, int y_crop, int scale)
{
	return; // FIXME

//	int wdisp = m_ui->globalImageLabel->width()-2;
//	int hdisp = m_ui->globalImageLabel->height()-2;
//fprintf(stderr, "MainImageWidget::%s:%d : crop %d,%d  sc=%d\n",
//		__func__, __LINE__, x_crop, y_crop, scale);

//	// Crop on center of image
//	if(scale > 1) {
//		QImage cropImage = m_fullImage.copy(
//			x_crop, y_crop,
//			wdisp/scale, hdisp/scale);

//		m_displayImage = cropImage.scaled( wdisp, hdisp,
//							Qt::KeepAspectRatio );
//	}
//	else if(scale == 1)
//	{
//		m_displayImage = m_fullImage.copy(
//			x_crop, y_crop,
//			wdisp, hdisp);
//	}
//	m_cropRect = QRect(x_crop, y_crop, m_displayImage.width(), m_displayImage.height());

//	if(scale <= 0){
//		scale = 0;
//		m_displayImage = m_fullImage.scaled( wdisp, hdisp,
//							Qt::KeepAspectRatio );
//	}

//	m_zoom_scale = scale;
////	m_ui->globalImageLabel->setRefImage(&m_fullImage);
//			//Pixmap(QPixmap::fromImage(m_displayImage));

//	emit signalCropRect(m_cropRect);
}

void MainImageWidget::on_globalImageLabel_signalPicker(QRgb colorRGB, int colorGrey, QPoint pt)
{
	QString str;
	str.sprintf("%d,%d : (%d,%d,%d)\ng=%d",
				pt.x(), pt.y(), qRed(colorRGB), qGreen(colorRGB), qBlue(colorRGB), colorGrey);
	m_ui->infoLabel->setText(str);
	str.sprintf("background-color: rgb(%d,%d,%d);\n"
				"color: rgb(%d,%d,%d);\n"
				,
				qRed(colorRGB), qGreen(colorRGB), qBlue(colorRGB),
				(128+qRed(colorRGB))%255, (128+qGreen(colorRGB))%255, (128+qBlue(colorRGB))%255
				);
	m_ui->infoLabel->setStyleSheet(str);
}

void MainImageWidget::on_globalImageLabel_signalMousePressEvent(QMouseEvent * e) {
	int x = e->pos().x(), y =  e->pos().y();

	EMAMIW_printf(EMALOG_DEBUG,
				  "click=%d,%d "
				  "/ display img=%dx%d / orig=%dx%d /widget=%dx%d",
			x, y,
			m_displayImage.width(), m_displayImage.height(),
			m_fullImage.width(), m_fullImage.height(),
			width(), height()
			);

//	if(mEditMode == EDITMODE_NONE) {
//		if(m_zoom_scale == 0) // toggle view to zoom 1:1
//		{
//			m_zoom_scale = 1;
//			m_mouse_has_moved = true;
//			float fx = e->pos().x() - (width()-m_displayImage.width())/2
//					   , fy =  e->pos().y() - (height()-m_displayImage.height())/2;

//			fx *= (float)m_fullImage.width() / (float)m_displayImage.width();
//			fy *= (float)m_fullImage.height() / (float)m_displayImage.height();

//			EMAMIW_printf(EMALOG_DEBUG, "click=%d,%d => in full=%g,%g"
//						  "/ display img=%dx%d / orig=%dx%d",
//						  x, y,
//						  fx, fy,
//						  m_displayImage.width(), m_displayImage.height(),
//						  m_fullImage.width(), m_fullImage.height()
//						  );

//			x = (int)roundf(fx);
//			y = (int)roundf(fy);
//			m_lastClick = QPoint( x, y );

//			zoomOn(x, y, m_zoom_scale);
//		}
//	} else if(mEditMode == EDITMODE_ZOOM) {

//		if(!m_mouse_has_moved)
//		{
//			m_lastClick = e->pos();
//			m_lastCrop = m_cropRect;
//		}
//	}
}

void MainImageWidget::on_globalImageLabel_signalMouseReleaseEvent(QMouseEvent * e) {
	if(m_fullImage.isNull()) { return; }
	if(!e) return;


//	if(m_zoom_scale == 0) // toggle view
//	{
//		m_zoom_scale = 1;
//	} else if(!m_mouse_has_moved ){
//		m_zoom_scale = 0;

//		int x = m_fullImage.width() / 2;
//		int y = m_fullImage.height() / 2;

//		zoomOn(x, y, m_zoom_scale);
//	}
}

void MainImageWidget::on_globalImageLabel_signalMouseMoveEvent(QMouseEvent * e) {
	if(!e) return;
	if(e->buttons() != Qt::LeftButton) return;


//	int dx = e->pos().x() - m_lastClick.x();
//	int dy = e->pos().y() - m_lastClick.y();

//	if(m_zoom_scale != 0) {
//		dx /= m_zoom_scale;
//		dy /= m_zoom_scale;
//	}

//	//fprintf(stderr, "MainImageWidget::%s:%d : dx,dy=%d,%d\n", __func__, __LINE__, dx, dy);
//	m_mouse_has_moved = (dx != 0) || (dy != 0);

//	cropAbsolute(m_lastCrop.x() - dx ,
//		   m_lastCrop.y() - dy,
//		   m_zoom_scale);
}

void MainImageWidget::on_globalImageLabel_signalWheelEvent( QWheelEvent * e )
{
	if(!e) return;
	int numDegrees = e->delta() / 8;
	int numSteps = numDegrees / 15;
//	m_zoom_scale += numSteps;

//	zoomOn(e->x(), e->y(), m_zoom_scale);
}

void MainImageWidget::on_zoomButton_toggled(bool checked)
{
	if(checked)
	{
		m_ui->infoLabel->setStyleSheet("");
		m_ui->infoLabel->setText(tr("Zoom/move"));
		m_ui->globalImageLabel->setEditMode(EDITMODE_ZOOM);
	}
}



void MainImageWidget::on_pickerButton_toggled(bool checked)
{
	if(checked) {
		m_ui->infoLabel->setText(tr("Picker"));
		m_ui->globalImageLabel->setEditMode(EDITMODE_PICKER);
	}

}

void MainImageWidget::on_roiButton_toggled(bool checked)
{
	if(checked) {
		m_ui->infoLabel->setStyleSheet("");
		m_ui->infoLabel->setText(tr("Edit ROIs"));
		m_ui->globalImageLabel->setEditMode(EDITMODE_ROIS);
	}
}

void MainImageWidget::on_grayscaleButton_toggled(bool checked)
{
	if(checked) {
		m_ui->infoLabel->setText(tr("Grayscale"));
	} else {
		m_ui->infoLabel->setText(tr("Color"));
	}
}

void MainImageWidget::slot_actNormalView_activated()
{
	m_ui->infoLabel->setText(tr("Normal"));
	m_ui->globalImageLabel->setColorMode(COLORMODE_GREY);
	m_ui->viewModeButton->setIcon(QIcon(":icons/22x22/IconColorNormal.png"));
}

void MainImageWidget::slot_actInvertedView_activated()
{
	m_ui->infoLabel->setText(tr("Inverted"));
	m_ui->globalImageLabel->setColorMode(COLORMODE_GREY_INVERTED);
	m_ui->viewModeButton->setIcon(QIcon(":icons/22x22/IconColorGreyInverted.png"));
//	m_ui->globalImageLabel->setRefImage();
}


void MainImageWidget::slot_actIndexedView_activated()
{
	m_ui->infoLabel->setText(tr("Indexed"));
	m_ui->globalImageLabel->setColorMode(COLORMODE_INDEXED);
	m_ui->viewModeButton->setIcon(QIcon(":icons/22x22/IconColorGreyIndexed.png"));

}


void MainImageWidget::slot_actBlack2RedView_activated()
{
	m_ui->infoLabel->setText(tr("Black->red"));
	m_ui->globalImageLabel->setColorMode(COLORMODE_THERMIC_BLACK2RED);
	m_ui->viewModeButton->setIcon(QIcon(":icons/22x22/IconColorThermicBlackToRed.png"));

}


void MainImageWidget::slot_actBlue2RedView_activated()
{
	m_ui->infoLabel->setText(tr("Blue->red"));
	m_ui->globalImageLabel->setColorMode(COLORMODE_THERMIC_BLUE2RED);
	m_ui->viewModeButton->setIcon(QIcon(":icons/22x22/IconColorThermicBlueToRed.png"));
}


/* Show/hide plugins button */
void MainImageWidget::showPluginsButton(bool on)
{
	m_ui->pluginsButton->setVisible(on);
}


void MainImageWidget::on_pluginsButton_clicked()
{
	emit signalPluginsButtonClicked();
}

void MainImageWidget::on_recordButton_toggled(bool checked)
{
	emit signalRecordButtonToggled(checked);
}

void MainImageWidget::on_snapButton_clicked()
{
	emit signalSnapButtonClicked();

	emit signalSnapshot(m_fullImage);
}
