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
#include "piaf-common.h"
#include <QToolTip>
#include <QFileInfo>

#include "virtualdeviceacquisition.h"


int g_debug_ThumbImageFrame = SWLOG_DEBUG;



#define THUMBIMGFR_MSG(a,...)       { \
			if( (a)>=g_debug_ThumbImageFrame ) { \
					struct timeval l_nowtv; gettimeofday (&l_nowtv, NULL); \
					fprintf(stderr,"%03d.%03d %s [ThumbFrame %p] [%s] %s:%d : ", \
							(int)(l_nowtv.tv_sec%1000), (int)(l_nowtv.tv_usec/1000), \
							SWLOG_MSG((a)), this, __FILE__,__func__,__LINE__); \
					fprintf(stderr,__VA_ARGS__); \
					fprintf(stderr,"\n"); \
			} \
		}

ThumbImageFrame::ThumbImageFrame(QWidget *parent) :
	QFrame(parent),
	m_ui(new Ui::ThumbImageFrame)
{
	m_ui->setupUi(this);
	mpTwin = NULL;
	mCtrl = mShift = false;

//	setFocusPolicy(Qt::WheelFocus);

	mActive = false;
	mSelected = true; // set it to opossite of setSelected call, because it it is the same, the setSelected does nothing
	setSelected( false );
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

void ThumbImageFrame::setImageInfoStruct(t_image_info_struct * pinfo)
{
	if(!pinfo) { return; }

	float score = pinfo->score;
	QString imagePath = pinfo->filepath;
	QFileInfo fi(imagePath);
	m_ui->nameLabel->setText(fi.baseName());
	m_ui->extLabel->setText(fi.suffix());

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
		else {
			score = 0;// for adding 5 x .
		}
		for(int star = score; star < 5; star++) {
			stars += ".";
		}

		m_ui->starsLabel->setText(stars);
	}

	IplImage * img = pinfo->thumbImage.iplImage;
	QPixmap fullImage;
	if( img ) {
		QImage qImg = iplImageToQImage(img, true); // because image in inverted by OpenCV

		fullImage = QPixmap::fromImage( qImg );
		fprintf(stderr, "ThumbImageFrame::%s:%d : load '%s' : "
				"widg=%dx%d iplImageToQImage(%dx%d) => qImg=%dx%d => pixmap=%dx%d\n",
				__func__, __LINE__,
				imagePath.toUtf8().data(),
				wdisp, hdisp,
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

	fprintf(stderr, "ThumbImageFrame::%s:%d : load '%s' : widg=%dx%d & full=%dx%d => display=%dx%d\n",
			__func__, __LINE__,
			imagePath.toUtf8().data(),
			wdisp, hdisp,
			fullImage.width(), fullImage.height(),
			l_displayImage.width(), l_displayImage.height()
			);

	float Mo = pinfo->filesize / (1024.f*1024.f);
	float ko = pinfo->filesize / (1024.f);

	m_ui->globalImageLabel->setPixmap( l_displayImage );
	QString tip = imagePath + "\n"
			+ QString::number(pinfo->width) + " x "
			+ QString::number(pinfo->height) + " x "
			+ QString::number(pinfo->nChannels);
	QString sizeStr;
	if(Mo>=1.)
	{
		sizeStr.sprintf("%g", roundf(Mo*10.)/10.f);
		tip += "\n" + tr("Size: ") + sizeStr + tr("MB");
	}
	else
	{
		sizeStr.sprintf("%g", roundf(ko*10.)/10.f);
		tip += "\n" + tr("Size: ") + sizeStr + tr("kB");
	}

//	m_ui->globalImageLabel->setToolTip(tip);
	setToolTip(tip);

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
	m_ui->extLabel->setText(fi.suffix());

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
	QString tip = imagePath + "\n"
			+ QString::number(fullImage.width()) + " x "
			+ QString::number(fullImage.height());

	m_ui->globalImageLabel->setToolTip(tip);
}
void ThumbImageFrame::on_globalImageLabel_signalMouseDoubleClickEvent ( QMouseEvent * )
{
	emit signalThumbDoubleClicked(m_imagePath);
}

void ThumbImageFrame::on_globalImageLabel_signalMousePressEvent(QMouseEvent * e) {
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
	THUMBIMGFR_MSG(SWLOG_INFO, "this=%p Thumb '%s' set selected='%c' shift=%c ctrl=%c",
			 this,
			 m_imagePath.toAscii().data(),
			 selected ? 'T':'F',
			 mShift ? 'T':'F',
			 mCtrl ? 'T':'F'
			 );

	if(mSelected == selected)
	{
		return;
	}
	mSelected = selected;

	if(mpTwin) {
		if(mpTwin->isSelected() != mSelected) {
			mpTwin->setSelected(mSelected);
		}
	}

	updateBackground();
}

void ThumbImageFrame::setActive(bool active)
{
	if(active)
	{
		THUMBIMGFR_MSG(SWLOG_DEBUG, "this=%p Thumb '%s' active=%c",
					   this,
					   m_imagePath.toAscii().data(),
					   active ? 'T':'F');
	}
	mActive = active;
	if(mpTwin)
	{
		if(mpTwin->isActive() != mActive) {
			mpTwin->setActive(mActive);
		}
	}
	updateBackground();
}
// update background color depending on mActive and mSelected
void ThumbImageFrame::updateBackground()
{
	if(mSelected) {
		THUMBIMGFR_MSG(SWLOG_INFO, "this=%p Thumb '%s' selected='%c' active='%c' "
					   "shift=%c ctrl=%c "
					   "=> changed background",
				 this,
				 m_imagePath.toAscii().data(),
				 mSelected ? 'T':'F',
				 mActive ? 'T':'F',
					   mShift? 'T':'F',
					   mCtrl ? 'T':'F');
		//if(mActive) {
			setStyleSheet("background-color: rgb(60, 70, 255);");
	//	} else
//		{
			setStyleSheet("background-color: rgb(190, 200, 255);");
//		}

	} else {
		THUMBIMGFR_MSG(SWLOG_INFO, "this=%p Thumb '%s' selected='%c' active='%c' => changed background",
				 this,
				 m_imagePath.toAscii().data(),
				 mSelected ? 'T':'F',
				 mActive ? 'T':'F' );
		setStyleSheet("background-color: rgba(128, 128, 128, 0);");

		//		if(mActive) {
//			m_ui->setStyleSheet("background-color: rgb(60, 70, 255);");
//		}
//		else {
////			setStyleSheet("background-color: rgb(60, 70, 255);");
//			setStyleSheet("background-color: rgba(128, 128, 128, 0);");
//		}
	}
	if(mActive) {
		m_ui->globalImageLabel->setStyleSheet("border: 2px solid rgb(60, 70, 255); ");
	}
	else
	{
		m_ui->globalImageLabel->setStyleSheet("border: 0px;");
	}

	repaint();

}


void ThumbImageFrame::mousePressEvent( QMouseEvent * e )
{
	fprintf(stderr, "ThumbImageFrame %p::%s:%d : received event=%p selected=%c\n",
			this, __func__, __LINE__,
			e,
			mSelected ? 'T' : 'F'
						);

	bool invert = !mSelected;
//	setActive( true );
	setSelected( invert );

	if( (e->modifiers() == Qt::ShiftModifier) )
	{
		THUMBIMGFR_MSG(SWLOG_DEBUG, "Shift modifier");
		mShift = true;
		emit signal_shiftClick(this);
	}
	else if( (e->modifiers() == Qt::ControlModifier) )
	{
		THUMBIMGFR_MSG(SWLOG_DEBUG, "Ctrl modifier");
		mCtrl = true;
		emit signal_ctrlClick(this);
	}
	else
	{
		THUMBIMGFR_MSG(SWLOG_DEBUG, "click with no modifier");
		emit signal_click(this);
	}

	emit signal_mousePressEvent ( e );
}
void ThumbImageFrame::mouseReleaseEvent ( QMouseEvent * e ) {
	emit signal_mouseReleaseEvent ( e );
}

void ThumbImageFrame::focusInEvent ( QFocusEvent * event )
{
	fprintf(stderr, "ThumbImageFrame %p::%s:%d : received event=%p\n",
			this, __func__, __LINE__, event);
	grabKeyboard();
}

void ThumbImageFrame::focusOutEvent ( QFocusEvent * event )
{
	fprintf(stderr, "ThumbImageFrame %p::%s:%d : received event=%p\n",
			this, __func__, __LINE__, event);
	releaseKeyboard();
}
void ThumbImageFrame::keyReleaseEvent ( QKeyEvent * e )
{
	fprintf(stderr, "ThumbImageFrame %p::%s:%d : release event=%p => shift=ctrl=false\n",
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
			THUMBIMGFR_MSG(SWLOG_DEBUG, "Shift !");
			mShift = true;
		}

		if(key  == Qt::Key_Control)
		{
			THUMBIMGFR_MSG(SWLOG_DEBUG, "Ctrl !");
			mCtrl = true;
		}
	}

	emit signal_keyPressEvent ( e );
}
