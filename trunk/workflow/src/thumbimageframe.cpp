/***************************************************************************
 *  thumbimageframe - Thumb display picture
 *
 *  Jul 2 21:10:56 2009
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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include "thumbimageframe.h"
#include "ui_thumbimageframe.h"
#include <QToolTip>
#include <QFileInfo>

ThumbImageFrame::ThumbImageFrame(QWidget *parent) :
	QFrame(parent),
	m_ui(new Ui::ThumbImageFrame)
{
	m_ui->setupUi(this);
	setSelected( false );

	grabKeyboard ();
}

ThumbImageFrame::~ThumbImageFrame()
{
	delete m_ui;
}

void ThumbImageFrame::changeEvent(QEvent *e)
{
	QFrame::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		m_ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void ThumbImageFrame::setImageFile(const QString & imagePath,
								   IplImage * img, int score )
{
	fprintf(stderr, "ThumbImageFrame::%s:%d this=%p ('%s', img=%p",
			__func__, __LINE__, this,
			imagePath.toUtf8().data(), img); fflush(stderr);
	if(img) {
		fprintf(stderr, ", img=%p %dx%dx%d, score=%d)\n",
				img,
				img?img->width : -1,
				img?img->height : -1,
				img?img->nChannels : -1,
				score);
	}
	else
	{
		fprintf(stderr, ")");
	}

	fflush(stderr);


	QFileInfo fi(imagePath);
	m_ui->nameLabel->setText(fi.baseName());
	m_ui->extLabel->setText(fi.extension(false));

	QPixmap l_displayImage;// local only
	m_imagePath = "";
	int wdisp = m_ui->globalImageLabel->width()-2;
	int hdisp = m_ui->globalImageLabel->height()-2;

	if(score < 0) { // hide
		m_ui->starsLabel->hide();
	} else {
		score --;
		if(score > 4) score = 4;
		QString stars = "";
		if(score > 0)
			for(int star = 0; star < score; star++) {
				stars += "*";
			}
		else
			score = 0;// for adding 5 x .

		for(int star = score; star < 5; star++) {
			stars += ".";
		}

		m_ui->starsLabel->setText(stars);
	}

	QPixmap fullImage;
	if( img ) {
		QImage qImg = iplImageToQImage(img);

		fullImage = QPixmap::fromImage( qImg );
		fprintf(stderr, "ThumbImageFrame::%s:%d : load '%s' : "
				"iplImageToQImage(%dx%d) => qImg=%dx%d => pixmap=%dx%d\n",
				__func__, __LINE__,
				imagePath.toUtf8().data(),
				img->width, img->height,
				qImg.width(), qImg.height(),
				fullImage.width(), fullImage.height()
				);
	} else {
		fprintf(stderr, "ThumbImageFrame::%s:%d no IplImage => has to load '%s'\n",
				__func__, __LINE__,
				imagePath.toUtf8().data() );
		fullImage.load(imagePath);
	}

	if(fullImage.isNull()
		|| fullImage.width() == 0
		|| fullImage.height() == 0
		) {
		l_displayImage.fill(127);
	}
	else {
		m_imagePath = imagePath;
		l_displayImage = fullImage.scaled( wdisp, hdisp,
								Qt::KeepAspectRatio );
	}

	fprintf(stderr, "ThumbImageFrame::%s:%d : load '%s' : %dx%d => %dx%d\n",
			__func__, __LINE__,
			imagePath.toUtf8().data(),
			fullImage.width(), fullImage.height(),
			l_displayImage.width(), l_displayImage.height()
			);

	m_ui->globalImageLabel->setPixmap( l_displayImage );
	m_ui->globalImageLabel->setToolTip(imagePath);
}

void ThumbImageFrame::on_globalImageLabel_signalMousePressEvent(QMouseEvent * ) {
	emit signalThumbClicked(m_imagePath);
}
void ThumbImageFrame::on_globalImageLabel_signalMouseMoveEvent(QMouseEvent * ) {
	//emit signalThumbSelected(m_imagePath);
}




// Forward mouse events
void ThumbImageFrame::mouseDoubleClickEvent ( QMouseEvent * event ) {
	// forward double click
	emit signal_mouseDoubleClickEvent ( event );

	// signal that we selected one item
	emit signal_mouseDoubleClickFile ( m_imagePath );
}

void ThumbImageFrame::mouseMoveEvent ( QMouseEvent * event ) {
	emit signal_mouseMoveEvent ( event );
}
/* set the selected flag */
void ThumbImageFrame::setSelected(bool selected)
{
	mSelected = selected;
	// Toggle state
	if(mSelected)
	{
		setStyleSheet("background-color: rgb(220, 230, 255);");

		if(mShift) {
			fprintf(stderr, "ThumbImageFrame %p::%s:%d : shift+click this=%p\n",
					this, __func__, __LINE__, this);

			emit signal_shiftClick(this);
		}
		if(mCtrl) {
			fprintf(stderr, "ThumbImageFrame %p::%s:%d : Ctrl+click this=%p\n",
					this, __func__, __LINE__, this);
			emit signal_ctrlClick(this);
		}

	} else {
//		setStyleSheet();
		setStyleSheet("background-color: rgba(128, 128, 128, 0);");
	}
	update();
}

void ThumbImageFrame::mousePressEvent ( QMouseEvent * e ) {
	setSelected( !mSelected );

	emit signal_mousePressEvent ( e );
}
void ThumbImageFrame::mouseReleaseEvent ( QMouseEvent * e ) {
	emit signal_mouseReleaseEvent ( e );
}

void ThumbImageFrame::keyReleaseEvent ( QKeyEvent * e )
{
	fprintf(stderr, "ThumbImageFrame %p::%s:%d : received event=%p\n",
			this, __func__, __LINE__, e);
	mCtrl = mShift = false;
	emit signal_keyReleaseEvent ( e );
}

void ThumbImageFrame::keyPressEvent ( QKeyEvent * e )
{
	fprintf(stderr, "ThumbImageFrame %p::%s:%d : received event=%p\n",
			this, __func__, __LINE__, e);
	if(e)
	{
		int key = e->key();
		//if(g_debug_Imagewidget)
		{
			fprintf(stderr, "ThumbImageFrame::%s:%d : received event=%p='%s' = %x\n", __func__, __LINE__,
					e,
					e->text().toUtf8().data(),
					e->key());
		}
		if(key == Qt::Key_Shift)
		{
//			fprintf(stderr, "ThumbImageFrame::%s:%d : Shift !\n",
//					__func__, __LINE__);
			mShift = true;
		}

		if(key  == Qt::Key_Control)
		{
//			fprintf(stderr, "ThumbImageFrame::%s:%d : Ctrl !\n",
//					__func__, __LINE__);
			mCtrl = true;
		}
	}

	emit signal_keyPressEvent ( e );
}
