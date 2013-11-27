/***************************************************************************
 *  colibrimainwindow.h - UI + Background processing thread for colibri console
 *
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

#ifndef ColibriMainWindow_H
#define ColibriMainWindow_H

#include <QtCore/QTimer>
#include <QtGui/QMainWindow>
#include <QThread>
#include <QMutex>
#include <QSettings>



// For capture mode
#include "imgutils.h"

#ifndef WIN32
#include "PiafFilter.h"
#endif

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


	void lockDisplay() { mDisplayMutex.lock(); }
	void unlockDisplay() { mDisplayMutex.unlock(); }



private:
	/// Iteration counter
	int m_iteration;

	/// Flag to let the thread loop run
	bool m_run;

	QMutex mDisplayMutex;

	/// Pointer to processing class
	ColibriMainWindow * m_pProcessor;

	/// Status flag of the running state of thread
	bool m_isRunning;

#ifndef WIN32
	/// Processing sequence filter
	FilterSequencer * m_pFilterManager;
#else
	/// \todo port to Win32
#endif
	IplImage * m_inputImage;
	IplImage * m_outputImage;
	CvCapture * m_capture;

};

/** \brief Saved settings for Colibri : paths, options ... */
typedef struct
{
	QString lastPluginsDir;	///< Last path of plugin sequence
	QString lastImagesDir;	///< Last path of loaded image
	QString lastMoviesDir;	///< Last path of loaded movie

} t_colibri_settings;

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

	/** @brief Set the path of sequence

		@return <0 if error, 0 is success
	*/
	int setPluginSequence(QString sequencepath);

private:
	Ui::ColibriMainWindow *ui;

	QTimer m_timer;
	int m_lastIteration;


	QImage qtImage; ///< QImage for display
	/** @brief Compute image and display result */
	void displayImage(IplImage * iplImage);

	/** @brief Start background thread for capture source */
	void startBackgroundThread();
#ifndef WIN32
	/// Processing sequence filter
	FilterSequencer mFilterManager;
	IplImage * mOutputImage; ///< Piaf plugin image for processing
#else
	/// \todo port to Win32
#endif
	/// Capture thread
	ColibriThread * m_pColibriThread;

	QRect m_grabRect;

/// Settings for piaf Colibri independant app
#define PIAFCOLIBRISETTINGS	"PiafColibri"

	QSettings mSettings;			///< Settings for restoring previous session
	void loadSettings();
	void saveSettings();

	t_colibri_settings mLastSettings; ///< Last directory settings (paths of plugin, image, movie ...)


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
	void on_gridButton_toggled(bool checked);
	void on_fullScreenButton_clicked();
	void on_actionFull_screen_activated();

};

#endif // ColibriMainWindow_H
