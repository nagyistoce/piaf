/***************************************************************************
	   PiafFilters.cpp  -  plugin core and plugin manager for Piaf
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

//#define __SWPLUGIN_DEBUG__

#include "PiafFilter.h"
#include "swimage_utils.h"
#include "swvideodetector.h"

//#include "workshoptool.h"
#include <SwTypes.h>
#include <sys/time.h>

#include "swimage_utils.h"

#ifndef CONSOLE
//// Qt includes
#include <QMessageBox>
#endif

// standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef WINDOWS
#include <sys/wait.h>
#endif

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

//#include "videocapture.h"
#include "piaf-common.h"

#define CUSTOM_SEQ_FILE	SHM_DIRECTORY "customseq.flist"


bool g_debug_PiafSignalHandler = false;
bool g_debug_FilterSequencer = false;
bool g_debug_PiafFilter = false;

/// Delete a filter in manager (parameters and widgets)
#define DELETE_FILTER(_pv)	if((_pv)) { \
				delete (_pv); \
				(_pv) = NULL; \
			}


int PiafSignalHandler::registerChild(int pid, PiafFilter * filter) {

	fprintf(stderr, "PiafSignalHandler::registerChild(%d, '%s')...\n",
			pid, filter->exec_name);
	if(pid == 0) return 0;

	sigPiafFilter * sigF = new sigPiafFilter;
	sigF->pid = pid;
	sigF->pFilter = filter;

	filterList.append(sigF);
	return 1;
}


int PiafSignalHandler::killChild() {


	if(filterList.isEmpty()) {
		fprintf(stderr, "PiafSignalHandler::%s:%d : nothing in list => return 0 !\n", __func__, __LINE__);
		return 0;
	}
#ifndef WINDOWS
	while(waitpid(-1, NULL, WNOHANG) >0) {
		if(g_debug_PiafSignalHandler)  {
			fprintf(stderr, "PiafSignalHandler::%s:%d : waitpid>0 !\n", __func__, __LINE__);
		}
	}
#else
        /// \todo FIXME
#endif

	QList<sigPiafFilter *>::iterator it;
	for(it = filterList.begin(); it != filterList.end(); ++it) {
		sigPiafFilter * sigF = (*it);
		if(g_debug_PiafSignalHandler)  {
			fprintf(stderr, "PiafSignalHandler::%s:%d: Testing child '%s' pid=%d...\n",
				__func__, __LINE__,
				sigF->pFilter->exec_name, sigF->pid);
		}
#ifndef WINDOWS

		if( kill(sigF->pid, SIGUSR1)<0)
		{
			int err = errno;
			if(err == ESRCH) {
				if(g_debug_PiafSignalHandler) {
					fprintf(stderr, "!! !! !! !! KILLING CHILD %d ('%s') !! !! !! !!\n",
						sigF->pid, sigF->pFilter->exec_name);
				}

				fflush(stderr);
				int status = 0;

				int retWIFEXITED = WIFEXITED(status);
				int retWEXITSTATUS = WEXITSTATUS(status);
				if(g_debug_PiafSignalHandler) {
					fprintf(stderr, "\tExited with WIFEXITED %d\n", retWIFEXITED );
					fprintf(stderr, "\tExited with WEXITSTATUS %d\n", retWEXITSTATUS );
				}

				if(WIFSIGNALED(status)) {
				//	if(g_debug_PiafSignalHandler)
					fprintf(stderr, "\tExited because a signal were not caught\n");
				}

				sigF->pFilter->forceCloseConnection();
//				filterList.remove(sigF);
//				sigF->pFilter = NULL;
//				delete sigF;
			}

			return 1;
		}
#else
                /// \todo FIXME
#endif
	}

	if(g_debug_PiafSignalHandler) {
		fprintf(stderr, "PiafSignalHandler::%s:%d : WARNING: filter not found in list => return 0 !\n",
				__func__, __LINE__);
	}

	return 0;
}

int PiafSignalHandler::removeChild(int pid) {

	fprintf(stderr, "PiafSignalHandler::%s:%d childpid=(%d)...\n",
			__func__, __LINE__,	pid);
	if(filterList.isEmpty()) {
		return 0;
	}

	QList<sigPiafFilter *>::iterator it;
	for(it = filterList.begin(); it != filterList.end(); ++it) {
		sigPiafFilter * sigF = (*it);
		if(sigF->pid == pid) {
			fprintf(stderr, "!! !! !! !! REMOVING CHILD %d ('%s') !! !! !! !!\n",
				sigF->pid, sigF->pFilter->exec_name);
			filterList.removeOne(sigF);
			sigF->pFilter = NULL;
			delete sigF;

			return 1;
		}
	}

	return 0;
}



/* default constructor
*/

FilterSequencer::FilterSequencer()
{
	init();
}

FilterSequencer::~FilterSequencer()
{
	purge();
}

void FilterSequencer::init() {
#define BUFFER_SIZE 4096
	swAllocateFrame(&frame, BUFFER_SIZE);


	mPluginSequenceFile[0] = '\0';
	idEditPlugin = 0;
	idViewPlugin = 0;

	lockProcess = false;
	mNoWarning = false;
	mAutoReloadSequence = false; // do not reload plugins when user need to confirm or make modifications
}

void FilterSequencer::purge() {
	fprintf(stderr, "FilterSequencer::%s:%d DELETE FilterSequencer\n", __func__, __LINE__);

	unloadAllLoaded();

	clearImageTmpList();

	fprintf(stderr, "FilterSequencer::%s:%d DELETE FilterSequencer\n", __func__, __LINE__);

	unloadAllAvailable();
}

void FilterSequencer::unloadAllAvailable()
{
	DEBUG_MSG("unloading all available filters process...");

	// empty available filter list
	PiafFilter * lastF = NULL;

	if(!mAvailableFiltersList.isEmpty()) {
		QList<PiafFilter*>::iterator it;
		for(it = mAvailableFiltersList.begin(); it != mAvailableFiltersList.end(); ++it)
		{
			PiafFilter * filter = (*it);
			if(filter != lastF) {
//#ifdef __SWPLUGIN_MANAGER__
				fprintf(stderr, "%s:%d Deleting filter '%s' from available filterColl...\n",
						__func__, __LINE__,
						filter->exec_name);
				fflush(stderr);
				///#endif
				lastF = filter;
				filter->unloadChildProcess();
			}
		}
	}

}






/********************************************************************

					FILTER LIST MANAGEMENT

 ********************************************************************/

/* add a new filter in filters list.
		@param id -1 for end of list, else rank from 0
*/
PiafFilter *  FilterSequencer::addFilter(PiafFilter * newFilter, int id)
{
	PIAF_MSG(SWLOG_INFO, "FilterSequencer: adding filter %p at idx=%d\n",
			newFilter, id);
	clearImageTmpList(); // clear before next use

	if(id==-1)
	{
		mLoadedFiltersList.append( newFilter );


		idEditPlugin = mLoadedFiltersList.count()-1;
		idViewPlugin = idEditPlugin;

	} else {

		if(id > (int)mLoadedFiltersList.count()) {
			fprintf(stderr, "[FilterSequencer]::%s:%d : invalid id=%d > count=%d\n",
					__func__, __LINE__,
					id, (int)mLoadedFiltersList.count());
			return NULL;
		}
		else
		{
			mLoadedFiltersList.insert( (uint)id, newFilter );
		}
	}

	// Save the sequence here because the plugin won't
	// send its params since it already crashed
	saveSequence(SHM_DIRECTORY "crashedsequence.flist");

	PIAF_MSG(SWLOG_INFO, "[FilterSequencer]:start process '%s' func %d\n",
			newFilter->exec_name, newFilter->indexFunction);
	// start process
	newFilter->loadChildProcess();
	newFilter->setEnabled(true);
	setFinal(newFilter);

	connect(newFilter, SIGNAL(signalParamsChanged()), this, SLOT(slot_signalParamsChanged()));

	// save current list
	strcpy(mPluginSequenceFile, CUSTOM_SEQ_FILE);
	saveSequence(mPluginSequenceFile);

	return newFilter;
}

