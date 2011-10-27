/***************************************************************************
	   PiafFilters.h  -  plugin core and plugin manager for Piaf
							 -------------------
	begin                : Fri Aug 12 2011
	copyright            : (C) 2002-2011 by Christophe Seyve
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
#ifndef __PIAF_FILTERLIST__
#define __PIAF_FILTERLIST__


// Qt includes
#include <QList>
#include <QLineEdit>


// SisellWorkshop filters defines
//#include <PiafFiltersDefs.h>
#include <SwTypes.h>
#include <SwImage.h>

#include "SwPluginCore.h"
#include "time_histogram.h"

#define PIAFFILTER_MSG(...)	{ \
			fprintf(stderr, "PIAFFILER  %s:%d: ", __FILE__, __func__, __LINE__); \
			fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); \
			}

/**
	\brief single filter plugin manager.

	Contains information about available filters, manage executable name, function and parameters list.

	\author Christophe Seyve- SISELL \email mailto:cseyve@free.fr
	*/
class PiafFilter : public QObject
{
	Q_OBJECT
	friend class PiafFilterManager;
	friend class FilterSequencer;

public:
	/** constructor
		\param path executable path string
		\param keepAlive tells if process should be left running after questionning
	*/
	PiafFilter(char * path, bool keepAlive);
	/** Copy constructor
	*/
	PiafFilter(PiafFilter * filter);

	/** Destructor */
	~PiafFilter();

	/** @brief close pipe */
	int forceCloseConnection();

	/** @brief read child pid */
	int getChildPid() { return childpid; };

	/** @brief wait until communication locked flag is down */
	bool waitForUnlock(int timeout_ms);

	/** @brief Return function descriptor */
	swFunctionDescriptor getFunction(int idx);

	/** @brief Get index of loaded function */
	int getIndexFunction() { return indexFunction; }

	/** @brief Send current parameters to plugin processus */
	int sendParams();

	/** \brief get time histogram */
	t_time_histogram getTimeHistogram() { return mTimeHistogram; }

	/** @brief index of loaded function
	  <0 if not loaded
	  */
	int indexFunction;


	bool mKeepAlive; ///< if the plugin should stay loaded

	/// Executable name
	char * exec_name;

	/// Category (obsolete = "Vision")
	char * category;

	/// Registered name for binary
	char * subcategory;

	/// User comment
	char * comment;
	/// returns number of functions
	int nbfunctions();

	///< Flag indicating that the plugin has died or must be disconnected
	bool plugin_died;

	/** @brief
	  */
	int processFunction(int indexFunction, void * data_in, void * data_out, int timeout_ms);

	/** @brief Force state of enabling flag */
	void setEnabled(bool on ) {enabled = on; }

	/** @brief Read state of enabling flag */
	bool isEnabled() { return enabled; }

	/** @brief Force state of final flag */
	void setFinal(bool on ) { final = on; }
	/** @brief Final flag */
	bool isFinal() { return final; }


	/** @brief tells if process is loaded */
	bool loaded() { return statusOpen; }

	/** @brief Set processing duration in us for statistics */
	void setTimeUS(int dt_us);

	/** @brief Return description for current function */
	swFunctionDescriptor * updateFunctionDescriptor();


private:
	/// Communication frame
	swFrame frame;

	/** @brief Load process properties from binary file */
	void loadBinary(char * filename);


	/// tells if process is loaded
	bool statusOpen;
	/// tells if process is enabled
	bool enabled;
	/// tells if process should stop at this filter
	bool final;


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

	/** @brief Lock communication
	\todo use mutex ?
	*/
	void lockComm() { comLock = true; }
	/** @brief Unlock communication */
	void unlockComm() { comLock = false; }

	/** send a request to child process
	*/
	void sendRequest(char * req);
	/** wait timeout_ms for an answer on pipeR then cancel

	*/
	int waitForAnswer(int timeout_ms);

	/// communication buffer
	char * buffer;
	// communication pipes
	FILE * pipeR; ///< File pipe for reading from plugin
	FILE * pipeW; ///< File pipe for writing to plugin
	int childpid; ///< child pid for signal and process control

	int pipeIn[2];
	int pipeOut[2];

	/** Reads function list from plugin */
	void readFunctionList();

	/** Functions list */
	swFunctionDescriptor * funcList;

	/** Number of functions */
	int nb_func;

	/* treat received framebuffer */
	int treatFrame(char *framebuffer, int framesize);

	/** @brief histogram of processing time */
	t_time_histogram mTimeHistogram;

	/** constructor common part */
	void init();
	/** destructor common part */
	int destroy();

signals:
	/** emitted when child died */
	void signalDied(int pid);

