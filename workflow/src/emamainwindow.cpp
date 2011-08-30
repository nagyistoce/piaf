/***************************************************************************
 *            emamainwindow.cpp
 *
 *  Wed Jun 11 08:47:41 2009
 *  Copyright  2009  Christophe Seyve
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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

//#include <stdlib.h>
//#include <stdio.h>


#include "emamainwindow.h"
#include "ui_emamainwindow.h"

#include "emaimagemanager.h"

#include "imgutils.h"
#include "imageinfo.h"
#include "thumbimageframe.h"
#include "thumbimagewidget.h"

#include <QFileDialog>
#include <QSplashScreen>

// External tools which open new windows
// Piaf Dialogs
#include "piafconfigurationdialog.h"
//#include "pluginlistdialog.h"
#include "plugineditdialog.h"
#include "batchfiltersmainwindow.h"
#include "imagetoavidialog.h"

#include "imagewidget.h"

// FIXME
#include "workshopimagetool.h"


int g_EMAMW_debug_mode = EMALOG_DEBUG;

#define EMAMW_printf(a,...)  { \
				if(g_EMAMW_debug_mode>=(a)) { \
					fprintf(stderr,"EmaMainWindow::%s:%d : ",__func__,__LINE__); \
					fprintf(stderr,__VA_ARGS__); \
					fprintf(stderr,"\n"); \
				} \
			}


EmaMainWindow::EmaMainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::EmaMainWindow),
	mSettings( PIAFWKFL_SETTINGS )
{
	ui->setupUi(this);
	ui->loadProgressBar->hide();

	connect(&m_timer, SIGNAL(timeout()),
			this, SLOT(on_timer_timeout()));
	connect(ui->filterManagerForm,
			SIGNAL(signal_resizeEvent(QResizeEvent *)),
			this,
			SLOT(on_gridWidget_signal_resizeEvent(QResizeEvent *)));

	// For the moment, collections are not implemented
	//ui->collecShowCheckBox->setChecked(false);
	pWorkspace = new QWorkspace(ui->workshopFrame);

	loadSettings();

	mWorkImage.load(":icons/ema-splash.png");
//	pWorkshopImage = new WorkshopImage(mWorkImage, "work image");
//	pWorkshopImageTool = new WorkshopImageTool(pWorkshopImage, ui->workshopImageToolFrame,
//		true /* show snapshot button */,
//		false,
//		"default",
//		Qt::WA_DeleteOnClose
//		);
//	ui->workshopImageToolFrame->layout()->addWidget(pWorkshopImageTool->display());
//	pWorkshopImageTool->display()->show();
	pWorkshopImage = NULL;
	pWorkshopImageTool = NULL;

	ui->mainImageWidget->setFilterSequencer(
		ui->pluginManagerForm->createFilterSequencer()
	);

	// FIXME : use last page saved in settings
	ui->stackedWidget->setCurrentIndex(0);
}

EmaMainWindow::~EmaMainWindow()
{
	saveSettings();
	delete ui;
}

void EmaMainWindow::loadSettings()
{
	int size = mSettings.beginReadArray("folders");
	for (int i = 0; i < size; ++i) {
		mSettings.setArrayIndex(i);
		QString path = mSettings.value("path").toString();

		mDirectoryList.append( appendDirectoryToLibrary(path));
	}
	mSettings.endArray();
}

void EmaMainWindow::saveSettings()
{
	EMAMW_printf(EMALOG_DEBUG, "Saving settings...");

	// Save list of files
	mSettings.beginWriteArray("folders");
	for (int i = 0; i < m_workflow_settings.directoryList.size(); ++i) {
		mSettings.setArrayIndex(i);
		mSettings.setValue("path", m_workflow_settings.directoryList.at(i));
		EMAMW_printf(EMALOG_DEBUG, "\tadded directory '%s'",
					 m_workflow_settings.directoryList.at(i).toAscii().data());
		//settings.setValue("password", list.at(i).password);
	}

	mSettings.endArray();

}

void EmaMainWindow::on_appendNewPictureThumb(QString filename)
{
	// update progress
	appendThumbImage(filename);
}

void EmaMainWindow::on_gridButton_clicked() {
	//
	ui->stackedWidget->setCurrentIndex(1);
}