void FilterSequencer::slot_signalParamsChanged()
{
	// save current list in temp sequence file
	strcpy(mPluginSequenceFile, CUSTOM_SEQ_FILE);

	fprintf(stderr, "[FilterSequencer]::%s:%d : params changed ! save tmp sequence as '%s'\n",
			__func__, __LINE__, mPluginSequenceFile);

	saveSequence(mPluginSequenceFile);
}

// get filter pointer from id.
PiafFilter * FilterSequencer::getFilter(int id)
{
	return mLoadedFiltersList.at(id);
}

// read number of filters into list
int FilterSequencer::getFilterCount()
{
	return mLoadedFiltersList.count();
}

void FilterSequencer::slotFilterDied(int pid)
{
	if(mLoadedFiltersList.isEmpty()) return;

	DEBUG_MSG("[FilterSequencer]: dead filter pid=%d...\n",
			pid);

	QList<PiafFilter *>::iterator fil;
	for(fil = mLoadedFiltersList.begin();
		fil != mLoadedFiltersList.end(); ++fil)
	{
		PiafFilter * filter = (*fil);
		if(filter->getChildPid() == pid)
		{
			fprintf(stderr, "[PiafFilterMng]::%s:%d : dead filter pid=%d (but no warning)...\n",
					__func__, __LINE__, pid);
			filter->plugin_died = true;
			emit signalFilterDied(filter);
		}
	}

	// Signal that it changed
	PIAF_MSG(SWLOG_WARNING,
			 "[PiafFilterMng]: dead filter pid=%d => send filter changed...",
			 pid);
	emit selectedFilterChanged();
}


int FilterSequencer::removeFilter(int id)
{
	// unload child process
	PiafFilter * pv = mLoadedFiltersList.at((uint)id);
	if(!pv) {
		PIAF_MSG( SWLOG_WARNING, "filter not found at idx=%d",
				  id );
		return -1;
	}

	return removeFilter(pv);
}


int FilterSequencer::removeFilter(PiafFilter * filter)
{
	if(!filter) {
		PIAF_MSG( SWLOG_WARNING, "filter is null" );
		return -1; }

	// unload child process
	int id = mLoadedFiltersList.indexOf(filter);
	bool reset = false;

	if(id>=0 && filter)
	{
		if(id==idEditPlugin || id==idViewPlugin || id==(int)mLoadedFiltersList.count()-1)
		{
			fprintf(stderr, "[PiafFilterMng]::%s:%d : will reset\n", __func__, __LINE__);
			reset = true;
		}

		if(filter) {
			fprintf(stderr, "[PiafFilterMng]::%s:%d : unloadChildProcess...\n", __func__, __LINE__);
			filter->unloadChildProcess();
		} else {
			fprintf(stderr, "[PiafFilterMngr]::%s:%d: filter=NULL for pv=%p\n", __func__, __LINE__,
					filter);
		}

		if(g_debug_FilterSequencer) {
			fprintf(stderr, "%s:%d : removing from mLoadedFiltersList...\n", __func__, __LINE__);
		}
		mLoadedFiltersList.removeOne(filter);

		if(g_debug_FilterSequencer) {
			fprintf(stderr, "%s:%d : deleting...\n", __func__, __LINE__);
		}

		DELETE_FILTER(filter);

		if(g_debug_FilterSequencer) {
			fprintf(stderr, "%s:%d : deleted.\n", __func__, __LINE__);
		}

		if(reset) {
			if(!mLoadedFiltersList.isEmpty()) {
				idEditPlugin = mLoadedFiltersList.count()-1;
				idViewPlugin = idEditPlugin;
			} else {
				idViewPlugin = -1;
				idEditPlugin = -1;
			}
		}

		emit selectedFilterChanged();
		return id;
	} else {
		fprintf(stderr, "[PiafFiler]::%s:%d : filter %p='%s' not found\n",
				__func__, __LINE__,
				filter, filter?filter->exec_name : "null"
				);
		if(filter) {
			fprintf(stderr, "[PiafFilterMng]::%s:%d : unloadChildProcess...\n", __func__, __LINE__);
			filter->unloadChildProcess();
		}
		if(g_debug_FilterSequencer) {
			fprintf(stderr, "%s:%d : '%s' deleted.\n", __func__, __LINE__,
					filter?filter->exec_name : "null");
		}
	}
	emit selectedFilterChanged();
	return -1;
}



/* ------------------- Filter loading ---------------------
*/
int FilterSequencer::loadFilters()
{
	// unload loaded ones
	PIAF_MSG(SWLOG_INFO, "Reload filters : 1) unload all availables...");
	unloadAllAvailable();

	PIAF_MSG(SWLOG_INFO, "Reload filters : 2) all available filter in file settings...");
	DEBUG_MSG("Loading filters in '%s'", BASE_DIRECTORY "filters/list");

	//list all executable in directory /usr/local/piaf/filters/list
	FILE * f = fopen(BASE_DIRECTORY "filters/list", "r");

	if(!f) {

		char home[512] = "/home";
		if(getenv("HOME")) {
			strcpy(home, getenv("HOME"));
		}

		strcat(home, "/.piaf_plugins");
		f = fopen(home, "r");
		if(!f) {
#ifndef CONSOLE

			QMessageBox::critical(NULL,
				tr("Filter Error"),
				tr("Cannot find filter list, check installation."),
				QMessageBox::Abort,
				QMessageBox::NoButton,
				QMessageBox::NoButton);
#else
			PIAF_MSG(SWLOG_ERROR, "Cannot find filter list, check installation");
#endif
			return 0;
		}
		PIAF_MSG(SWLOG_INFO, "Loading files from '%s'", home);
	}
	else
	{
		PIAF_MSG(SWLOG_INFO, "Loading files from " BASE_DIRECTORY "filters/list");
	}

	// read each filename and load file mesures
	char line[512]="";

	while(!feof(f))
	{
		char * ret = fgets(line, 511, f);
		// read executable name
		if(ret && line[0] != '#') {
			// last char should be '\n'
			if( line[strlen(line)-1]== '\n')
			{
				line[strlen(line)-1] = '\0';
			}

			// ok, we've got the name, now create filters from this name
			if(strlen(line)>0) {
				char * arg = nextTab(line);
				if(arg) {
					char *stepper = arg-1;
					while( stepper > line && (*stepper == ' ' || *stepper == '\t')) {
						*stepper = '\0';
						stepper--;
					}

					if(*arg != '\t' && *arg != '\n' && *arg != '\0')
					{
						*(arg-1) = '\0';
						//printf("Line='%s' a bIcon='%s'\n", line, arg);
						PIAF_MSG(SWLOG_INFO, "\t[FilterSequencer] loading filter '%s'\n",
								line);
						loadFilter(line);
					}
					else {
						PIAF_MSG(SWLOG_INFO, "\t[FilterSequencer] loading filter '%s'\n",
								line);
						loadFilter(line);
					}
				} else {
					char *stepper = line+strlen(line)-1;
					while( stepper > line && (*stepper == ' ' || *stepper == '\n' || *stepper == '\t')) {
						*stepper = '\0';
						stepper--;
					}

					PIAF_MSG(SWLOG_INFO, "\t[FilterSequencer] loading filter '%s'\n",
							line);
					loadFilter(line);
				}
			}

		}
		line[0] = '#';
	}

	fclose(f);
	return 1;

}

int FilterSequencer::loadFilter(char *filename)
{
	// fork and ask plugin
	fprintf(stderr, "[PiafFilterMng]::%s:%d : Loading filter file '%s'...\n", __func__, __LINE__, filename);
	FILE * f = fopen(filename, "rb");

	if(!f) {
		fprintf(stderr, "[PiafFilterMng]::%s:%d : Filter file '%s' does not exist !!\n", __func__, __LINE__, filename);
		return 0;
	}
	fclose(f);

	// allocate first filter
	PiafFilter * newFilter = new PiafFilter(filename,
			false /* kill process after loading properties */);
	connect(newFilter, SIGNAL(signalDied(int)), this, SLOT(slotFilterDied(int)));


	mAvailableFiltersList.append(newFilter);
	return 1;
}


