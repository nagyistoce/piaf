/***************************************************************************
 *      Main image display in middle of GUI
 *
 *  Sun Aug 30 14:06:41 2011
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

#include "inc/maindisplaywidget.h"
#include "ui_maindisplaywidget.h"


#include "piaf-common.h"
#include "PiafFilter.h"



MainDisplayWidget::MainDisplayWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainDisplayWidget)
{
    ui->setupUi(this);
	mpFilterSequencer = NULL;

}

MainDisplayWidget::~MainDisplayWidget()
{
    delete ui;
	mpFilterSequencer = NULL;

}

void MainDisplayWidget::setFilterSequencer(FilterSequencer * pFS)
{
	if(mpFilterSequencer)
	{
		mpFilterSequencer->disconnect();
	}
	mpFilterSequencer = pFS;

}

int MainDisplayWidget::setImageFile(QString imagePath,
								   t_image_info_struct * pinfo )
{
	int ret = 0;
	// print allocated images
	tmPrintIplImages();

	fprintf(stderr, "NavImageWidget::%s:%d ('%s')\n",
			__func__, __LINE__,
			imagePath.toAscii().data());

	if(!m_fullImage.load(imagePath)) {
		fprintf(stderr, "NavImageWidget::%s:%d could not load '%s'\n",
				__func__, __LINE__,
				imagePath.toAscii().data());
		ret = -1;
		m_fullImage = QImage(":/icons/error.png");
	}

	ui->mainImageWidget->setImage(m_fullImage, pinfo);

	return ret;
}

void  MainDisplayWidget::zoomOn(int x, int y, int scale) {

	ui->mainImageWidget->zoomOn(x,y,scale);
}


/** @brief Set the movie file */
int MainDisplayWidget::setMovieFile(QString moviePath, t_image_info_struct * pinfo)
{
	mFileVA.stopAcquisition();

	tBoxSize boxsize; memset(&boxsize, 0, sizeof(tBoxSize));

	int ret = mFileVA.openDevice(moviePath.toUtf8().data(), boxsize);
	if(ret >= 0) {
		m_fullImage = iplImageToQImage( mFileVA.readImageRGB32() );
	} else {
		PIAF_MSG(SWLOG_ERROR, "Could not open file '%s'", moviePath.toUtf8().data());
	}

	ui->mainImageWidget->setImage(m_fullImage, pinfo);

	return ret;
}
