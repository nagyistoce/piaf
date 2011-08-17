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

	ui->tabWidget->setCurrentIndex(0);
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
			QLabel * _labelptr = (_label); \
			if(_labelptr) _labelptr->setText(str); \
			(_slider)->blockSignals(true); \
			(_slider)->setValue((int)(_value)); \
			(_slider)->blockSignals(false); \
		}
#define CHANGE_COMBO_INDEX(_combo,_label,_value)	\
		if((_combo)->currentItem() != (_value)) { \
			str.sprintf("%d", (int)(_value)); \
			QLabel * _labelptr = (_label); \
			if(_labelptr) _labelptr->setText(str); \
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
		CHANGE_SLIDER(ui->jpegQualityHorizontalSlider, ui->jpegQualityLabel, m_video_properties.jpeg_quality);

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
		CHANGE_SLIDER(ui->gammaSlider, ui->gammaLabel, m_video_properties.gamma);
		CHANGE_SLIDER(ui->sharpnessSlider, ui->sharpnessLabel, m_video_properties.sharpness);

		CHANGE_COMBO_INDEX(ui->wbModeComboBox, ui->wbModeLabel, m_video_properties.auto_white_balance);

		CHANGE_SLIDER(ui->wbSlider, ui->wbLabel, m_video_properties.white_balance);

		if(m_video_properties.backlight != ui->backLightCheckBox->isOn())
		{
			ui->backLightCheckBox->blockSignals(true);
			ui->backLightCheckBox->setOn(m_video_properties.backlight);
			ui->backLightCheckBox->blockSignals(false);
		}

		// Exposure values
		CHANGE_SLIDER(ui->exposureSlider, ui->exposureLabel, m_video_properties.exposure);
		CHANGE_SLIDER(ui->gainSlider, ui->gainLabel, m_video_properties.gain);

		CHANGE_COMBO_INDEX(ui->exposureModeComboBox, ui->exposureModeLabel, m_video_properties.auto_exposure);
		CHANGE_COMBO_INDEX(ui->gainModeComboBox, ui->gainModeLabel, m_video_properties.auto_gain);
		CHANGE_COMBO_INDEX(ui->powerLineComboBox, ui->powerLineLabel, m_video_properties.power_line_frequency);
		CHANGE_COMBO_INDEX(ui->colorFxComboBox, ui->colorFxLabel, m_video_properties.color_fx);
		CHANGE_COMBO_INDEX(ui->colorKillerComboBox, ui->colorKillerLabel, m_video_properties.color_killer);

		// FOCUS
		CHANGE_COMBO_INDEX(ui->focusModeComboBox, ui->focusModeLabel, m_video_properties.auto_focus);
		CHANGE_SLIDER(ui->relativeFocusSlider, ui->relativeFocusLabel, m_video_properties.relative_focus);
		CHANGE_SLIDER(ui->absoluteFocusSlider, ui->absoluteFocusLabel, m_video_properties.absolute_focus);


		// PTZ
		CHANGE_SLIDER(ui->panAbsoluteSlider, NULL, m_video_properties.pan_absolute);
		CHANGE_SLIDER(ui->panRelativeSlider, NULL, m_video_properties.pan_relative);
		CHANGE_SLIDER(ui->tiltAbsoluteSlider, NULL, m_video_properties.tilt_absolute);
		CHANGE_SLIDER(ui->tiltRelativeSlider, NULL, m_video_properties.tilt_relative);
		CHANGE_SLIDER(ui->zoomAbsoluteSlider, NULL, m_video_properties.zoom_absolute);
		CHANGE_SLIDER(ui->zoomRelativeSlider, NULL, m_video_properties.zoom_relative);

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

void VidAcqSettingsWindow::on_brightnessSlider_valueChanged(int value)
{

	m_video_properties.brightness = value;
	sendVideoProperties();
	fprintf(stderr, "VidAcqSettings::%s:%d : changed brightness to %d\n",
			__func__, __LINE__, m_video_properties.brightness);fflush(stderr);
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


	// Set focus
	m_video_properties.auto_focus = false;
	m_video_properties.absolute_focus = 0;


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

void VidAcqSettingsWindow::on_backLightCheckBox_toggled(bool checked)
{
	m_video_properties.backlight = checked;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_gammaSlider_valueChanged(int value)
{
	m_video_properties.gamma = value;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_horizontalSlider_valueChanged(int value)
{
	m_video_properties.sharpness = value;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_focusModeComboBox_currentIndexChanged(int index)
{
	m_video_properties.auto_focus = index;
	sendVideoProperties();

}

void VidAcqSettingsWindow::on_relativeFocusSlider_valueChanged(int value)
{
	m_video_properties.relative_focus = value;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_absoluteFocusSlider_valueChanged(int value)
{
	m_video_properties.absolute_focus = value;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_panResetButton_clicked()
{
	m_video_properties.pan_reset = true;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_tiltResetButton_clicked()
{
	m_video_properties.tilt_reset = true;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_tiltRelativeSlider_valueChanged(int value)
{
	m_video_properties.tilt_relative = value;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_tiltAbsoluteSlider_valueChanged(int value)
{
	m_video_properties.tilt_absolute = value;
	sendVideoProperties();

}

void VidAcqSettingsWindow::on_panRelativeSlider_valueChanged(int value)
{
	m_video_properties.pan_relative = value;
	sendVideoProperties();

}

void VidAcqSettingsWindow::on_panAbsoluteSlider_valueChanged(int value)
{
	m_video_properties.pan_absolute = value;
	sendVideoProperties();

}

void VidAcqSettingsWindow::on_zoomRelativeSlider_valueChanged(int value)
{
	m_video_properties.zoom_relative = value;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_zoomAbsoluteSlider_valueChanged(int value)
{
	m_video_properties.zoom_absolute = value;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_powerLineComboBox_currentIndexChanged(int index)
{
	m_video_properties.power_line_frequency = index;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_colorFxComboBox_currentIndexChanged(int index)
{
	m_video_properties.color_fx = index;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_colorKillerComboBox_currentIndexChanged(int index)
{
	m_video_properties.color_killer = index;
	sendVideoProperties();
}

void VidAcqSettingsWindow::on_jpegQualityHorizontalSlider_valueChanged(int value)
{
	m_video_properties.jpeg_quality = value;
	fprintf(stderr, "VidAcqSettings::%s:%d : changed quality to %d\n",
			__func__, __LINE__, m_video_properties.jpeg_quality);fflush(stderr);

	//sendVideoProperties();
}