// Copy a filter and append its function to process list. Now, it reloads the process.
PiafFilter * FilterSequencer::addFilter(PiafFilter * filter)
{
	if(!filter) {
		DEBUG_MSG("null filter");
		return NULL;
	}


	PiafFilter * newFilter = new PiafFilter(filter);
	fprintf(stderr, "FilterSequencer::%s:%d : adding filter %p at end\n",
			__func__ ,__LINE__, newFilter);

	// start process
	newFilter->loadChildProcess();

	// append to list
	mLoadedFiltersList.append(newFilter);
	setFinal(newFilter);

	connect(newFilter, SIGNAL(signalDied(int)), this, SLOT(slotFilterDied(int)));
	connect(newFilter, SIGNAL(signalParamsChanged()), this, SLOT(slot_signalParamsChanged()));
//signalParams
//FIXME	newFilter->enabled = true;

	fprintf(stderr, "[FilterSequencer]::%s:%d : ADDED!!!\n", __func__, __LINE__);
	emit selectedFilterChanged();

	return newFilter;
}

void FilterSequencer::update()
{
	emit selectedFilterChanged();
}

/* Set final plugin in sequence: the next plugins won't be processed */
void FilterSequencer::setFinal(PiafFilter * filter)
{
	QList<PiafFilter *>::iterator it;
	for(it=mLoadedFiltersList.begin();
		it!=mLoadedFiltersList.end();  // use !found to restart at every iteration
		++it)
	{
		PiafFilter * pv = (*it);
		if( pv )
		{
			pv->setFinal((pv == filter));
		}
	}
	filter->setFinal(true);

	emit selectedFilterChanged();
}

void FilterSequencer::unloadAllLoaded()
{
//	if(g_debug_FilterSequencer) {
	fprintf(stderr, "FilterSequencer::%s:%d: unloading all %d loaded plugins...\n", __func__,__LINE__,
			mLoadedFiltersList.count());
//	}

	// looking for filter plugin
	if(!mLoadedFiltersList.isEmpty())
	{
		QList<PiafFilter *>::iterator it;

		for(it=mLoadedFiltersList.begin();
			it!=mLoadedFiltersList.end();
			++it)
		{
			PiafFilter * pv = (*it);
			fprintf(stderr, "\tDelete loaded? filter %p = '%s' \n",
					pv,
					(pv ? pv->exec_name : "(null)") );
			if( pv && pv->loaded() )
			{
				// then add it to selectedListView
//#ifdef __SWPLUGIN_MANAGER__
				if(pv->indexFunction < pv->nb_func) {
					fprintf(stderr, "\tDelete filter '%s' \n",
						pv->funcList[pv->indexFunction].name);
				}
//#endif
				// wait for process flag to go down
				int retry = 0;
				while(lockProcess && retry < 10000) {
					fprintf(stderr, "\tDelete filter '%s': process is locked\n",
						pv->funcList[pv->indexFunction].name);
					retry += 10;
					usleep(10000);
				}

				removeFilter( pv );
			}
		}
	}

	if(g_debug_FilterSequencer) {
		fprintf(stderr, "FilterSequencer::%s:%d: unload of all plugins DONE\n", __func__,__LINE__);
	}
	emit selectedFilterChanged();
}


/*
	move ONE item up for one step
	*/
int FilterSequencer::moveUp(PiafFilter * filter)
{
	// change order into selectedFiltersList
	// change order into selectedFiltersList
	// looking for filer plugin


	int idx = mLoadedFiltersList.indexOf(filter);
	if(idx < 0)
	{
		fprintf(stderr, "FilterSequencer::%s:%d: item not found, cannot move up !\n",
				__func__, __LINE__);
		return -1;
	}
	if(idx == 0) {
		// Cannot move up
		fprintf(stderr, "FilterSequencer::%s:%d: already first item, cannot move up !\n",
				__func__, __LINE__);
		return 0;
	}

	// remove then insert at next place
	mLoadedFiltersList.removeOne(filter);
	mLoadedFiltersList.insert(idx-1, filter);

	idEditPlugin = mLoadedFiltersList.indexOf(filter);
// FIXME  : check new item index
	fprintf(stderr, "FilterSequencer::%s:%d : new idx=%d\n", __func__, __LINE__,
			idEditPlugin);

	emit selectedFilterChanged();
	return 0;

}
/*
	move ONE item up for one step
	*/
int FilterSequencer::moveDown(PiafFilter * filter)
{
	// change order into selectedFiltersList
	// change order into selectedFiltersList
	// looking for filer plugin

	int idx = mLoadedFiltersList.indexOf(filter);
	if(idx < 0)
	{
		fprintf(stderr, "FilterSequencer::%s:%d: item not found, cannot move up !\n",
				__func__, __LINE__);
		return -1;
	}
	if(idx == mLoadedFiltersList.count()) {
		// Cannot move up
		fprintf(stderr, "FilterSequencer::%s:%d: already first item, cannot move up !\n",
				__func__, __LINE__);
		return 0;
	}
///  \bug FIXME index mismatch
	// remove then insert at next place
	PIAF_MSG(SWLOG_INFO, "Remove @ index %d", idx);
	mLoadedFiltersList.removeAt(idx);
	// insert at same place because it's been removed, so the next index will be
	// one item after
	PIAF_MSG(SWLOG_INFO, "Insert @ index %d", idx+1);
	mLoadedFiltersList.insert(idx+1, filter);

	idEditPlugin = mLoadedFiltersList.indexOf(filter);;
	fprintf(stderr, "FilterSequencer::%s:%d : moved to %d\n",
			__func__, __LINE__, idEditPlugin);
	emit selectedFilterChanged();
	return 0;

}




void FilterSequencer::toggleFilterDisableFlag(PiafFilter * filter)
{
	idEditPlugin = mLoadedFiltersList.indexOf(filter);

	if(idEditPlugin<0) {
		fprintf(stderr, "Disable filter: cannot find filter.\n");
		return;
	}

	filter->enabled = !(filter->enabled);


	printf("ENABLE/DISABLE!!!\n");
	emit selectedFilterChanged();
}





/* slotSave : saves filters configuration into a script file for later opening
Format is simple : exec name and function index are needed, then parameters,
	which are simply read from plugin.
*/
#define FILTERLIST_EXTENSION ".flist"

int FilterSequencer::saveSequence(char * filename)
{
	PIAF_MSG(SWLOG_INFO, "[FilterSequencer] saving('%s')...\n",
			 filename);

	if(!strstr(filename, FILTERLIST_EXTENSION)) {
		strcat(filename, FILTERLIST_EXTENSION);
	}

	if(mLoadedFiltersList.isEmpty()) {
		PIAF_MSG(SWLOG_WARNING, "no filter in list => save nothing");
		 return -1;
	 }

	PIAF_MSG(SWLOG_INFO, "[FilterSequencer] Save filter list into file '%s'",
			 filename);

	// process each filter
	FILE * f = fopen(filename, "w");
	if(!f)
	{
		int errnum = errno;
		PIAF_MSG(SWLOG_ERROR, "[FilterSequencer] error while opening file '%s' "
				 "for writing: %d='%s'\n",
				filename, errnum, strerror(errnum)
				);
		return -errnum;
	}

	QList<PiafFilter *>::iterator it;
	int idx = 0;
	for(it = mLoadedFiltersList.begin(); it != mLoadedFiltersList.end(); ++it, ++idx) {
		PiafFilter * filter = (*it);
		// process
		PIAF_MSG(SWLOG_INFO, "[FilterSequencer]: saving filter exec='%s' "
				 "/ func[%d]='%s' \n",
				 filter->exec_name,
				 filter->indexFunction, filter->funcList[ filter->indexFunction].name
				 );

		fprintf(f, "%s\t%d\n",
				filter->exec_name,
				filter->indexFunction);

		// --- read parameters from plugin
		// send request and wait for answer
		filter->sendRequest(SWFRAME_ASKFUNCTIONDESC);

		// then save reply string
		char txt[1024]="";
		if(filter->pipeR) {
			char * ret = fgets(txt, 1023, filter->pipeR);
			if(ret)
			{
				PIAF_MSG(SWLOG_INFO, "[FilterSequencer]: read params "
						"from plugin [%d]:'%s' : params='%s'\n",
						idx, filter->name(),
						txt);
				fprintf(f, "%s", txt);
			}
			else
			{
				PIAF_MSG(SWLOG_ERROR, "[FilterSequencer]: could not read parameters answer "
						"from plugin [%d]:'%s'\n",
						idx, filter->name());
			}
		}
	}

	fclose(f);



	return 0;
}


