#ifndef IMAGETOAVIDIALOG_H
#define IMAGETOAVIDIALOG_H

#include <QDialog>
#include <QList>

namespace Ui {
	class ImageToAVIDialog;
}

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
private slots:
	void on_filesButton_clicked();
	void on_dirButton_clicked();
	void on_goButton_clicked();
	void on_destButton_clicked();
signals:
	void signalNewMovie(QString);
};

#endif // IMAGETOAVIDIALOG_H
