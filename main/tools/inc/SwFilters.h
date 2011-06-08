/***************************************************************************
	   SwFilters.h  -  plugin core and plugin manager for Piaf
							 -------------------
	begin                : Fri Nov 29 2002
	copyright            : (C) 2002 by Christophe Seyve - SISELL
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
#ifndef __SW_FILTERLIST__
#define __SW_FILTERLIST__


// Qt includes
#include <qwidget.h>
#include <qobject.h>
#include <q3mainwindow.h>
#include <qpushbutton.h>
#include <q3buttongroup.h>
#include <qlist.h>
#include <qlabel.h>
#include <q3hbox.h>
#include <q3listview.h>
#include <q3textedit.h>
#include <qcombobox.h>

//Added by qt3to4:
#include <QCloseEvent>
#include <Q3PtrList>
#include <QLineEdit>

// SisellWorkshop filters defines
//#include <SwFiltersDefs.h>
#include <SwTypes.h>
#include <SwImage.h>

#include "SwPluginCore.h"


#include <QWorkspace>

/**
	\brief single filter plugin manager.

	Contains information about available filters, manage executable name, function and parameters list.

	\author Christophe Seyve- SISELL \email mailto:cseyve@free.fr
	*/

class SwFilter : public QObject
{
	Q_OBJECT
	friend class SwFilterManager;
public:
	/** constructor
		\param path executable path string
		\param keepAlive tells if process should be left running after questionning
	*/
	SwFilter(char * path, bool keepAlive);
	/** @brief Set the pointer to MDI workspace */
	void setWorkspace(QWorkspace * wsp);


	/** Destructor */
	~SwFilter();
	/** close pipe */
	int forceCloseConnection();
	/** read child pid */
	int getChildPid() { return childpid; };

	/** wait until communication locked flag is down */
	bool waitForUnlock(int timeout_ms);

	/// Executable name
	char * exec_name;
	char * category;
	char * subcategory;
	char * comment;
	/// returns number of functions
	int nbfunctions();

	/** @brief
	  */
	int processFunction(int indexFunction, void * data_in, void * data_out, int timeout_ms);
private:
	swFrame frame;

	/// tells if process is loaded
	bool statusOpen;
	bool readFlag;
	bool writeFlag;
	void setReadFlag(int sig);
	void setWriteFlag(bool b);

	/** start child process */
	int loadChildProcess();
	/** stops child process */
	int unloadChildProcess();

	/// boolean to tell if communication is locked
	bool comLock;

	/// communication buffer
	char * buffer;
	// communication pipes
	FILE * pipeR; // for reading from plugin
	FILE * pipeW; // for writing to plugin
	int childpid; // child pid for signal and process control

	int pipeIn[2];
	int pipeOut[2];

	/** Reads function list from plugin */
	void readFunctionList();
	/** Function list */
	swFunctionDescriptor * funcList;
	int nb_func;

	/* treat received framebuffer */
	int treatFrame(char *framebuffer, int framesize);
	/** send a request to child process */
	void sendRequest(char * req);
	/** wait timeout_ms for an answer on pipeR then cancel */
	int waitForAnswer(int timeout_ms);

	/** constructor common part */
	void init();
	/** destructor common part */
	int destroy();

signals:
	/** emitted when child died */
	void signalDied(int pid);
};

/** \brief Time calculation display for a plugin
	This class displays a window with processing time mean, min/max.
	\author Christophe Seyve - SISELL \mailto cseyve@free.fr
*/
class SwTimeWidget : public QWidget
{
	Q_OBJECT

public:
	SwTimeWidget(QWidget* parent, const char *name, Qt::WindowFlags wflags);
	~SwTimeWidget();
	int setTimeUS(long dt);
	int reset();

private:
	int refresh();
	QLabel * meanLabel;
	QLabel * minLabel;
	QLabel * maxLabel;

	int nb;

	long cumul;
	float mean;
	long mean_time;
	long min_time;
	long max_time;

};


/** \brief GUI view for plugin (name, QListViewItems ...)
	Structure for Filter management in GUI : contains SwFilter, icon path,
	function index, enabled flag, QListViewItems, parameters editor widget
*/
typedef struct _swPluginView {
	SwFilter * filter;
	char * icon;
	int indexFunction;
	bool enabled;
	Q3ListViewItem * availItem;
	Q3ListViewItem * selItem;
	QWidget * mwPluginEdit;
	SwTimeWidget * mwPluginTime;
} swPluginView;

class VideoCaptureDoc;

typedef void (*sighandler_t)(int);

