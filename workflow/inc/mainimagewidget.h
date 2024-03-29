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

#ifndef MAINIMAGEWIDGET_H
#define MAINIMAGEWIDGET_H

#include "imageinfo.h"
#include <QtGui/QWidget>

namespace Ui {
	class MainImageWidget;
}

class FilterSequencer;

/** @brief Main image display with toolbars for : image control */

class MainImageWidget : public QWidget {
	Q_OBJECT
	Q_DISABLE_COPY(MainImageWidget)
public:
	explicit MainImageWidget(QWidget *parent = 0);
	virtual ~MainImageWidget();


	/** @brief Show/hide plugins button */
	void showPluginsButton(bool on);

	/** @brief Set the image */
	int setImage(QImage imageIn, t_image_info_struct * pinfo = NULL);

	/** @brief Set the image as IplImage. Depth may be >8bit : IPL_DEPTH_16U, IPL_DEPTH_32F...*/
	int setImage(IplImage * iplImageIn, t_image_info_struct * pinfo = NULL);

	/** @brief Get output ipl image, with filters applied */
	IplImage * getOutputImage() { return m_outputIplImage; }

	/** @brief Zoom on a part of input image at a specified scale */
	void zoomOn(int unscaled_x, int unscaled_y, float scale);

	/** @brief crop absolute part of image for display */
	void cropAbsolute(int crop_x, int crop_, int scale);

	/** @brief Set pointer to filter sequencer */
	void setFilterSequencer(FilterSequencer *);

protected:
	virtual void changeEvent(QEvent *e);

	t_image_info_struct mImageInfo;

	QRect m_displayRect;
	IplImage * m_inputIplImage;	///< input as IplImage, may be more than 8bit
	IplImage * m_outputIplImage;///< output of filters as IplImage, may be more than 8bit
	QImage m_fullImage;	///< QImage for display
	QImage m_displayImage;
	bool mSwapR_B; ///< swap R and B colors because source is RGB32 instead of BGR32

	FilterSequencer * mpFilterSequencer;
/// \bug obsolete zooming variable, now inside ImageWidget
//	int m_zoom_scale;
//	QPoint m_lastClick;
//	QRect m_cropRect;
//	QRect m_lastCrop;


	//bool m_mouse_has_moved;
private slots:
	// Zoom options
	void on_roiButton_toggled(bool checked);
	void on_pickerButton_toggled(bool checked);
	void on_grayscaleButton_toggled(bool checked);
	void on_zoomButton_toggled(bool checked);

	void on_globalImageLabel_signalMousePressEvent(QMouseEvent * e);
	void on_globalImageLabel_signalMouseReleaseEvent(QMouseEvent * e);
	void on_globalImageLabel_signalMouseMoveEvent(QMouseEvent * e);
	void on_globalImageLabel_signalWheelEvent( QWheelEvent * e );

	void on_globalImageLabel_signalPicker(QRgb colorRGB, int colorGrey, QPoint pt);
	void on_globalImageLabel_signalZoomRect(QRect cropRect);
	void on_globalImageLabel_signalZoomChanged(float);

	void slotUpdateImage();

	void slot_actNormalView_activated();
	void slot_actInvertedView_activated();
	void slot_actIndexedView_activated();
	void slot_actBlack2RedView_activated();
	void slot_actBlue2RedView_activated();

	void on_pluginsButton_clicked();

	void on_recordButton_toggled(bool checked);

	void on_snapButton_clicked();

	void on_depthButton_toggled(bool checked);

	void on_gtButton_toggled(bool checked);

private:
	Ui::MainImageWidget *m_ui;

signals:

	/// Signal to tell where the crop window is located
	void signalCropRect(QRect);

	/** @brief Viewed region has been updated

		@param coordinates of rect in original image reference
	 */
	void signalZoomRect(QRect cropRect);
	void signalZoomChanged(float scale);

	/** @brief The greyscale mode has been toggled */
	void signalGreyscaleToggled(bool);

	void signalPluginsButtonClicked();
	void signalRecordButtonToggled(bool);
	void signalSnapButtonClicked();
	void signalSnapshot(QImage);
};

#endif // MAINIMAGEWIDGET_H
