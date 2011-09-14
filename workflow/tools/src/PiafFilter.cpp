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

//#include "workshoptool.h"
#include <SwTypes.h>
#include <sys/time.h>


//// Qt includes
//#include <qlabel.h>
//#include <q3vbox.h>
//#include <qlayout.h>
//#include <q3filedialog.h>
//#include <qmessagebox.h>
//#include <qtooltip.h>
////Added by qt3to4:
//#include <Q3GridLayout>
//#include <QPixmap>
//#include <QCloseEvent>
//#include <QTimer>

#include <QMessageBox>

// standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

//#include "videocapture.h"
#include "piaf-common.h"


#include <QWorkspace>
bool g_debug_PiafSignalHandler = false;
bool g_debug_FilterSequencer = false;
bool g_debug_PiafFilter = false;

/// Delete a filter in manager (parameters and widgets)
#define DELETE_FILTER(_pv)	{ \
				delete (_pv); \
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

	while(waitpid(-1, NULL, WNOHANG) >0) {
		if(g_debug_PiafSignalHandler)  {
			fprintf(stderr, "PiafSignalHandler::%s:%d : waitpid>0 !\n", __func__, __LINE__);
		}
	}

	QList<sigPiafFilter *>::iterator it;
	for(it = filterList.begin(); it != filterList.end(); ++it) {
		sigPiafFilter * sigF = (*it);
		if(g_debug_PiafSignalHandler)  {
			fprintf(stderr, "PiafSignalHandler::%s:%d: Testing child '%s' pid=%d...\n",
				__func__, __LINE__,
				sigF->pFilter->exec_name, sigF->pid);
		}

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
			filterList.remove(sigF);
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

	imageTmp = NULL;
}

void FilterSequencer::purge() {
	fprintf(stderr, "FilterSequencer::%s:%d DELETE FilterSequencer\n", __func__, __LINE__);

	unloadAllLoaded();
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
	fprintf(stderr, "FilterSequencer::%s:%d : adding filter %p at id=%d\n",
			__func__ ,__LINE__, newFilter, id);

	if(strlen(mPluginSequenceFile) == 0)
	{
		strcat(mPluginSequenceFile, "(custom)");
	}

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


	fprintf(stderr, "[FilterSequencer]::%s:%d : start process '%s' func %d\n",
			__func__, __LINE__,
			newFilter->exec_name, newFilter->indexFunction);
	// start process
	newFilter->loadChildProcess();
	newFilter->setEnabled(true);
	setFinal(newFilter);

	return newFilter;
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
	fprintf(stderr, "[PiafFilterMng]::%s:%d : dead filter pid=%d => send filter changed...\n",
			__func__, __LINE__, pid);
	emit selectedFilterChanged();
}


int FilterSequencer::removeFilter(int id)
{
	// unload child process
	PiafFilter * pv = mLoadedFiltersList.at((uint)id);
	if(!pv) { return -1; }

	return removeFilter(pv);
}