int FilterSequencer::loadFilterList(char * filename)
{
	return loadSequence(filename);
}

/* read configuration file for filters order
*/
int FilterSequencer::loadSequence(char * filename)
{
	char fname[1024];
	strcpy(fname, (char *)filename);
	if(!strstr(fname, FILTERLIST_EXTENSION)) {
		strcat(fname, FILTERLIST_EXTENSION);
	}

	PIAF_MSG(SWLOG_INFO, "[FilterSequencer %p]: Loading file '%s'...\n", this, filename);
	// process each image

	FILE * f = fopen(fname, "r");
	if(!f) {
		int errnum = errno;
		PIAF_MSG(SWLOG_ERROR, "[FilterSequencer %p]: cannot load file '%s'. Err=%d='%s'",
				 this, filename,
				 errnum, strerror(errnum)
				 );
		return -1;
	}

	if(mPluginSequenceFile != filename) {
		strcpy(mPluginSequenceFile, filename);
	}

	//
	char line[512] = "";
	while(!feof(f))
	{
		// read exec name and function index
		char * retline = fgets(line, 512, f);
		if(retline) {
			// check if filter is available
			PiafFilter * foundPV = NULL;
			if(mAvailableFiltersList.isEmpty())
			{
				PIAF_MSG(SWLOG_INFO, "Loading available plugins list ...");
				loadFilters();
			}

			PIAF_MSG(SWLOG_INFO, "Loading available plugins list ...");
			if(!mAvailableFiltersList.isEmpty())
			{
				int retry = 0;
				while(foundPV == NULL && retry < 2) // Two chances to load the filter
				{
					retry++;
					QList<PiafFilter *>::iterator it;
					for(it = mAvailableFiltersList.begin();
							it!=mAvailableFiltersList.end() && !foundPV; ++it)
					{
						PiafFilter * filter = (*it);
						if(filter) {
							if(strncmp(filter->exec_name, line, strlen((filter->exec_name)))==0) {
								foundPV = filter;
								PIAF_MSG(SWLOG_INFO, "\t[FilterSequencer %p]: FOUND filter for filter=%p\n",
										this, filter);
							}
						} else {
							PIAF_MSG(SWLOG_ERROR, "\t[FilterSequencer %p]: no filter for filter=%p\n",
									this, filter);
						}
					}

					if(!foundPV) // load directly the file into available plugins list
					{ // because the plugin file may exist and is no more in the list handled by the GUI,
						// so it is not loaded by loadFilters()
						char filename[1024];
						strcpy(filename, line);
						// split by tab
						char * tab = strstr(filename, "\t");
						if(tab)
						{
							*tab = '\0';
						}
						PIAF_MSG(SWLOG_WARNING, "plugin '%s' is not in available list. "
								 "Try to load it directly '%s' ...",
								 line, filename);
						loadFilter(filename);
					}
				}
			}
			else {
				PIAF_MSG(SWLOG_ERROR, "Available plugin list is not loaded");
			}


			int id = -1;
			if(!foundPV) {
				PIAF_MSG(SWLOG_ERROR, "\t[FilterSequencer %p]: Filter exec '%s' "
						 "seems not to be unknown.\n",
						this, line);
				QString errMsg = tr("Filter binary '")+line+ tr("' seems not to be unknown.");
#ifndef CONSOLE
				QMessageBox::critical(NULL, tr("Plugin loading error"),
									  errMsg
									  );
#endif
				emit signalPluginError(errMsg);

			} else {
				// read function index
				char * pid = nextTab(line);
				if(pid)
				{
					sscanf(pid, "%d", &id);
					if(id >= foundPV->nbfunctions()) {
#ifdef __SWPLUGIN_DEBUG__
					fprintf(stderr, "slotLoad: read function index for filter from exec '%s' is superior to function number %d\n",
						foundPV->exec_name, foundPV->nbfunctions());
#endif
						id = -1;
					}
					else
					{

					}
				}
				else {
					PIAF_MSG(SWLOG_ERROR, "\t[FilterSequencer %p]: Cannot read function index for filter from exec '%s'\n",
							this,
							foundPV->exec_name);
				}
			}

			// read parameters
			line[0] = '\0';
			fgets(line, 512, f);

			// after checking load filter
			if(id>=0 && foundPV)
			{
				PiafFilter * newPV = new PiafFilter( foundPV );// keep it alive
				connect(newPV, SIGNAL(signalDied(int)), this, SLOT(slotFilterDied(int)));

				newPV->indexFunction = id;
				newPV->enabled = true;

				// add filter to collection
				addFilter(newPV, -1);

#ifdef __SWPLUGIN_DEBUG__
				PIAF_MSG(SWLOG_INFO, "\t[FilterSequencer %p]: Loaded filter '%s'",
						 this,
						 newPV->filter->funcList[newPV->indexFunction].name);
#endif

				// send parameters
				if(newPV->pipeW) {
					// Send params to filter
					fprintf(newPV->pipeW, "%s", line);
				}
			}
		}
	}

	fclose(f);

	// Emit signal to refresh GUI
	emit selectedFilterChanged();
	return 0;
}








/******************************************************************
IMAGE PROCESSING  SECTION
*/


void FilterSequencer::clearImageTmpList()
{
	QList<IplImage *>::iterator it;
	for(it = imageTmpList.begin(); it != imageTmpList.end(); it++)
	{
		///< Temporary image for storing different steps of plugin sequence
		IplImage * img = (*it);
		swReleaseImage(&img);
	}
	// Remove all items
	imageTmpList.clear();
}

