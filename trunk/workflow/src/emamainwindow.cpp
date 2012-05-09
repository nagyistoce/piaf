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

#include <sstream>

#define EMAMAINWINDOW_CPP
// Implement global variable
#define WORKSHOP_CPP

#include "emamainwindow.h"
#include "ui_emamainwindow.h"

#include "mainimagewidget.h"
#include "emaimagemanager.h"
#include "piaf-common.h"
#include "piaf-settings.h"

#include "imgutils.h"
#include "imageinfo.h"
#include "thumbimageframe.h"
#include "thumbimagewidget.h"

#include "batchqueuewidget.h"

#include "sequence_select_dialog.h"

#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>

#include <QFileDialog>
#include <QSplashScreen>
#include <QMdiArea>
#include <QStatusBar>



// External tools which open new windows
// Piaf Dialogs
#include "preferencesdialog.h"

//#include "pluginlistdialog.h"
#include "plugineditdialog.h"

#include "batch_progress_widget.h"

#include "imagetoavidialog.h"

#include "imagewidget.h"

#include "collectioneditdialog.h"


// OpenKinect libfreenect's driver for Microsoft kinect
#ifdef HAS_FREENECT
#include "freenectvideoacquisition.h"
#endif

// OpenNI's driver for Asus Xtion Pro and Microsoft kinect
#ifdef HAS_OPENNI
#include "openni_videoacquisition.h"
#endif

// Video4Linux
#ifdef _LINUX
#include "V4L2Device.h"
#endif

#include "piaf-common.h"

// anyway, inclue CpenCV for many kind of devices
#include "opencvvideoacquisition.h"

QSplashScreen * g_splash = NULL;

int g_EMAMW_debug_mode = SWLOG_DEBUG;

#define EMAMW_printf(a,...)  { \
				if((a) >= g_EMAMW_debug_mode) { \
					fprintf(stderr,"EmaMainWindow::%s:%d : ",__func__,__LINE__); \
					fprintf(stderr,__VA_ARGS__); \
					fprintf(stderr,"\n"); \
				} \
			}


EmaMainWindow::EmaMainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::EmaMainWindow),
	  mSettings( PIAFWKFL_SETTINGS ),
	  mSettingsDoc(PIAFWKFL_SETTINGS)
{

	ui->setupUi(this);
	g_splash->showMessage(QObject::tr("Restore settings ..."), Qt::AlignBottom | Qt::AlignHCenter);
	g_splash->update();

	ui->loadProgressBar->hide();
	ui->stopLoadButton->hide();
	ui->stackedWidget->setCurrentIndex(1); // by default, the grid page

	connect(&m_timer, SIGNAL(timeout()),
			this, SLOT(slot_timer_timeout()));

	// For the moment, collections are not implemented
	//ui->collecShowCheckBox->setChecked(false);
	pWorkspace = ui->mdiArea;

	// ======================== DEVICES =======================
	// Parent items for devices tree widget items
	mV4l2Item = mOpenNIItem = mFreenectItem = mOpenCVItem = NULL;


	g_splash->showMessage(QObject::tr("Restore settings ..."), Qt::AlignBottom | Qt::AlignHCenter);
	loadSettings();

	// ======================== COLLECTIONS =======================
	mSelectedThumbImageFrame = NULL;
	mpCurrentCollection = NULL;

	// Add quick collection
	mpQuickCollection = NULL;
	QString quickTitle = tr("Quick collection");
	QList<EmaCollection *>::iterator cit;
	for(cit = m_workflow_settings.collectionList.begin();
		cit != m_workflow_settings.collectionList.end();
		++cit)
	{
		if(quickTitle == (*cit)->title)
		{
			mpQuickCollection = (*cit);
		}
	}

	if(!mpQuickCollection)
	{
		mpQuickCollection = new EmaCollection();
		mpQuickCollection->title = quickTitle;
		mpQuickCollection->comment = tr("Quick collection to create temporary selection");
		mpQuickCollection->treeViewItem =
		mpQuickCollectionRootItem = new CollecTreeWidgetItem(ui->collecTreeWidget, mpQuickCollection);
		m_workflow_settings.collectionList.append(mpQuickCollection);
	}

	updateCollectionsTreeWidgetItems();


#ifdef PIAF_LEGACY
	pWorkshopImage = NULL;
	pWorkshopImageTool = NULL;
#endif
	g_splash->showMessage(QObject::tr("Load plugins ..."), Qt::AlignBottom | Qt::AlignHCenter);
	g_splash->repaint();
	ui->mainDisplayWidget->setFilterSequencer(
			ui->pluginManagerForm->createFilterSequencer()
			);

	// Default on file tree // FIXME: use last saved setting
	ui->inputsTabWidget->setCurrentIndex(0);

	mpBatchQueueWidget = new BatchQueueWidget(ui->page_batch);
	ui->page_batch->layout()->addWidget(mpBatchQueueWidget);

	// Hide splash
	if(g_splash)
	{
//		g_splash->hide();
	}



	connect(ui->mainDisplayWidget, SIGNAL(signalZoomRect(QRect)),
			ui->globalNavImageWidget, SLOT(slot_mainImageWidget_signalZoomRect(QRect)));
	connect(ui->mainDisplayWidget, SIGNAL(signalZoomChanged(float)),
			ui->globalNavImageWidget, SLOT(slot_mainImageWidget_signalZoomChanged(float)));
	connect(ui->mainDisplayWidget, SIGNAL(signalImageChanged(QImage)),
			ui->globalNavImageWidget, SLOT(slot_signalImageChanged(QImage)));
}

EmaMainWindow::~EmaMainWindow()
{
	saveSettings();
	delete ui;
}





void EmaMainWindow::appendCollection(QDomElement collecElem, EmaCollection * parent_collec)
{
	// Read parameters
	QString title = collecElem.attribute("title");
	EmaCollection * newCollec = new EmaCollection;
	newCollec->title = title;
	newCollec->comment = collecElem.attribute("comment");
	newCollec->treeViewItem = NULL;

	EMAMW_printf(SWLOG_INFO, "\t\tAdding collection '%s' / '%s'...",
			 title.toAscii().data(),
			 newCollec->comment.toAscii().data());

	// Read files
	QDomNode subcollecNode = collecElem.firstChild();
	while(!subcollecNode.isNull()) {
		QDomElement subcollecElem = subcollecNode.toElement(); // try to convert the node to an element.
		if(!subcollecElem.isNull()) {
			if(subcollecElem.tagName().compare("collection_files")==0)
			{
				EMAMW_printf(SWLOG_INFO, "\t\t\tAdding files...");
				QDomNode fileNode = subcollecElem.firstChild();
				while(!fileNode.isNull()) {
					// Add file
					QDomElement fileElem = fileNode.toElement(); // try to convert the node to an element.
					if(!fileElem.isNull()) {
						t_collection_file * pfile = new t_collection_file;

						pfile->fullpath = fileElem.attribute("fullpath");
						pfile->filename = QFileInfo(pfile->fullpath).baseName();
						pfile->pCollection = newCollec;
						pfile->treeViewItem = NULL;
						pfile->type = EMAFILE_TYPE_UNKNOWN;
						newCollec->filesList.append(pfile);

						EMAMW_printf(SWLOG_INFO, "\t\t\tAdding file '%s'...",
								 pfile->filename.toAscii().data()
								 );
					}

					fileNode = fileNode.nextSibling();
				}
			}
			// Adding sub-connections !
			if(subcollecElem.tagName().compare("subcollections")==0)
			{
				EMAMW_printf(SWLOG_INFO, "\t\t\tAdding sub-collections...");
				QDomNode subcollecNode2 = subcollecElem.firstChild();
				while(!subcollecNode2.isNull()) {
					QDomElement subcollecElem2 = subcollecNode2.toElement(); // try to convert the node to an element.
					if(!subcollecElem2.isNull()) {
						EMAMW_printf(SWLOG_INFO, "\t\t\t\tAdding sub-collection '%s'...", subcollecElem2.attribute("title").toAscii().data());
						appendCollection(subcollecElem2, newCollec);
					}
					subcollecNode2 = subcollecNode2.nextSibling();
				}
			}


		}
		subcollecNode = subcollecNode.nextSibling();
	}

	// append this collection to parent
	if(parent_collec)
	{
		parent_collec->appendSubCollection(newCollec);
	}
	else
	{
		// append to collections list
		m_workflow_settings.collectionList.append(newCollec);
	}
}



