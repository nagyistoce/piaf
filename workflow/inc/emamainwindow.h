/***************************************************************************
 *            emamainwindow.h
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

#ifndef EMAMAINWINDOW_H
#define EMAMAINWINDOW_H

#include <QtCore/QTimer>

#include <QtGui/QMainWindow>
#include <QtGui/QTreeWidgetItem>
#include <QWorkspace>
#include <QSettings>
#include <QDomDocument>
#include <QMdiArea>

#ifdef PIAF_LEGACY
#include "workshopimagetool.h"
#endif


#include "preferencesdialog.h"

#include "virtualdeviceacquisition.h"
#include "videocapture.h"


class EmaMainWindow;
class ThumbImageFrame;
class BatchQueueWidget;

/** @brief Types for EmaTreeWidgetItems */
typedef enum {	TREE_NONE,  ///< undefined
				TREE_FOLDER, ///< Folder : contains sub-folders and/or files
				TREE_FILE, ///< File item : links to a file
				TREE_COLLECTION, ///< Collection : contains sub-collectionss and/or files
				TREE_DEVICE_ROOT, ///< Device category, eg V4L2, OpenNI ...
				TREE_DEVICE_NODE  ///< Device node
			 } t_treeitem_category;


class EmaTreeWidgetItem : QTreeWidgetItem
{
public:
	EmaTreeWidgetItem(QTreeWidget * treeWidgetParent, t_treeitem_category cat);
	EmaTreeWidgetItem(QTreeWidgetItem * treeWidgetItemParent, t_treeitem_category cat);
	t_treeitem_category getCategory() { return mCategory; }
protected:
	t_treeitem_category mCategory;
};

/** \brief Directory tree item */
class DirectoryTreeWidgetItem : QTreeWidgetItem
{
//	Q_OBJECT
	friend class EmaMainWindow;
public:
	DirectoryTreeWidgetItem(QTreeWidgetItem * treeWidgetItemParent, QString path);
	DirectoryTreeWidgetItem(QTreeWidget * treeWidgetParent, QString path);
	~DirectoryTreeWidgetItem();

	/** \brief Expand contents: read directory contents and display it */
	void expand();
	/** \brief collapse contents: collapse directory and clear data */
	void collapse();

	QString getFullPath() { return mFullPath; }

	bool isFile() { return mIsFile; }
	QStringList getFileList();

protected:
	void init(); ///< init internal data and fill nb files
	QString mName;
	QString mFullPath;
	int mNbFiles;
	bool mIsFile;///< true if the item is a file
	QTreeWidgetItem * mFakeSubItem;///< fake item to see the content

private:
	QList<DirectoryTreeWidgetItem *> mSubDirsList;
	QList<t_file *> mFilesList;

};


/** \brief Collection tree item */
class CollecTreeWidgetItem : QTreeWidgetItem
{
//	Q_OBJECT
	friend class EmaMainWindow;
public:
	CollecTreeWidgetItem(QTreeWidgetItem * treeWidgetItemParent, EmaCollection * pcollec);
	CollecTreeWidgetItem(QTreeWidget * treeWidgetParent, EmaCollection * pcollec);
	~CollecTreeWidgetItem();

	/** \brief Expand contents: read directory contents and display it */
	void expand();
	/** \brief collapse contents: collapse directory and clear data */
	void collapse();

	/** \brief Return true if entry is a file, false if it is a list of collections */
	bool isFile() { return mIsFile; }

	/** \brief Return collection */
	EmaCollection * getCollection() { return mpCollec; }

protected:
	void init(); ///< init internal data and fill nb files
	void purge(); ///< purge internal data and free memory
	void updateDisplay(); ///< display information about collection


	EmaCollection * mpCollec; ///< pointer to collection
	bool mIsFile;///< true if the item is a file
	QTreeWidgetItem * subItem;///< fake item to see the content
private:
	QList<CollecTreeWidgetItem *> subCollecsList;