int FilterSequencer::processImage(IplImage * imageIn, IplImage ** pimageOut,
								  u32 date, u32 tick)
{
	if(!imageIn)
	{
		PIAF_MSG(SWLOG_ERROR, "[FilterSequencer %p]: ERROR: input image is NULL", this);
		return -1;
	}

	int deltaTus = 0;
	if(g_debug_FilterSequencer)
	{
		fprintf(stderr, "[FilterSequencer]::%s:%d: process image ! swImageStruct=%p = %dx%dx%dx%d\n",
				__func__, __LINE__,
				imageIn,
				imageIn->width, imageIn->height, imageIn->depth, imageIn->nChannels);
	}

	PIAF_MSG(SWLOG_TRACE, "Process image IplImage *=%p = %dx%dx%dx%d date=%lu.%lu",
			 imageIn,
			 imageIn->width, imageIn->height, imageIn->depth, imageIn->nChannels,
			 (ulong)date, (ulong)tick);


	if(mLoadedFiltersList.isEmpty()) {
		PIAF_MSG(SWLOG_TRACE, "[FilterSequencer %p]: ERROR: filter list is empty", this );
		return -1;
	}

	// process each image
	int step = 0;
	int ret = 1;

	int global_return = 0;
	QList<PiafFilter *> crashedPlugins;

	if(lockProcess) {
		fprintf(stderr, "[FilterSequencer]::%s:%d : process is locked !! do not process\n",
				__func__, __LINE__);

	} else {
		lockProcess = true;

		ret = 1;
		int id=0;

		// Time accumulation in us
		float total_time_us = 0.f;
		if(g_debug_FilterSequencer)
		{
			fprintf(stderr, "[FilterSequencer]::%s:%d : processing %d filters...\n",
					__func__, __LINE__,
					mLoadedFiltersList.count());
		}

		IplImage * imagePrev = imageIn;
		if(!mLoadedFiltersList.isEmpty())
		{

			bool finalreached = false;
			QList<PiafFilter *>::iterator it;
			int filtidx = 0;
			for(it = mLoadedFiltersList.begin();
				it != mLoadedFiltersList.end()
					&& ret && !finalreached;
					++it, ++filtidx )
			{
				IplImage * imageOut = NULL;

				if(filtidx >= imageTmpList.count())
				{

				} else {
					imageOut = imageTmpList.at(filtidx);
				}
				PiafFilter * pv = (*it);
				finalreached |= pv->isFinal();

				// process
				if( !pv->enabled ) // not enabled = just copy output
				{
					if(imageOut && (imageOut->height != imagePrev->height
									|| imageOut->widthStep != imagePrev->widthStep )
							)
					{
						swReleaseImage(&imageOut);
					}

					if(!imageOut)
					{
						imageOut = cvCloneImage(imagePrev);
					}
					else
					{
						cvCopy(imagePrev, imageOut );
					}
				} else {
//					if(g_debug_FilterSequencer)
					{
						PIAF_MSG(SWLOG_DEBUG, "[FilterSequencer]: processing "
								"filter [%d] func[%d]='%s' : in =%p:%dx%dx%dx%d "
								"-> out=%p\n",
								filtidx,
								pv->indexFunction,
								pv->funcList[pv->indexFunction].name,
								imageIn, imageIn->width, imageIn->height, imageIn->depth, imageIn->nChannels,
								imageOut
								);
					}

					ret = pv->processFunction(pv->indexFunction,
											  imagePrev,
											  &imageOut, 2000,
											  date, tick);
					if(ret>=0)
					{
						step++;

						total_time_us += ret;

						// time statistics
						pv->setTimeUS( ret );
					}
					else
					{	// Processing has one error
						global_return --;
						crashedPlugins.append(pv);

						PIAF_MSG(SWLOG_ERROR, "[FilterSequencer]: filter [%d] '%s' failed "
								"=> will return global=%d\n",
								filtidx, pv->exec_name,
								global_return
								);
					}
				}

				if(imageOut)
				{
					imagePrev = imageOut;
					if(filtidx >= imageTmpList.count())
					{
						// Append this image
						PIAF_MSG(SWLOG_INFO,
								 "Append image pointer for filter [%d] -> %p",
								 filtidx,
								 imageOut);
						imageTmpList.append(imageOut);
					}
					else // check if the pointer has changed its size
					{
						if(imageOut != imageTmpList.at(filtidx))
						{
							PIAF_MSG(SWLOG_INFO, "Pointer for filter [%d] changed : %p -> %p",
									 filtidx,
									 imageTmpList.at(filtidx), imageOut);
							// Remove and reinsert
							imageTmpList.removeAt(filtidx);
							imageTmpList.insert(filtidx, imageOut);
						}
					}
				}

			}

			id++;
		}

		lockProcess = false;

		if(pimageOut)
		{
			*pimageOut = imagePrev;
		}

		// store accumulated time in output image
		deltaTus = total_time_us;
	}


	if(global_return < 0) {
		if(mAutoReloadSequence)
		{
			fprintf(stderr, "[FilterSequencer]::%s:%d : a plugin failed & auto-reload is on "
					"=> unload then reload plugin sequence '%s'\n",
					__func__, __LINE__,
					getPluginSequenceFile());

			// unload previously loaded filters
			unloadAllLoaded();

			// reload same file
			loadSequence(getPluginSequenceFile());
		}
		else
		{
			fprintf(stderr, "[FilterSequencer]::%s:%d : a plugin failed & auto-reload is OFF\n",
					__func__, __LINE__
					);
#ifndef CONSOLE
			int answer = QMessageBox::question(NULL, tr("A plugin crashed"),
											   tr("A plugin in sequence has crashed. "
												  "Do you want to reload the whole sequence ? If you answer 'No', only the plugins with success will be restored"),
												  QMessageBox::Yes, QMessageBox::No);
			if(answer == QMessageBox::Yes)
			{
				// Too late to save the sequence here because the plugin won't
				// send its params since it already crashed
				//saveSequence(SHM_DIRECTORY "crashedsequence.flist");

				// unload previously loaded filters
				unloadAllLoaded();

				// reload same file
				loadSequence(getPluginSequenceFile());
			}
			else
#else
			// no dot reload
#endif
			{
				QList<PiafFilter *>::iterator pvit;
				for(pvit = crashedPlugins.begin();
					pvit != crashedPlugins.end(); ++pvit)
				{
					PiafFilter * deadpv = (*pvit);
					fprintf(stderr, "[Sequence]::%s:%d: removing crashed plugin '%s'\n",
							__func__, __LINE__, deadpv->name());
					mLoadedFiltersList.removeOne(deadpv);
				}
			}
		}
	}
	else { // on succes, signal that the processing is done so the widtgets can be updated
		emit signalProcessingDone();
	}

	if(global_return >= 0)
	{
		return deltaTus;
	}
	return global_return;
}












/**************************************************************************
 *                                                                        *
 *                                                                        *
 *                             PiafFilter                                   *
 *                                                                        *
 *                                                                        *
 **************************************************************************/
PiafFilter::PiafFilter(PiafFilter * filter)
{
	init();

	// Copy, so keep it alive
	mKeepAlive = true;

	// load its properties
	loadBinary(filter->exec_name);

	// Copy function index
	indexFunction = filter->indexFunction;
	DEBUG_MSG("PIAF FILTER: Created copy of filter '%s' with activated func = %d",
			  filter->exec_name, indexFunction
			  );
	if(indexFunction >= nb_func)
	{
		DEBUG_MSG("Invalid index function=%d >= nb_func=%d => disactivate func", indexFunction, nb_func);
		indexFunction = -1;
	}
}

PiafFilter::PiafFilter(char * filename, bool keepAlive)
{
	init();

	mKeepAlive = keepAlive;

	// load its properties
	loadBinary(filename);
}

void PiafFilter::loadBinary(char * filename)
{
	// store filename
	exec_name = new char [ strlen(filename)+2 ];
	strcpy(exec_name, filename);
	exec_name[ strlen(filename) ]='\0';

	// --- reads plugin properties
	swAllocateFrame(&frame, 1024);

	if(!loadChildProcess()) {
		fprintf(stderr, "[PiafFilter]::%s:%d : could not load process '%s'. Return.\n",
				__func__, __LINE__, exec_name);
		return;
	}


	// reads category and subcategory
	sendRequest(SWFRAME_ASKCATEGORY);

	// Read functions list
	sendRequest(SWFRAME_ASKFUNCLIST);

	int iter=0;
	nb_func=0;
	while(iter<10 && nb_func==0) {
		iter++;
		waitForAnswer(100);
	}

	// if wants to keep it alive
	if(!mKeepAlive) {
		// unload process
		unloadChildProcess();
	}
}


void PiafFilter::init()
{
	indexFunction = -1;
	exec_name = NULL;
	category = NULL;
	subcategory = NULL;
	mKeepAlive = false;

	memset(&data_in, 0, sizeof(swImageStruct));
	memset(&data_out, 0, sizeof(swImageStruct));

	enabled = false;
	final = false;
	plugin_died = false;

	// functions
	funcList = NULL;
	nb_func = 0;
	childpid = 0;

	comLock = false;
	buffer = new char [BUFFER_SIZE];
	memset(buffer, 0, sizeof(BUFFER_SIZE));

	memset(&mTimeHistogram, 0, sizeof(t_time_histogram) );

	// pipes initialisation
	pipeIn[0]  = 0;
	pipeIn[1]  = 0;
	pipeOut[0] = 0;
	pipeOut[1] = 0;

	pipeStderrIn[0]  = 0;
	pipeStderrIn[1]  = 0;
	pipeStderrOut[0] = 0;
	pipeStderrOut[1] = 0;

	pipeR = NULL;
	pipeW = NULL;
	pipeStderrR = NULL;

	statusOpen = false;
}

/** @brief Return function descriptor */
swFunctionDescriptor PiafFilter::getFunction(int idx)
{
	if(idx <0 || idx >= nb_func || !funcList)
	{
		swFunctionDescriptor empty;
		memset(&empty, 0, sizeof(swFunctionDescriptor ));
		return empty;
	}

	return funcList[idx];
}