void EmaMainWindow::loadSettings()
{
	int size = mSettings.beginReadArray("folders");
	for (int i = 0; i < size; ++i) {
		mSettings.setArrayIndex(i);
		QString path = mSettings.value("path").toString();

//		mDirectoryList.append( appendDirectoryToLibrary(path));
	}
	mSettings.endArray();



	// READ CONFIGURATION IN XML FILE
	char home[1024] = "/home";
	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
	}


	//================ DEFAULT SETTINGS ================
	m_workflow_settings.defaultImageDir = QString(home) + "/" + tr("Images");
	m_workflow_settings.defaultMovieDir = QString(home) + "/" + tr("Movies");
	m_workflow_settings.defaultMeasureDir = QString(home) + "/" + tr("Measures");

	m_workflow_settings.maxV4L2 = 5;// because fast
	m_workflow_settings.maxOpenCV = 5;// because slow: need to open devices
	m_workflow_settings.maxOpenNI = 5;// because slow: need to open devices
	m_workflow_settings.maxFreenect = 5;// because slow: need to open devices

	// Concatenate configuration directories
	QFile file( QString(home) + "/" + PIAFWKFL_SETTINGS_XML );
	if (!file.open(QIODevice::ReadOnly))
	{
		EMAMW_printf(SWLOG_ERROR, "could not open file '%s' for reading: err=%s",
				 file.fileName().toAscii().data(),
				 file.errorString().toAscii().data());
		return;
	}
	else
	{
		EMAMW_printf(SWLOG_ERROR, "loading settings in file '%s'...",
				 file.fileName().toAscii().data());
		QString errorMsg;
		int errorLine = 0;
		int errorColumn = 0;
		if( !mSettingsDoc.setContent(&file, &errorMsg, &errorLine, &errorColumn) )
		{
			EMAMW_printf(SWLOG_ERROR,
					 "could not read content of file '%s' as XML doc: "
					 "err='%s' line %d col %d",
					 file.fileName().toAscii().data(),
					 errorMsg.toAscii().data(),
					 errorLine, errorColumn
					 );

			//do not return because it can work ! return;
		}
		file.close();
	}

	// print out the element names of all elements that are direct children
	// of the outermost element.
	QDomElement docElem = mSettingsDoc.documentElement();
	QDomNode n = docElem.firstChild();
	while(!n.isNull()) {
		QDomElement e = n.toElement(); // try to convert the node to an element.
		if(!e.isNull()) {
			EMAMW_printf(SWLOG_INFO, "\tCategory '%s'", e.tagName().toAscii().data()); // the node really is an element.

			if(e.tagName().compare("General")==0) // Read default directories
			{
				m_workflow_settings.defaultImageDir = e.attribute("ImageDir", m_workflow_settings.defaultImageDir);
				m_workflow_settings.defaultMovieDir = e.attribute("MovieDir", m_workflow_settings.defaultMovieDir);
				m_workflow_settings.defaultMeasureDir = e.attribute("MeasureDir", m_workflow_settings.defaultMeasureDir);

				m_workflow_settings.maxV4L2 = e.attribute("maxV4L2", "5").toInt();
				m_workflow_settings.maxOpenCV = e.attribute("maxOpenCV", "5").toInt();
				m_workflow_settings.maxOpenNI = e.attribute("maxOpenNI", "1").toInt();
				m_workflow_settings.maxFreenect = e.attribute("maxFreenect", "5").toInt();

			}

			if(e.tagName().compare("Folders")==0) // Read folders
			{
				// Clear previous list
				mDirectoryList.clear();

				QDomNode folderNode = e.firstChild();
				while(!folderNode.isNull()) {
					QDomElement folderElem = folderNode.toElement(); // try to convert the node to an element.
					if(!folderElem.isNull()) {
						// Read parameters
						QString path = folderElem.attribute("fullpath");
						EMAMW_printf(SWLOG_INFO, "\t\tAdding folder '%s'...", path.toAscii().data());
						mDirectoryList.append( appendDirectoryToLibrary(path));
					}
					folderNode = folderNode.nextSibling();
				}
			}

			if(e.tagName().compare("Collections")==0) // Read folders
			{
				// Clear previous list
				QDomNode collecNode = e.firstChild();
				while(!collecNode.isNull()) {
					QDomElement collecElem = collecNode.toElement(); // try to convert the node to an element.
					if(!collecElem.isNull()) {
						appendCollection(collecElem, NULL);
					}
					collecNode = collecNode.nextSibling();
				}
			}
		}
		n = n.nextSibling();
	}



}

void EmaMainWindow::addFolderToXMLSettings(QDomElement * parent_elem, t_folder folder)
{

}

void EmaMainWindow::addCollectionToXMLSettings(QDomElement * parent_elem,
											   EmaCollection collec)
{
	EMAMW_printf(SWLOG_INFO, "\tAdding collection '%s' to XML",
			 collec.title.toAscii().data());
	QDomElement elem = mSettingsDoc.createElement("collection");
	elem.setAttribute("title", collec.title);
	elem.setAttribute("comment", collec.comment);

	// Create list for files
	QDomElement filecollec_elem = mSettingsDoc.createElement("collection_files");
	QList<t_collection_file *>::iterator fit;
	for(fit = collec.filesList.begin(); fit != collec.filesList.end(); ++fit)
	{
		QDomElement file_elem = mSettingsDoc.createElement("collection_files");
		t_collection_file *pfile = (*fit);
		file_elem.setAttribute("filename", pfile->filename);
		file_elem.setAttribute("fullpath", pfile->fullpath);
		file_elem.setAttribute("type", pfile->type);
		filecollec_elem.appendChild(file_elem);
	}
	elem.appendChild(filecollec_elem);


	QDomElement subcollec_elem = mSettingsDoc.createElement("subcollections");
	QList<EmaCollection *>::iterator subcolit;
	for(subcolit = collec.subCollectionsList.begin();
		subcolit != collec.subCollectionsList.end(); ++subcolit)
	{
		EmaCollection * pcol = (*subcolit);
		if(pcol)
		{
			addCollectionToXMLSettings(&subcollec_elem, *pcol);
		}
	}
	elem.appendChild(subcollec_elem);

	parent_elem->appendChild(elem);
}



void EmaMainWindow::saveSettings()
{
	EMAMW_printf(EMALOG_DEBUG, "Saving settings...");

	// Save list of files
	mSettings.beginWriteArray("folders");
	for (int i = 0; i < m_workflow_settings.directoryList.size(); ++i) {
		mSettings.setArrayIndex(i);
		mSettings.setValue("path", m_workflow_settings.directoryList.at(i)->fullpath);
		EMAMW_printf(EMALOG_DEBUG, "\tadded directory '%s'",
					 m_workflow_settings.directoryList.at(i)->fullpath.toAscii().data());
		//settings.setValue("password", list.at(i).password);
	}

	mSettings.endArray();
	// Clear XML tree
	mSettingsDoc.clear();

//	QFile file("piafworkflowsettings.xml");
//	if (!file.open(QIODevice::ReadOnly)) {
//		EMAMW_printf(SWLOG_ERROR, "could not open file '%s' for reading: err=%s",
//				 file.name().toAscii().data(),
//				 file.errorString().toAscii().data());
//		//	 return;
//	}
//	else {
//		if (!doc.setContent(&file)) {
//			EMAMW_printf(SWLOG_ERROR, "could not read content of file '%s' dor XML doc: err=%s",
//					 file.name().toAscii().data(),
//					 file.errorString().toAscii().data());
//			file.close();

//			// return;
//		}
//		file.close();
//	}
//	 // print out the element names of all elements that are direct children
//	 // of the outermost element.
//	 QDomElement docElem = doc.documentElement();
//	 QDomNode n = docElem.firstChild();
//	 while(!n.isNull()) {
//		 QDomElement e = n.toElement(); // try to convert the node to an element.
//		 if(!e.isNull()) {
//			 cout << qPrintable(e.tagName()) << endl; // the node really is an element.
//		 }
//		 n = n.nextSibling();
//	 }
	QDomElement elemGUI = mSettingsDoc.createElement("GUISettings");

	// Append default directories
	QDomElement elemDirectories = mSettingsDoc.createElement("General");

	elemDirectories.setAttribute("ImageDir", m_workflow_settings.defaultImageDir);
	elemDirectories.setAttribute("MovieDir", m_workflow_settings.defaultMovieDir);
	elemDirectories.setAttribute("MeasureDir", m_workflow_settings.defaultMeasureDir);

	elemDirectories.setAttribute("maxV4L2", m_workflow_settings.maxV4L2);
	elemDirectories.setAttribute("maxOpenCV", m_workflow_settings.maxOpenCV);
	elemDirectories.setAttribute("maxOpenNI", m_workflow_settings.maxOpenNI);
	elemDirectories.setAttribute("maxFreenect", m_workflow_settings.maxFreenect);

	elemGUI.appendChild(elemDirectories);


	// Here we append a new element to the end of the document
	QDomElement elemFolders = mSettingsDoc.createElement("Folders");
	for (int i = 0; i < m_workflow_settings.directoryList.size(); ++i) {
		//mSettings.setValue("path", m_workflow_settings.directoryList.at(i));

		t_folder * folder = m_workflow_settings.directoryList.at(i) ;
		QDomElement elem = mSettingsDoc.createElement("folder");
		elem.setAttribute("fullpath", folder->fullpath);
		elem.setAttribute("filename", folder->filename);
		elem.setAttribute("extension", folder->extension);
		elem.setAttribute("expanded", folder->expanded);

		elemFolders.appendChild(elem);

		EMAMW_printf(EMALOG_INFO, "\tadded directory '%s' to XML",
					 m_workflow_settings.directoryList.at(i)->fullpath.toAscii().data()
					 );
	}

	elemGUI.appendChild(elemFolders);

	// SAVE COLLECTIONS
	QDomElement elemCollecs = mSettingsDoc.createElement("Collections");
	for(int i = 0; i < m_workflow_settings.collectionList.size(); ++i)
	{
		EmaCollection * collec = m_workflow_settings.collectionList.at(i);
		addCollectionToXMLSettings(&elemCollecs, *collec);
	}
	elemGUI.appendChild(elemCollecs);
	mSettingsDoc.appendChild(elemGUI);

	char home[1024] = "/home";
	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
	}
	// Concatenate configuration directories
	QFile fileout( QString(home) + "/" + PIAFWKFL_SETTINGS_XML );

	if( !fileout.open( QFile::WriteOnly ) ) {
		EMAMW_printf(SWLOG_ERROR, "cannot save " PIAFWKFL_SETTINGS_XML);
		return ;
	}
	QTextStream sout(&fileout);
	mSettingsDoc.save(sout, 4);

	fileout.flush();
	fileout.close();

	EMAMW_printf(SWLOG_INFO, " saved string '%s' in $HOME/" PIAFWKFL_SETTINGS_XML,
			 mSettingsDoc.toString().toAscii().data()
			 );
}