void EmaMainWindow::on_workspaceButton_clicked()
{
	ui->stackedWidget->setCurrentIndex(2);
}


void EmaMainWindow::on_imgButton_clicked() {
	//
	ui->stackedWidget->setCurrentIndex(0);
}








QSplashScreen * g_splash = NULL;
void EmaMainWindow::on_actionQuit_activated()
{
	exit(0);
}

void EmaMainWindow::on_actionEdit_plugins_activated()
{
	PluginEditDialog * pluginDialog = new PluginEditDialog(NULL);
	pluginDialog->show();
}

void EmaMainWindow::on_actionBatch_processor_activated()
{
	BatchFiltersMainWindow * batchProc = new BatchFiltersMainWindow();
	batchProc->show();
}

void EmaMainWindow::on_actionConvert_images_to_AVI_activated()
{
	ImageToAVIDialog * pconvertDialog = new ImageToAVIDialog();

	connect(pconvertDialog, SIGNAL(signalNewMovie(QString)),
			this, SLOT(on_pconvertDialog_signalNewMovie(QString)));

	pconvertDialog->show();
}

void EmaMainWindow::on_actionAbout_activated()
{
	QPixmap pix(":/icons/icons/ema-splash.png");
	if(g_splash) {
		g_splash->setPixmap(pix);
	}
	else {
		g_splash = new QSplashScreen(pix, Qt::WindowStaysOnTopHint );
	}

	QString verstr, cmd = QString("Ctrl+");
	QString item = QString("<li>"), itend = QString("</li>\n");
	g_splash->showMessage(

			QString("<br><br><br><br><br><br><br><br><br>") +
			QString("<br><br><br><br><br><br><br><br><br>") +
			QString("<br><br><br><br><br><br><br>") +
			tr("<b>Ema</b> version: ")

						  + verstr.sprintf("svn%04d%02d%02d", VERSION_YY, VERSION_MM, VERSION_DD)

						  + QString("<br>Website & Wiki: <a href=\"http://github.com/athoune/ema/\">http://github.com/athoune/ema/</a><br><br>")
						  + tr("Shortcuts :<br><ul>\n")
//							+ item + cmd + tr("O: Open a picture file") + itend
//							+ item + cmd + tr("S: Save corrected image") + itend
//							+ item + cmd + tr("H: Display version information") + itend
//							+ item + tr("M: Mark current crop area in red") + itend
//							+ item + tr("A: Apply proposed correction") + itend
//							+ item + tr("&rarr;: Go to next proposal") + itend
//							+ item + tr("&larr;: Go to previous proposal") + itend
//							+ item + tr("C: Switch to clone tool mode") + itend
//							+ item + tr("") + itend
//							+ item + cmd + tr("") + itend
//							+ item + tr("") + itend
//							+ item + cmd + tr("") + itend
						+ QString("</ul>\n")
							);
	repaint();// to force display of splash

	g_splash->show();
	g_splash->raise(); // for full screen mode
	g_splash->update();
}


void EmaMainWindow::on_groupBox_7_clicked()
{

}

void EmaMainWindow::on_zoomx1Button_clicked()
{

}

void EmaMainWindow::on_zoomx2Button_clicked()
{

}


/***************** FILE EXPLORER *********************/
void EmaMainWindow::on_filesShowCheckBox_stateChanged(int state) {
	if(state == Qt::Checked)
		ui->filesTreeWidget->show();
	else
		ui->filesTreeWidget->hide();


}


void EmaMainWindow::on_filesClearButton_clicked() {
	m_imageList.clear();
	ui->filesTreeWidget->clear();
//	ui->scrollAreaWidgetContents->layout()->removeWidget();
}

void EmaMainWindow::on_filesLoadButton_clicked()
{
	QString dir =  QFileDialog::getExistingDirectory(this,
					 tr("Add existing directory"),
					 "");
	DirectoryTreeWidgetItem * new_dir = appendDirectoryToLibrary(dir);
	fprintf(stderr, "EmaMW::%s:%d : added new_dir=%p = { item = %p }\n",
			__func__, __LINE__, new_dir, new_dir);
	mDirectoryList.append( new_dir );
//	appendFileList(list);
}


