/***************************************************************************
           videocapture.h  -  doc class for video capture in Piaf
                             -------------------
    begin                : Wed Nov 13 10:07:22 CET 2002
    copyright            : (C) 2002 by Christophe Seyve
    email                : cseyve@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef VIDEOCAPTUREDOC_H
#define VIDEOCAPTUREDOC_H


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

// include files for QT
#include <qobject.h>
#include <qstring.h>
#include <qlist.h>
#include <qrect.h>

#include <QList>
#include <QTimer>

#include <QThread>
#include <QWaitCondition>
#include <QMutex>


// application specific includes
#include "virtualdeviceacquisition.h"

// forward declaration of the MultiVideo classes
class WorkshopVideoCaptureView;

/**	
  
  The VideoCaptureDoc class provides a document object that can be used in conjunction with the classes
  VideoCaptureApp and VideoCaptureView to create a document-view model for MDI (Multiple Document Interface)
  Qt 2.1 applications based on QApplication and QMainWindow as main classes and QWorkspace as MDI manager widget.
  Thereby, the document object is created by the VideoCaptureApp instance (and kept in a document list) and contains
  the document structure with the according methods for manipulating the document
  data by VideoCaptureView objects. Also, VideoCaptureDoc contains the methods for serialization of the document data
  from and to files.

  \brief VideoCaptureDoc provides a document object for a document-view model.
  \author Christophe Seyve \mail cseyve@free.fr
  \version 0.1.1 CVS
  */
class VideoCaptureDoc : public QThread
{
    Q_OBJECT

    friend class WorkshopVideoCaptureView;

public:
    /** Constructor for the fileclass of the application */
    VideoCaptureDoc();
	/** Constructor with existing OpenCVVideoAcquisition */
	VideoCaptureDoc(VirtualDeviceAcquisition * vAcq);
    /** Destructor for the fileclass of the application */
    ~VideoCaptureDoc();


	/** @brief Start acquisition loop */
	void start(Priority = InheritPriority);

    /** @brief Threaded acquisition loop */
    virtual void run();

	/** @brief Stop acquisition loop */
	void stop();


    //bool canCloseFrame(WorkshopVideoCaptureView* pFrame);
	/** @brief sets the modified flag for the document after a modifying action on the view connected to the document.*/
    void setModified(bool _m=true){ modified=_m; };
	/** @brief returns if the document is modified or not. Use this to determine if your document needs saving by the user on closing.*/
    bool isModified(){ return modified; };
	/** @brief deletes the document's contents */
    void deleteContents();
	/** @brief initializes the document generally */
    bool newDocument(int dev);
	/** @brief closes the current document */
    void closeDocument();
	/** @brief loads the document by filename and format and emits the updateViews() signal */
    bool openDocument(const QString &filename, const char *format=0);
	/** @brief saves the document under filename and format.*/
    bool saveDocument(const QString &filename, const char *format=0);
	/** @brief sets the path to the file connected with the document */
    void setPathName(const QString &name);
	/** @brief returns the pathname of the current document file*/
    const QString& pathName() const;

	/** @brief sets the filename of the document */
    void setTitle(const QString &title);
	/** @brief returns the title of the document */
    const QString& title() const;
    
	/** @brief Wait from one image in grabber (threaded)
		@return true if one image has been grabbed before timeout
	*/
	int waitForImage();

	/** @brief Set the video acquisition properties

	  @return @see VirtualDeviceAcquisition
	*/
	int setVideoProperties(t_video_properties props);

	/** @brief Get the video acquisition properties
	*/
	t_video_properties getVideoProperties();

private:
    /** the modified flag of the current document */
    bool modified;
    QString m_title;
    QString m_filename;

    /** the list of the views currently connected to the document */
    QList<WorkshopVideoCaptureView> *pViewList;

    // --- DETECTION MASK SECTION ----
public:
    /// add a new mask into detection masks list
    void addMask(int x, int y, int w, int h);
    /// get mask pointer from position
    QRect * getMaskFromPos(int x, int y);
    /// remove a rectangle from mask rectangle list
    void removeMask(QRect * area);
    /// save detection mask
    void saveMask();
    /// reset detection mask
    void clearMask();
    /// read detection mask buffer
    unsigned char * getMaskBuffer();
    /// set detection mask from file (PGM)
    void setDetectionMask(char * maskfile);
private:
    /// save detection mask
	QList<QRect *> * pMaskList;
    unsigned char * mask;
    int currentPixel;


    // ---- IMAGE ACQUISITION SECTION ----
public:
    /** returns current image buffer in RGB32 format */
    unsigned char *getCurrentImageRGB();
    /** reads image size */
	CvSize getImageSize();

    // -> PICTURE QUALITY/PARAMETERS
    /// set picture acquisition parameters (brightness, contrast, hue, whiteness...)
    int setpicture(int br, int hue, int col, int cont, int white);
    /// read picture acquisition parameters (brightness, contrast, hue, whiteness...)
    int getpicture(video_picture * pic);

	/** \brief Set exposure value in us */
	int setexposure( int exposure_us );
	/** \brief Set gain value */
	int setgain( int gain );

	// read device capabilities (min and max size)
    int getcapability(video_capability * vc);
    // change video acquisition size
    int changeAcqParams(tBoxSize newSize, int ch);
    // change channel
    int setChannel(int ch);

	/** @brief Get current image */
	IplImage * readImage();

private:
	/// Generic acquisition module
	VirtualDeviceAcquisition *mpVAcq;
    /// Run command (control flag)
    bool m_run;
    /// Run status
    bool m_running;

    QTimer *pImageTimer;
	QMutex mImageBufferMutex;

	QMutex mMutex;
	QWaitCondition mWaitCondition;

public slots:
    int loadImage();
private:
    /// image size
	CvSize imageSize;
    int allocateImages();

    bool acqReady;
	IplImage * imageRGBA;

signals:
	void documentChanged();
	void documentClosed(VideoCaptureDoc *);
};

#endif