void EmaMainWindow::slot_appendNewPictureThumb(QString filename)
{
	// update progress
	appendThumbImage(filename);
}


// SWITCH BETWEEN CENTRAL DISPLAY MODES


void EmaMainWindow::on_imgButton_clicked() {
	//
	ui->stackedWidget->setCurrentIndex(0);
//	ui->toolsTabWidget->show();
	ui->actionView_left_column->setChecked(true);
	ui->actionView_right_column->setChecked(true);
}

void EmaMainWindow::on_gridButton_clicked() {
	//
	ui->stackedWidget->setCurrentIndex(1);
	//ui->toolsTabWidget->show();
	ui->actionView_left_column->setChecked(true);

}

void EmaMainWindow::on_batchPlayerButton_clicked()
{
	ui->stackedWidget->setCurrentIndex(3);
//	ui->toolsTabWidget->hide();
	ui->actionView_left_column->setChecked(false);
	ui->actionView_right_column->setChecked(false);
}


void EmaMainWindow::on_workspaceButton_clicked()
{
	ui->stackedWidget->setCurrentIndex(2);

//	ui->lateralSplitter->setChildrenCollapsible(true);
	ui->actionView_left_column->setChecked(false);
//		ui->toolsTabWidget->hide();
}
void EmaMainWindow::on_actionView_left_column_toggled(bool on)
{
	if(on)
		ui->leftWidget->show();
	else
		ui->leftWidget->hide();
}

void EmaMainWindow::on_actionView_right_column_toggled(bool on)
{
	if(on)
		ui->toolsTabWidget->show();
	else
		ui->toolsTabWidget->hide();

}

void EmaMainWindow::on_actionView_bottom_line_toggled(bool on)
{
	if(on)
		ui->bottomFrame->show();
	else
		ui->bottomFrame->hide();
}

void EmaMainWindow::on_mainDisplayWidget_signalPluginsButtonClicked()
{
	ui->toolsTabWidget->setCurrentIndex(2);
	ui->actionView_right_column->setChecked(true);
}









void EmaMainWindow::on_actionQuit_activated()
{
	saveSettings();
	close();
}

void EmaMainWindow::on_actionEdit_plugins_activated()
{
	PluginEditDialog * pluginDialog = new PluginEditDialog(NULL);
	pluginDialog->show();

	connect(pluginDialog, SIGNAL(accepted()), ui->pluginManagerForm, SLOT(slot_refreshFilters()));
}

void EmaMainWindow::on_actionEdit_sequences_triggered()
{
	SequenceSelectDialog * seqDialog = new SequenceSelectDialog(NULL);
	seqDialog->show();
}

void EmaMainWindow::on_actionBatch_processor_activated()
{
	statusBar()->showMessage( tr("Starting batch in new window") );
	BatchProgressWidget * batchProc = new BatchProgressWidget(NULL);
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
	QPixmap pix(":/icons/piaf-about.png");
	if(g_splash) {
		g_splash->setPixmap(pix);
	}
	else {
		g_splash = new QSplashScreen(pix, Qt::WindowStaysOnTopHint );
	}

	QString verstr, cmd = QString("Ctrl+");
//	QString item = QString("<li>"), itend = QString("</li>\n");
	g_splash->showMessage(
			tr("<b>Piaf workflow</b> version: ")
						  + verstr.sprintf("svn%04d%02d%02d", VERSION_YY, VERSION_MM, VERSION_DD)
						  + tr(" Website & Wiki: <a href=\"http://piaf.googlecode.com/\">http://piaf.googlecode.com/</a>")
//						  + tr("Shortcuts :<br><ul>\n")
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
//						+ QString("</ul>\n")
				, (Qt::AlignBottom | Qt::AlignHCenter)
				);
	repaint();// to force display of splash

	g_splash->show();
	g_splash->raise(); // for full screen mode
	g_splash->update();

}



void EmaMainWindow::on_zoomx1Button_clicked()
{

}

void EmaMainWindow::on_zoomx2Button_clicked()
{

}


/***************** FILE EXPLORER *********************/
void EmaMainWindow::slot_filesShowCheckBox_stateChanged(int state) {
	if(state == Qt::Checked) {
		ui->filesTreeWidget->show();
	} else {
		ui->filesTreeWidget->hide();
	}

}


void EmaMainWindow::on_filesClearButton_clicked()
{
	if(ui->filesTreeWidget->selectedItems().isEmpty()) { return; }

	QTreeWidgetItem * item = ui->filesTreeWidget->selectedItems().at(0);
	if(!item) { return; }
	//
	DirectoryTreeWidgetItem * del_dir = (DirectoryTreeWidgetItem *)item;
	// remove from list
	QList<t_folder *>::iterator it;
	for(it = m_workflow_settings.directoryList.begin();
		it != m_workflow_settings.directoryList.end(); )
	{
		t_folder * folder = (*it);
		if(folder->fullpath == del_dir->getFullPath())
		{
			it = m_workflow_settings.directoryList.erase(it);
		} else {
			++it;
		}
	}

	mDirectoryList.removeOne(del_dir);
	saveSettings();

	// remove in display
	ui->filesTreeWidget->takeTopLevelItem( ui->filesTreeWidget->indexOfTopLevelItem(item) );
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
	saveSettings();

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
	fprintf(stderr, "EmaMW::%s:%d : expanded %p !\n", __func__, __LINE__, item);
	if(!item) return;

	fprintf(stderr, "EmaMW::%s:%d : expanded %p !\n", __func__, __LINE__, item);
	DirectoryTreeWidgetItem * curdir = (DirectoryTreeWidgetItem*)item;

	// add sub items
	fprintf(stderr, "EmaMW::%s:%d : EXPAND THIS TREE '%s' !\n",
					__func__, __LINE__,
					curdir->mFullPath.toAscii().data());

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

	fileList.append(directoryFileScan(mFullPath));

	return fileList;
}

void DirectoryTreeWidgetItem::expand()
{
	EMAMW_printf(SWLOG_DEBUG, "Expanding directory '%s'",
				 mFullPath.toAscii().data()
				 );
	QDir dir(mFullPath);
	QFileInfoList fiList = dir.entryInfoList();
	QFileInfoList::iterator it;
	for(it = fiList.begin(); it != fiList.end(); ++it)
	{
		QFileInfo fi = (*it);
		if(fi.isFile() && fi.absoluteFilePath().length() > mFullPath.length())
		{
			EMAMW_printf(EMALOG_TRACE, "\tadding FILE '%s'\n",
					fi.absoluteFilePath().toAscii().data());


			// Add item for file
			t_file * new_file = new t_file;
			new_file->name = fi.baseName();
			new_file->fullPath = fi.absoluteFilePath();

			//QStringList columns; columns << new_file->name;

			new_file->treeViewItem = new DirectoryTreeWidgetItem(
					this,
					new_file->fullPath);
		}
		else if(fi.isDir() && fi.absoluteFilePath().length() > mFullPath.length())
		{
			EMAMW_printf(EMALOG_TRACE, "adding subdir '%s'\n",
					fi.absoluteFilePath().toAscii().data());

			mSubDirsList.append(
					new DirectoryTreeWidgetItem(this,
							fi.absoluteFilePath())
					);
		}
	}

	// delete fake "waiting" item
	if(mFakeSubItem) {
		removeChild( mFakeSubItem );
		delete mFakeSubItem;
		mFakeSubItem = NULL;
	}

}
EmaTreeWidgetItem::EmaTreeWidgetItem(QTreeWidget * treeWidgetParent, t_treeitem_category cat)
	: QTreeWidgetItem(treeWidgetParent)
{
	mCategory = cat;
}

EmaTreeWidgetItem::EmaTreeWidgetItem(QTreeWidgetItem * treeWidgetItemParent, t_treeitem_category cat)
	: QTreeWidgetItem(treeWidgetItemParent)
{
	mCategory = cat;
}



DirectoryTreeWidgetItem::DirectoryTreeWidgetItem(QTreeWidgetItem * treeWidgetItemParent, QString path)
	: QTreeWidgetItem(treeWidgetItemParent),
	mFullPath(path), mNbFiles(0)
{
	init();
}

DirectoryTreeWidgetItem::DirectoryTreeWidgetItem(QTreeWidget * treeWidgetParent, QString path)
	: QTreeWidgetItem(treeWidgetParent),
	mFullPath(path), mNbFiles(0)
{
	init();
}

DirectoryTreeWidgetItem::~DirectoryTreeWidgetItem()
{

}

void DirectoryTreeWidgetItem::init()
{
	QFileInfo fi(mFullPath);
	mName = fi.baseName();
	setText(0, fi.completeBaseName());
	mFakeSubItem = NULL;
	mIsFile = false;

	if(fi.isDir()) {
		mNbFiles = 0;
		scanDirectory(mFullPath, &mNbFiles);

		setText(1, QString::number(mNbFiles));

		if(mNbFiles > 0) {
			// Add fake sub item "waiting"
			QStringList columns;
			columns.clear();
			columns << "...";

			mFakeSubItem = new QTreeWidgetItem(this, columns);
		}
	}
	else if(fi.isFile())
	{
		mIsFile = true;

		setText(0, fi.baseName());
		setText(1, fi.suffix());
	}
}