void scanDirectory(QString path, int * p_nb_files)
{
	QFileInfo fidir(path);
	if(fidir.isFile()) return;

	EMAMW_printf(EMALOG_TRACE, "scanning path '%s' (total for the moment = %d)\n",
			path.toAscii().data(), *p_nb_files);

	QDir dir(path);
	int nb_files = 0;
	QFileInfoList fiList = dir.entryInfoList();
	QFileInfoList::iterator it;
	for(it = fiList.begin(); it != fiList.end(); ++it)
	{
		QFileInfo fi = (*it);
		if(fi.isFile())
		{
			nb_files++;
		}
		else if(fi.isDir() && fi.absoluteFilePath().length() > path.length())
		{
			EMAMW_printf(EMALOG_TRACE, "\tscanning path '%s' (total for the moment = %d)\n",
					fi.absoluteFilePath().toAscii().data(), *p_nb_files);

			// scan recursively
			scanDirectory(fi.absoluteFilePath(), p_nb_files);
		}
	}

	*p_nb_files = *p_nb_files + nb_files;


}


void EmaMainWindow::on_filesTreeWidget_itemExpanded(QTreeWidgetItem * item)
{
	if(!item) return;

	fprintf(stderr, "EmaMW::%s:%d : expanded %p !\n", __func__, __LINE__, item);
	DirectoryTreeWidgetItem * curdir = (DirectoryTreeWidgetItem*)item;

	// add sub items
	fprintf(stderr, "EmaMW::%s:%d : EXPAND THIS TREE '%s' !\n",
					__func__, __LINE__,
					curdir->fullPath.toAscii().data());

	curdir->expand();
}
QStringList directoryFileScan(QString fullPath)
{
	QStringList fileList;
	QDir dir(fullPath);
	QFileInfoList fiList = dir.entryInfoList();
	QFileInfoList::iterator it;
	for(it = fiList.begin(); it != fiList.end(); ++it)
	{
		QFileInfo fi = (*it);
		if(fi.isFile() && fi.absoluteFilePath().length() > fullPath.length())
		{
			fileList.append( fi.absoluteFilePath() );
		}
		else if(fi.isDir() && fi.absoluteFilePath().length() > fullPath.length())
		{
			fileList.append( directoryFileScan(fi.absoluteFilePath() ));
		}
	}

	return fileList;
}

QStringList DirectoryTreeWidgetItem::getFileList()
{
	QStringList fileList;

	fileList.append(directoryFileScan(fullPath));

	return fileList;
}

void DirectoryTreeWidgetItem::expand()
{
	QDir dir(fullPath);
	QFileInfoList fiList = dir.entryInfoList();
	QFileInfoList::iterator it;
	for(it = fiList.begin(); it != fiList.end(); ++it)
	{
		QFileInfo fi = (*it);
		if(fi.isFile() && fi.absoluteFilePath().length() > fullPath.length())
		{
			EMAMW_printf(EMALOG_TRACE, "\tadding FILE '%s'\n",
					fi.absoluteFilePath().toAscii().data());


			// Add item for file
			t_file * new_file = new t_file;
			new_file->name = fi.baseName();
			new_file->fullPath = fi.absoluteFilePath();

			QStringList columns; columns << new_file->name;

			new_file->treeViewItem = new QTreeWidgetItem(
					this,
					columns);
		}
		else if(fi.isDir() && fi.absoluteFilePath().length() > fullPath.length())
		{
			EMAMW_printf(EMALOG_TRACE, "adding subdir '%s'\n",
					fi.absoluteFilePath().toAscii().data());

			subDirsList.append(
					new DirectoryTreeWidgetItem(this,
							fi.absoluteFilePath())
					);
		}
	}

	// delete fake "waiting" item
	if(subItem) {
		removeChild( subItem );
		delete subItem;
		subItem = NULL;
	}

}

DirectoryTreeWidgetItem::DirectoryTreeWidgetItem(QTreeWidgetItem * treeWidgetItemParent, QString path)
	: QTreeWidgetItem(treeWidgetItemParent),
	fullPath(path), nb_files(0)
{
	init();
}