PiafFilter::~PiafFilter()
{
	destroy();
}

int PiafFilter::destroy()
{
	fprintf(stderr, "[PiafFilter]::%s:%d : DESTROYing PiafFilter %p='%s'...\n",
			__func__, __LINE__, this, exec_name);

	if(!exec_name)
	{
		return 0;
	}
	if(exec_name[0] == '\0')
	{
		return 0;
	}

	unloadChildProcess();

	deleteHistogram(&mTimeHistogram);
	swFreeFrame(&frame);

	freeSwImage(&data_in);
	freeSwImage(&data_out);


	if(buffer) { delete [] buffer; buffer = NULL; }
	if(exec_name) {	delete [] exec_name; exec_name = NULL; }
	if(category) { delete [] category; category = NULL; }
	if(subcategory) { delete [] subcategory;subcategory = NULL; }

	if(nb_func)
	{
		for(int i = 0; i < nb_func; i++)
		{
			// destroy it
			if( funcList[i].nb_params && funcList[i].param_list ) {
				for(int j=0; j< funcList[i].nb_params; j++) {
					if(funcList[i].param_list[j].name) {
						delete [] funcList[i].param_list[j].name;
						funcList[i].param_list[j].name = NULL;
					}
				}

				delete [] funcList[i].param_list;
				funcList[i].param_list = NULL;
			}

			if( funcList[i].name) {
				delete [] funcList[i].name;
				funcList[i].name = NULL;
			}
		}

		if( funcList ) {
			delete [] funcList;
			funcList = NULL;
		}
	}
	if(pipeR) {
		fclose(pipeR);
	}
	pipeR = NULL;
	if(g_debug_PiafFilter) {
		fprintf(stderr, "[PiafFilter]::%s:%d : DESTROYed PiafFilter %p.\n", __func__, __LINE__, this);
	}
	return 1;
}
// signal handling
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#ifdef PIAF_LEGACY
#include "swsignalhandler.h"
#include "SwFilters.h"
SwSignalHandler SigHandler;
int registerChildSig(int pid, SwFilter * filter)
{
	//
	fprintf(stderr, "!! !! !! !! REGISTERING PROCESS %d FOR FILTER %s !! !! !! !!\n",
			pid, filter->exec_name);
	SigHandler.registerChild(pid, filter);
	return 0;
}
#endif

PiafSignalHandler piafSigHandler;






int registerChildSig(int pid, PiafFilter * filter)
{
	//
	fprintf(stderr, "!! !! !! !! REGISTERING PROCESS %d FOR FILTER %s !! !! !! !!\n",
		pid, filter->exec_name);
	return piafSigHandler.registerChild(pid, filter);
}

int removeChildSig(int pid)
{
	//
	fprintf(stderr, "!! !! !! !! REMOVING PROCESS %d !! !! !! !!\n",
		pid);
#ifdef PIAF_LEGACY
	SigHandler.removeChild(pid);
#endif
	piafSigHandler.removeChild(pid);
	return 0;
}

void sigchld(int pid)
{
	fprintf(stderr, "Received signal SIGCHLD = %d\n", pid);
	piafSigHandler.killChild();
}
void sigpipe(int pid)
{
	fprintf(stderr, "Received signal SIGPIPE = %d\n", pid);
	piafSigHandler.killChild();
}
void sigusr1(int pid)
{
	fprintf(stderr, "Received signal SIGUSR1 = %d\n", pid);
}


// when pipe is broken
int PiafFilter::forceCloseConnection() {
	fprintf(stderr, "PiafFilter::forceCloseConnection : closing write pipe...\n");
	fflush(stderr);

//	if(pipeW)
//		fclose(pipeW);
	pipeW = NULL;

	fprintf(stderr, "PiafFilter::forceCloseConnection : closing read pipe to child %d...\n",childpid );
	fflush(stderr);

	// CSE : 2011-05-30 : was commented
	plugin_died = true;

	removeChildSig(childpid);
	statusOpen = false;

	childpid = 0;
	return 1;
}

/* Function waitForAnswer. Wait for an answer from plugin then treat it
	@author CSE
*/
int PiafFilter::waitForAnswer(int timeout)
{
	if(!buffer)
	{
		PIAF_MSG(SWLOG_ERROR, "No buffer for output ! (buffer=NULL)");
		return -1;
	}

#define TIMEOUT_STEP 20000
	if(timeout == 0) {
		timeout = 1000; // millisec
	}
	int iter = 0, maxiter = timeout * 1000 / TIMEOUT_STEP;

	while(iter < maxiter && buffer)
	{
		iter++;
		// Waiting for answer :
		buffer[0] = '\0';
#ifdef __SWPLUGIN_DEBUG__
		printf("PARENT : waiting for reply...\n");
#endif
		if(pipeR) {
			char * ret = fgets(buffer, BUFFER_SIZE, pipeR);
			if(ret)
			{
	#ifdef __SWPLUGIN_DEBUG__
				printf("PARENT : Read '%s'\n", buffer);
	#endif
				buffer[strlen(buffer)] = '\0';
				treatFrame(buffer, strlen(buffer));
				return 1;
			}
			else
			{
				usleep(TIMEOUT_STEP);
			}
		}
	}
	return 0;
}



/* Function loadChildProcess. Loads plugin process.
	@author CSE
*/
int PiafFilter::loadChildProcess()
{
	if(!exec_name) {
		fprintf(stderr, "PiafFilter::%s:%d : exec_name is null\n", __func__, __LINE__);
		return 0;
	}

	if(statusOpen) {
		fprintf(stderr, "PiafFilter::%s:%d : exec_name='%s' already opened\n",
				__func__, __LINE__, exec_name);
		return 0;
	}

	// Go to this directory
	char path[512]="";
	strcpy(path, exec_name);
	int pos = strlen(path)-1;
	bool found = false;
	do {
		if(path[pos] == '/' || path[pos] == '\\') {
			found = true;
			path[pos] = '\0';
		} else {
			pos --;
		}
	} while(!found && pos>0);

	if(g_debug_PiafFilter) {
		fprintf(stderr, "PiafFilter::%s:%d : file='%s' => path='%s'\n", __func__, __LINE__,
			exec_name, path );
	}

	// fork and
	// Commmunication processing
#ifndef WINDOWS
	if( pipe( pipeOut ) == -1 || pipe( pipeIn ) == -1)
	{
		int errnum = errno;
		fprintf(stderr, "PiafFilter::%s:%d : Error in pipe() : err=%d='%s'\n",
				__func__, __LINE__, errnum, strerror(errnum));
	}

	else
	{
		struct timeval tv1, tv2;
		gettimeofday(&tv1, NULL);
		childpid = fork();
		gettimeofday(&tv2, NULL);
		fprintf(stderr, "PiafFilter::%s:%d : FORK: %03d.%03d / %03d.%03d\n",
				__func__, __LINE__,
				(int)tv1.tv_sec%1000, (int)tv1.tv_usec/1000,
				(int)tv2.tv_sec%1000, (int)tv2.tv_usec/1000
				);

		switch(childpid)
		{
		case -1: // error
				perror("fork() error !!");
				break;
		case 0 : // child process

			printf("CHILD PROCESS =====> %s / PID=%d\n",
				   exec_name, getpid());

			// Child to Parent
			close( pipeIn[0] );

			if( dup2(pipeIn[1], STDOUT_FILENO) ==-1)
			{
				perror("Child C2P dup2() error");
			}
			close( pipeIn[1]);

			// Parent to Child
			close( pipeOut[1] );
			if( dup2(pipeOut[0], STDIN_FILENO)==-1)
			{
				perror("Child C2P dup() error");
			}

			// now close STDIN
			close( pipeOut[0]);

			chdir(path);

			if(execv(exec_name, NULL) == -1)
			{
				int errnum = errno;
				PIAF_MSG(SWLOG_ERROR, "Child execv('%s') error %d='%s'.\n",
						 exec_name, errnum, strerror(errnum));
				exit(EXIT_FAILURE);;
			}
			exit(EXIT_SUCCESS);

			break;
		default : // parent process = Piaf
			// Child to Parent
			PIAF_MSG(SWLOG_INFO, " PARENT PROCESS ====>\n");
			close( pipeIn[1] );

			// set non blocking
			//fcntl(pipeIn[0], F_SETFL, fcntl(pipeIn[0], F_GETFL) | O_NONBLOCK);

			pipeR = fdopen( pipeIn[0], "r");
			if( pipeR == NULL )
			{
				int errnum = errno;
				PIAF_MSG(SWLOG_ERROR, "PiafFilter: ERROR: unable to open pipeR err=%d='%s'\n",
						errnum, strerror(errnum));
				return 0;
			}

			// Parent to Child
			close( pipeOut[0] );

			// set non blocking
			//fcntl(pipeOut[1], F_SETFL, fcntl(pipeOut[1], F_GETFL) | O_NONBLOCK);
			pipeW = fdopen( pipeOut[1], "w");
			if( pipeW == NULL)
			{
				int errnum = errno;

				fprintf(stderr, "PiafFilter::%s:%d : ERROR: unable to open pipeW err=%d='%s'\n",
						__func__, __LINE__, errnum, strerror(errnum));
				return 0;
			}
			statusOpen = true;
			enabled = true;

			// ignore child death signals...
			//signal(SIGCHLD, SIG_IGN);

			// register child
			registerChildSig(childpid, this);

			// reset flags
			readFlag = false;
			writeFlag = false;
		}
	}
#else
        /// \todo
        if(0)
        {

        }
#endif
	return 1;
}

