/***************************************************************************
	 imagetoavidialog.h  -  MJPEG encoder from list of still  images
							 -------------------
	begin                : Thu Sep 9 2010
	copyright            : (C) 2010 by Christophe Seyve (CSE)
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

#ifndef IMAGETOAVIDIALOG_H
#define IMAGETOAVIDIALOG_H

#include <QDialog>
#include <QList>
#include <QSettings>

namespace Ui {
	class ImageToAVIDialog;
}

/** \brief Conversion between a list of image files to AVI */
class ImageToAVIDialog : public QDialog
{
	Q_OBJECT
public:
	ImageToAVIDialog(QWidget *parent = 0);
	~ImageToAVIDialog();

	//virtual void done(int r);

protected:
	void changeEvent(QEvent *e);

private:
	Ui::ImageToAVIDialog *ui;

	/// Number of files
	QStringList m_filesList;
	void appendFileList(QStringList files);

    QSettings mSettings;

private slots:

    void on_filesButton_clicked();
	void on_dirButton_clicked();
	void on_goButton_clicked();
	void on_destButton_clicked();

signals:
	void signalNewMovie(QString);
};

#endif // IMAGETOAVIDIALOG_H
