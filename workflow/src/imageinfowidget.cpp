/***************************************************************************
 *            imageinfowidget.cpp : display information by image processing
 *
 *  Wed Jun 11 08:47:41 2009
 *  Copyright  2009  Christophe Seyve
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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <sstream>
#include <string>

#include "imageinfowidget.h"
#include "ui_imageinfowidget.h"

#include "imgutils.h"
#include "piaf-common.h"

#include "virtualdeviceacquisition.h"

ImageInfoWidget::ImageInfoWidget(QWidget *parent) :
	QWidget(parent),
	m_ui(new Ui::ImageInfoWidget)
{
	m_imgProc = NULL;
	m_ui->setupUi(this);

	m_starButtons[0] = m_ui->scoreButton0;
	m_starButtons[1] = m_ui->scoreButton1;
	m_starButtons[2] = m_ui->scoreButton2;
	m_starButtons[3] = m_ui->scoreButton3;
	m_starButtons[4] = m_ui->scoreButton4;
	m_starButtons[5] = m_ui->scoreButton5;

	m_curInfo = NULL;
}

// Read metadata
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <iostream>
#include <iomanip>
#include <cassert>


ImageInfoWidget::~ImageInfoWidget()
{
	delete m_ui;

	delete m_imgProc;
}







/* Set the background image */
void ImageInfoWidget::setImageFile(const QString &  imagePath) {
	// Load image on image processing object
	if(!m_imgProc) {
		m_imgProc = new ImageInfo();
	}

	QByteArray arrStr = imagePath.toUtf8();
	m_imgProc->loadFile( arrStr.data() );

	// RGB Histogram
	IplImage * histo = m_imgProc->getHistogram();
	if(histo) {
		// Display in label
		QImage img = iplImageToQImage(histo).scaled(
				m_ui->sharpnessImageLabel->width(),
				m_ui->sharpnessImageLabel->height(),
				Qt::KeepAspectRatio
				);
		m_ui->histoImageLabel->setPixmap(QPixmap::fromImage(img));
	}


	// And sharpness
	IplImage * sharpMask = m_imgProc->getSharpnessImage();
	if(sharpMask) {
		// Display in label
		QImage img = iplImageToQImage(sharpMask).scaled(
				m_ui->sharpnessImageLabel->width(),
				m_ui->sharpnessImageLabel->height(),
				Qt::KeepAspectRatio
				);
		m_ui->sharpnessImageLabel->setPixmap(QPixmap::fromImage(img));
	}

	// Then process HSV histogram
	IplImage * colHisto = m_imgProc->getColorHistogram();
	if(colHisto) {
		// Display in label
		QImage img = iplImageToQImage(colHisto).scaled(
				m_ui->hsvImageLabel->width(),
				m_ui->hsvImageLabel->height(),
				Qt::KeepAspectRatio
				);
		m_ui->hsvImageLabel->setPixmap(QPixmap::fromImage(img));
	}
}


void ImageInfoWidget::setImageInfo(t_image_info_struct * pinfo) {
	if(!pinfo
	   ) {
		// FIXME : clear display

		return ;
	}
	if(pinfo == m_curInfo) {
		// same
		return;
	}
	m_curInfo = pinfo;
	// try tu use sharpness
	int level = roundf(pinfo->score * 5.f / 100.f);
	if(level > 5) { level = 5; }

	if(level > 0) {// don't load picture if not necessary
		QIcon starOn(":/icons/star_on.png");
		for(int star = 1; star <= level; star++) {
			m_starButtons[star]->setIcon(starOn);
		}
	}

	if(level < 5) {
		QIcon starOff(":/icons/star_off.png");
		for(int star = level+1; star < 6; star++) {
			m_starButtons[star]->setIcon(starOff);
		}
	}

	// RGB Histogram
	if(pinfo->log_histogram[0]) {
		IplImage * histo =
				drawHistogram(pinfo->log_histogram, pinfo->grayscaled);

		if(histo) {
			// Display in label
			QImage img = iplImageToQImage(histo).scaled(
					m_ui->sharpnessImageLabel->width(),
					m_ui->sharpnessImageLabel->height(),
					Qt::KeepAspectRatio
					);
			int sh = tmmin(100, (int)pinfo->histo_score);
			m_ui->histoDial->setValue(sh);
			m_ui->histoImageLabel->setPixmap(QPixmap::fromImage(img));
		}
	}

	// And sharpness
	IplImage * sharpMask = pinfo->sharpnessImage;
	if(sharpMask) {
		// Display in label
		QImage img = iplImageToQImage(sharpMask).scaled(
				m_ui->sharpnessImageLabel->width(),
				m_ui->sharpnessImageLabel->height(),
				Qt::KeepAspectRatio
				);
		m_ui->sharpnessImageLabel->setPixmap(QPixmap::fromImage(img));
		int sh = tmmin(100, (int)pinfo->sharpness_score);
		m_ui->sharpnessDial->setValue(sh);
	}

	// Then process HSV histogram
	IplImage * colHisto = pinfo->hsvImage;
	if(colHisto) {
		// Display in label
		QImage img = iplImageToQImage(colHisto).scaled(
				m_ui->hsvImageLabel->width(),
				m_ui->hsvImageLabel->height(),
				Qt::KeepAspectRatio
				);
		m_ui->hsvImageLabel->setPixmap(QPixmap::fromImage(img));
	}
}


void ImageInfoWidget::changeEvent(QEvent *e)
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

/* Manually force score of a picture */
void ImageInfoWidget::forceScore(int percent) {
	if(!m_curInfo) return;

	m_curInfo->score = percent;
	t_image_info_struct * pinfo = m_curInfo;
	m_curInfo = 0; // to force update
	setImageInfo(pinfo);
}


void ImageInfoWidget::on_scoreButton0_clicked() {
	forceScore(0);
}
void ImageInfoWidget::on_scoreButton1_clicked() {
	forceScore(20);
}

void ImageInfoWidget::on_scoreButton2_clicked() {
	forceScore(40);
}
void ImageInfoWidget::on_scoreButton3_clicked() {
	forceScore(60);
}
void ImageInfoWidget::on_scoreButton4_clicked() {
	forceScore(80);
}

void ImageInfoWidget::on_scoreButton5_clicked() {
	forceScore(100);
}