	/// List of files
	QList<t_file *> filesList;
};



/** \brief Capture device tree item */
class CaptureTreeWidgetItem : QTreeWidgetItem
{
//	Q_OBJECT
	friend class EmaMainWindow;
public:
	/// Constructor for main classes: IP, V4L2 ...
	CaptureTreeWidgetItem(QTreeWidgetItem * treeWidgetItemParent, QString devname, VideoCaptureDoc * pVCapDoc);
	/// Constructor for devices classes
	CaptureTreeWidgetItem(QTreeWidget * treeWidgetParent, QString category);
	~CaptureTreeWidgetItem();

	VideoCaptureDoc * getVideoCaptureDoc() { return m_pVideoCaptureDoc; }

	bool isCategory() { return (m_pVideoCaptureDoc == NULL); }

protected:
	void init(); ///< init internal data and get device properties
	void purge(); ///< stop aquisition
	QString mName;

	///< Pointer on video acquisition device
	VideoCaptureDoc * m_pVideoCaptureDoc;

private:

};




namespace Ui
{
	class EmaMainWindow;
}




/** \brief Main window for workflow app


*/
class EmaMainWindow : public QMainWindow
{
	Q_OBJECT


public:
	EmaMainWindow(QWidget *parent = 0);
	~EmaMainWindow();

private:

	Ui::EmaMainWindow *ui;

	void loadSettings();
#define PIAFWKFL_SETTINGS	"piaf-workflow"
#define PIAFWKFL_SETTINGS_XML	"piafsettings.xml"
	QSettings mSettings;	///< Qt Settings object for piaf workflow



	/// List of input directories
	QList<DirectoryTreeWidgetItem *> mDirectoryList;

	/// Quick collection
	EmaCollection * mpQuickCollection;
	/// Current selected collection
	EmaCollection * mpCurrentCollection;

	/// Root for quick collection
	CollecTreeWidgetItem * mpQuickCollectionRootItem;

	/// update collection tree widget
	void updateCollectionsTreeWidgetItems();

	/** \brief Append recursivelly a collection from a XML tree branch */
	void appendCollection(QDomElement collecElem, EmaCollection * parent_collec);

	/// Opened image paths
	QStringList m_imageList;
	QStringList m_appendFileList;
	QStringList m_removeFileList;

	// Thumbnails sorter
	int m_sorter_nbitems_per_row;

	/** @brief append a directory to managed directories */
	DirectoryTreeWidgetItem * appendDirectoryToLibrary(QString path, QTreeWidgetItem * itemParent = NULL);


	/** @brief Get selected collection, even if it's the wuick collection */
	EmaCollection * getSelectedCollection();

	/** @brief Get list of selected files */
	QStringList getSelectedFiles();

	/** @brief append a list of files to displayed files */
	void appendFileList(QStringList list);

	/** @brief remove a list of files to displayed files */
	void removeFileList(QStringList list);

	void appendThumbImage(QString fileName);
	void removeThumbImage(QString fileName);

	/// List of frames for Grid
	QList<ThumbImageFrame *> mGridThumbImageFrameList;
	/// List of frames for bottom line
	QList<ThumbImageFrame *> mBottomThumbImageFrameList;

	/// Selected image frame on bottom line
	ThumbImageFrame * mSelectedThumbImageFrame;


	/** @brief Set the file to be displayed in main display */
	void setCurrentMainFile(QString fileName);

	/** @brief Set the file to be added in workspace */
	void openInWorkspace(QString fileName);

private:
	QDomDocument mSettingsDoc;
	void addFolderToXMLSettings(QDomElement * parent_elem, t_folder folder);
	void addCollectionToXMLSettings(QDomElement * elem, EmaCollection collect);

	QTimer m_timer;
	QMdiArea * pWorkspace;
	BatchQueueWidget * mpBatchQueueWidget;