DirectoryTreeWidgetItem::DirectoryTreeWidgetItem(QTreeWidget * treeWidgetParent, QString path)
	: QTreeWidgetItem(treeWidgetParent),
	fullPath(path), nb_files(0)
{
	init();
}

DirectoryTreeWidgetItem::~DirectoryTreeWidgetItem()
{

}

void DirectoryTreeWidgetItem::init()
{
	QFileInfo fi(fullPath);
	name = fi.baseName();
	setText(0, fi.completeBaseName());
	subItem = NULL;
	mIsFile = false;

	if(fi.isDir()) {
		nb_files = 0;
		scanDirectory(fullPath, &nb_files);


		setText(1, QString::number(nb_files));

		if(nb_files > 0) {
			// Add fake sub item "waiting"
			QStringList columns;
			columns.clear();
			columns << "...";

			subItem = new QTreeWidgetItem(this, columns);
		}
	}
	else if(fi.isFile())
	{
		mIsFile = true;
	}
}

DirectoryTreeWidgetItem * EmaMainWindow::appendDirectoryToLibrary(QString path,
																  QTreeWidgetItem * itemParent)
{
	QFileInfo fi(path);
	if(!fi.exists()) { return NULL; }

	m_workflow_settings.directoryList.append(path);

	DirectoryTreeWidgetItem * new_dir ;
	if(!itemParent) {
		new_dir = new DirectoryTreeWidgetItem(ui->filesTreeWidget, path);
	} else {
		new_dir = new DirectoryTreeWidgetItem(itemParent, path);
	}

	return new_dir;
}

void EmaMainWindow::appendFileList(QStringList list) {
	// Append to list
	QStringList::Iterator it = list.begin();
	QString fileName;

	while(it != list.end()) {
		fileName = (*it);
		++it;

		m_appendFileList.append( fileName );
	}

	ui->loadProgressBar->setValue(0);
	ui->loadProgressBar->show();

	// load in manager
	emaMngr()->appendFileList(list);

	// start timer to update while processing
	if(!m_timer.isActive()) {
		m_timer.start(500);
	}
}


void EmaMainWindow::on_timer_timeout() {

	int val = emaMngr()->getProgress();

	EMAMW_printf(EMALOG_TRACE, "Timeout => progress = %d", val)

	// update known files : appended files

	QString fileName;
	int nb = m_appendFileList.count();
	EMAMW_printf(EMALOG_TRACE, "Test if we can append %d files...", nb)

	QStringList::Iterator it = m_appendFileList.begin();
	while( !m_appendFileList.isEmpty() &&
			(it != m_appendFileList.end()) ) {
		fileName = (*it);
		ui->loadProgressBar->setValue(emaMngr()->getProgress());

		//if(!m_managedFileList.contains(fileName))
// FIXME : there's a problem when suppressing data in m_appendFileList and removing then in the same loop
		{
			appendThumbImage(fileName);
		}
		++it;

	}

	// Delete the files which have been added
	it = m_imageList.begin();
	while( (it != m_imageList.end()) ) {
		fileName = (*it);
		if(	m_appendFileList.contains(fileName) )
		{
			m_appendFileList.removeOne(fileName);
		}
		++it;
	}


	ui->loadProgressBar->setValue(emaMngr()->getProgress());

	if(!m_appendFileList.isEmpty()) {
		// next update in 500 ms
		m_timer.start(500);
	}

	if(emaMngr()->getProgress() == 100) {
		ui->loadProgressBar->hide();
		m_timer.stop();
	}

}