void PiafFilter::setReadFlag(int b)
{
	readFlag = (b?true:false);

}


// Unload child process
int PiafFilter::unloadChildProcess()
{
	if(!statusOpen) {
		fprintf(stderr, "PiafFilter::%s:%d child '%s' no opened\n",
				__func__, __LINE__, exec_name);
		return 0;
	}

	plugin_died = true; // force swReceiveImage to end after pipe read timeout

	removeChildSig(childpid);

	// send quit message
//#ifdef __SWPLUGIN_MANAGER__
	fprintf(stderr, "PiafFilter::%s:%d : Sending quit message to child '%s'\n",
			__func__, __LINE__, exec_name);
	sendRequest(SWFRAME_QUIT);

//#endif
	if(pipeR) {
		fclose(pipeR);
		pipeR = NULL;
	}

	if(pipeW) { // CSE : 2011-05-30 : was commented
		fclose(pipeW);
		pipeW = NULL;
	}

	statusOpen = false;
	usleep(100000); // let the plugin disconnect
//#ifdef __SWPLUGIN_MANAGER__
	fprintf(stderr, "PiafFilter::%s:%d : Killing child '%s'\n",
			__func__, __LINE__, exec_name);
	fflush(stderr);
//#endif

#ifndef WINDOWS
	while(waitpid(childpid, NULL, WNOHANG) >0) {
		fprintf(stderr, "%s:%d : waitpid(%d, WNOHANG) !\n", __func__, __LINE__,
				childpid);
	}
#else
        /// \todo
#endif
	fprintf(stderr, "PiafFilter::%s:%d : OK. Killed child '%s'\n",
			__func__, __LINE__, exec_name);

	childpid = 0;

	return 1;
}

// Returns functions number.
int PiafFilter::nbfunctions()
{
	return nb_func;
}
/** @brief Set processing duration in us for statistics */
void PiafFilter::setTimeUS(int dt_us)
{
	// FIXME

}

// Wait until comLock flag came to false.
bool PiafFilter::waitForUnlock(int timeout_ms)
{
	int iter=0;
	struct timeval tv1, tv2;
	gettimeofday(&tv1, NULL);
	bool ok = false;

	long dt_ms = 0;
	while(!ok && dt_ms < timeout_ms)
	{
		if(comLock) {
			usleep(20000);
			gettimeofday(&tv2, NULL);
			dt_ms = (tv2.tv_sec - tv1.tv_sec)*1000+(tv2.tv_usec - tv1.tv_usec)/1000;
			PIAF_MSG(SWLOG_INFO, "Waited for %ld ms/max=%d",
					 dt_ms, timeout_ms);
		} else {
			ok = true;
		}
	}

	return ok;
}

// Send request to child process
void PiafFilter::sendRequest(char * req)
{
	if( ! statusOpen )
		return;
#ifdef __SWPLUGIN_MANAGER__
	printf("\tRequest for '%s'...\n", req);
#endif

	if(pipeW) {
		swAddHeaderToFrame(&frame);
		swAddSeparatorToFrame(&frame);

		swAddStringToFrame(&frame, req);
		swAddSeparatorToFrame(&frame);

		// close and send
		swCloseFrame(&frame);

		fwrite(frame.buffer, frame.pos, sizeof(unsigned char), pipeW);
		fflush(pipeW);
	}
}


int PiafFilter::processFunction(int indexFunction,
								IplImage * img_in,
								IplImage ** pimg_out,
								int timeout_ms,
								u32 date, u32 tick)
{
	// wait for unlock on stdin/out
	if(waitForUnlock(200) && !plugin_died)
	{
		mapIplImageToSwImage(img_in, &data_in);

		data_in.Date = date;
		data_in.TickCount = tick;

		PIAF_MSG(SWLOG_TRACE, "Date=%lu tick=%lu",
					   (ulong)date, (ulong)tick);

		comLock = true; // lock
		int ret = -1;
		if(pipeW)
		{
			ret = swSendImage(indexFunction, &frame, swImage,
							  &data_in, pipeW);
		}
		else
		{
			fprintf(stderr, "PiafFilters::%s:%d : ERROR: pipeW is null\n",
						__func__, __LINE__);
		}

		// read image from pipeR
		if(ret) {
			if(pipeR) {
				ret = swReceiveImage(&data_out, pipeR, timeout_ms, &plugin_died);

				if(plugin_died || ret == 0) {
					ret = -1;
					PIAF_MSG(SWLOG_ERROR, "Plugin died for plugin pid=%d", childpid);
					// First stop reception
					fprintf(stderr, "PiafFilters::%s:%d : plugin died pid=%d => close pipe...\n",
								__func__, __LINE__, childpid);
//					fprintf(stderr, "PiafFilters::%s:%d : plugin died pid=%d => close pipe...\n",
//								__func__, __LINE__, childpid);
//					emit signalDied(childpid);
				}
				else
				{
					convertSwImageToIplImage(&data_out, pimg_out);

					// append processing time
					//PIAF_MSG(SWLOG_INFO, "\tdeltaTus=%d us", (int)data_out->deltaTus);
					appendTimeUS(&mTimeHistogram, data_out.deltaTus);

					// Return deltaTus
					ret = data_out.deltaTus;
				}

			} else {
				PIAF_MSG(SWLOG_ERROR, "Invalid PipeR for plugin pid=%d", childpid);

				ret = -1;
			}
		}
		else
		{
			// Plugin died from timeout
			plugin_died = true;
			fprintf(stderr, "PiafFilters::%s:%d : plugin died in swSendImage : pid=%d => close pipe...\n",
						__func__, __LINE__, childpid);
			emit signalDied(childpid);
		}

		comLock = false;

		return ret;
	}

	return 0;
}

/************** TREAT FRAMES ****************
	Function : treatFrame. Decompose and process frame information
 */