DirectoryTreeWidgetItem * EmaMainWindow::appendDirectoryToLibrary(QString path,
																  QTreeWidgetItem * itemParent)
{
	QFileInfo fi(path);
	if(!fi.exists()) { return NULL; }

	t_folder * folder = new t_folder;
	folder->fullpath = path;
	folder->extension = fi.suffix();
	folder->filename = fi.baseName();
	folder->expanded = false;
	m_workflow_settings.directoryList.append(folder);

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
	ui->stopLoadButton->hide();

	// load in manager
	emaMngr()->appendFileList(list);

	// start timer to update while processing
	if(!m_timer.isActive()) {
		m_timer.start(500);
	}
}


void EmaMainWindow::slot_timer_timeout() {

	int val = emaMngr()->getProgress();
	ui->stopLoadButton->show();
	EMAMW_printf(EMALOG_TRACE, "Timeout => progress = %d", val)

	// update known files : appended files

	QString fileName;
	int nb = m_appendFileList.count();
	EMAMW_printf(EMALOG_TRACE, "Test if we can append %d files...", nb)

	QStringList::Iterator it = m_appendFileList.begin();
	while( !m_appendFileList.isEmpty() &&
			(it != m_appendFileList.end()) )
	{
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


	// update progress bar
	ui->loadProgressBar->setValue(emaMngr()->getProgress());

	if(!m_appendFileList.isEmpty()) {
		// next update in 500 ms
		m_timer.start(500);
	}

	if(emaMngr()->getProgress() == 100)
	{
		ui->loadProgressBar->hide();
		ui->stopLoadButton->hide();
		m_timer.stop();
	}

}
void EmaMainWindow::on_stopLoadButton_clicked()
{
	m_appendFileList.clear();
	// do not hide, let timer stop and hide
	emaMngr()->clearImportList();
}


void EmaMainWindow::slot_gridWidget_signal_resizeEvent(QResizeEvent * e)
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
	slot_thumbImage_selected( fileName );
}

void EmaMainWindow::on_filesTreeWidget_itemDoubleClicked (
		QTreeWidgetItem * item, int /*unused column */)
{
	if(!item) return;

	DirectoryTreeWidgetItem * curdir = (DirectoryTreeWidgetItem *)item;
	// read image file
	if(curdir->isFile()) {
		QString fileName = curdir->mFullPath; // col 0 has the full path
		slot_thumbImage_clicked( fileName );

		// display main widget
		ui->stackedWidget->setCurrentIndex(0);
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
		mGridThumbImageFrameList.append(newThumb);

		ui->scrollAreaWidgetContents->layout()->addWidget(newThumb);
//		newThumb->setImageFile(fileName, pinfo->thumbImage.iplImage);
		newThumb->setImageInfoStruct(pinfo);

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
//		newThumb2->setImageFile(fileName, pinfo->thumbImage.iplImage,
//								pinfo->score);
		mBottomThumbImageFrameList.append(newThumb2);
		newThumb2->setImageInfoStruct(pinfo);

		// Cross-connection
		newThumb2->setTwin(newThumb);
		newThumb->setTwin(newThumb2);

		// now delete thumb ?
		/// \todo FIXME : check if there is no side effect of this deletion
		tmReleaseImage(&pinfo->thumbImage.iplImage);

		QGridLayout * grid_layout = (QGridLayout *)ui->gridWidget->layout();
		grid_layout->addWidget( newThumb2, row, col );

		// connect this signal
		connect(newThumb, SIGNAL(signalThumbDoubleClicked(QString)), this, SLOT(slot_thumbImage_doubleClicked(QString)));
		connect(newThumb, SIGNAL(signalThumbClicked(QString)), this, SLOT(slot_thumbImage_clicked(QString)));

		connect(newThumb, SIGNAL(signalThumbSelected(QString)), this, SLOT(slot_thumbImage_selected(QString)));

		connect(newThumb, SIGNAL(signal_click(ThumbImageFrame *)), this, SLOT(slot_thumbImage_signal_click(ThumbImageFrame *)));
		connect(newThumb, SIGNAL(signal_shiftClick(ThumbImageFrame *)), this, SLOT(slot_thumbImage_signal_shiftClick(ThumbImageFrame *)));
		connect(newThumb, SIGNAL(signal_ctrlClick(ThumbImageFrame *)), this, SLOT(slot_thumbImage_signal_ctrlClick(ThumbImageFrame *)));

		if(newThumb2) {
			connect(newThumb2, SIGNAL(signalThumbDoubleClicked(QString)), this, SLOT(slot_thumbImage_doubleClicked(QString)));
			connect(newThumb2, SIGNAL(signalThumbClicked(QString)), this, SLOT(slot_thumbImage_clicked(QString)));

			connect(newThumb2, SIGNAL(signalThumbSelected(QString)), this, SLOT(slot_thumbImage_selected(QString)));

			connect(newThumb2, SIGNAL(signal_click(ThumbImageFrame *)), this, SLOT(slot_thumbImage_signal_click(ThumbImageFrame *)));
			connect(newThumb2, SIGNAL(signal_shiftClick(ThumbImageFrame *)), this, SLOT(slot_thumbImage_signal_shiftClick(ThumbImageFrame *)));
			connect(newThumb2, SIGNAL(signal_ctrlClick(ThumbImageFrame *)), this, SLOT(slot_thumbImage_signal_ctrlClick(ThumbImageFrame *)));
		}

	}

}

void EmaMainWindow::slot_mainGroupBox_resizeEvent(QResizeEvent * e) {
	if(!e) { return ; }
	
	EMAMW_printf(EMALOG_DEBUG, "Resize event : %dx%d\n",
				 e->size().width(), e->size().height())
}



void EmaMainWindow::slot_thumbImage_selected(QString fileName)
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

void EmaMainWindow::slot_thumbImage_signal_click(ThumbImageFrame * thumb)
{
	EMAMW_printf(EMALOG_INFO, "simple-click on %p = '%s'", thumb,
				 thumb ? thumb->getFilename().toAscii().data():"null");
	mSelectedThumbImageFrame = thumb;

	if(mGridThumbImageFrameList.contains(thumb))
	{
		// use pointer on bottom line
		if(thumb->getTwin())
		{
			mSelectedThumbImageFrame = thumb->getTwin();
		}
	}

}


void EmaMainWindow::slot_thumbImage_clicked(QString fileName)
{
	QFileInfo fi(fileName);
	if(!fi.exists()) {
		EMAMW_printf(SWLOG_ERROR, "File '%s' does not exists",
				 fileName.toAscii().data());
		return;
	}

	setCurrentMainFile(fileName);
}

void EmaMainWindow::slot_thumbImage_doubleClicked(QString fileName)
{
	EMAMW_printf(EMALOG_INFO, "double-click on '%s'", fileName.toUtf8().data());
	QFileInfo fi(fileName);
	if(!fi.exists()) {
		EMAMW_printf(SWLOG_ERROR, "File '%s' does not exists",
				 fileName.toAscii().data());
		return;
	}

	setCurrentMainFile(fileName);

	ui->stackedWidget->setCurrentIndex(0);
}

void EmaMainWindow::slot_thumbImage_signal_shiftClick(ThumbImageFrame * thumb)
{
	EMAMW_printf(SWLOG_INFO, "Selected thumb %p with Shift", thumb);
	if(mGridThumbImageFrameList.contains(thumb))
	{
		EMAMW_printf(SWLOG_INFO, "Selected thumb %p from grid with Shift", thumb);
		// consider its twin in bottom line
		if(thumb->getTwin())
		{
			thumb = thumb->getTwin();
		}
	}

	if(mBottomThumbImageFrameList.contains(thumb))
	{
		EMAMW_printf(SWLOG_INFO, "Selected thumb %p from bottom with Shift", thumb);
		if(mSelectedThumbImageFrame)
		{
			bool activated = false;
			QList<ThumbImageFrame *>::iterator it;
			for(it = mBottomThumbImageFrameList.begin();
				it !=mBottomThumbImageFrameList.end();
				++it)
			{
				ThumbImageFrame * curIt = (*it);
				if(activated)
				{
					curIt->setSelected(mSelectedThumbImageFrame->isSelected());
					if(curIt == thumb || curIt == mSelectedThumbImageFrame)
					{
						EMAMW_printf(SWLOG_INFO, "thumb %p => disactivated", curIt);

						activated = false;
					}
				}
				else
				{
					if(curIt == thumb || curIt == mSelectedThumbImageFrame)
					{
						EMAMW_printf(SWLOG_INFO, "thumb %p => activated", curIt);

						activated = true;
					}

				}

			}
		}
	}
}

void EmaMainWindow::slot_thumbImage_signal_ctrlClick(ThumbImageFrame * thumb)
{
	EMAMW_printf(SWLOG_INFO, "Selected thumb %p with Ctrl", thumb);

}













