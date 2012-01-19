/***************************************************************************
 *            emaimagemanager.h
 *		Manage all images info, cache and thumbnails
 *
 *  Sun Aug 16 19:32:41 2009
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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "imgutils.h"
#include "emaimagemanager.h"

int g_EMAImgMng_debug_mode = EMALOG_DEBUG;

#define EMAIM_printf(a,...)  { \
		if(g_EMAImgMng_debug_mode>=(a)) { \
			fprintf(stderr,"EmaImageManager::%s:%d : ",__func__,__LINE__); \
			fprintf(stderr,__VA_ARGS__); \
			fprintf(stderr,"\n"); \
		} \
	}

EmaImageManager * g_EmaImageManager = NULL;

EmaImageManager * emaMngr()
{
	if(!g_EmaImageManager) {
		EMAIM_printf(EMALOG_INFO, "Creating main manager...")
		g_EmaImageManager = new EmaImageManager();

		EMAIM_printf(EMALOG_INFO, "Starting main manager's thread")
		g_EmaImageManager->start();
	}

	return g_EmaImageManager;
}

EmaImageManager::EmaImageManager()
		: QThread()
{
	init();
}

void EmaImageManager::init()
{
	// READ CONFIGURATION IN XML FILE
	char home[1024] = "/home";
	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
	}
	m_cacheDirectory = QString(home) + "/.piafcache";
	QDir dir(home);
	if(!dir.exists(".piafcache"))
	{
		dir.mkdir(".piafcache", true);
	}
}

EmaImageManager::~EmaImageManager()
{

}
void EmaImageManager::purge()
{

}


/* Append a file list to managed pictures */
int EmaImageManager::appendFileList(QStringList list) {

	m_progress = 0;
	// Process image
	QStringList::Iterator it = list.begin();

	QString fileName;
	int nb = list.count();
	EMAIM_printf(EMALOG_DEBUG, "Appending %d files...", nb);

	m_appendFileListMutex.lock();
	while(it != list.end()) {
		fileName = (*it);
		++it;
		m_appendFileList.append(fileName);
	}
	m_appendFileListMutex.unlock();

	// Unlock thread
	mutex.lock();
	waitCond.wakeAll();
	mutex.unlock();

	return 0;
}

/* Clear the file list which is imported now */
int EmaImageManager::clearImportList()
{
	m_appendFileListMutex.lock();
	EMAIM_printf(EMALOG_DEBUG, "clear import list (there were %d files)...",
				 m_appendFileList.count());
	m_appendFileList.clear();
	m_appendFileListMutex.unlock();

	return 0;
}

t_image_info_struct * EmaImageManager::getInfo(QString filename)
{
	m_managedFileListMutex.lock();
	if(!m_managedFileList.contains(filename)) {
		EMAIM_printf(EMALOG_TRACE, "File '%s' not found", filename.toUtf8().data());
		m_managedFileListMutex.unlock();
		return NULL;
	}

	// Fill with managed
	int idx = m_managedFileList.indexOf(filename);
	if(idx < 0) {
		EMAIM_printf(EMALOG_TRACE, "File '%s' not found", filename.toUtf8().data());
		m_managedFileListMutex.unlock();
		return NULL;
	}
	m_managedInfoMutex.lock();

	t_image_info_struct * pinfo = m_managedInfo.at(idx);
	if(!pinfo) {
		m_managedInfoMutex.unlock();
		m_managedFileListMutex.unlock();
		return NULL;
	}
	if(!pinfo->valid) {
		m_managedInfoMutex.unlock();
		m_managedFileListMutex.unlock();
		return NULL;
	}

	EMAIM_printf(EMALOG_DEBUG, "File '%s' matches ?",
				 filename.toAscii().data());
	EMAIM_printf(EMALOG_DEBUG, " pinfo=%p (file='%s')",
				 pinfo, pinfo->filepath.toAscii().data());

	// Security : check filename
	if(filename.compare(pinfo->filepath) != 0) {
		EMAIM_printf(EMALOG_TRACE, "File '%s' mismatches pinfo ('%s')",
					 filename.toUtf8().data(),
					 pinfo->filepath.toUtf8().data())
		m_managedInfoMutex.unlock();
		m_managedFileListMutex.unlock();
		return NULL;
	}

	m_managedInfoMutex.unlock();
	m_managedFileListMutex.unlock();

	return pinfo ;
}
/* Append a file to managed pictures */
int EmaImageManager::appendFile(QString filename) {

	m_progress = 0;

	m_managedFileListMutex.lock();
	// if not already managed
	if(m_managedFileList.contains(filename)) {
		EMAIM_printf(EMALOG_DEBUG, "Already managed : '%s'", filename.toUtf8().data());
		m_managedFileListMutex.unlock();
		return 0;
	}

	// Process image
	m_appendFileList.append(filename);
	m_managedFileListMutex.unlock();

	// Unlock thread
	mutex.lock();
	waitCond.wakeAll();
	mutex.unlock();

	return 0;
}

/* Remove a file to managed pictures */
int EmaImageManager::removeFile(QString filename) {
	m_progress = 0;

	m_managedFileListMutex.lock();
	// if not already managed
	if(!m_managedFileList.contains(filename)) {
		EMAIM_printf(EMALOG_DEBUG, "Unknown / not managed : '%s'", filename.toUtf8().data());
		m_managedFileListMutex.unlock();
		return 0;
	}

	m_removeFileListMutex.lock();
	m_removeFileList.append(filename);
	m_removeFileListMutex.unlock();

	m_managedFileListMutex.unlock();

	// Unlock thread
	mutex.lock();
	waitCond.wakeAll();
	mutex.unlock();

	return 0;
}