	/// Main file diplayed
	QString mMainFileName;

	// Parent items for devices tree widget items
	CaptureTreeWidgetItem * mV4l2Item;
	CaptureTreeWidgetItem * mOpenNIItem;
	CaptureTreeWidgetItem * mFreenectItem;
	CaptureTreeWidgetItem * mOpenCVItem;


#ifdef PIAF_LEGACY
	WorkshopImage * pWorkshopImage;
	WorkshopImageTool * pWorkshopImageTool;
#endif

public slots:
	void slot_thumbImage_clicked(QString fileName);
	void slot_thumbImage_doubleClicked(QString fileName);
	void slot_thumbImage_selected(QString);


	void slot_thumbImage_signal_click(ThumbImageFrame *);
	void slot_thumbImage_signal_shiftClick(ThumbImageFrame *);
	void slot_thumbImage_signal_ctrlClick(ThumbImageFrame *);


private slots:
	void saveSettings();

	void on_actionClean_activated();
	void on_actionView_right_column_toggled(bool );
	void on_actionView_left_column_toggled(bool );
	void on_batchPlayerButton_clicked();
	void on_filesTreeWidget_itemExpanded(QTreeWidgetItem* item);
	void on_collecTreeWidget_itemExpanded(QTreeWidgetItem* item);
	void on_workspaceButton_clicked();
	void slot_filesShowCheckBox_stateChanged(int);
	void on_filesLoadButton_clicked();
	void on_filesClearButton_clicked();
	void on_zoomx2Button_clicked();
	void on_zoomx1Button_clicked();


	void slot_gridWidget_signal_resizeEvent(QResizeEvent *);

	// Collections
	void on_collecShowCheckBox_stateChanged(int);

	void slot_timer_timeout();

	void on_gridButton_clicked();
	void on_imgButton_clicked();

	void slot_mainGroupBox_resizeEvent(QResizeEvent *);
	//
	void on_globalNavImageWidget_signalZoomOn(int, int, float);

	//void on_mainDisplayWidget_signalGreyscaleToggled(bool);
	void on_mainDisplayWidget_signalPluginsButtonClicked();

	// MAIN MENU ACTION
	void on_actionQuit_activated();
	void on_actionEdit_plugins_activated();
	void on_actionBatch_processor_activated();
	void on_actionConvert_images_to_AVI_activated();
	void on_actionAbout_activated();

	void slot_appendNewPictureThumb( QString  );

	void on_filesTreeWidget_itemClicked ( QTreeWidgetItem * item, int column );
	void on_filesTreeWidget_itemDoubleClicked ( QTreeWidgetItem * item, int column );
	void on_collecAddButton_clicked();
	void slot_newCollDialog_signalNewCollection(EmaCollection);
	void slot_newCollDialog_signalCollectionChanged(EmaCollection *);
	void on_collecDeleteButton_clicked();
	void on_collecEditButton_clicked();
	void on_stopLoadButton_clicked();
	void on_actionView_bottom_line_toggled(bool arg1);
	void on_actionPreferences_activated();
	void on_actionOpen_file_in_workspace_triggered();

	/// Refresh list of available acquisition devices
	void on_deviceRefreshButton_clicked();
	void on_deviceTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
	void on_addCriterionButton_clicked();
	void on_actionAdd_selected_to_quick_collection_triggered();
	void on_excludeCriterionButton_clicked();
	void on_actionClear_quick_collection_triggered();
	void on_actionRemove_selected_from_quick_collec_triggered();
	void on_addToCurrentCollectionButton_clicked();
	void on_removeFromCurrentButton_clicked();
	void on_actionAdd_selected_to_current_collection_triggered();
	void on_actionRemove_selected_from_current_collection_triggered();
	void on_collecTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
	void on_collecTreeWidget_itemSelectionChanged();
	void on_actionRemove_all_files_triggered();
};

#endif // EmaMAINWINDOW_H