void EmaMainWindow::setCurrentMainFile(QString fileName)
{
	mMainFileName = fileName;
	QList<ThumbImageFrame *>::iterator it;
	for(it = mBottomThumbImageFrameList.begin();
		it !=mBottomThumbImageFrameList.end();
		++it)
	{
		ThumbImageFrame * frame = (*it);
		if( frame )
		{
			frame->setActive( frame->getFilename() == fileName);
			// twin is toggled automatically
		}
	}

	ui->globalNavImageWidget->setImageFile(fileName);
	t_image_info_struct * pinfo = NULL;

	int retry = 0;

	while(!pinfo && retry<4E6 // wait for 4 s
		  ) {
		pinfo = emaMngr()->getInfo(fileName);
		if(!pinfo) {
			// Create its info
			emaMngr()->appendFile(fileName);
			usleep(100000);
		}
		retry += 100000;
	}

	// print allocated images
	tmPrintIplImages();

	if(pinfo) {

		if(!pinfo->isMovie)
		{
			ui->mainDisplayWidget->setImageFile(fileName, pinfo);
			EMAMW_printf(SWLOG_INFO, "picture image=%dx%dx%d",
					 ui->mainDisplayWidget->getImage().width(),
					 ui->mainDisplayWidget->getImage().height(),
					 ui->mainDisplayWidget->getImage().depth());
			/*
			  FIXME : this works !
			  Enjoy

			MainImageWidget * newDisplay = new MainImageWidget(this);
			ui->mdiArea->addSubWindow(newDisplay);
			QImage imageIn(fileName);
			newDisplay->setImage(imageIn, pinfo);
			newDisplay->show();
			*/
		}
		else // use main widget decoder to read the image
		{
			ui->mainDisplayWidget->setMovieFile(fileName, pinfo);

			EMAMW_printf(SWLOG_INFO, "movie image=%dx%dx%d",
					 ui->mainDisplayWidget->getImage().width(),
					 ui->mainDisplayWidget->getImage().height(),
					 ui->mainDisplayWidget->getImage().depth());
			ui->globalNavImageWidget->setImage( ui->mainDisplayWidget->getImage() );
/*
			MainDisplayWidget * newDisplay = new MainDisplayWidget(this);
			ui->mdiArea->addSubWindow(newDisplay);
			newDisplay->setMovieFile(fileName, pinfo);
			newDisplay->show();
			*/
			//
//			ui->globalNavImageWidget->setImage( iplImageToQImage(pinfo->thumbImage.iplImage) );
		}
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

/* Set the file to be added in workspace */
void EmaMainWindow::openInWorkspace(QString fileName)
{
	if(fileName.isNull())
	{
		return;
	}
	t_image_info_struct * pinfo = NULL;

	int retry = 0;
	while(!pinfo && retry<4E6 // wait for 4 s
		  ) {
		pinfo = emaMngr()->getInfo(fileName);
		if(!pinfo) {
			// Create its info
			emaMngr()->appendFile(fileName);
			usleep(100000);
		}
		retry += 100000;
	}

	MainDisplayWidget * newDisplay = new MainDisplayWidget(this);
	if(pinfo)
	{
		if(!pinfo->isMovie)
		{
			newDisplay->setImageFile(fileName, pinfo);
		}
		else // use main widget decoder to read the image
		{
			newDisplay->setMovieFile(fileName, pinfo);
		}
	}
	else
	{
		QImage testImage(fileName);
		if(testImage.isNull())
		{
			newDisplay->setMovieFile(fileName, pinfo);
		}
		else
		{
			newDisplay->setImageFile(fileName, pinfo);
		}
	}

	ui->mdiArea->addSubWindow(newDisplay);
	newDisplay->show();
}



void EmaMainWindow::on_globalNavImageWidget_signalZoomOn(int x, int y, float scale) {
	ui->mainDisplayWidget->zoomOn(x,y,scale);
	ui->stackedWidget->setCurrentIndex(0);
}






void EmaMainWindow::on_actionClean_activated()
{
	ui->pluginManagerForm->cleanAllPlugins();
}

void EmaMainWindow::on_actionPreferences_activated()
{
	PreferencesDialog * newPrefDialog = new PreferencesDialog(NULL);
	/// FIXME
	newPrefDialog->setSettings(&m_workflow_settings);
	newPrefDialog->setAttribute( Qt::WA_DeleteOnClose );

	connect(newPrefDialog, SIGNAL(destroyed()), this, SLOT(saveSettings()));
	newPrefDialog->show();
}

void EmaMainWindow::on_actionOpen_file_in_workspace_triggered()
{
	openInWorkspace(mMainFileName);
	on_workspaceButton_clicked();
}



/******************************************************************************

				COLLECTIONS

  *****************************************************************************/

void EmaMainWindow::on_collecAddButton_clicked()
{
	if(mpCurrentCollection)
	{
		int answer = QMessageBox::question(NULL, tr("Add as sub-collection ?"),
										   tr("You have selected a collection. "
											  "Do you want to add a new subcollection ? "
											  "If you say No, it will be added as a new collection.)"
										   ),
										   QMessageBox::Yes, QMessageBox::No );
		if(answer == QMessageBox::No)
		{
			ui->collecTreeWidget->clearSelection();
			mpCurrentCollection = NULL;
		}
	}

	CollectionEditDialog * newCollDialog = new CollectionEditDialog();
	connect(newCollDialog, SIGNAL(signalNewCollection(EmaCollection)),
			this, SLOT(slot_newCollDialog_signalNewCollection(EmaCollection)));

	newCollDialog->show();
}

void EmaMainWindow::slot_newCollDialog_signalNewCollection(EmaCollection collec)
{
	EMAMW_printf(SWLOG_INFO, "Adding new collection '%s'", collec.title.toAscii().data());

	EmaCollection * new_collec = new EmaCollection(collec);
	m_workflow_settings.collectionList.append(new_collec);
	if(mpCurrentCollection )
	{
		EMAMW_printf(SWLOG_INFO, "\tAdding '%s' AS SUB-collection OF '%s'",
					 collec.title.toAscii().data(),
					 mpCurrentCollection->title.toAscii().data()
					 );

		new CollecTreeWidgetItem(mpCurrentCollection->treeViewItem, new_collec);
	}
	else
	{
		EMAMW_printf(SWLOG_INFO, "\tAdding '%s' AS new top-level collection",
					 collec.title.toAscii().data());
		new CollecTreeWidgetItem(ui->collecTreeWidget, new_collec);
	}
	saveSettings();
}


void EmaMainWindow::on_collecDeleteButton_clicked()
{
	EmaCollection * selectedCollection = getSelectedCollection();

	if(!selectedCollection)
	{
		QMessageBox::warning(NULL, tr("No collection selected"),
							 tr("No collection is selected: please select a collection before removing it."));
		return;
	}
	if(selectedCollection==mpQuickCollection)
	{
		QMessageBox::warning(NULL, tr("Can't delete the quick collection"),
							 tr("You can't delete the quick collection. "));
		return;
	}

	if(QMessageBox::question(NULL, tr("Remove collection ")+selectedCollection->title+tr("?"),
							 tr("Do you really want to delete collection ")+selectedCollection->title+tr("?")
							 ) == QMessageBox::Yes)
	{
		removeCollection(&m_workflow_settings.collectionList, selectedCollection);

		saveSettings();
	}

}

void EmaMainWindow::on_collecEditButton_clicked()
{
	if(ui->collecTreeWidget->selectedItems().isEmpty()) return;

	// Edit the selected one
	CollecTreeWidgetItem *pCollecItem = (CollecTreeWidgetItem *)ui->collecTreeWidget->selectedItems().at(0);
	if(!pCollecItem) return;

	CollectionEditDialog * newCollDialog = new CollectionEditDialog();
	connect(newCollDialog, SIGNAL(signalChangedCollection(EmaCollection *)),
			this, SLOT(slot_newCollDialog_signalCollectionChanged(EmaCollection *)));
	newCollDialog->setCollection(pCollecItem->getCollection());
	newCollDialog->show();
}

void EmaMainWindow::slot_newCollDialog_signalCollectionChanged(EmaCollection * pcollec)
{
	if(!pcollec) { return; }

	/// \todo Update display
	//CollecTreeWidgetItem *pCollecItem = (CollecTreeWidgetItem *)ui->collecTreeWidget->selectedItems().at(0);
	QTreeWidgetItemIterator it(ui->collecTreeWidget);
	while (*it) {
		CollecTreeWidgetItem *pCollecItem = (CollecTreeWidgetItem *)(*it);
		if(pCollecItem && pCollecItem->getCollection() == pcollec)
		{
			pCollecItem->updateDisplay();
		}
		++it;
	}
}







/*
QStringList CollecTreeWidgetItem::getFileList()
{
	QStringList fileList;

	fileList.append(directoryFileScan(fullPath));

	return fileList;
}

void CollecTreeWidgetItem::expand()
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

			//QStringList columns; columns << new_file->name;

			new_file->treeViewItem = new DirectoryTreeWidgetItem(
					this,
					new_file->fullPath);
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
*/

CollecTreeWidgetItem::CollecTreeWidgetItem(QTreeWidgetItem * treeWidgetItemParent,
										   EmaCollection * pcollec
										   )
	: QTreeWidgetItem(treeWidgetItemParent),
	mpCollec(pcollec)
{
	init();
}

CollecTreeWidgetItem::CollecTreeWidgetItem(QTreeWidget * treeWidgetParent,
										   EmaCollection * pcollec )
	: QTreeWidgetItem(treeWidgetParent),
	  mpCollec(pcollec)
{
	init();
}

CollecTreeWidgetItem::~CollecTreeWidgetItem()
{
	purge();
}

void EmaMainWindow::on_collecTreeWidget_itemExpanded(QTreeWidgetItem* item)
{
	fprintf(stderr, "EmaMW::%s:%d : expanded %p !\n", __func__, __LINE__, item);
	if(!item) return;

	fprintf(stderr, "EmaMW::%s:%d : expanded %p !\n", __func__, __LINE__, item);
	CollecTreeWidgetItem * curdir = (CollecTreeWidgetItem*)item;
	// add sub items
	fprintf(stderr, "EmaMW::%s:%d : EXPAND THIS TREE '%s' !\n",
					__func__, __LINE__,
					curdir->mpCollec->title.toAscii().data());

	item->setExpanded(true);
	curdir->expand();
}

void CollecTreeWidgetItem::expand()
{
	EMAMW_printf(SWLOG_INFO, "Expanding collection %p='%s'",
				 mpCollec, mpCollec ? mpCollec->title.toAscii().data() : "null"
				 );

	updateDisplay();
}

// display information about collection
void CollecTreeWidgetItem::updateDisplay()
{
	if(!mpCollec)
	{
		EMAMW_printf(SWLOG_WARNING, "this=%p has no collec",
					 this
					 );
		return;
	}

	EMAMW_printf(SWLOG_INFO, "update collection %p='%s' / %d files / %d subcolls",
				 mpCollec, mpCollec ? mpCollec->title.toAscii().data() : "null",
				 mpCollec ? mpCollec->filesList.count() : -1,
				 mpCollec ? mpCollec->subCollectionsList.count() : -1
				 );
	if(mpCollec) {

		setText(0, mpCollec->title);
		setText(1,
				QString::number(mpCollec->filesList.count()) + QObject::tr(" files, ")
				+ QString::number(mpCollec->subCollectionsList.count()) + QObject::tr(" sub-coll.")
				);
		if(isExpanded())
		{
			if(subItem && (!mpCollec->filesList.isEmpty()
						   || !mpCollec->subCollectionsList.isEmpty()))
			{
				// remove fake item
				removeChild(subItem);
				subItem = NULL;
			}

			// update sub items
//			while(childCount() > 0)
//			{
//				removeChild(child(0));
//			}

			// Add items for files
			QList<t_collection_file *>::iterator fit;
			for(fit = mpCollec->filesList.begin();
				fit != mpCollec->filesList.end(); ++fit)
			{
				t_collection_file * pfile = (*fit);
				EMAMW_printf(SWLOG_DEBUG, "Create item for file '%s'",
							 pfile->filename.toAscii().data());

				if(!pfile->treeViewItem) {
					// create one item per child
					DirectoryTreeWidgetItem * item = new DirectoryTreeWidgetItem(this, pfile->fullpath);
//					item->setText(0, pfile->filename);
//					item->setToolTip(0, pfile->fullpath);
					pfile->treeViewItem = (QTreeWidgetItem *) item;
				}
			}
		}
		else
		{
			EMAMW_printf(SWLOG_WARNING, "'%s' not expanded",
						 mpCollec->title.toAscii().data()
						 );
		}

		// Add items for collections
		QList<EmaCollection *>::iterator cit;
		for(cit = mpCollec->subCollectionsList.begin();
			cit != mpCollec->subCollectionsList.end();
			++cit)
		{
			EmaCollection * subcol = (*cit);
			EMAMW_printf(SWLOG_DEBUG, "Create item for collection '%s'",
						 subcol->title.toAscii().data());
			if(!subcol->treeViewItem)
			{
				subcol->treeViewItem = (QTreeWidgetItem *)new CollecTreeWidgetItem(
							this, subcol);
			}
		}
	}


}

void CollecTreeWidgetItem::init()
{
	if(mpCollec) {
		setText(0, mpCollec->title);
		mpCollec->treeViewItem = (QTreeWidgetItem *)this;
	}
	if(subCollecsList.isEmpty())
	{
		QStringList columns;
		columns << QObject::tr("(expanding...)");
		subItem = new QTreeWidgetItem(this, columns );
	}
	else
	{
		subItem = NULL;
	}
	mIsFile = false;

	// the creation of sub -items is done in updateDisplay()
	updateDisplay();


/*	if(fi.isDir()) {
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

		setText(0, fi.baseName());
		setText(1, fi.extension());
	}*/
}

void CollecTreeWidgetItem::purge()
{

}





/******************************************************************************
			VIDEO DEVICE ACQUISITION
*******************************************************************************/
CaptureTreeWidgetItem::CaptureTreeWidgetItem(QTreeWidgetItem * treeWidgetItemParent,
											 QString devname,
											 VideoCaptureDoc * pVideoCaptureDoc)
	: QTreeWidgetItem(treeWidgetItemParent), mName(devname), m_pVideoCaptureDoc(pVideoCaptureDoc)
{
	setText(0, mName);

}

/// Constructor for devices classes
CaptureTreeWidgetItem::CaptureTreeWidgetItem(QTreeWidget * treeWidgetParent,
											 QString category)
	: QTreeWidgetItem(treeWidgetParent), mName(category)
{
	m_pVideoCaptureDoc = NULL;
	setText(0, mName);
}

CaptureTreeWidgetItem::~CaptureTreeWidgetItem()
{
	purge();
}

void CaptureTreeWidgetItem::purge()
{
	if(m_pVideoCaptureDoc)
	{
		/// FIXME : broadcast signal to stop capturing
		delete m_pVideoCaptureDoc;
		m_pVideoCaptureDoc = NULL;
	}
}






void EmaMainWindow::on_deviceRefreshButton_clicked()
{
	// list all video devices from :
	// V4L2: /proc/video/dev/video*
	// OpenCV: V4L2, GigE, FireWire...
	// Freenect: Microsoft Kinect
	// OpenNI: Microsoft Kinect and Asus Xtion Pro & Xtion Pro Live

	char txt[512]="", name[256]="";

	bool found = false;
	int dev=0, failed =0;

	// Find available items
	// ================================== V4L(2) ===============================
#ifdef _LINUX // for Linux, use the V4L /sys or /proc virtual file system
	if(!mV4l2Item)
	{
		mV4l2Item = new CaptureTreeWidgetItem(ui->deviceTreeWidget, tr("V4L2"));
		mV4l2Item->setIcon(0, QIcon(":/icons/16x16/camera-web.png"));
	}

	fprintf(stderr, "[Workshop]::%s:%d : scanning V4L2 devices in /sys/class/video4linux/ ...\n", __func__, __LINE__);
	int dev_idx = 0;
	do {
		found = false;

		sprintf(txt, "/sys/class/video4linux/video%d/name", dev_idx);
		bool kern2_6 = true;
		FILE * fv = fopen(txt, "r");
		if(!fv) {
			sprintf(txt, "/proc/video/dev/video%d", dev_idx);
			fv = fopen(txt, "r");
			kern2_6 = false;
		}
		char *ptname = name;

		// READ DEVICE NAME
		if(fv) {
			fgets(name, 127, fv);
			fprintf(stderr, "Workshop::%s:%d : opened dev '%s' => name='%s'\n",
					__func__, __LINE__, txt, name);
			found = true;
			fclose(fv);

			if(strlen(name)>1 && name[strlen(name) -1]=='\n') {
				name[strlen(name) -1]='\0';

				// split for seeking name
				if(!kern2_6) {
					for(ptname = strstr(name, ":")+1; (*ptname == ' ' || *ptname == '\t') && (ptname < (name+127)); ptname++) {}
				} else {
					ptname = name;
				}
			}

			sprintf(txt, "%d - %s", dev_idx, ptname);
		} else {
			sprintf(name, "Device # %d", dev_idx);
			sprintf(txt, "%d - %s", dev_idx, ptname);
		}

		fprintf(stderr, "[Workshop]::%s:%d : scanning V4L2 devices...\n", __func__, __LINE__);
		V4L2Device * myVAcq = new V4L2Device(dev_idx);

		if(!myVAcq->isDeviceReady())
		{
			fprintf(stderr, "Workshop::%s:%d : V4L2 acquisition dev '%s' not ready\n",
					__func__, __LINE__, txt);
			failed++;
			statusBar()->showMessage(tr("V4L2 : Video device init failed for ")+ QString(txt));
		}
		else {
//			QTreeWidgetItem * subv4l2Item = new QTreeWidgetItem(v4l2Item);
//			subv4l2Item->setText(0, txt);

			statusBar()->showMessage(tr("V4L2 : Video device Init OK for ")+ QString(txt));

			found = true;

			fprintf(stderr, "Workshop::%s:%d : starting acquisition on dev '%s' ...\n",
					__func__, __LINE__, txt);
			// acquisition init
			if(myVAcq->startAcquisition()<0)
			{
				fprintf(stderr, "Workshop::%s:%d : could not start acquisition on dev '%s'.\n",
						__func__, __LINE__, txt);

				statusBar()->showMessage(tr("Error: canot initialize acquisition for ")+ QString(txt));
				delete myVAcq;
			}
			else
			{
				fprintf(stderr, "Workshop::%s:%d : acquisition started on dev '%s' ...\n",
						__func__, __LINE__, txt);

				if(myVAcq->isAcquisitionRunning())
				{
					myVAcq->stopAcquisition();

					fprintf(stderr, "Workshop::%s:%d : acquisition running on dev '%s' ...\n",
							__func__, __LINE__, txt);

					statusBar()->showMessage(tr("Initialization OK for ")+ QString(txt));

					// Check if it's already created
					if( ui->deviceTreeWidget->findItems(QString(txt), Qt::MatchExactly).isEmpty() )
					{
						VideoCaptureDoc * pVCD = new VideoCaptureDoc(myVAcq);

						CaptureTreeWidgetItem * item = new CaptureTreeWidgetItem(mV4l2Item, txt, pVCD);
						item->setIcon(0, QIcon(":/icons/16x16/camera-web.png"));
					}
				}
				else {
					statusBar()->showMessage(tr("Initialization FAILURE for ")+ QString(txt));
					delete myVAcq;
				}
			}
		}

		dev_idx++;

	} while( dev_idx<m_workflow_settings.maxV4L2 );

	mV4l2Item->setExpanded(true);

#endif // Linux


	// ================================== OPENCV ===============================
	if(!mOpenCVItem)
	{
		mOpenCVItem = new CaptureTreeWidgetItem(ui->deviceTreeWidget, tr("OpenCV"));
		mOpenCVItem->setIcon(0, QIcon(":/icons/22x22/opencv.png"));
	}

	fprintf(stderr, "[Workshop]::%s:%d : scanning OpenCV devices ...\n", __func__, __LINE__);
	statusBar()->showMessage(tr("Scanning OpenCV devices ... please wait"));
	int opencv_idx = 0;
	do {
		found = false;

		fprintf(stderr, "[Workshop]::%s:%d : scanning OpenCV device [%d]...\n", __func__, __LINE__, opencv_idx);
		OpenCVVideoAcquisition * myVAcq = new OpenCVVideoAcquisition(opencv_idx);
		sprintf(txt, "OpenCV [%d]", opencv_idx);

		if(!myVAcq->isDeviceReady())
		{
			fprintf(stderr, "Workshop::%s:%d : OpenCV acquisition dev '%s' not ready\n",
					__func__, __LINE__, txt);
			failed++;
			statusBar()->showMessage(tr("OpenCV: Video device init failed on ") + QString(txt));
		}
		else {
			statusBar()->showMessage(tr("OpenCV: Video device Init OK on ") + QString(txt));

			found = true;

			fprintf(stderr, "Workshop::%s:%d : starting acquisition on dev '%s' ...\n",
					__func__, __LINE__, txt);
			// acquisition init
			if(myVAcq->startAcquisition()<0)
			{
				fprintf(stderr, "Workshop::%s:%d : could not start acquisition on dev '%s'.\n",
						__func__, __LINE__, txt);

				statusBar()->showMessage(tr("Error: cannot initialize acquisition on ") + QString(txt));
				delete myVAcq;
			}
			else
			{
				fprintf(stderr, "Workshop::%s:%d : acquisition started on dev '%s' ...\n",
						__func__, __LINE__, txt);
				if(myVAcq->isAcquisitionRunning())
				{
					fprintf(stderr, "Workshop::%s:%d : acquisition running on dev '%s' ...\n",
							__func__, __LINE__, txt);
					statusBar()->showMessage(tr("Initialization on ") + QString(txt));
					myVAcq->stopAcquisition();

					// Check if it's already created
					if( ui->deviceTreeWidget->findItems(QString(txt), Qt::MatchExactly).isEmpty() )
					{
						VideoCaptureDoc * pVCD = new VideoCaptureDoc(myVAcq);

						CaptureTreeWidgetItem * item = new CaptureTreeWidgetItem(mOpenCVItem, txt, pVCD);
						item->setIcon(0, QIcon(":/icons/22x22/opencv.png"));
					}

		//FIXME			pObjectsExplorer->addVideoAcquisition(pVCD, txt);
				}
				else {
					statusBar()->showMessage(tr("Initialization FAILURE !"));
					delete myVAcq;
				}
			}
		}

		opencv_idx++;

	} while(opencv_idx < m_workflow_settings.maxOpenCV);

	mOpenCVItem->setExpanded(true);





	// check if there are Kinects connected
#ifdef HAS_FREENECT
	fprintf(stderr, "[Workshop]::%s:%d : scanning Freenect devices...\n", __func__, __LINE__);
	if(!mFreenectItem)
	{
		mFreenectItem = new CaptureTreeWidgetItem(ui->deviceTreeWidget, tr("Freenect"));
		mFreenectItem->setIcon(0, QIcon(":/icons/22x22/kinect.png"));
	}

	int freenect_idx = 0;
	bool freenect_found;
	do {
		freenect_found = false;

		statusBar()->showMessage(tr("Creating new Freenect Video Acquisition...") + tr("try Kinect"));
		FreenectVideoAcquisition * freenectDevice = new FreenectVideoAcquisition(freenect_idx);

		if(freenectDevice->isDeviceReady())
		{
			freenect_found = true;

			// append to Piaf
			statusBar()->showMessage(tr("(VideoAcquisition) : Freenect device Init OK"));

			sprintf(txt, "Kinect[%d]", freenect_idx);

			// acquisition init
			if(freenectDevice->startAcquisition()<0)
			{
				statusBar()->showMessage(tr("Error: cannot initialize acquisition !"));
				freenect_found = false;
			}
			else
			{

				if(freenectDevice->isAcquisitionRunning())
				{
					freenectDevice->stopAcquisition();

					statusBar()->showMessage(tr("Initialization OK"));
					if(ui->deviceTreeWidget->findItems(QString(txt), Qt::MatchExactly).isEmpty())
					{

						VideoCaptureDoc * pVCD = new VideoCaptureDoc(freenectDevice);
						CaptureTreeWidgetItem * item = new CaptureTreeWidgetItem(mFreenectItem, txt, pVCD);
						item->setIcon(0, QIcon(":/icons/22x22/kinect.png"));
					}

					// add into explorer
		//FIXME			pObjectsExplorer->addVideoAcquisition(pVCD, txt);
				}
				else {
					statusBar()->showMessage(tr("Initialization FAILURE !"));
					delete freenectDevice;
				}
			}

		} else {
			delete freenectDevice;
		}

		freenect_idx++;

	} while(freenect_found && freenect_idx<m_workflow_settings.maxFreenect);
	mFreenectItem->setExpanded(true);

#endif // HAS_FREENECT


	// check if there are OpenNI supported devices are connected
#ifdef HAS_OPENNI
	if(!mOpenNIItem)
	{
		mOpenNIItem = new CaptureTreeWidgetItem(ui->deviceTreeWidget, tr("OpenNI"));
		mOpenNIItem->setIcon(0, QIcon(":/icons/22x22/openni.png"));
	}

	fprintf(stderr, "[Workshop]::%s:%d : scanning OpenNI devices...\n", __func__, __LINE__);
	int openni_idx = 0;
	bool openni_found;
	do {
		openni_found = false;

		statusBar()->showMessage(tr("Creating new OpenNI Video Acquisition...") + tr("try OpenNI"));
		OpenNIVideoAcquisition * openniDevice = new OpenNIVideoAcquisition(openni_idx);

		if(openniDevice->isDeviceReady())
		{
			openni_found = true;

			// append to Piaf
			statusBar()->showMessage(tr("(VideoAcquisition) : openni device Init OK"));

			sprintf(txt, "OpenNI[%d]", openni_idx);

			// acquisition init
			if(openniDevice->startAcquisition()<0)
			{
				statusBar()->showMessage(tr("Error: cannot initialize acquisition !"));
				openni_found = false;
			}
			else
			{
				openni_idx++;

				if(openniDevice->isAcquisitionRunning())
				{
					//openniDevice->stopAcquisition();

					statusBar()->showMessage(tr("Initialization OK"));
					// add into explorer
					if(ui->deviceTreeWidget->findItems(QString(txt), Qt::MatchExactly).isEmpty())
					{
						VideoCaptureDoc * pVCD = new VideoCaptureDoc(openniDevice);
						CaptureTreeWidgetItem * item = new CaptureTreeWidgetItem(mOpenNIItem, txt, pVCD);
						item->setIcon(0, QIcon(":/icons/22x22/openni.png"));
					}
				}
				else {
					statusBar()->showMessage(tr("Initialization FAILURE !"));
				}
			}

		} else {
			delete openniDevice;
			openniDevice = NULL;
		}
	} while(openni_found
			&& openni_idx<m_workflow_settings.maxOpenNI
			);
	mOpenNIItem->setExpanded(true);
#endif // HAS_OPENNI

	// FIXME : add opencv devices

	// FIXME : add Axis IP cameras
	fprintf(stderr, "[Workshop]::%s:%d : scanning done.\n", __func__, __LINE__);


}


void EmaMainWindow::on_deviceStopButton_clicked()
{
	if(ui->deviceTreeWidget->selectedItems().isEmpty()) { return; }
	CaptureTreeWidgetItem * captItem = (CaptureTreeWidgetItem *)ui->deviceTreeWidget->selectedItems().at(0);
	if(!captItem->isCategory())
	{
		if(captItem->getVideoCaptureDoc())
		{
			captItem->getVideoCaptureDoc()->stop();

			statusBar()->showMessage(tr("Stopped device ") + captItem->text(0));
			ui->mainDisplayWidget->setVideoCaptureDoc(NULL);

			// display main widget
			ui->stackedWidget->setCurrentIndex(0);
		}
		else
		{
			statusBar()->showMessage(tr("No video capture doc"));
		}
	}
	else
	{
		statusBar()->showMessage(tr("Select device instead of category"));
	}
}


void EmaMainWindow::on_deviceTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
	CaptureTreeWidgetItem * captItem = (CaptureTreeWidgetItem *)item;
	if(!captItem->isCategory())
	{
		if(captItem->getVideoCaptureDoc())
		{
			statusBar()->showMessage(tr("Opening device ") + captItem->text(0));
			ui->mainDisplayWidget->setVideoCaptureDoc(captItem->getVideoCaptureDoc());

			// display main widget
			ui->stackedWidget->setCurrentIndex(0);

		}
		else
		{
			statusBar()->showMessage(tr("No video capture doc"));
		}
	}
	else
	{
		statusBar()->showMessage(tr("Select device instead of category"));
	}

}
/*******************************************************************************

						COLLECTION EDITION

  *****************************************************************************/
void EmaMainWindow::updateCollectionsTreeWidgetItems()
{
	QList<EmaCollection *>::iterator cit;
	for(cit = m_workflow_settings.collectionList.begin();
		cit != m_workflow_settings.collectionList.end();
		++cit )
	{
		EmaCollection * pcol = (*cit);
		if(!pcol->treeViewItem)
		{
			EMAMW_printf(SWLOG_INFO, "added missing item for collec '%s'",
						 pcol->title.toAscii().data());
			pcol->treeViewItem = (QTreeWidgetItem *)new CollecTreeWidgetItem(
						ui->collecTreeWidget, pcol );
		}
	}

}


QStringList EmaMainWindow::getSelectedFiles()
{
	QStringList files;
	// append selected thumbs to quick collection
	QList<ThumbImageFrame *>::iterator it;
	for(it = mBottomThumbImageFrameList.begin();
		it !=mBottomThumbImageFrameList.end();
		++it)
	{
		ThumbImageFrame * curIt = (*it);
		if(curIt && curIt->isSelected())
		{
			files.append(curIt->getFilename());
		}
	}

	return files;
}

void EmaMainWindow::on_addCriterionButton_clicked()
{
	QStringList files = getSelectedFiles();
	if(!files.isEmpty())
	{
		mpQuickCollection->appendFiles(files);
		mpQuickCollectionRootItem->updateDisplay();
		// save settings to prevent from loosing data in case of crash
		saveSettings();
	}
}

void EmaMainWindow::on_actionAdd_selected_to_quick_collection_triggered()
{
	on_addCriterionButton_clicked();
}

EmaCollection * EmaMainWindow::getSelectedCollection()
{
	if(ui->collecTreeWidget->selectedItems().isEmpty())
	{
		return NULL;
	}


	QTreeWidgetItem * item = ui->collecTreeWidget->selectedItems().at(0);
	if(item->childCount()>0) // this is a collection
	{
		//
		CollecTreeWidgetItem * collecItem = (CollecTreeWidgetItem *)item;
		mpCurrentCollection = collecItem->mpCollec;

		if(!collecItem->mpCollec) { return NULL; }
		if(collecItem->mpCollec == mpQuickCollection )
		{
			return mpQuickCollection;
		}

		ui->curCollecLabel->setText(collecItem->mpCollec->title);
		ui->currentCollWidget->setEnabled(true);

		return collecItem->mpCollec;
	}

	return NULL;
}

void EmaMainWindow::on_excludeCriterionButton_clicked()
{
	QStringList files = getSelectedFiles();
	if(!files.isEmpty())
	{
		mpQuickCollection->removeFiles(files);
		mpQuickCollectionRootItem->updateDisplay();
		// save settings to prevent from loosing data in case of crash
		saveSettings();
	}
}



void EmaMainWindow::on_actionRemove_selected_from_quick_collec_triggered()
{
	on_excludeCriterionButton_clicked();
}

void EmaMainWindow::on_actionClear_quick_collection_triggered()
{
	mpQuickCollection->clearFiles();
	mpQuickCollectionRootItem->updateDisplay();
}

void EmaMainWindow::on_addToCurrentCollectionButton_clicked()
{
	if(!mpCurrentCollection)
	{
		EMAMW_printf(SWLOG_ERROR, "no selected collection");
		return;
	}

	// append selected thumbs to quick collection
	QStringList files = getSelectedFiles();

	EMAMW_printf(SWLOG_INFO, "append %d files to collection '%s'",
				 files.count(),
				 mpCurrentCollection->title.toAscii().data() );
	mpCurrentCollection->appendFiles(files);

	updateCollectionsTreeWidgetItems();
	on_collecTreeWidget_itemSelectionChanged();

	// save settings to prevent from loosing data in case of crash
	saveSettings();
}

void EmaMainWindow::on_removeFromCurrentButton_clicked()
{
	QStringList files = getSelectedFiles();
	if(files.isEmpty())
	{
		QMessageBox::warning(NULL, tr("No selected files"),
							 tr("Nothing to remove: no selected files"));
		return;
	}
	EmaCollection * pcol = getSelectedCollection();
	if(!pcol)
	{
		EMAMW_printf(SWLOG_ERROR, "no selected collection");
		QMessageBox::warning(NULL, tr("No selected collection"),
							 tr("Remove from what? no selected collection"));
		return;
	}

	pcol->removeFiles(files);

	updateCollectionsTreeWidgetItems();
	on_collecTreeWidget_itemSelectionChanged();

	// save settings to prevent from loosing data in case of crash
	saveSettings();

}

void EmaMainWindow::on_actionAdd_selected_to_current_collection_triggered()
{
	on_addToCurrentCollectionButton_clicked();
}

void EmaMainWindow::on_actionRemove_selected_from_current_collection_triggered()
{
	on_removeFromCurrentButton_clicked();
}

void EmaMainWindow::on_collecTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
	if(!item)
	{
		return;
	}

	if(item->childCount()>0) // this is a collection
	{
		CollecTreeWidgetItem * collecItem = (CollecTreeWidgetItem *)item;
		mpCurrentCollection = collecItem->mpCollec;
		if(!collecItem->mpCollec) { return; }

		EMAMW_printf(SWLOG_INFO, "Open all files of collection '%s'",
					 collecItem->mpCollec->title.toAscii().data()
					 );

//		if(collecItem->mpCollec == mpQuickCollection ) { return; }

		ui->curCollecLabel->setText(collecItem->mpCollec->title);
		ui->currentCollWidget->setEnabled(true);

		// append files
		QStringList files;
		QList<t_collection_file *>::iterator fit;
		for(fit = collecItem->mpCollec->filesList.begin();
			fit != collecItem->mpCollec->filesList.end();
			fit++)
		{
			EMAMW_printf(SWLOG_INFO, "\tadd thumb for file '%s'",
						 (*fit)->fullpath.toAscii().data());
			files.append( (*fit)->fullpath );
		}

		EMAMW_printf(SWLOG_INFO, "Append %d files in grids",
					 files.count());
		appendFileList(files);

	}

}

