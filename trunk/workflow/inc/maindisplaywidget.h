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

#include "imageinfo.h"

#include "FileVideoAcquisition.h"

namespace Ui {
    class MainDisplayWidget;
}


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

	/** @brief Zoom on a part of input image at a specified scale */
	void zoomOn(int unscaled_x, int unscaled_y, int scale);
	/** @brief crop absolute part of image for display */
	void cropAbsolute(int crop_x, int crop_, int scale);

	/** @brief Set pointer to filter sequencer */
	void setFilterSequencer(FilterSequencer *);
private:
    Ui::MainDisplayWidget *ui;

	/// Video acquisition
	FileVideoAcquisition mFileVA;

	/// Input image
	QImage m_fullImage;

	FilterSequencer * mpFilterSequencer;

};

#endif // MAINDISPLAYWIDGET_H
