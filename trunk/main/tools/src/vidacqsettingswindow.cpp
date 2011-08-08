/***************************************************************************
	vidacqsettingswindow.cpp  -  V4L2/OpenCV camera acquisition control window
							 -------------------
	begin                : Thu Aug 04 2011
	copyright            : (C) 2011 by Christophe Seyve (CSE)
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

#include "vidacqsettingswindow.h"
#include "ui_vidacqsettingswindow.h"
#include <QMessageBox>

VidAcqSettingsWindow::VidAcqSettingsWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::VidAcqSettingsWindow)
{
	mpVideoCaptureDoc = NULL;
	memset(&m_video_properties, 0, sizeof(t_video_properties));
    ui->setupUi(this);
}

VidAcqSettingsWindow::~VidAcqSettingsWindow()
{
    delete ui;
}
void VidAcqSettingsWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void VidAcqSettingsWindow::on_brightnessSlider_valueChanged(int value)
{

}

void VidAcqSettingsWindow::setVideoCaptureDoc(VideoCaptureDoc * pdoc)
{
	if(mpVideoCaptureDoc) {
		disconnect(mpVideoCaptureDoc);
	}

	mpVideoCaptureDoc = pdoc;
	if(mpVideoCaptureDoc) {
		connect(mpVideoCaptureDoc, SIGNAL(documentClosed(VideoCaptureDoc*)), this, SLOT(on_mpVideoCaptureDoc_documentClosed(VideoCaptureDoc*)));
		connect(mpVideoCaptureDoc, SIGNAL(documentChanged()), this, SLOT(updateVideoProperties()));
	}

	updateVideoProperties();
}

void VidAcqSettingsWindow::on_mpVideoCaptureDoc_documentClosed(VideoCaptureDoc * doc)
{
	// disconnect
	if(doc == mpVideoCaptureDoc && mpVideoCaptureDoc)
	{
		disconnect(mpVideoCaptureDoc);
		mpVideoCaptureDoc = NULL;
	}
}

void VidAcqSettingsWindow::sendVideoProperties()
{
	if(mpVideoCaptureDoc) {
		mpVideoCaptureDoc->setVideoProperties(m_video_properties);
	}
	updateVideoProperties();
}

void VidAcqSettingsWindow::updateVideoProperties()
{
	if(mpVideoCaptureDoc) {
		ui->centralwidget->setEnabled(true);

		m_video_properties = mpVideoCaptureDoc->getVideoProperties();

		QString str;

#define CHANGE_SLIDER(_slider,_label,_value)	\
		if((_slider)->value() != (_value)) { \
			str.sprintf("%d", (int)(_value)); \
			(_label)->setText(str); \
			(_slider)->blockSignals(true); \
			(_slider)->setValue((int)(_value)); \
			(_slider)->blockSignals(false); \
		}
#define CHANGE_COMBO_INDEX(_combo,_label,_value)	\
		if((_combo)->currentItem() != (_value)) { \
			str.sprintf("%d", (int)(_value)); \
			(_label)->setText(str); \
			(_combo)->blockSignals(true); \
			(_combo)->setCurrentIndex((_value)); \
			(_combo)->blockSignals(false); \
		}
#define CHANGE_COMBO_TEXT(_combo,_value)	\
		{ \
			QString valstr; valstr.sprintf("%d", (int)(_value)); \
			if(valstr.compare((_combo)->currentText()) != 0) { \
				(_combo)->blockSignals(true); \
				(_combo)->setCurrentIndex((_combo)->findText(valstr)); \
				(_combo)->blockSignals(false); \
			} \
		}

		str.sprintf("%g", m_video_properties.fps);
		ui->fpsLabel->setText(str);
		if((int)roundf(m_video_properties.fps) != ui->fpsSpinBox->value() ) {
			str.sprintf("%g", m_video_properties.fps);
			ui->fpsSpinBox->blockSignals(true);
			ui->fpsSpinBox->setValue((int)roundf(m_video_properties.fps));
			ui->fpsSpinBox->blockSignals(false);
		}

		// SIZE
		CHANGE_COMBO_TEXT(ui->widthComboBox, m_video_properties.frame_width);
		CHANGE_COMBO_TEXT(ui->heightComboBox, m_video_properties.frame_height);
		str.sprintf("%d x %d", (int)m_video_properties.frame_width,
					(int) m_video_properties.frame_height);
		ui->sizeLabel->setText(str);

		// BRIGHTNESS...
		CHANGE_COMBO_INDEX(ui->brightnessModeComboBox, ui->brightnessModeLabel, m_video_properties.auto_brightness);

		CHANGE_SLIDER(ui->brightnessSlider, ui->brightnessLabel, m_video_properties.brightness);
		CHANGE_SLIDER(ui->contrastSlider, ui->contrastLabel, m_video_properties.contrast);
		CHANGE_SLIDER(ui->hueSlider, ui->hueLabel, m_video_properties.hue);
		CHANGE_SLIDER(ui->saturationSlider, ui->saturationLabel, m_video_properties.saturation);

		CHANGE_COMBO_INDEX(ui->wbModeComboBox, ui->wbModeLabel, m_video_properties.auto_white_balance);

		CHANGE_SLIDER(ui->wbSlider, ui->wbLabel, m_video_properties.white_balance);

		// Exposure values
		CHANGE_SLIDER(ui->exposureSlider, ui->exposureLabel, m_video_properties.exposure);
		CHANGE_SLIDER(ui->gainSlider, ui->gainLabel, m_video_properties.gain);

		CHANGE_COMBO_INDEX(ui->exposureModeComboBox, ui->exposureModeLabel, m_video_properties.auto_exposure);
		CHANGE_COMBO_INDEX(ui->gainModeComboBox, ui->gainModeLabel, m_video_properties.auto_gain);

	} else {
		ui->centralwidget->setEnabled(false);
	}
}

void VidAcqSettingsWindow::on_exposureModeComboBox_currentIndexChanged(int index)
{
	m_video_properties.auto_exposure = index;
	sendVideoProperties();
}
void VidAcqSettingsWindow::on_exposureSlider_valueChanged(int value)
{
	m_video_properties.exposure = value;
	sendVideoProperties();
}
void VidAcqSettingsWindow::on_gainSlider_valueChanged(int value)
{
	m_video_properties.gain = value;
	sendVideoProperties();
}

/* BRIGHTNESS, CONTRAST .... */
void VidAcqSettingsWindow::on_brightnessModeComboBox_currentIndexChanged(int index)
{
	m_video_properties.auto_brightness = index;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_contrastSlider_valueChanged(int value)
{
	m_video_properties.contrast = value;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_hueSlider_valueChanged(int value)
{
	m_video_properties.hue = value;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_saturationSlider_valueChanged(int value)
{
	m_video_properties.saturation = value;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_wbSlider_valueChanged(int value)
{
	m_video_properties.white_balance = value;
	sendVideoProperties();
}



void VidAcqSettingsWindow::on_wbModeComboBox_currentIndexChanged(int index)
{
	m_video_properties.auto_white_balance = index;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_wbDoButton_clicked()
{
	m_video_properties.do_white_balance = true;
}

void VidAcqSettingsWindow::on_resetButton_clicked()
{
	m_video_properties.do_white_balance = true;
	m_video_properties.auto_white_balance = 1;
	m_video_properties.auto_brightness = 1;
	m_video_properties.auto_exposure = 3; // Av
	m_video_properties.auto_gain = 1;

	// by security, force br and contrast (tested on Lifecam)
	m_video_properties.brightness = 128;
	m_video_properties.saturation = 128;

	sendVideoProperties();
}

/*********************************************************************/

void VidAcqSettingsWindow::on_applySizeButton_clicked()
{
	int w, h;
	bool okw, okh;
	w = ui->widthComboBox->currentText().toInt(&okw, 10);
	h = ui->heightComboBox->currentText().toInt(&okh, 10);

	if(okw && okh)
	{
		m_video_properties.frame_width = w;
		m_video_properties.frame_height = h;

		m_video_properties.fps = ui->fpsSpinBox->value();

		fprintf(stderr, "[VidAcqSettings]::%s:%d : %dx%d\n",
				__func__, __LINE__,
				w, h );

		sendVideoProperties();
	}
	else
	{
		QMessageBox::critical(NULL, tr("Invalid size"), tr("Could not read image size in combos"));
	}
}
