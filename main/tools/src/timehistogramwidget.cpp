/***************************************************************************
	   timehistogramwidget.cpp  -  time histogram widget
							 -------------------
	begin                : Wed Aug 24 2011
	copyright            : (C) 2002-2011 by Christophe Seyve
	email                : cseyve@free.fr
 ***************************************************************************/


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


#include "timehistogramwidget.h"
#include "ui_timehistogramwidget.h"

#include "swvideodetector.h"


TimeHistogramWidget::TimeHistogramWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TimeHistogramWidget)
{
    ui->setupUi(this);
}

TimeHistogramWidget::~TimeHistogramWidget()
{
    delete ui;
}

void TimeHistogramWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void TimeHistogramWidget::displayHisto(t_time_histogram time_histo)
{
	if(time_histo.nb_iter > 0) {
		IplImage * histoImg = swCreateImage(cvSize(time_histo.max_histogram, ui->histogramLabel->height()-2),
											IPL_DEPTH_8U, 4);
		// get max
		time_histo.index_max = 0;
		time_histo.value_max = time_histo.histogram[0];
		double cumul_us = 0.;
		unsigned int cumul_nb = 0;
		for(int h = 1; h<time_histo.max_histogram; h++)
		{
			cumul_us += (double)h * (double)(time_histo.histogram[h])/ (double)time_histo.time_scale;
			cumul_nb += time_histo.histogram[h];

			if(time_histo.histogram[h] > time_histo.value_max) {
				time_histo.value_max = time_histo.histogram[h];
				time_histo.index_max = h;
			}
		}

		if(time_histo.value_max == 0) {
			swReleaseImage(&histoImg);
			return;
		}
		if(time_histo.value_max < time_histo.overflow_count) {
			time_histo.value_max = time_histo.overflow_count ;
		}


		// Draw log of histogram
		float scale_log = histoImg->height / log(time_histo.value_max);
		float max_ms = time_histo.max_histogram / time_histo.time_scale / 1000.f;


		for(int ms = 10; ms<max_ms; ms+=10) {
			int h = roundf(time_histo.time_scale * ms * 1000.f);
			cvLine(histoImg,
				   cvPoint(h, histoImg->height-1),
				   cvPoint(h, 0),
				   cvScalarAll(64), 1);
		}

		for(int ms = 50; ms<max_ms; ms+=50) {
			int h = roundf(time_histo.time_scale * ms * 1000.f);
			cvLine(histoImg,
				   cvPoint(h, histoImg->height-1),
				   cvPoint(h, 0),
				   cvScalarAll(192), 1);
		}


		int median_idx = -1;
		unsigned int median_cumul = 0;
		double mean_ms = cumul_us / (1000. * cumul_nb);

		double stddev_cumul = 0.f;

		// draw log histo
		for(int h = 0; h<time_histo.max_histogram; h++) {
			if(time_histo.histogram[h]>0) {
				if(median_idx < 0) {
					median_cumul += time_histo.histogram[h];
					if(median_cumul > cumul_nb/2) {
						median_idx = h;
					}
				}
				float time_ms = (float)h / time_histo.time_scale / 1000.;
				stddev_cumul += fabs(time_ms - mean_ms)*time_histo.histogram[h];

				CvScalar drawColor = CV_RGB(64,192,64);
				if(time_ms > 40) {
					// draw yellow
					drawColor = CV_RGB(192,192,64);
					if(time_ms > 80) {
						// draw orange
						drawColor = CV_RGB(255,142,64);

						if(time_ms > 1000) { // more than 1 second
							// draw red
							drawColor = CV_RGB(255,64,64);
						}
					}

				}
				cvLine(histoImg,
					   cvPoint(h, histoImg->height-1),
					   cvPoint(h, histoImg->height-1 - (int)log(time_histo.histogram[h]) * scale_log),
					   drawColor, 1);
			}
		}



		QString maxStr;
#define BATCHSTATS_DISPLAY_RES	10.f
		maxStr.sprintf("%gms", roundf(max_ms*10.f)/10.f);
		QString medianStr; medianStr.sprintf("%gms", roundf((float)median_idx / time_histo.time_scale)/1000.f);
		QString majStr; majStr.sprintf("%gms", roundf((float)time_histo.index_max / time_histo.time_scale / (1000.*BATCHSTATS_DISPLAY_RES))/BATCHSTATS_DISPLAY_RES);

		QString meanStr; meanStr.sprintf("%gms", roundf(mean_ms*BATCHSTATS_DISPLAY_RES)/BATCHSTATS_DISPLAY_RES);
		float overflows = 100.f * time_histo.overflow_count / cumul_nb;
		QString overflowStr; overflowStr.sprintf("%g %%", overflows);

		stddev_cumul /= (double)cumul_nb;
		QString stddevStr; stddevStr.sprintf("%gms", roundf((stddev_cumul)*BATCHSTATS_DISPLAY_RES)/BATCHSTATS_DISPLAY_RES);

		QString statStr =
				//						  tr("majority=") + majStr
				tr("mean=") + meanStr + ", "
				+ tr("stddev=") + stddevStr + " / "
				+ tr("median=") + medianStr
				//+ tr(" /display max=") + maxStr
				;
		ui->histoStatsLabel->setText(statStr);

		ui->histoMaxLabel->setText(tr("over=") + overflowStr + tr(" display max:") + maxStr + "|");

		// draw mean & median
		int hmean = time_histo.time_scale * mean_ms * 1000.f;
		int hstddev = time_histo.time_scale * stddev_cumul * 1000.f;

		cvLine(histoImg,
			   cvPoint(hmean-hstddev , 1),
			   cvPoint(hmean+hstddev , 1),
			   CV_RGB(60,60,190), 3);
		cvLine(histoImg,
			   cvPoint(hmean, histoImg->height-1),
			   cvPoint(hmean, 0),
			   CV_RGB(0,0,255), 1);

		hmean = median_idx;
		cvLine(histoImg,
			   cvPoint(hmean, histoImg->height-1),
			   cvPoint(hmean, 0),
			   CV_RGB(255,0,255), 1);

		// Draw overflow indicator
		if(time_histo.overflow_count>0) {
			cvLine(histoImg,
				   cvPoint(histoImg->width-1, histoImg->height-1),
				   cvPoint(histoImg->width-2,
						   histoImg->height-1
						   - (int)log(time_histo.overflow_count) * scale_log),
				   CV_RGB(255,0,0), 1);
		}
		QImage qImage( (uchar*)histoImg->imageData,
					   histoImg->width, histoImg->height, histoImg->widthStep,
					   ( histoImg->nChannels == 4 ? QImage::Format_RGB32://:Format_ARGB32 :
						 QImage::Format_RGB888 //Set to RGB888 instead of ARGB32 for ignore Alpha Chan
						 )
					   );
		QPixmap pixImage = QPixmap::fromImage(qImage);
		ui->histogramLabel->setPixmap(pixImage);

		swReleaseImage(&histoImg);
	}
}

