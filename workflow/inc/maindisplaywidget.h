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
#ifndef MAINDISPLAYWIDGET_H
#define MAINDISPLAYWIDGET_H

#include <QWidget>
#include <QTimer>

#include "imageinfo.h"

#include "FileVideoAcquisition.h"

namespace Ui {
    class MainDisplayWidget;
}

class MovieBookmarkForm;
class FilterSequencer;

/** @brief Main image display in middle of GUI

  */
class MainDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainDisplayWidget(QWidget *parent = 0);
    ~MainDisplayWidget();

	/** @brief Set the image */
	int setImageFile(QString imagePath, t_image_info_struct * pinfo = NULL);

	/** @brief Set the movie file */
	int setMovieFile(QString imagePath, t_image_info_struct * pinfo = NULL);

	/** @brief Get current image at full resolution */
	QImage getImage() { return m_fullImage; }

	/** @brief Zoom on a part of input image at a specified scale */
	void zoomOn(int unscaled_x, int unscaled_y, float scale);
	/** @brief crop absolute part of image for display */
	void cropAbsolute(int crop_x, int crop_, float scale);

	/** @brief Set pointer to filter sequencer */
	void setFilterSequencer(FilterSequencer *);
private:
    Ui::MainDisplayWidget *ui;

	/// Video acquisition
	FileVideoAcquisition mFileVA;
	QTimer mPlayTimer;
	bool mPlayGrayscale;///< if true, only decode grayscaled image (Y buffer)
	float mPlaySpeed; ///< play speed ratio

	/// Input image
	QImage m_fullImage;

	FilterSequencer * mpFilterSequencer;
	t_image_info_struct * mpImageInfoStruct;

	// Bookmarks
	QList<video_bookmark_t> m_listBookmarks;
	MovieBookmarkForm * m_editBookmarksForm;
	void appendBookmark(t_movie_pos);

private slots:
	void on_addBkmkButton_clicked();
	void on_bookmarksButton_clicked();
	void on_magneticButton_toggled(bool on);
	void on_speedComboBox_currentIndexChanged(QString );
	void on_grayscaleButton_toggled(bool);
	void on_goLastButton_clicked();
	void on_goNextButton_clicked();
	void on_playButton_toggled(bool checked);
	void on_goPrevButton_clicked();
	void on_goFirstButton_clicked();
	void updateDisplay();
	void on_mPlayTimer_timeout();

	void on_timeLineWidget_signalCursorBookmarkChanged(t_movie_pos);
	void on_timeLineWidget_signalCursorPositionChanged(unsigned long long);
	void on_mainImageWidget_signalZoomRect(QRect cropRect);
	void on_mainImageWidget_signalZoomChanged(float zoom_scale);

	void slotNewBookmarkList(QList<video_bookmark_t>);
signals:
	/** @brief Signal that the image has changed (when play/pause) to update navigation image widget
	*/
	void signalImageChanged(QImage);
	void signalZoomRect(QRect);
	void signalZoomChanged(float);
};

#endif // MAINDISPLAYWIDGET_H