void EmaMainWindow::on_gridWidget_signal_resizeEvent(QResizeEvent * e)
{
	if(!e) { return ; }

	QGridLayout * grid = (QGridLayout *)ui->gridWidget->layout();
	QLayoutItem * item0 = grid->itemAt(0);
	if(!item0) {
		EMAMW_printf(EMALOG_ERROR, "not item at 0,0");
		return; }

	int thumb_count = m_imageList.count();

	QList<QLayoutItem *> itemList;

	// Add a slide to sorter
	int items_per_row = e->size().width() / item0->widget()->width();
	EMAMW_printf(EMALOG_DEBUG, "ResizeEvent e=%dx%d "
				 "=> %d/%d = %d items/row"
				 " %d items in whole scrollarea",
				 e->size().width(), e->size().height(),
				 e->size().width(), item0->widget()->width(),
				 items_per_row, thumb_count
				 );

	for(int thumb_idx = 0; thumb_idx<thumb_count; ) {
		QLayoutItem * item = grid->itemAt(0);
//		grid->itemAtPosition(row, col);

		if(item) { // Move item
			EMAMW_printf(EMALOG_DEBUG, "Removing thumb [%d] :"
					 " item=%p",
					 thumb_idx,
					 item);
			grid->removeItem(item);

			itemList.append(item);

//			grid->addItem(item, row2, col2);
			thumb_idx++;
		}
	}

	for(int thumb_idx = 0; thumb_idx<thumb_count; thumb_idx++) {
		int row2 = thumb_idx / items_per_row;
		int col2 = thumb_idx % items_per_row;

		QLayoutItem * item = itemList.at(thumb_idx);
		EMAMW_printf(EMALOG_DEBUG, "Moving thumb [%d] :"
				 " item=%p => %d,%d %% %d/row",
				 thumb_idx,
				 item,
				 row2, col2, items_per_row
				 );
		grid->addItem(item, row2, col2);
	}
	m_sorter_nbitems_per_row = items_per_row;

/*
	ThumbImageFrame * newThumb2 = new ThumbImageFrame(ui->gridWidget);
	newThumb2->setImageFile(fileName, pinfo->thumbImage.iplImage);

	QGridLayout * grid_layout = (QGridLayout *)ui->gridWidget->layout();
	grid_layout->addWidget( newThumb2, row, col );
*/

}

void EmaMainWindow::on_collecShowCheckBox_stateChanged(int state) {
	if(state == Qt::Checked)
		ui->collecTreeWidget->show();
	else
		ui->collecTreeWidget->hide();
}



void EmaMainWindow::on_filesTreeWidget_itemClicked (
		QTreeWidgetItem * item, int /*unused column */) {
	if(!item) return;

	// read image file
	QString fileName = item->text(0); // col 0 has the full path
	on_thumbImage_selected( fileName );
}

void EmaMainWindow::on_filesTreeWidget_itemDoubleClicked (
		QTreeWidgetItem * item, int /*unused column */) {
	if(!item) return;

	DirectoryTreeWidgetItem * curdir = (DirectoryTreeWidgetItem *)item;
	// read image file
	if(curdir->isFile()) {
		QString fileName = curdir->fullPath; // col 0 has the full path
		on_thumbImage_clicked( fileName );
	} else {
		// Add all its content recursively
		appendFileList(curdir->getFileList());
	}

}


void EmaMainWindow::appendThumbImage(QString fileName) {

	t_image_info_struct * pinfo = emaMngr()->getInfo(fileName);

	if(!pinfo)
	{
		EMAMW_printf(EMALOG_TRACE, "Image file '%s' is NOT YET  managed",
					 fileName.toUtf8().data())
	} else {
		// image is already managed
		EMAMW_printf(EMALOG_TRACE, "Image file '%s' is now managed pinfo=%p",
					 fileName.toUtf8().data(), pinfo);

		// Append to managed pictures
		m_imageList.append(fileName);

		ui->nbFilesLabel->setText(QString::number(m_imageList.count()));

		// append to file display list
		QFileInfo fi(fileName);

		// And remove it from files being waited for
		// m_appendFileList.remove(fileName);
		// removed after because itmakes the iterator crash the app

		// And display it
		ThumbImageFrame * newThumb = new ThumbImageFrame(
				//ui->imageScrollArea);
				ui->scrollAreaWidgetContents );

		ui->scrollAreaWidgetContents->layout()->addWidget(newThumb);
		newThumb->setImageFile(fileName, pinfo->thumbImage.iplImage);


		newThumb->update();

		// Add a slide to sorter
		int items_per_row = ui->gridWidget->width()
							/ newThumb->width();
		int thumb_idx = m_imageList.count() - 1;
		if(items_per_row < 3) items_per_row = 3;
		int row = thumb_idx / items_per_row;
		int col = thumb_idx % items_per_row;

		m_sorter_nbitems_per_row = thumb_idx;

		EMAMW_printf(EMALOG_DEBUG, "Adding thumb '%s' at %d,%d / %d items/row",
					 fileName.toUtf8().data(),
					 row, col, items_per_row
					 );

		ThumbImageFrame * newThumb2 = new ThumbImageFrame(ui->gridWidget);
	//	ThumbImageWidget * newThumb2 = new ThumbImageWidget(ui->gridWidget);
		newThumb2->setImageFile(fileName, pinfo->thumbImage.iplImage,
								pinfo->score);

		// now delete thumb ?
		// FIXME : check if there is no side effect of this deletion
		tmReleaseImage(&pinfo->thumbImage.iplImage);

		QGridLayout * grid_layout = (QGridLayout *)ui->gridWidget->layout();
		grid_layout->addWidget( newThumb2, row, col );

		// connect this signal
		connect(newThumb, SIGNAL(signalThumbClicked(QString)), this, SLOT(on_thumbImage_clicked(QString)));
		connect(newThumb, SIGNAL(signalThumbSelected(QString)), this, SLOT(on_thumbImage_selected(QString)));
		if(newThumb2) {
			connect(newThumb2, SIGNAL(signalThumbClicked(QString)), this, SLOT(on_thumbImage_clicked(QString)));
			connect(newThumb2, SIGNAL(signalThumbSelected(QString)), this, SLOT(on_thumbImage_selected(QString)));
		}
	}
}