int registerChildSig(int pid, SwFilter * filter);
int removeChildSig(int pid);
void sigpipe(int pid);
void sigusr1(int pid);
void sigchld(int pid);

/**
	\brief Filters list manager.

	Class for managing a collection of filters. It can save/load filter lists, manage filters collection, disable any filter...
	\author Christophe Seyve - SISELL \mailto cseyve@free.fr
*/

class SwFilterManager : public Q3MainWindow
{
	Q_OBJECT

public:
	/** Constructor */
	SwFilterManager(VideoCaptureDoc * vdoc, QWidget* pparent = NULL, const char *name = NULL, Qt::WFlags  wflags = 0);
	/** Constructor */
	SwFilterManager(QWidget* pparent = NULL, const char *name=NULL, Qt::WFlags wflags = 0);

	/** Destructor */
	~SwFilterManager();

	/** @brief Set the pointer to MDI workspace */
	void setWorkspace(QWorkspace * wsp);

	/** @brief process an image and store result into input structure
		@param image input image in swImageStruct structure

		@return <0 if error, >=0 if success
	*/
	int processImage(swImageStruct * image);

	/** @brief Show filters manager window */
	void showWindow();

	/** @brief Load a plugin sequence : { plugin>function>parameters => ... }

		@returns <0 if error, 0 if success
	*/
	int loadFilterList(char * filename);
	/** \brief Save a plugin sequence : { plugin>function>parameters => ... } */
	void saveFilterList(char * filename);

	/** @brief Get last file sequence loaded */
	char * getPluginSequenceFile() { return mPluginSequenceFile; }
	/** @brief show warning on plugin crash */
	void enableWarning(bool on) { mNoWarning = !on; }
private:
	swImageStruct *imageTmp;
	bool lockProcess;

	bool mNoWarning; ///< Don't show warning on plugin crash
	/// List of all available filters
	Q3PtrList<swPluginView> *filterColl;
	/// List of active filters
	Q3PtrList<swPluginView> *selectedFilterColl;

	VideoCaptureDoc * doc;
protected slots:


private:
	swFrame frame;
	virtual void closeEvent(QCloseEvent*);

	QWorkspace * pWorkspace;

	// main window for filters
	Q3MainWindow * filtersWindow;
	Q3HBox * filtersHBox;
	Q3ListView * filtersListView;
	Q3ListView * selectedListView;

	Q3ListViewItem *originalItem;

	// buttons and widgets
	QPushButton * bAdd;
	QPushButton * bDel;
	QPushButton * bEdit;
	QPushButton * bDisable;
	QPushButton * bUp;
	QPushButton * bDown;

	// load/save buttons
	QPushButton * bSave;
	QPushButton * bLoad;

	/// Last file sequence loaded
	char mPluginSequenceFile[1024];

	/// current edited plugin
	QLineEdit ** paramsEdit;
	QComboBox ** comboEdit;
	int idEditPlugin;
	int idViewPlugin;
	/** Constructors common part */
	void init();
	/** Load a filter with its icon */
	int loadFilter(char *filename, char * iconname);
	/** Load all filters */
	int loadFilters();



	/** add a new filter in filters list.
		\param newFilter allocated SwFilter object
		\param id -1 for end of list, else rank from 0
		*/
	int addFilter(swPluginView * newFilter, int id);
	/** get filter pointer from id. */
	swPluginView * getFilter(int id); // get filter from index
	/** read number of filters into list */
	int getFilterCount();

	/// remove filter by index
	int removeFilter(int id);
	/// remove filter by pointer
	int removeFilter(swPluginView * filter);


	void updateSelectedView();

public slots:
	/// Unload loaded plugins from background thread => will not update display
	void slotUnloadAll();

	/// Delete all selected plugins (unload all plugin processus)
	void slotDeleteAll();

protected slots:
	void slotAdd();
	void slotPluginDoubleClicAdd(Q3ListViewItem *);
	void slotDelete();
	void slotEdit();
	void slotDisable();
	void slotUp();
	void slotDown();

	void slotSave();
	void slotLoad();

	void slotTime();

	// edit parameters
	void applyEditParameters();
	void cancelEditParameters();

	void simpleClic(Q3ListViewItem *);
	void doubleClic(Q3ListViewItem *);

	void slotFilterDied(int);

signals:
	void selectedFilterChanged();

};


typedef struct _sigFilter {
	int pid;
	SwFilter * pFilter;
} sigFilter;

class SwSignalHandler
{
public:
	SwSignalHandler() { filterList.setAutoDelete(false); };
	int registerChild(int pid, SwFilter * filter);
	int killChild();
	int removeChild(int pid);
private:
	Q3PtrList<sigFilter> filterList;
};


#endif