int FilterSequencer::removeFilter(PiafFilter * filter)
{
	if(!filter) { return -1; }

	// unload child process
	int id = mLoadedFiltersList.findIndex(filter);
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
		mLoadedFiltersList.remove(filter);

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
			QMessageBox::critical(NULL,
				tr("Filter Error"),
				tr("Cannot find filter list, check installation."),
				QMessageBox::Abort,
				QMessageBox::NoButton,
				QMessageBox::NoButton);

			return 0;
		}
	}

	// read each filename and load file mesures
	char line[512]="";

	while(!feof(f)) {
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
						loadFilter(line);
					}
					else
						loadFilter(line);
				} else {
					char *stepper = line+strlen(line)-1;
					while( stepper > line && (*stepper == ' ' || *stepper == '\n' || *stepper == '\t')) {
						*stepper = '\0';
						stepper--;
					}

					fprintf(stderr, "\t[FilterSequencer] %s:%d: loading filter '%s'\n",
							__func__, __LINE__, line);
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

//FIXME	newFilter->enabled = true;

	fprintf(stderr, "[FilterSequencer]::%s:%d : ADDED!!!\n", __func__, __LINE__);
	emit selectedFilterChanged();

	return newFilter;
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
			fprintf(stderr, "\tDelete loaded? filter '%s' \n",
				pv->exec_name);
			if(  pv->loaded() )
			{
				// then add it to selectedListView
//#ifdef __SWPLUGIN_MANAGER__
				if(pv->indexFunction < pv->nb_func) {
					fprintf(stderr, "\tDelete filter '%s' \n",
						pv->funcList[pv->indexFunction].name);
				}
//#endif
				// wait for process flag to go down
				while(lockProcess) {
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


	int idx = mLoadedFiltersList.findIndex(filter);
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
	mLoadedFiltersList.remove(filter);
	mLoadedFiltersList.insert(idx-1, filter);

	idEditPlugin = mLoadedFiltersList.findIndex(filter);
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


	int idx = mLoadedFiltersList.findIndex(filter);
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

	// remove then insert at next place
	mLoadedFiltersList.removeAt(idx);
	mLoadedFiltersList.insert(idx-1, filter);

	idEditPlugin = mLoadedFiltersList.findIndex(filter);;
	fprintf(stderr, "FilterSequencer::%s:%d : moved to %d\n",
			__func__, __LINE__, idEditPlugin);
	emit selectedFilterChanged();
	return 0;

}

#if 0
////////////////////// TO BE COPIED/PASTED ////////////////////////////////

void FilterSequencer::simpleClic(Q3ListViewItem * item)
{
	if(!item) { return; }

	fprintf(stderr, "FilterManager::%s:%d : selected item : %p\n", __func__, __LINE__, item);
	// look for selected item
	if(mLoadedFiltersList.isEmpty()) return;

	swPluginView * pv;

	// enabled by default
	bEdit->setEnabled(true);

	for(pv=mLoadedFiltersList.first(); pv ; pv=mLoadedFiltersList.next())
	{
		if(pv->selItem == item) {
			swFunctionDescriptor * func = &(pv->filter->funcList[pv->indexFunction]);
			fprintf(stderr, "%s:%d SELECT FILTER '%s' => func=%p nb params=%d\n",
					__func__, __LINE__,
					func ? func->name : "(empty)",
					func ,
					func ?func->nb_params : -1
					);
			if(!func
			   || (func && func->nb_params == 0)) {
				// ---- ask plugin process for parameters values ----
				char txt[1024];
				sprintf(txt, SWFRAME_ASKFUNCTIONDESC "\t%d", pv->indexFunction);

				// send function
				if(pv->filter->waitForUnlock(1000)) {
					pv->filter->comLock = true; // lock communication pipes

					pv->filter->sendRequest(txt);
					// wait for answer then draw widgets
					if(!pv->filter->waitForAnswer(1000))
					{
						fprintf(stderr, "Error : Plugin does not answer.\n");
						pv->filter->comLock = false;
						return;
					}
					pv->filter->comLock = false; // unlock communication pipes

				}

				func = &(pv->filter->funcList[pv->indexFunction]);
			}

			if(func) {
				if(func->nb_params) {
					bEdit->setEnabled(true);
				} else {
					bEdit->setEnabled(false);
				}
			}

//#ifdef __SWPLUGIN_MANAGER__
			fprintf(stderr, "%s:%d SELECT FILTER '%s'\n", __func__, __LINE__,
					pv->filter->funcList[pv->indexFunction].name);
//#endif
			return;
		}
	}
}



void FilterSequencer::doubleClic(Q3ListViewItem * item)
{
	if(!item) { return; }

	// look for selected item
	swPluginView * pv;

	int id=0;
	idViewPlugin = -1;

	if(!mLoadedFiltersList.isEmpty()) {
		for(pv=mLoadedFiltersList.first();
			pv && idViewPlugin<0;
			pv=mLoadedFiltersList.next())
		{
			if( pv->selItem == item ) {
				idViewPlugin = id;

#ifdef __SWPLUGIN_MANAGER__
				printf("SELECT FILTER '%s' as end viewing filter\n", pv->filter->funcList[pv->indexFunction].name);
#endif
			}
			id++;
		}
	}

	updateSelectedView();

	emit selectedFilterChanged();
}


/* slot for editing the parameters of a filter = aotogenerated interface
*/
void FilterSequencer::slotEdit()
{
	// ---- look for filer plugin ----
	swPluginView * pv = NULL, *pEditPlugin = NULL;
	idEditPlugin = -1;
	int id=0;
	if(!mLoadedFiltersList.isEmpty()) {
		for(pv=mLoadedFiltersList.first(); pv && idEditPlugin<0; pv=mLoadedFiltersList.next())
		{
			if( pv->selItem && selectedListView->isSelected(pv->selItem) )
			{
				idEditPlugin = id;
				pEditPlugin = pv;
	#ifdef __SWPLUGIN_MANAGER__
				printf("Edit filter '%s' \n", pv->filter->funcList[pv->indexFunction].name);
	#endif
			}
			id++;
		}
	}


	if(idEditPlugin<0) {
		fprintf(stderr, "Edit filter: cannot find filter.\n");
		return;
	}

	if(pEditPlugin->mwPluginEdit) {
		pEditPlugin->mwPluginEdit->show();

		return;
	}

	// ---- ask plugin process for parameters values ----
	char txt[1024];
	sprintf(txt, SWFRAME_ASKFUNCTIONDESC "\t%d", pEditPlugin->indexFunction);
	// send function
	if(pEditPlugin->filter->waitForUnlock(1000)) {
		pEditPlugin->filter->comLock = true; // lock communication pipes

		pEditPlugin->filter->sendRequest(txt);
		// wait for answer then draw widgets
		if(!pEditPlugin->filter->waitForAnswer(1000))
		{
			fprintf(stderr, "Error : Plugin does not answer.\n");
			pEditPlugin->filter->comLock = false;
			return;
		}
		pEditPlugin->filter->comLock = false; // unlock communication pipes

	}

	// ---- generate automatically the interface (text+combo) ---
	swFunctionDescriptor * func = &(pEditPlugin->filter->funcList[pEditPlugin->indexFunction]);
	if(func->nb_params) {
		// ---------- creates window for editing parameters ------------
		pEditPlugin->mwPluginEdit = (QWidget *)new QWidget(NULL, func->name, 0);

		QIcon pixIcon("configure.png");
		pEditPlugin->mwPluginEdit->setWindowIcon(pixIcon);

		char txt[512];
		sprintf(txt, "%s parameters", func->name);
		pEditPlugin->mwPluginEdit->setCaption(txt);
		// names, labels ...
		Q3VBox * vBox = new Q3VBox(pEditPlugin->mwPluginEdit);
		QWidget * wid = new QWidget( vBox );
		wid->resize(246, (2+func->nb_params)* 30);
		wid->updateGeometry();

		Q3GridLayout * fgrid = new Q3GridLayout(wid, 1+func->nb_params, 2, 0, -1, NULL);

		// add function name
		// PID
		QLabel *paramsPid = new QLabel(tr("PID:"), wid, 0, 0);
		fgrid->addWidget(paramsPid, 0, 0);

		QString pidStr;
		pidStr.setNum(pEditPlugin->filter->childpid);
		QLabel *paramsPid2 = new QLabel(pidStr, wid, 0, 0);
		fgrid->addWidget(paramsPid2, 0, 1);

		QLabel *funcL = new QLabel(tr("Function:"), wid, 0, 0);
		fgrid->addWidget(funcL, 1, 0);
		QLabel *funcL2 = new QLabel(QString(func->name), wid, 0, 0);
		fgrid->addWidget(funcL2, 1, 1);

		// Parameters
		QLabel *paramsL = new QLabel(tr("Parameters:"), wid, 0, 0);
		sprintf(txt, "%d", func->nb_params);
		fgrid->addWidget(paramsL, 2, 0);

		QLabel *paramsL2 = new QLabel(QString(txt), wid, 0, 0);
		fgrid->addWidget(paramsL2, 2, 1);




		// for each parameter :
		if(paramsEdit)
			delete [] paramsEdit;
		if(comboEdit)
			delete [] comboEdit;

		paramsEdit = new QLineEdit * [ func->nb_params ]; // for scalars
		memset(paramsEdit, 0, sizeof(QLineEdit *) * func->nb_params);

		comboEdit = new QComboBox * [ func->nb_params ]; // for lists
		memset(comboEdit, 0, sizeof(QComboBox *) * func->nb_params);

		int txtcount = 0;
		int combocount = 0;

		for(int i=0; i<func->nb_params; i++)
		{
			// Add param name then param value
			QLabel *pname = new QLabel(QString(func->param_list[i].name), wid, 0, 0);
			fgrid->addWidget(pname, 3+i, 0);

			swGetStringValueFromType(func->param_list[i].type, func->param_list[i].value, txt);

			switch(func->param_list[i].type) {
			case swStringList: {
				comboEdit[combocount] = new QComboBox(wid);
				fgrid->addWidget(comboEdit[combocount], 3+i, 1);

				swStringListStruct * s = (swStringListStruct *)func->param_list[i].value;
				comboEdit[combocount]->setFixedHeight(28);
				for(int item=0;item<s->nbitems;item++) {
					fprintf(stderr, "[PiafFilters] %s:%d insert item [%d/%d]='%s'\n",
							__func__, __LINE__, item, s->nbitems, s->list[item]);
					comboEdit[combocount]->insertItem( s->list[item], -1);
				}
				combocount++;
				}
				break;
			default:
				paramsEdit[txtcount] = new QLineEdit( QString(txt), wid);
				fgrid->addWidget(paramsEdit[txtcount], 3+i, 1);
				paramsEdit[txtcount]->setFixedHeight(28);
				txtcount++;
				break;
			}
		}


		QPushButton * applyButton = new QPushButton(QString(tr("Apply")), vBox);
		applyButton->setFlat(false);
		applyButton->setToggleButton(false);
		connect(applyButton, SIGNAL(clicked()), this, SLOT(applyEditParameters()));

		vBox->setFixedSize(248, (2+func->nb_params)* 30 + 30);
		vBox->resize(248, (2+func->nb_params)* 30 + 30);
//		pEditPlugin->mwPluginEdit->setFixedSize(200, (2+func->nb_params)* 26 + 28);
		pEditPlugin->mwPluginEdit->resize(250, (2+func->nb_params)* 30 + 34);
		pEditPlugin->mwPluginEdit->updateGeometry();
		pEditPlugin->mwPluginEdit->update();


		// Set to workspace
		if(pWorkspace) {
			pWorkspace->addWindow( pEditPlugin->mwPluginEdit );
		}
		pEditPlugin->mwPluginEdit->show();

	}

}


/*********************** TIME DISPLAY *******************/

SwTimeWidget::SwTimeWidget(QWidget* parent, const char *name, Qt::WindowFlags wflags)
	: QWidget(parent, name, wflags) {

	setCaption(name);
	setFixedSize(180,70);

	QIcon pixIcon("chronometer.png");
	setWindowIcon(pixIcon);

	Q3GridLayout * grid = new Q3GridLayout(this, 3, 3, 0, -1, NULL);
	int row=0;
	QLabel *meanL = new QLabel(tr("Mean:"), this, 0, 0);
	grid->addWidget(meanL, row, 0);

	meanLabel = new QLabel("", this, 0, 0);
	grid->addWidget(meanLabel, row, 1);

	QLabel *meanL2 = new QLabel("us", this, 0, 0);
	grid->addWidget(meanL2, row, 2);
// Min
	row++;
	QLabel *minL = new QLabel(tr("Min :"), this, 0, 0);
	grid->addWidget(minL, row, 0);

	minLabel = new QLabel("", this, 0, 0);
	grid->addWidget(minLabel, row, 1);

	QLabel *minL2 = new QLabel("us", this, 0, 0);
	grid->addWidget(minL2, row, 2);
// Max
	row++;
	QLabel *maxL = new QLabel(tr("Max :"), this, 0, 0);
	grid->addWidget(maxL, row, 0);

	maxLabel = new QLabel("", this, 0, 0);
	grid->addWidget(maxLabel, row, 1);

	QLabel *maxL2 = new QLabel("us", this, 0, 0);
	grid->addWidget(maxL2, row, 2);
	reset();
	show();
}

SwTimeWidget::~SwTimeWidget() {
}

int SwTimeWidget::reset() {
	nb=0;
	cumul = 0;
	min_time = 10000000;
	max_time = 0;
	mean_time = 0;
	refresh();
	return 0;
}

int SwTimeWidget::setTimeUS(long dt) {
	cumul += dt;
	nb++;
	mean = (float)((double)cumul / (double)nb);
	mean_time = (long)mean;
	if(dt<min_time) min_time = dt;
	if(dt>max_time) max_time = dt;

	refresh();
	return 0;
}

int SwTimeWidget::refresh() {
	QString str;
	str.sprintf("%ld", mean_time);
	meanLabel->setText(str);
	str.sprintf("%ld", min_time);
	minLabel->setText(str);
	str.sprintf("%ld", max_time);
	maxLabel->setText(str);
	return 0;
}

void FilterSequencer::slotTime() {
	// ---- look for filer plugin ----
	swPluginView * pv = NULL, *pEditPlugin = NULL;
	idEditPlugin = -1;
	int id=0;
	if(!mLoadedFiltersList.isEmpty())
	for(pv=mLoadedFiltersList.first(); pv && idEditPlugin<0; pv=mLoadedFiltersList.next())
	{
		if( pv->selItem && selectedListView->isSelected(pv->selItem) )
		{
			idEditPlugin = id;
			pEditPlugin = pv;
#ifdef __SWPLUGIN_MANAGER__
			printf("Edit filter '%s' \n", pv->filter->funcList[pv->indexFunction].name);
#endif
		}
		id++;
	}
	if(idEditPlugin<0) {
		fprintf(stderr, "Edit filter: cannot find filter.\n");
		return;
	}

	if(pEditPlugin->mwPluginTime) {
		pEditPlugin->mwPluginTime->show();
		return;
	}

	swFunctionDescriptor * func = &(pEditPlugin->filter->funcList[pEditPlugin->indexFunction]);
	if(!func) {
		fprintf(stderr, "[PiafFiltersMngr]::%s:%d : func is null !\n", __func__, __LINE__);
	} else {
		fprintf(stderr, "[PiafFiltersMngr]::%s:%d : create time for func '%s'\n", __func__, __LINE__, func->name);
		pEditPlugin->mwPluginTime = new SwTimeWidget(pWorkspace, func->name, 0);

		if(pWorkspace) {
			pWorkspace->addWindow((QWidget *)pEditPlugin->mwPluginTime);
		}
		pEditPlugin->mwPluginTime->show();
	}

}

void FilterSequencer::applyEditParameters()
{
	swPluginView * pEditPlugin = mLoadedFiltersList.at(idEditPlugin);
	swFunctionDescriptor * func = &(pEditPlugin->filter->funcList[pEditPlugin->indexFunction]);
	// check if values are coherent with types
	int i;
	int txtcount=0;
	int combocount = 0;
	char par[2048]="";// big buffer because may be an accumulation of many strings
	for(i=0; i < func->nb_params;i++)
	{
		switch(func->param_list[i].type) {
		case swStringList: {
			// no problem with coherence for combo
			// read value from combo
			swStringListStruct *s = (swStringListStruct *)func->param_list[i].value;
			s->curitem = comboEdit[combocount]->currentItem();
			combocount++;
			}
			break;
		default:
			if(!swGetValueFromTypeAndString(func->param_list[i].type,
					(char *)paramsEdit[txtcount]->text().latin1(),
					func->param_list[i].value))
				fprintf(stderr, "Error: incompatible type and value.\n");

			swGetStringValueFromType(func->param_list[i].type, func->param_list[i].value, par);
			paramsEdit[txtcount]->setText(QString(par));
			txtcount++;
			break;
		}
	}

	// send params to plugin
	char txt[4096];
	sprintf(txt, "%d", func->nb_params);

	// ask for parameters values
	sprintf(txt, SWFRAME_SETFUNCTIONDESC "\t%d" // function index
		"\t%s" // function name
		"\t%d" // number of params
		"\t",
		pEditPlugin->indexFunction,
		func->name,
		func->nb_params
		);

	txtcount=0;
	combocount = 0;
	if(func->param_list) {
		for(int i=0; i < func->nb_params; i++)
		{
			swGetStringValueFromType(func->param_list[i].type,
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
	if(pEditPlugin->filter->waitForUnlock(1000)) {
		pEditPlugin->filter->comLock = true;
		pEditPlugin->filter->sendRequest(txt);
		pEditPlugin->filter->comLock = false;
	}

	emit selectedFilterChanged();
}

void FilterSequencer::cancelEditParameters()
{


}

#endif


void FilterSequencer::toggleFilterDisableFlag(PiafFilter * filter)
{
	idEditPlugin = mLoadedFiltersList.findIndex(filter);

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
	fprintf(stderr, "FilterSequencer::%s:%d('%s')...\n",
			__func__, __LINE__, filename);

	if(!strstr(filename, FILTERLIST_EXTENSION)) {
		strcat(filename, FILTERLIST_EXTENSION);
	}

	if(mLoadedFiltersList.isEmpty()) {
		 return -1;
	 }

	printf("Save filter list into file '%s'\n", filename);

	// process each filter
	FILE * f = fopen(filename, "w");
	if(!f)
	{
		int errnum = errno;
		fprintf(stderr, "PiafFilters::%s:%d : error while opening file '%s' : %d='%s'\n",
				__func__,__LINE__,
				filename, errnum, strerror(errnum)
				);
		return -errnum;
	}

	QList<PiafFilter *>::iterator it;
	for(it = mLoadedFiltersList.begin(); it != mLoadedFiltersList.end(); ++it) {
		PiafFilter * filter = (*it);
		// process
#ifdef __SWPLUGIN_DEBUG__
		fprintf(stderr, "FILTERMANAGER: saving filter %s\n",
				filter->funcList[filter->indexFunction].name);
#endif
		fprintf(f, "%s\t%d\n",
				filter->exec_name,
				filter->indexFunction);
		// --- read parameters
		// send request and wait for answer
		filter->sendRequest(SWFRAME_ASKFUNCTIONDESC);

		// then save reply string
		char txt[256];
		if(filter->pipeR) {
			fgets(txt, 256, filter->pipeR);
#ifdef __SWPLUGIN_DEBUG__
			fprintf(stderr, "slotSave : read answer '%s'\n", txt);
#endif
			fprintf(f, "%s", txt);
		}
	}

	fclose(f);
	return 0;
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

	fprintf(stderr, "[PiafFilterMngr]::%s:%d : Loading file '%s'...\n", __func__, __LINE__, filename);
	// process each image

	FILE * f = fopen(fname, "r");
	if(!f) {
		fprintf(stderr, "[PiafFilterMngr]::%s:%d : cannot load file '%s'\n", __func__, __LINE__, filename);
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
			if(!mAvailableFiltersList.isEmpty())
			{
				QList<PiafFilter *>::iterator it;
				for(it = mAvailableFiltersList.begin();
						it!=mAvailableFiltersList.end() && !foundPV; ++it)
				{
					PiafFilter * filter = (*it);
					if(filter) {
						if(strncmp(filter->exec_name, line, strlen((filter->exec_name)))==0) {
							foundPV = filter;
						}
					} else {
						fprintf(stderr, "[PiafFilterMngr]::%s:%d : no filter for filter=%p\n",
								__func__, __LINE__, filter);
					}
				}
			}

			int id = -1;
			if(!foundPV) {
				fprintf(stderr, "[FilterSequencer]::%s:%d : Filter exec '%s' seems not to be unknown.\n",
						__func__, __LINE__, line);
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
				}
				else {
					fprintf(stderr, "[PiafFilters]::%s:%d: Cannot read function index for filter from exec '%s'\n",
							__func__, __LINE__,
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
				fprintf(stderr, "[PiafFilterMng]::%s:%d : Loaded filter '%s' \n", __func__, __LINE__,
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


	emit selectedFilterChanged();
	return 0;
}








/******************************************************************
IMAGE PROCESSING  SECTION
*/




int FilterSequencer::processImage(swImageStruct * image)
{
	// send image to process
	if(!imageTmp)
	{
		imageTmp = new swImageStruct;
		memcpy(imageTmp, image, sizeof(swImageStruct));

		// allocate buffer
		imageTmp->buffer = new unsigned char * [ imageTmp->buffer_size];
		memset(imageTmp->buffer, 0, sizeof(unsigned char)*imageTmp->buffer_size);
	}

	if(mLoadedFiltersList.isEmpty()) {
		fprintf(stderr, "[FilterSequencer]::%s:%d : filter list is empty\n", __func__, __LINE__);
		return -1;
	}

	// process each image
	int step = 0;
	int ret = 1;

	int global_return = 0;

	if(!lockProcess) {
		lockProcess = true;

		swImageStruct *im1 = image, *im2 = imageTmp;

		ret = 1;
		int id=0;

		// Time accumulation in us
		float total_time_us = 0.f;

		if(!mLoadedFiltersList.isEmpty())
		{
			QList<PiafFilter *>::iterator it;
			for(it = mLoadedFiltersList.begin();
					it!=mLoadedFiltersList.end() && ret && id<=idViewPlugin;
					++it ) {
				PiafFilter * pv = (*it);
				// process
				if( pv->enabled ) {
					if(g_debug_FilterSequencer) {
						fprintf(stderr, "FilterSequencer::%s:%d processing filter func[%d]=%s\n",
								__func__, __LINE__,
								pv->indexFunction,
								pv->funcList[pv->indexFunction].name);
					}

					ret = pv->processFunction(pv->indexFunction, im1, im2, 2000 );
					if(ret)
					{
						step++;

						// invert buffers
						swImageStruct *imTmp = im2;
						im2 = im1;
						im1 = imTmp;

						total_time_us += imTmp->deltaTus;

						// time statistics
						pv->setTimeUS( imTmp->deltaTus );
					}
					else
					{	// Processing has one error
						global_return --;

						fprintf(stderr, "[PiafFiltersManager]::%s:%d : filter '%s' failed "
								"=> will return %d\n",
								__func__, __LINE__, pv->exec_name,
								global_return
								);
					}
				}
			}

			id++;
		}

		lockProcess = false;

		// even or odd ??
		if(ret && (step % 2) == 1) // odd, must invert
		{
			memcpy(image->buffer, imageTmp->buffer, image->buffer_size);
		}

		// store accumulated time in output image
		image->deltaTus = total_time_us;
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
			QMessageBox::critical(NULL, tr("A plugin crashed"),
								  tr("A plugin in sequence has crashed"));
		}
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



	// pipes initialisation
	pipeIn[0]  = 0;
	pipeIn[1]  = 0;
	pipeOut[0] = 0;
	pipeOut[1] = 0;
	pipeR = NULL;
	pipeW = NULL;
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

int PiafFilter::destroy() {
	fprintf(stderr, "[PiafFilter]::%s:%d : DESTROYing PiafFilter %p='%s'...\n",
			__func__, __LINE__, this, exec_name);

	if(!exec_name)
		return 0;
	if(exec_name[0] == '\0')
		return 0;

	unloadChildProcess();

	swFreeFrame(&frame);


	if(buffer) { delete [] buffer; buffer = NULL; }
	if(exec_name) {	delete [] exec_name; exec_name = NULL; }
	if(category) { delete [] category; category = NULL; }
	if(subcategory) { delete [] subcategory;subcategory = NULL; }

	if(nb_func) {
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
#include "SwFilters.h"

PiafSignalHandler piafSigHandler;
SwSignalHandler SigHandler;

int registerChildSig(int pid, SwFilter * filter)
{
	//
	fprintf(stderr, "!! !! !! !! REGISTERING PROCESS %d FOR FILTER %s !! !! !! !!\n",
		pid, filter->exec_name);
	SigHandler.registerChild(pid, filter);
	return 0;
}




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
	SigHandler.removeChild(pid);
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
#define TIMEOUT_STEP 20000
	if(timeout == 0)
		timeout = 1000; // millisec
	int iter = 0, maxiter = timeout * 1000 / TIMEOUT_STEP;

	while(iter < maxiter) {
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
				usleep(TIMEOUT_STEP);
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
			printf("CHILD PROCESS =====> %s\n", exec_name);
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

			close( pipeOut[0]);

			chdir(path);
			if(execv(exec_name, NULL) == -1)
			{
				fprintf(stderr, "Child execv('%s') error.\n",
					exec_name);
				exit(EXIT_FAILURE);;
			}
			exit(EXIT_SUCCESS);

			break;
		default : // parent process
			// Child to Parent
			printf(" PARENT PROCESS ====>\n");
			close( pipeIn[1] );

			// set non blocking
			//fcntl(pipeIn[0], F_SETFL, fcntl(pipeIn[0], F_GETFL) | O_NONBLOCK);

			pipeR = fdopen( pipeIn[0], "r");

			if( pipeR == NULL)
			{
				int errnum = errno;
				fprintf(stderr, "PiafFilter::%s:%d : ERROR: unable to open pipeR err=%d='%s'\n",
						__func__, __LINE__, errnum, strerror(errnum));
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


	while(waitpid(childpid, NULL, WNOHANG) >0) {
		fprintf(stderr, "%s:%d : waitpid(%d, WNOHANG) !\n", __func__, __LINE__,
				childpid);
	}
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
	int itermax = timeout_ms*1000/20000;
	bool ok = false;
	while(!ok && iter<itermax)
	{
		if(comLock)
			usleep(20000);
		else ok = true;
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


int PiafFilter::processFunction(int indexFunction, void * data_in, void * data_out, int timeout_ms)
{
	// wait for unlock on stdin/out
	if(waitForUnlock(200) && !plugin_died) {

		comLock = true; // lock
		int ret = 0;
		if(pipeW) {
			ret = swSendImage(indexFunction, &frame, swImage, data_in, pipeW);
		}

		// read image from pipeR
		if(ret) {
			if(pipeR) {
				ret = swReceiveImage(data_out, pipeR, timeout_ms, &plugin_died);

				if(plugin_died) {
					// First stop reception
					fprintf(stderr, "PiafFilters::%s:%d : plugin died pid=%d => close pipe...\n",
								__func__, __LINE__, childpid);
					emit signalDied(childpid);
				}

			} else {
				ret = 0;
			}
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
		return 0;

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

		if(!category)
			category = new char [ len+1 ];
		memcpy(category, arg, len);
		category[len] = '\0';

		if(!subcategory)
			subcategory = new char [ slen+1 ];

		memcpy(subcategory, scat, slen);
		subcategory[slen] = '\0';

//#ifdef __SWPLUGIN_MANAGER__
		printf("\tReceived category = '%s'\tsub-category='%s'\n",
			category, subcategory);
//#endif

		return 1;
	}

	// Function list
	if(strncmp(command, SWFRAME_SETFUNCLIST, strlen(SWFRAME_SETFUNCLIST)) == 0)
	{
#ifdef __SWPLUGIN_MANAGER__
		printf("\tReceiving function list...\n");
#endif
		// read nb functions
		sscanf(arg , "%d", &nb_func);

#ifdef __SWPLUGIN_DEBUG__
		printf("\t\tNb_functions = %d\n", nb_func);
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
		if(sscanf(arg, "%d", &nb))
			funcList[fnum].nb_params = nb;

#ifdef __SWPLUGIN_MANAGER__
		printf("\t\tNumber of params : %d\n", funcList[fnum].nb_params );
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