void EmaMainWindow::on_mainGroupBox_resizeEvent(QResizeEvent * e) {
	if(!e) { return ; }
	
	EMAMW_printf(EMALOG_DEBUG, "Resize event : %dx%d\n", e->size().width(), e->size().height())
	
}



void EmaMainWindow::on_thumbImage_selected(QString fileName)
{
	QFileInfo fi(fileName);
	if(fi.exists()) {
		t_image_info_struct * pinfo = emaMngr()->getInfo(fileName);
		if(!pinfo) {
			EMAMW_printf(EMALOG_WARNING, "File '%s' is not managed : reload and process file info\n", fileName.toUtf8().data())
			ui->imageInfoWidget->setImageFile(fileName);
		} else {

			EMAMW_printf(EMALOG_TRACE, "File '%s' is managed : use cache info", fileName.toUtf8().data())
			ui->imageInfoWidget->setImageInfo(pinfo);
			ui->exifScrollArea->setImageInfo(pinfo);
		}
	}
}


void EmaMainWindow::on_mainImageWidget_signalZoomRect(QRect cropRect)
{
	ui->globalNavImageWidget->setZoomRect(cropRect);
}

void EmaMainWindow::on_thumbImage_clicked(QString fileName)
{
	QFileInfo fi(fileName);
	if(fi.exists()) {
		ui->globalNavImageWidget->setImageFile(fileName);
		t_image_info_struct * pinfo = emaMngr()->getInfo(fileName);

		mWorkImage.load(fileName);
		//ui->mainImageWidget->setRefImage(&mWorkImage);
		if(!pinfo->isMovie)
		{
			ui->mainImageWidget->setImageFile(fileName, pinfo);

			if(pWorkshopImage) {
				pWorkshopImage->load(fileName);
			}
			if(pWorkshopImageTool) {
				pWorkshopImageTool->setWorkshopImage(pWorkshopImage);
				//pWorkshopImageTool->update();
			}
		}
		else
		{
			ui->mainImageWidget->setMovieFile(fileName, pinfo);
		}



		if(!pinfo) {
			EMAMW_printf(EMALOG_WARNING, "File '%s' is not managed : reload and process file info\n", fileName.toUtf8().data())
			ui->imageInfoWidget->setImageFile(fileName);
		} else {
			EMAMW_printf(EMALOG_DEBUG, "File '%s' is managed : use cache info\n", fileName.toUtf8().data())
			ui->imageInfoWidget->setImageInfo(pinfo);

			EMAMW_printf(EMALOG_DEBUG, "File '%s' show exif date\n", fileName.toUtf8().data())
			ui->exifScrollArea->setImageInfo(pinfo);
		}
	}
}


void EmaMainWindow::on_globalNavImageWidget_signalZoomOn(int x, int y, int scale) {
	ui->mainImageWidget->zoomOn(x,y,scale);
	ui->stackedWidget->setCurrentIndex(0);
}