int PiafFilter::treatFrame(char *frame, int framesize)
{
	if(framesize< (int)(strlen(SWFRAME_HEADER)+2+strlen(SWFRAME_END)))
	{
		return 0;
	}

	// Decompose frame
	// First arg =? SWFRAME_HEADER ??
	char * header = strstr(frame, SWFRAME_HEADER );
	if( ! header )
	{
		fprintf(stderr, "PiafFilter: invalid header from child.\n");
		return 0;
	}

	// well, header is ok
	// second arg: function name
	char *command = nextTab(header);
	char *arg = nextTab( command );

	// category ??
	if(strncmp(command, SWFRAME_SETCATEGORY, strlen(SWFRAME_SETCATEGORY)) == 0)
	{
		// read category and subcategory
		char *scat = nextTab(arg);
		char *scat2 = nextTab(scat);
		int len = (int)(scat-arg)-1;
		int slen = (int)(scat2-scat)-1;

		if(len<=0) {
			fprintf(stderr, "Read category error ! (plugin '%s') String : '%s'\n",
				exec_name, command);
			return 0;
		}
		if(slen<=0) {
			fprintf(stderr, "Read sub-category error ! (plugin '%s') String : '%s'\n",
				exec_name, command);
			return 0;
		}

		if(!category) {
			category = new char [ len+1 ];
		}
		memcpy(category, arg, len);
		category[len] = '\0';

		if(!subcategory) {
			subcategory = new char [ slen+1 ];
		}
		memcpy(subcategory, scat, slen);
		subcategory[slen] = '\0';

//#ifdef __SWPLUGIN_MANAGER__
		fprintf(stderr, "\tReceived category = '%s'\tsub-category='%s'\n",
			category, subcategory);
//#endif

		return 1;
	}

	// Function list
	if(strncmp(command, SWFRAME_SETFUNCLIST, strlen(SWFRAME_SETFUNCLIST)) == 0)
	{
#ifdef __SWPLUGIN_MANAGER__
		fprintf(stderr, "\tReceiving function list...\n");
#endif
		// read nb functions
		sscanf(arg , "%d", &nb_func);

#ifdef __SWPLUGIN_DEBUG__
		fprintf(stderr, "\t\tNb_functions = %d\n", nb_func);
#endif

		//allocate nb_func functions in function list

		if(nb_func) {
			funcList = new swFunctionDescriptor[nb_func];

			char *nArg = nextTab(arg);

			// ask for each description
			for(int i=0; i<nb_func;i++)
			{
				char * nArg2 = nextTab(nArg);
				int len = (int)(nArg2-nArg)-1;
				funcList[i].name = new char [len+1 ];
				strncpy(funcList[i].name, nArg, len); // includes separator
				funcList[i].name[len] = '\0';

				// reset parameters list
				funcList[i].nb_params = 0;
				funcList[i].param_list = NULL;
//				funcList[i].comment = NULL;

#ifdef __SWPLUGIN_MANAGER__
				printf("\t\t\tAdding function '%s'\n", funcList[i].name);
#endif
				nArg = nArg2;
			}
		}
		return 1;
	}

	// Function descriptor ??
	if(strncmp(command, SWFRAME_SETFUNCTIONDESC, strlen(SWFRAME_SETFUNCTIONDESC)) == 0)
	{
		if(nb_func == 0)
			return 0;

		// read : num \t name \t nb_params \t par1.type \t par1.type \t par1.value (ascii) ...
		int fnum = nb_func;
		sscanf( arg, "%d", &fnum );
		if(fnum>=nb_func) {
			fprintf(stderr, "\t\tError: read function number is %d though nb_func=%d\n",
				fnum, nb_func);
			return 0;
		}

#ifdef __SWPLUGIN_DEBUG__
		printf("PARENT : Read function # %d :\n", fnum);
#endif

		// read name
		arg = nextTab(arg);
		char * arg2 = nextTab(arg);

		// read parameters number
		arg = arg2;
		arg2 = nextTab(arg);

		int nb=0;
		if(sscanf(arg, "%d", &nb) == 1) {
			funcList[fnum].nb_params = nb;
		}
#ifdef __SWPLUGIN_MANAGER__
		fprintf(stderr, "\t\tNumber of params : %d\n", funcList[fnum].nb_params );
#endif
		// allocate param list
		if(funcList[fnum].nb_params>0 && !funcList[fnum].param_list) { // first pass
			funcList[fnum].param_list = new swFuncParams [ funcList[fnum].nb_params ];
			funcList[fnum].procedure = NULL;

			// clear pointers
			for(int i=0; i<funcList[fnum].nb_params; i++) {
				funcList[fnum].param_list[i].name = NULL;
				funcList[fnum].param_list[i].type = 0;
				funcList[fnum].param_list[i].value = NULL;
			}
		}
		// else it(s just an update of parameters
		for(int i=0; i<funcList[fnum].nb_params; i++) {

			// read name
			arg  = arg2;
			arg2 = nextTab(arg);
			int len = (int)(arg2 - arg)-1;

			if(! funcList[fnum].param_list[i].name)
				funcList[fnum].param_list[i].name = new char [len+1];
			memcpy(funcList[fnum].param_list[i].name, arg, len);
			funcList[fnum].param_list[i].name[len] = '\0';

			// read type
			arg  = arg2;
			arg2 = nextTab(arg);
			funcList[fnum].param_list[i].type = *(swType *)arg;

			// read value
			arg  = arg2;
			arg2 = nextTab(arg);
			if(!funcList[fnum].param_list[i].value)
				swAllocValueFromType(funcList[fnum].param_list[i].type,
						&(funcList[fnum].param_list[i].value));

			swGetValueFromTypeAndString(funcList[fnum].param_list[i].type,
				arg,
				funcList[fnum].param_list[i].value );

#ifdef __SWPLUGIN_DEBUG__
			char txt[512];
			swGetStringValueFromType(funcList[fnum].param_list[i].type,
					funcList[fnum].param_list[i].value,
					txt );

			printf("\t\tParam # %d : %s = '%s'\n", i,
					funcList[fnum].param_list[i].name,
					txt);
#endif

		}
		return 1;
	}

	return 0;
}



swFunctionDescriptor * PiafFilter::updateFunctionDescriptor()
{
	// Add an item for each param
	// ---- ask plugin process for parameters values ----
	char txt[1024];
	sprintf(txt, SWFRAME_ASKFUNCTIONDESC "\t%d", indexFunction);

	// send function
	if(waitForUnlock(1000)) {
		lockComm();
		sendRequest(txt);
		// wait for answer then draw widgets
		if( ! waitForAnswer(1000) )
		{
			fprintf(stderr, "PiafFilter::%s:%d : Error : Plugin does not answer.\n",
					__func__, __LINE__);
			unlockComm();
			return NULL;
		}
		unlockComm(); // unlock communication pipes
	}

	// ---- generate automatically the interface (text+combo) ---
	swFunctionDescriptor * func = &(funcList[indexFunction]);
	return func;
}


/*
 * Send current params to plugin
 */
int PiafFilter::sendParams()
{
	swFunctionDescriptor * func = &(funcList[indexFunction]);
	if(!func) { return -1; }

	// send params to plugin
	char txt[4096];
	sprintf(txt, "%d", func->nb_params);

	// ask for parameters values
	sprintf(txt, SWFRAME_SETFUNCTIONDESC "\t%d" // function index
			"\t%s" // function name
			"\t%d" // number of params
			"\t",
			indexFunction,
			func->name,
			func->nb_params
			);
	char par[2048]="";// big buffer because may be an accumulation of many strings

	if(func->param_list)
	{
		for(int i=0; i < func->nb_params; i++)
		{
			if(func->param_list[i].type == swStringList)
			{
				swStringListStruct *s = (swStringListStruct *)func->param_list[i].value;
				fprintf(stderr, "[PiafFilter]::%s:%d : stringList s=%p->curitem = %d\n",
						__func__, __LINE__, s, s->curitem);

			}
			swGetStringValueFromType(func->param_list[i].type,
									 func->param_list[i].value,
									 par);
			fprintf(stderr, "\t%s:%d\tparam[%d]:'%s' => val=%p='%s'\n",
					__func__, __LINE__, i,
					func->param_list[i].name,
					func->param_list[i].value,
					par);

			sprintf(txt,"%s" // old text
						"%s\t%c\t%s\t", // param : name \t type \t value \t
						txt,
						func->param_list[i].name,
						func->param_list[i].type,
						par
						);
		}
	}

	// send function
	if(waitForUnlock(1000)) {
		lockComm();
		PIAF_MSG(SWLOG_INFO, "Sending message '%s' to plugin", txt);
		sendRequest(txt);
		unlockComm();

		emit signalParamsChanged();
		return 0;
	}

	PIAF_MSG(SWLOG_INFO, "Wait for unlock failed");

	return -1;
}