// =============================================================================
//				PROCESSING THREAD
// =============================================================================
void EmaImageManager::run() {
	m_run = true;
	m_running = true;


	while(m_run) {
		int wait_ms = 400;

		mutex.lock();
		waitCond.wait(&mutex, wait_ms);
		mutex.unlock();

		QStringList appendList;
		m_appendFileListMutex.lock();
		appendList = m_appendFileList;
		m_appendFileList.clear();
		m_appendFileListMutex.unlock();

		// Check if new files have been added
		if(!appendList.isEmpty()) {
			//
			// Process image info extraction, then add files
			QStringList::Iterator it = appendList.begin();

			QString fileName;
			int nb = appendList.count();
			EMAIM_printf(EMALOG_DEBUG, "Adding %d files...", nb)

			int cur = 0;
			while(it != appendList.end()) {
				fileName = (*it);
				++it;
				EMAIM_printf(EMALOG_DEBUG, "\tAdding file '%s'...", fileName.toUtf8().data());

				// check if image is already known, eg if it already has a thumb and data
				QFileInfo fi(fileName);
				QFileInfo thumb_fi(m_cacheDirectory+"/"+fi.fileName()+".jpg");
				QFileInfo data_fi(m_cacheDirectory+"/"+fi.fileName()+".xml");
				QString thumbPath = thumb_fi.absoluteFilePath();
				EMAIM_printf(EMALOG_DEBUG, "\tThumb ? for file '%s' : thumb='%s'",
							 fileName.toUtf8().data(),
							 thumb_fi.absoluteFilePath().toAscii().data());
				t_image_info_struct * new_info = NULL;
				if(thumb_fi.exists() && data_fi.exists())
				{
					EMAIM_printf(EMALOG_DEBUG, "\tThumb & data found for file '%s' : thumb='%s'",
								 fileName.toUtf8().data(),
								 thumb_fi.absoluteFilePath().toAscii().data());
					// Check if data exists
					new_info = new t_image_info_struct;
					clearImageInfoStruct(new_info);

					// load data
					loadImageInfoStruct(new_info, data_fi.absoluteFilePath());

					printImageInfoStruct(new_info);

					// Copy thumb
					IplImage * rgbImage =
					new_info->thumbImage.iplImage = cvLoadImage(thumb_fi.absoluteFilePath().toUtf8().data());
					if(rgbImage && rgbImage->nChannels > 1)
					{
						// Swap B<->R because colors are inverted
						new_info->thumbImage.iplImage = tmCloneImage(rgbImage);
						switch(rgbImage->nChannels)
						{
						default:
							break;
						case 3:
							cvCvtColor(rgbImage, new_info->thumbImage.iplImage, CV_BGR2RGB);
							break;
						case 4:
							cvCvtColor(rgbImage, new_info->thumbImage.iplImage, CV_BGRA2RGBA);
							break;
						}
						tmReleaseImage(&rgbImage);
					}

					new_info->thumbImage.fullpath = thumb_fi.absoluteFilePath().toUtf8().data();
					new_info->thumbImage.compressed = NULL;
					new_info->thumbImage.compressed_size = 0;

					fprintf(stderr, "%s:%d : Thumb = %p %dx%dx%d\n",
							__func__, __LINE__, new_info->thumbImage.iplImage,
							new_info->thumbImage.iplImage? new_info->thumbImage.iplImage->width : -1,
							new_info->thumbImage.iplImage? new_info->thumbImage.iplImage->height : -1,
							new_info->thumbImage.iplImage?new_info->thumbImage.iplImage->nChannels : -1
							);
					new_info->hsvImage = new_info->sharpnessImage = NULL;
				}

				if(!new_info) {
					// Process info extraction
					m_imgProc.loadFile(fileName);

					// Create a new storage
					t_image_info_struct l_info = m_imgProc.getImageInfo();
					if(l_info.valid) {
						new_info = new t_image_info_struct(l_info);

						// Copy thumb
						new_info->thumbImage.iplImage = tmCloneImage(l_info.thumbImage.iplImage);

						// save thumb
						cvSaveImage(thumbPath.toUtf8().data(),
									new_info->thumbImage.iplImage );

						// Copy HSV histo
						new_info->hsvImage = tmCloneImage(l_info.hsvImage);

						// Copy sharpness
						new_info->sharpnessImage = tmCloneImage(l_info.sharpnessImage);

						// Save into XML file
						saveImageInfoStruct(new_info, data_fi.absoluteFilePath());

						// Copy RGB histo
	//					new_info->histogram

					}
				}
				if(new_info) {
					// append to managed list
					m_managedFileListMutex.lock();
					m_managedFileList.append(fileName);
					m_managedFileListMutex.unlock();

					m_managedInfoMutex.lock();
					m_managedInfo.append(new_info);
					m_managedInfoMutex.unlock();
				}

				cur++;
				m_progress = 100 * cur / nb;
			}
		}

	}

	m_running = false;
	EMAIM_printf(EMALOG_INFO, "Processing thread ended.")
}







