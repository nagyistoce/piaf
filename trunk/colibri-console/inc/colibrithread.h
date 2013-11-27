#ifndef COLIBRITHREAD_H
#define COLIBRITHREAD_H
#include <QtCore/QTimer>
#include <QtGui/QMainWindow>
#include <QThread>
#include <QMutex>
#include <QSettings>



// For capture mode
#include "imgutils.h"
#include "PiafFilter.h"


#define COLIBRI_MSG(...) fprintf(stderr, "[%s] %s:%d ", __FILE__, __func__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");

/// Live video device, V4L2, by default
#define V4L2_DEVICE		0

/// HTTP video device
#define HTTP_DEVICE		1

/// OpenNI video device
#define OPENNI_DEVICE	2

/** @brief Background acquisition and processing thread
  */
class ColibriThread : public QThread
{
	Q_OBJECT

public:
	/** \brief Create thread with a processor function */
	ColibriThread();
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

	/** \brief Compute input image through sequencer */
	int computeImage(IplImage * inputImage);

	/** \brief Set the sequence file */
	int setSequenceFile(char * path);

private:
	/// Iteration counter
	int m_iteration;

	/// Flag to let the thread loop run
	bool m_run;

	QMutex mDisplayMutex;

	/// Status flag of the running state of thread
	bool m_isRunning;

#ifndef WIN32
	/// Processing sequence filter
	FilterSequencer mFilterSequencer;
#else
	/// \todo port to Win32
#endif
	IplImage * m_inputImage;
	IplImage * m_outputImage;
	CvCapture * m_capture;

};

#endif // COLIBRITHREAD_H