	/** emitted when parameters have been changed on filter (by GUI user) */
	void signalParamsChanged();

};




typedef void (*sighandler_t)(int);

int registerChildSig(int pid, PiafFilter * filter);
int removeChildSig(int pid);
void sigpipe(int pid);
void sigusr1(int pid);
void sigchld(int pid);

/**
	\brief Filters list sequencer.

	Class for managing a collection of filters. It can save/load filter lists, manage filters collection, disable any filter...
	\author Christophe Seyve - SISELL \mailto cseyve@free.fr
*/
class FilterSequencer : public QObject
{
	Q_OBJECT

public:
	FilterSequencer();
	~FilterSequencer();

	/** @brief Emit a signal to force update of processing */
	void update();

	/** @brief Unload all loaded filters */
	void unloadAllLoaded();

	/** @brief Append a filter to loaded filters list and return newly created one */
	PiafFilter * addFilter(PiafFilter * filter);

	/** @brief Remove a filter from loaded filters list */
	int removeFilter(PiafFilter * filter);

	/** @brief Move a filter upway from loaded filters list */
	int moveUp(PiafFilter * filter);
	/** @brief Move a filter upway from loaded filters list */
	int moveDown(PiafFilter * filter);

	/** @brief Disable a filter from loaded filters list */
	void toggleFilterDisableFlag(PiafFilter * filter);

	/** @brief Get list of available filters */
	QList<PiafFilter *> getAvailableFilters() { return mAvailableFiltersList; }

	/** @brief Get loaded filter by index */
	PiafFilter * getFilter(int idx);
	/** @brief Get list of loaded filters */
	QList<PiafFilter *> getLoadedFilters() { return mLoadedFiltersList; }

	/** @brief Save current sequence in file */
	int saveSequence(char *filename);

	/** @brief Load current sequence from file */
	int loadSequence(char * filename);

	/** @brief process an image and store result into input structure
		@param image input image in swImageStruct structure

		@return <0 if error, >=0 if success
	*/
	int processImage(swImageStruct * image);

	/** @brief Load a plugin sequence : { plugin>function>parameters => ... }

		@returns <0 if error, 0 if success
	*/
	int loadFilterList(char * filename);

	/** \brief Save a plugin sequence : { plugin>function>parameters => ... } */
	void saveFilterList(char * filename);

	/** @brief Set final plugin in sequence: the next plugins won't be processed */
	void setFinal(PiafFilter * filter);

	/** @brief Get last file sequence loaded */
	char * getPluginSequenceFile() { return mPluginSequenceFile; }

	/** @brief show warning on plugin crash (default: true) */
	void enableWarning(bool on) { mNoWarning = !on; }

	/** @brief reload automatically plugin sequence when a plugin crash (default: false) */
	void enableAutoReloadSequence(bool on) { mAutoReloadSequence = on; }

	/** Load list of available filters */
	int loadFilters();

protected:
	void init();
	void purge();

	swImageStruct *imageTmp;
	bool lockProcess;

	bool mNoWarning; ///< Don't show warning on plugin crash

	/// List of available filters
	QList<PiafFilter *> mAvailableFiltersList;

	/// List of loaded filters
	QList<PiafFilter *> mLoadedFiltersList;

	/// Auto reload of plugin sequence on a plugin crash
	bool mAutoReloadSequence;

	/// Buffer frame for communication with plugins
	swFrame frame;

//	virtual void closeEvent(QCloseEvent*);

	/// Last file sequence loaded
	char mPluginSequenceFile[1024];

	/** Load a filter */
	int loadFilter(char *filename);

	/** add a new filter in filters list.
		\param newFilter allocated PiafFilter object
		\param id -1 for end of list, else rank from 0
		*/
	PiafFilter * addFilter(PiafFilter * newFilter, int id);

	/** read number of filters into list */
	int getFilterCount();

	/// remove filter by index
	int removeFilter(int id);

	int idEditPlugin;	///< index of edited plugin
	int idViewPlugin;	///< index of viewed plugin

public:
	/// Unload plugins processes used for creating the available list from background thread => will not update display
	void unloadAllAvailable();

	/// Delete all selected plugins (unload all plugin processus)
	void deleteAll();

protected slots:

	void slotFilterDied(int);
	void slot_signalParamsChanged();

signals:
	void selectedFilterChanged();
	void signalFilterDied(PiafFilter *);
};



typedef struct {
	int pid;
	PiafFilter * pFilter;
} sigPiafFilter;

class PiafSignalHandler
{
public:
	PiafSignalHandler() {  };
	int registerChild(int pid, PiafFilter * filter);
	int killChild();
	int removeChild(int pid);
private:
	QList<sigPiafFilter *> filterList;
};


#endif