void EmaMainWindow::on_collecTreeWidget_itemSelectionChanged()
{
	mpCurrentCollection = NULL;
	ui->currentCollWidget->setEnabled(false);

	if(ui->collecTreeWidget->selectedItems().isEmpty())
	{
		return;
	}


	QTreeWidgetItem * item = ui->collecTreeWidget->selectedItems().at(0);
	if(item->childCount()>0) // this is a collection
	{
		//
		CollecTreeWidgetItem * collecItem = (CollecTreeWidgetItem *)item;
		mpCurrentCollection = collecItem->mpCollec;

		if(!collecItem->mpCollec) { return; }
		if(collecItem->mpCollec == mpQuickCollection ) { return; }

		ui->curCollecLabel->setText(collecItem->mpCollec->title);
		ui->currentCollWidget->setEnabled(true);

//		// append files
//		QList<t_collection_file *>::iterator fit;
//		for(fit = collecItem->mpCollec->filesList.begin();
//			fit != collecItem->mpCollec->filesList.end();
//			fit++)
//		{
//			EMAMW_printf(SWLOG_INFO, "Load file '%s'",
//						 (*fit)->fullpath.toAscii().data());
//			appendThumbImage( (*fit)->fullpath );
//		}

		//item->setExpanded(true);
	}
	else
	{
		// Find item

		QTreeWidgetItem * parentItem = item->parent();
		if(!parentItem) { return; }

		CollecTreeWidgetItem * collecItem = (CollecTreeWidgetItem *)parentItem;
		if(collecItem->mpCollec == mpQuickCollection )
		{
//			mpCurrentCollection = NULL;
		}
		else {
			mpCurrentCollection = collecItem->mpCollec;
		}

		if(!collecItem->mpCollec)
		{
			EMAMW_printf(SWLOG_INFO, "Invalid parent");
			return;
		}

		EMAMW_printf(SWLOG_INFO, "parent collection = '%s' / %d files",
					 collecItem->mpCollec->title.toAscii().data(),
					 collecItem->mpCollec->filesList.count()
					 );

		QList<t_collection_file *>::iterator fit;
		for(fit = collecItem->mpCollec->filesList.begin();
			fit != collecItem->mpCollec->filesList.end();
			fit++)
		{
			t_collection_file * pfile = (*fit);
			if(pfile->treeViewItem == item)
			{
				EMAMW_printf(SWLOG_INFO, "open file '%s'", pfile->fullpath.toAscii().data());
				// open file
				slot_thumbImage_clicked(pfile->fullpath);
			}
		}
	}


}

void EmaMainWindow::on_actionRemove_all_files_triggered()
{
	EMAMW_printf(SWLOG_INFO, "Remove all files from grids");

	QList<ThumbImageFrame *>::iterator it;
	for(it = mGridThumbImageFrameList.begin();
		it != mGridThumbImageFrameList.end(); )
	{
		ThumbImageFrame * thumb = (*it);
		it = mGridThumbImageFrameList.erase(it);
		delete thumb;
	}
	for(it = mBottomThumbImageFrameList.begin();
		it != mBottomThumbImageFrameList.end(); )
	{
		ThumbImageFrame * thumb = (*it);
		it = mBottomThumbImageFrameList.erase(it);
		delete thumb;
	}

	//ui->scrollAreaWidgetContents
	//ui->gridWidget

}


