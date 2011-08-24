/***************************************************************************
	   timehistogramwidget.h  -  time histogram widget
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

#ifndef TIMEHISTOGRAMWIDGET_H
#define TIMEHISTOGRAMWIDGET_H

#include <QWidget>

namespace Ui {
    class TimeHistogramWidget;
}

/** \brief Processing time statistics
*/
typedef struct {
	int nb_iter;
	unsigned int * histogram;
	int max_histogram;	///< nb of items in max_histogram array
	float time_scale;	///< time scale : item us to index in array
	int index_max;		///< index of histogram max
	unsigned int value_max;
	int overflow_count; ///< nb of overflow count
	double overflow_cumul_us; ///< Cumul of overflow times
} t_time_histogram;


class TimeHistogramWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimeHistogramWidget(QWidget *parent = 0);
    ~TimeHistogramWidget();

	void displayHisto(t_time_histogram time_histo);

protected:
    void changeEvent(QEvent *e);

private:
    Ui::TimeHistogramWidget *ui;
};

#endif // TIMEHISTOGRAMWIDGET_H
