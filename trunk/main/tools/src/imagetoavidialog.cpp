#include "imagetoavidialog.h"
#include "ui_imagetoavidialog.h"
#include <QFileDialog>
#include "OpenCVEncoder.h"
#include <QMessageBox>

ImageToAVIDialog::ImageToAVIDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ImageToAVIDialog)
{
	ui->setupUi(this);
}

ImageToAVIDialog::~ImageToAVIDialog()
{
	delete ui;
}

void ImageToAVIDialog::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}



void ImageToAVIDialog::on_destButton_clicked()
{
	QString filePath = QFileDialog::getSaveFileName(NULL,
												   tr("Create a AVI file"),
												   ".",
												   tr("MJPEG/AVI (*.avi *.AVI)")
												   );
	if(filePath.isEmpty() || filePath.isNull()) {
		ui->goButton->setEnabled(false);

		return;
	}
	ui->destPathLineEdit->setText(filePath);

	// enable Go button
	ui->goButton->setEnabled(true);
}

void ImageToAVIDialog::on_goButton_clicked()
{
	if(m_filesList.isEmpty()) {
		ui->progressBar->setEnabled(false);
		ui->goButton->setEnabled(false);
		fprintf(stderr, "ImgToAVI::%s:%d : no file in list: abort.\n",
				__func__, __LINE__);
		return;
	}


	fprintf(stderr, "ImgToAVI::%s:%d : encoding %d files in list...\n",
			__func__, __LINE__, m_filesList.count());

	ui->progressBar->setValue(0);

	OpenCVEncoder * encoder = NULL;
	QStringList list = m_filesList;
	list.sort();

	QStringList::Iterator it = list.begin();
	QFileInfo fi;
	int width=0, height=0, depth=0;
	QImage inputImage;
	ui->progressBar->setEnabled(true);

	int error_count = 0;
	QString errorStr;

	int progress = 0, file_count = 0;
	while(it != list.end()) {
		QString fileName = (*it);
		++it;

		inputImage.load(fileName);
		// if no encoder, open first image
		if(!inputImage.isNull()) {
			if(!encoder) {
			// read size
				width = inputImage.width();
				height = inputImage.height();
				depth = inputImage.depth();

				encoder = new OpenCVEncoder(width, height, ui->fpsSpinBox->value());
				encoder->startEncoder(ui->destPathLineEdit->text().toUtf8().data());
			}

			if(encoder) {
				int ret_ok = 0;
				if( width == inputImage.width()
					&& height == inputImage.height()
					&& depth == inputImage.depth()) {

					switch(depth) {
					default:
						break;
					case 32:
						//fprintf(stderr, "%s:%d : encode file '%s'\n", __func__, __LINE__,
						//		fileName.toUtf8().data());
						ret_ok = encoder->encodeFrameRGB32(inputImage.bits());
						break;
					case 8:
						//fprintf(stderr, "%s:%d : encode file '%s'\n", __func__, __LINE__,
						//		fileName.toUtf8().data());
						ret_ok = encoder->encodeFrameY(inputImage.bits());
						break;
					}
				}

				if(!ret_ok) {
					QString errFile;
					errFile.sprintf("%dx%dx%d), ",
									inputImage.width(),inputImage.height(), inputImage.depth()/8);
					errorStr += fileName + tr("(could not encode:") + errFile + "\n";
					error_count++;
				}
			}
		}

		file_count++;
		progress = (int)(100.f * (float)file_count / m_filesList.count());
		ui->progressBar->setValue(progress);
	}

	// Stop encoder
	if(encoder) {
		encoder->stopEncoder();
		delete encoder;
	}

	ui->progressBar->setValue(100);
	if(error_count > 0) {
		QMessageBox::warning(NULL, tr("Error in encoding"),
							 tr("Error while encoding files ")+errorStr);
	}

	emit signalNewMovie(ui->destPathLineEdit->text());
}

void ImageToAVIDialog::on_dirButton_clicked()
{
	QString dirPath = QFileDialog::getExistingDirectory(NULL,
														tr("Select a directory containing image files"),
														"." //imageDir
														);
	QDir dir(dirPath);
	QStringList files = dir.entryList(QDir::Files, QDir::Name);
	fprintf(stderr, "ImageToAVIDialog::%s:%d : choosen dir '%s' : %d files\n",
			__func__, __LINE__, dirPath.toUtf8().data(),
			files.count());
	if(!files.isEmpty()) {
		dir.cd(dirPath);
		chdir(dirPath);
		appendFileList(files);
	}
}


void ImageToAVIDialog::on_filesButton_clicked()
{
	QStringList files = QFileDialog::getOpenFileNames(
							 NULL,
							 tr("Select one or more image files to open"),
							 "",
							 tr("Images (*.png *.xpm *.jpg *.jpeg *.bmp *.tif*)"));
/*
	fileName = Q3FileDialog::getOpenFileName(imageDirName,
											 "Images (*.png *.jpg *.jpeg *.bmp *.tif*)", this,
											 "open image dialog", "Choose an image to open" );
											 */
	appendFileList(files);
}


void ImageToAVIDialog::appendFileList(QStringList files) {

	m_filesList.clear();

	QStringList list = files;
	QStringList::Iterator it = list.begin();
	QFileInfo fi;
	while(it != list.end()) {
		QString fileName = (*it);
		++it;

		// Append file
		fi.setFile(fileName);
		if(fi.exists() && fi.isFile()) {
			fprintf(stderr, "%s:%d : Append file '%s'\n", __func__, __LINE__,
					fi.absFilePath().toUtf8().data());
			m_filesList.append(fi.absFilePath());
		} else {
			fprintf(stderr, "%s:%d : Missing file '%s'=>%s'\n", __func__, __LINE__,
					fileName.toUtf8().data(),
					fi.absFilePath().toUtf8().data()
					);
		}
	}

	QString nbStr;
	nbStr.sprintf("%d", m_filesList.count());
	ui->nbFilesLabel->setText(nbStr + tr(" files"));

	if(m_filesList.isEmpty()) {
		ui->statusIconLabel->setPixmap(QPixmap(":/images/16x16/dialog-error.png"));

	} else {
		// Display properties

		it = list.begin();
		while(it != list.end()) {

			QString fileName = (*it);
			it++;

			QImage firstImage(fileName);
			if(!firstImage.isNull()) {
				nbStr.sprintf("%d x %d x%d bits", firstImage.width(), firstImage.height(), firstImage.depth());
				ui->infoLabel->setText(nbStr);
				break;
			}
		}

		ui->statusIconLabel->setPixmap(QPixmap(":/images/16x16/dialog-ok-apply.png"));
	}

}


