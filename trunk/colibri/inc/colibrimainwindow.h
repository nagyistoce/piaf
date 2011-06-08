/***************************************************************************
 *  wafmeter - Woman Acceptance Factor measurement / Qt GUI class
 *
 *  2009-08-10 21:22:13
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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef ColibriMainWindow_H
#define ColibriMainWindow_H

#include <QtCore/QTimer>
#include <QtGui/QMainWindow>
#include <QThread>
#include <QMutex>



// For capture mode
#include "imgutils.h"
#include "SwFilters.h"

namespace Ui
{
	class ColibriMainWindow;
}

class ColibriMainWindow;

/** @brief Background acquisition and processing thread
  */
class ColibriThread : public QThread
{
	Q_OBJECT

public:
	/** \brief Create thread with a processor function */
	ColibriThread(ColibriMainWindow *);
	~ColibriThread();

	/** @brief Set the acquisition source */
	void setCapture(CvCapture * capture);

	/** @brief Return image used as input for plugin sequence */
	IplImage * getInputImage() { return m_inputImage; };

	/** @brief Return image after processing by plugin sequence */
	IplImage * getOutputImage() { return m_outputImage; };

	/** @brief Tell the thread to stop */
	void stop();

	/** @brief Return iteration count */
	int getIteration() { return m_iteration; };

	/** @brief Thread loop */
	virtual void run();

private:
	/// Iteration counter
	int m_iteration;

	/// Flag to let the thread loop run
	bool m_run;

	/// Pointer to processing class
	ColibriMainWindow * m_pProcessor;

	/// Status flag of the running state of thread
	bool m_isRunning;

	SwFilterManager * m_pFilterManager;

	IplImage * m_inputImage;
	IplImage * m_outputImage;
	CvCapture * m_capture;

};

/** @brief Main WAFmeter window
  */
class ColibriMainWindow : public QMainWindow
{
    Q_OBJECT

public:
	ColibriMainWindow(QWidget *parent = 0);
	~ColibriMainWindow();
	/** @brief Compute the incoming image */
	void computeImage(IplImage * );

	/** @brief Set the path of sequence */
	void setPluginSequence(QString sequencepath);

private:
	Ui::ColibriMainWindow *ui;
	QString m_path;

	QTimer m_timer;
	int m_lastIteration;

	swImageStruct mSwImage; ///< Piaf plugin image struct for processing

	QImage qtImage; ///< QImage for display
	QMutex mDisplayMutex;
	/** @brief Compute image and display result */
	void displayImage(IplImage * iplImage);

	/** @brief Start background thread for capture source */
	void startBackgroundThread();

	/// Processing sequence filter
	SwFilterManager mFilterManager;

	/// Capture thread
	ColibriThread * m_pColibriThread;

	QRect m_grabRect;

	QImage decorImage;
	QImage resultImage;
private slots:
	void on_fileButton_clicked();
	void on_snapButton_clicked();
	void on_deskButton_clicked();
	void on_camButton_toggled(bool checked);
	void on_movieButton_toggled(bool checked);
	void on_m_timer_timeout();

	void on_grabTimer_timeout();

	void on_actionFull_screen_activated();

};

#endif // ColibriMainWindow_H
