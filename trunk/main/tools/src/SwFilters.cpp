/***************************************************************************
	   SwFilters.cpp  -  plugin core and plugin manager for Piaf
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

//#define __SWPLUGIN_DEBUG__

#include "SwFilters.h"
#include "workshoptool.h"
#include <SwTypes.h>


// Qt includes
#include <qlabel.h>
#include <q3vbox.h>
#include <qlayout.h>
#include <q3filedialog.h>
#include <qmessagebox.h>
#include <qtooltip.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <QPixmap>
#include <QCloseEvent>

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

#include "videocapture.h"

#define DELETE_FILTER(_pv)	{ \
				if((_pv)->filter) { \
					delete (_pv)->filter; \
				} \
				if((_pv)->icon) { \
					delete [] (_pv)->icon; \
					(_pv)->icon = NULL; \
				} \
				if((_pv)->mwPluginEdit) { \
					delete (_pv)->mwPluginEdit; \
					(_pv)->mwPluginEdit = NULL; \
				} \
				if((_pv)->mwPluginTime) { \
					delete (_pv)->mwPluginTime; \
					(_pv)->mwPluginTime = NULL; \
				} \
				delete (_pv); \
			}

#include <QWorkspace>

int SwSignalHandler::registerChild(int pid, SwFilter * filter) {

	fprintf(stderr, "SwSignalHandler::registerChild(%d, '%s')...\n",
		pid, filter->exec_name);
	if(pid == 0) return 0;

	sigFilter * sigF = new sigFilter;
	sigF->pid = pid;
	sigF->pFilter = filter;

	filterList.append(sigF);
	return 1;
}

int SwSignalHandler::killChild() {

	sigFilter * sigF = NULL;
	if(filterList.isEmpty()) {
		fprintf(stderr, "SwSignalHandler::%s:%d : nothing in list => return 0 !\n", __func__, __LINE__);
		return 0;
	}
	while(waitpid(-1, NULL, WNOHANG) >0) {
		fprintf(stderr, "SwSignalHandler::%s:%d : waitpid>0 !\n", __func__, __LINE__);
	}
	for(sigF = filterList.first(); sigF; sigF = filterList.next()) {
		fprintf(stderr, "SwSignalHandler::%s:%d: Testing child '%s' pid=%d...\n",
				__func__, __LINE__,
				sigF->pFilter->exec_name, sigF->pid);

		if( kill(sigF->pid, SIGUSR1)<0)
		{
			int err = errno;
			if(err == ESRCH) {
				fprintf(stderr, "!! !! !! !! KILLING CHILD %d ('%s') !! !! !! !!\n",
					sigF->pid, sigF->pFilter->exec_name);
				fflush(stderr);
				int status = 0;

				fprintf(stderr, "\tExited with WIFEXITED %d\n", WIFEXITED(status) );
				fprintf(stderr, "\tExited with WEXITSTATUS %d\n", WEXITSTATUS(status) );
				if(WIFSIGNALED(status))
					fprintf(stderr, "\tExited because a signal were not caught\n");


				sigF->pFilter->forceCloseConnection();
//				filterList.remove(sigF);
//				sigF->pFilter = NULL;
//				delete sigF;
			}

			return 1;
		}
	}

	fprintf(stderr, "SwSignalHandler::%s:%d : filter not found in list => return 0 !\n",
			__func__, __LINE__);

	return 0;
}

int SwSignalHandler::removeChild(int pid) {

	fprintf(stderr, "SwSignalHandler::removeChild(%d)...\n",
		pid);
	sigFilter * sigF = NULL;
	if(filterList.isEmpty())
		return 0;

	for(sigF = filterList.first(); sigF; sigF = filterList.next()) {
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
SwFilterManager::SwFilterManager(VideoCaptureDoc * vdoc,
				QWidget* pparent, const char *name, Qt::WFlags wflags):
				 Q3MainWindow(pparent, name, wflags)
{
	pWorkspace = NULL;
	doc = vdoc;
	init();
	hide();
}


SwFilterManager::SwFilterManager(QWidget* pparent, const char *name, Qt::WFlags wflags):
				 Q3MainWindow(pparent, name, wflags)
{
	pWorkspace = NULL;
	doc = NULL;

  //  doc=vdoc;
	init();
	hide();
}
void SwFilter::setWorkspace(QWorkspace * wsp) {

	if(wsp) {
		//((QWorkspace *)wsp)->addWindow((QWidget *)this);
	}

}


void SwFilterManager::setWorkspace(QWorkspace * wsp) {
	pWorkspace = wsp;

	if(pWorkspace) {
		((QWorkspace *)pWorkspace)->addWindow((QWidget *)this);

		show();
	}

	QPixmap winIcon = QPixmap(BASE_DIRECTORY "images/pixmaps/IconFilters.png");
	setIcon(winIcon);

}

/*  Initialisation :
	- initializes buffers, reset pointers
	- load filter list
	- creates widgets */
void SwFilterManager::init() {
#define BUFFER_SIZE 4096
	swAllocateFrame(&frame, BUFFER_SIZE);

	mPluginSequenceFile[0] = '\0';

	lockProcess = false;
	mNoWarning = false;

	idEditPlugin = 0;
	idViewPlugin = 0;
	paramsEdit = NULL;
	comboEdit = NULL;

	imageTmp = NULL;

//QGridLayout ( QWidget * parent, int nRows = 1, int nCols = 1, int margin = 0, int space = -1, const char * name = 0 )
	Q3GridLayout * qgrid = new Q3GridLayout( this, 1, 3, 0, -1, "filtersGrid");

	// Available filters
	filtersListView = new Q3ListView(this);
	filtersListView->addColumn( "Filter" );
	filtersListView->addColumn( "Comment" );

	qgrid->addWidget(filtersListView, 0, 0);

	// load filters list...
	filterColl = new Q3PtrList<swPluginView>;
	filterColl->setAutoDelete(false);

	selectedFilterColl = new Q3PtrList<swPluginView>;
	selectedFilterColl->setAutoDelete(false);

	loadFilters();

	chdir(BASE_DIRECTORY "images/pixmaps");
	QDir(BASE_DIRECTORY "images/pixmaps");

	// draw filter list
	swPluginView * curF;

	char lastcat[128] = "";
	char lastsubcat[128] = "";
	Q3ListViewItem *catItem = NULL, *subcatItem = NULL;
	QPixmap bIcon;
	QPixmap lena;
	lena = QPixmap("lena.png");
	filtersListView->setRootIsDecorated( TRUE );

	if(!filterColl->isEmpty()) {
		for(curF = filterColl->first(); curF; curF = filterColl->next())
		{
			if( strcmp(lastcat, curF->filter->category) != 0
				|| catItem == NULL) // different
			{
				// create a new category
				sprintf(lastcat, "%s", curF->filter->category);
				catItem = new Q3ListViewItem(filtersListView);
				catItem->setText( 0, lastcat);
				catItem->setPixmap(0, lena);
				catItem->setOpen(true);
			}
			if( strcmp(lastsubcat, curF->filter->subcategory) != 0
				|| subcatItem == NULL) // different
			{ // create a new subcategory
				sprintf(lastsubcat, "%s", curF->filter->subcategory);
				subcatItem = new Q3ListViewItem(catItem);
				subcatItem->setText( 0, lastsubcat);
				if(curF->icon)
					bIcon.load( curF->icon );
				else
					bIcon.load("filterwide.png");
				subcatItem->setPixmap(0, bIcon);
			}

			// functions items
			curF->availItem = new Q3ListViewItem(subcatItem,
												 curF->filter->funcList[curF->indexFunction].name);
			//curF->availItem->setPixmap(0, bIcon);
		}
	}
	// buttons to add, suppress... filters


	//QVBox * filtersVBox = new QVBox(filtersHBox, 0, 0);
	Q3VBox * filtersVBox = new Q3VBox(this, 0, 0);
	qgrid->addWidget(filtersVBox, 0, 1);

	bAdd = new QPushButton(filtersVBox);
	if(bIcon.load("go-next.png")) {
		bAdd->setPixmap(bIcon);
	} else {
		bAdd->setText(tr("Add"));
	}
	bAdd->setFlat(false);
	bAdd->setPixmap(bIcon);
	bAdd->setToggleButton(false);
	connect(bAdd, SIGNAL(pressed()), this, SLOT(slotAdd()));
	connect( filtersListView, SIGNAL(doubleClicked(Q3ListViewItem *)),
			 this, SLOT(slotPluginDoubleClicAdd(Q3ListViewItem *)));
	QToolTip::add(bAdd, tr("Add plugin"));


	bDel = new QPushButton(filtersVBox);
	if(bIcon.load("go-previous.png")) {
		bDel->setPixmap(bIcon);
	} else {
		bDel->setText(tr("Delete"));
	}
	bDel->setFlat(false);
	bDel->setToggleButton(false);
	connect(bDel, SIGNAL(pressed()), this, SLOT(slotDelete()));
	QToolTip::add(bDel, tr("Remove plugin"));

	bEdit = new QPushButton(filtersVBox);
	if(bIcon.load("configure.png")) {
		bEdit->setPixmap(bIcon);
	} else {
		bEdit->setText(tr("Edit"));
	}
	bEdit->setFlat(false);
	bEdit->setToggleButton(false);
	connect(bEdit, SIGNAL(pressed()), this, SLOT(slotEdit()));
	QToolTip::add(bEdit, tr("Edit plugin's parameters"));

	bDisable = new QPushButton(filtersVBox);
	if(bIcon.load("network-disconnect16.png")) {
		bDisable->setPixmap(bIcon);
	} else {
		bDisable->setText(tr("Disable"));
	}
	bDisable->setFlat(false);
	bDisable->setToggleButton(false);
	connect(bDisable, SIGNAL(pressed()), this, SLOT(slotDisable()));
	QToolTip::add(bDisable, tr("Disable plugin"));


	QPushButton * bTime = new QPushButton(filtersVBox);
	if(bIcon.load("chronometer.png"))
		bTime->setPixmap(bIcon);
	else
		bTime->setText(tr("Time"));
	bTime->setFlat(false);
	bTime->setToggleButton(false);
	connect(bTime, SIGNAL(pressed()), this, SLOT(slotTime()));
	QToolTip::add(bTime, tr("Processing time measure") );

	Q3HBox * bBox = new Q3HBox(filtersVBox, 0);

	bUp = new QPushButton(bBox);
	if(bIcon.load("go-up.png"))
		bUp->setPixmap(bIcon);
	else
		bUp->setText(tr("Up"));
	bUp->setFlat(false);
	bUp->setToggleButton(false);
	connect(bUp, SIGNAL(pressed()), this, SLOT(slotUp()));
	QToolTip::add(bUp, tr("Move up selected plugin"));

	bDown = new QPushButton(bBox);
	if(bIcon.load("go-down.png"))
		bDown->setPixmap(bIcon);
	else
		bDown->setText(tr("Down"));
	bDown->setFlat(false);
	bDown->setToggleButton(false);
	connect(bDown, SIGNAL(pressed()), this, SLOT(slotDown()));
	QToolTip::add(bDown, tr("Move up selected plugin"));

	bBox->updateGeometry();

	// Save and load buttons
	Q3HBox * bBox2 = new Q3HBox(filtersVBox, 0);

	bSave = new QPushButton(bBox2);
	if(bIcon.load("document-save.png"))
		bSave->setPixmap(bIcon);
	else
		bSave->setText(tr("Save list"));
	bSave->setFlat(false);
	bSave->setToggleButton(false);
	connect(bSave, SIGNAL(pressed()), this, SLOT(slotSave()));
	QToolTip::add(bSave, tr("Save plugins list & parameters"));

	bLoad = new QPushButton(bBox2);
	if(bIcon.load("document-open.png"))
		bLoad->setPixmap(bIcon);
	else
		bLoad->setText(tr("Load list"));
	bLoad->setPixmap(bIcon);
	bLoad->setFlat(false);
	bLoad->setToggleButton(false);
	connect(bLoad, SIGNAL(pressed()), this, SLOT(slotLoad()));
	QToolTip::add(bLoad, tr("Load a plugins list & parameters"));


	bBox2->updateGeometry();

	filtersVBox->updateGeometry();

	selectedListView = new Q3ListView(this, 0, 0);
	qgrid->addWidget(selectedListView, 0, 2);

	//selectedListView->setMinimumSize(100,200);
	//selectedListView->updateGeometry();
	selectedListView->setMultiSelection(true);
	selectedListView->setSelectionMode(Q3ListView::Extended);
	selectedListView->addColumn( "Filter" );
	selectedListView->addColumn( "Use" );
	selectedListView->addColumn( "View" );
	selectedListView->setSorting(-1, FALSE);
	selectedListView->setAllColumnsShowFocus( TRUE );

	connect( selectedListView, SIGNAL(currentChanged(Q3ListViewItem*)),
			 this, SLOT(simpleClic(Q3ListViewItem *)));
	connect( selectedListView, SIGNAL(doubleClicked(Q3ListViewItem *)),
			 this, SLOT(doubleClic(Q3ListViewItem *)));

	updateSelectedView();

/*	filtersHBox->setMinimumSize(500, 200);
	filtersHBox->updateGeometry();
  */

	// list  initialisation

//	filtersList =
	//setMinimumSize(500,200);
	setFixedSize(550,260);
	updateGeometry();
	setCaption(tr("Filters Selection Window"));
	update();

	show();
}


// destructor.
SwFilterManager::~SwFilterManager()
{
	fprintf(stderr, "SwFilterManager::%s:%d DELETE SwFilterManager\n", __func__, __LINE__);

	// empty selected filter list (to kill child process)
	swPluginView * pv;
	if(!selectedFilterColl->isEmpty())
	for(pv = selectedFilterColl->first(); pv; pv = selectedFilterColl->next())
	{
		removeFilter(pv);
	}

	// empty available filter list
	SwFilter * lastF = NULL;
	if(!filterColl->isEmpty())
		for(pv = filterColl->first(); pv; pv = filterColl->next())
		{
			if(pv->filter != lastF) {
#ifdef __SWPLUGIN_MANAGER__
				fprintf(stderr, "Deleting filter '%s'[%d]='%s' from available filterColl...\n",
					pv->filter->exec_name, pv->indexFunction, pv->filter->funcList[pv->indexFunction].name);
					fflush(stderr);
#endif
				lastF = pv->filter;

				DELETE_FILTER(pv)

			}
		}

	// clear frame struct
	swFreeFrame(&frame);
}






/********************************************************************

					FILTER LIST MANAGEMENT

 ********************************************************************/
/* add a new filter in filters list.
		@param id -1 for end of list, else rank from 0
*/
int SwFilterManager::addFilter(swPluginView * newFilter, int id)
{
	if(strlen(mPluginSequenceFile) == 0)
	{
		strcat(mPluginSequenceFile, "(custom)");
	}

	if(id==-1)
	{
		selectedFilterColl->append(newFilter);
		idEditPlugin = selectedFilterColl->count()-1;
		idViewPlugin = idEditPlugin;
	}
	else {
		if(id > (int)selectedFilterColl->count())
			return -1;
		else
		{
			selectedFilterColl->insert( (uint)id, newFilter );
		}
	}

	// start process
//    newFilter->filter->loadChildProcess();

	return selectedFilterColl->count();
}

// get filter pointer from id.
swPluginView * SwFilterManager::getFilter(int id)
{
	return selectedFilterColl->at(id);
}

// read number of filters into list
int SwFilterManager::getFilterCount()
{
	return selectedFilterColl->count();
}

void SwFilterManager::slotFilterDied(int pid)
{
	if(selectedFilterColl->isEmpty()) return;

	fprintf(stderr, "[SwFilterMng]::%s:%d : dead filter pid=%d...\n",
			__func__, __LINE__, pid);

	swPluginView * fil = NULL;
	for(fil = selectedFilterColl->first(); fil != NULL; )
	{
		if(fil->filter->getChildPid() == pid)
		{
			if(!mNoWarning ) {
				QMessageBox::critical(NULL,
					tr("Filter Error"),
					tr("Filter process ") + fil->selItem->text(0).latin1() + tr(" died."),
					QMessageBox::Abort,
					QMessageBox::NoButton,
					QMessageBox::NoButton);
			}
			removeFilter(fil);

			fil = selectedFilterColl->current();
		} else
			fil = selectedFilterColl->next();
	}

	emit selectedFilterChanged();
}

int SwFilterManager::removeFilter(int id)
{
	// unload child process
	swPluginView * pv = selectedFilterColl->at((uint)id);
	bool r = false;
	bool reset = false;

	if(pv) {
		if(id==idEditPlugin || id==idViewPlugin || id==(int)selectedFilterColl->count()-1)
			reset = true;

		pv->filter->unloadChildProcess();

		r = selectedFilterColl->remove((uint)id);
		DELETE_FILTER(pv)

		if(reset){
			if(!selectedFilterColl->isEmpty()) {
				idEditPlugin = selectedFilterColl->count()-1;
				idViewPlugin = idEditPlugin;
			}
			else {
				idViewPlugin = -1;
				idEditPlugin = -1;
			}
		}

	}

	return (r ? 1 : 0);
}


int SwFilterManager::removeFilter(swPluginView * pv)
{

	// unload child process
	uint r = selectedFilterColl->contains(pv);
	bool reset = false;
	if(r>0
	   && pv) {
		int id = r;
		if(id==idEditPlugin || id==idViewPlugin || id==(int)selectedFilterColl->count()-1)
		{
			reset = true;
		}

		if(pv->filter) {
			pv->filter->unloadChildProcess();
		} else {
			fprintf(stderr, "[SwFilterMngr]::%s:%d: filter=NULL for pv=%p\n", __func__, __LINE__,
					pv);
		}

		fprintf(stderr, "%s:%d : removing...\n", __func__, __LINE__);
		if(pv->selItem) {
			selectedListView->removeItem(pv->selItem);
			pv->selItem = NULL;
		}

		fprintf(stderr, "%s:%d : removing item...\n", __func__, __LINE__);
		r = selectedFilterColl->remove(pv);

		fprintf(stderr, "%s:%d : removing pv...\n", __func__, __LINE__);

		DELETE_FILTER(pv);
		fprintf(stderr, "%s:%d : removed...\n", __func__, __LINE__);

		if(reset){
			if(!selectedFilterColl->isEmpty()) {
				idEditPlugin = selectedFilterColl->count()-1;
				idViewPlugin = idEditPlugin;
			}
			else {
				idViewPlugin = -1;
				idEditPlugin = -1;
			}
		}

		return (int)r;
	}
	return 0;
}



/* ------------------- Filter loading ---------------------
*/
int SwFilterManager::loadFilters()
{
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
	char line[512];

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
						loadFilter(line, arg);
					}
					else
						loadFilter(line, NULL);
				} else {
					char *stepper = line+strlen(line)-1;
					while( stepper > line && (*stepper == ' ' || *stepper == '\n' || *stepper == '\t')) {
						*stepper = '\0';
						stepper--;
					}
					loadFilter(line, NULL);
				}
			}

		}
		line[0] = '#';
	}

	fclose(f);
	return 1;

}

int SwFilterManager::loadFilter(char *filename, char *bIconname)
{
	// fork and ask plugin
	fprintf(stderr, "[SwFilterMng]::%s:%d : Loading filter file '%s'...\n", __func__, __LINE__, filename);
	FILE * f = fopen(filename, "rb");

	if(!f) {
		fprintf(stderr, "[SwFilterMng]::%s:%d : Filter file '%s' does not exist !!\n", __func__, __LINE__, filename);
		return 0;
	}
	fclose(f);

	// allocate first filter
	SwFilter * newFilter = new SwFilter(filename,
			false /* kill process after loading properties */);
	connect(newFilter, SIGNAL(signalDied(int)), this, SLOT(slotFilterDied(int)));

	newFilter->setWorkspace(pWorkspace);
	char *bIcon = NULL;
	if(bIconname) {
		bIcon = new char [strlen(bIconname)+1];
		strcpy(bIcon, bIconname);
	}

	for(int i=0; i<newFilter->nbfunctions(); i++) {
		swPluginView * pv = new swPluginView;
		memset(pv, 0, sizeof(swPluginView));
		pv->availItem = NULL;
		pv->filter = newFilter;
		pv->indexFunction = i;
		pv->icon = bIcon;

		fprintf(stderr, "[SwFilterMng]::%s:%d : append filter (exec='%s') / func [ %d/%d ]='%s'\n", __func__, __LINE__,
				pv->filter->exec_name,
				i, pv->filter->nbfunctions(),
				pv->filter->funcList[i].name
				);

		filterColl->append(pv);
	}

	return 1;
}



// update selected filters QListView.
void SwFilterManager::updateSelectedView()
{
	// clear listview then add filters
	swPluginView * curF;

	selectedListView->clear();

	// --- add new items ---
	chdir(BASE_DIRECTORY "images/pixmaps");

	// add the other items
	QPixmap eye(":images/16x16/layer-visible-on.png");
	QPixmap eye_grey(":images/16x16/layer-visible-off.png");

	QPixmap check("check.png");
	QPixmap uncheck("uncheck.png");

	int count = (int)selectedFilterColl->count()-1;
	if(idViewPlugin > count)
		idViewPlugin = count;
	if(idEditPlugin > count)
		idEditPlugin = count;

	int i = selectedFilterColl->count()-1;
	if(!selectedFilterColl->isEmpty()) {
		for(curF = selectedFilterColl->last(); curF; curF = selectedFilterColl->prev()) {
			if(curF->filter && curF->filter->funcList) {
				curF->selItem = new Q3ListViewItem(selectedListView,
					curF->filter->funcList[curF->indexFunction].name, "" );
				if(curF->enabled)
					curF->selItem->setPixmap(1, check);
				else
					curF->selItem->setPixmap(1, uncheck);

				if(i == idViewPlugin) {
					curF->selItem->setPixmap(2, eye);
				}
				else
					if(i<idViewPlugin)
						curF->selItem->setPixmap(2, eye_grey);

				if(i == idEditPlugin)
					selectedListView->setSelected(curF->selItem, true);
				i--;
			}
			else
			{
				fprintf(stderr, "[SwFilters] %s:%d : invalid function in filter\n", __func__, __LINE__);
			}
		}
	}

	// create an item to visualize original item
	originalItem = new Q3ListViewItem( selectedListView, tr("Original") );

	update();
}

void  SwFilterManager::slotPluginDoubleClicAdd(Q3ListViewItem *item) {
	if(!item) return;

	filtersListView->setSelected(item, TRUE);
	slotAdd();
}

// Add filter function to process list. Now, it reloads the process.
void SwFilterManager::slotAdd()
{
	// adding filter
	// looking for filer plugin
	swPluginView * pv;
	if(!filterColl->isEmpty()) {
		for(pv=filterColl->first(); pv; pv=filterColl->next())
		{
			if(  filtersListView->isSelected(pv->availItem) )
			{
				// then add it to selecttedListView
				swPluginView * newPV = new swPluginView;
				memset(newPV, 0, sizeof(swPluginView));

				newPV->filter = new SwFilter( pv->filter->exec_name, true );// keep it alive
				connect(newPV->filter, SIGNAL(signalDied(int)), this, SLOT(slotFilterDied(int)));

				newPV->indexFunction = pv->indexFunction;
				newPV->enabled = true;

				// add filter to collection
				addFilter(newPV, -1);

#ifdef __SWPLUGIN_MANAGER__
				fprintf(stderr, "[SwFilterMngr]::%s:%d : Added filter '%s' \n", __func__, __LINE__,
						newPV->filter->funcList[newPV->indexFunction].name);
#endif
			}
		}
	}

	idViewPlugin = selectedFilterColl->count()-1;

	updateSelectedView();

	printf("ADD!!!\n");
	emit selectedFilterChanged();
}

void SwFilterManager::slotUnloadAll()
{
	// looking for filter plugin
	swPluginView * pv;
	bool found = true;
	while(found) {
		found = false;
		if(!selectedFilterColl->isEmpty())
		{
			for(pv=selectedFilterColl->first(); pv && !found; pv=selectedFilterColl->next())
			{
				if(  pv->selItem  )
				{
					// then add it to selectedListView
#ifdef __SWPLUGIN_MANAGER__
					fprintf(stderr, "Delete filter '%s' \n", pv->filter->funcList[pv->indexFunction].name);
#endif
					// wait for process flag to go down
					while(lockProcess) {
						usleep(10000);
					}

					removeFilter(pv);
					found = true;
				}
			}
		}
	}
	updateSelectedView();
	emit selectedFilterChanged();

}

void SwFilterManager::slotDelete()
{
	// looking for filter plugin
	swPluginView * pv;
	bool found = true;
	while(found) {
		found = false;
		if(!selectedFilterColl->isEmpty())
		{
			for(pv=selectedFilterColl->first(); pv && !found; pv=selectedFilterColl->next())
			{
				if(  selectedListView->isSelected(pv->selItem) )
				{
					// then add it to selectedListView
#ifdef __SWPLUGIN_MANAGER__
					fprintf(stderr, "Delete filter '%s' \n", pv->filter->funcList[pv->indexFunction].name);
#endif
					// wait for process flag to go down
					while(lockProcess) {
						usleep(10000);
					}

					removeFilter(pv);
					found = true;
				}
			}
		}
	}
	updateSelectedView();
	emit selectedFilterChanged();
}

void SwFilterManager::slotDeleteAll()
{
	fprintf(stderr, "[FilteRManager]::%s:%d : deleting all plugins...\n", __func__, __LINE__);

	bool found = true;
	while(found) {
		found = false;
		if(!selectedFilterColl->isEmpty())
		{
			swPluginView * pv;
			for(pv = selectedFilterColl->first();
				pv && !found; // break loop at every removal
				pv=selectedFilterColl->next())
			{
#ifdef __SWPLUGIN_MANAGER__
				fprintf(stderr, "Delete filter '%s' \n", pv->filter->funcList[pv->indexFunction].name);
#endif
				// wait for process flag to go down
				while(lockProcess) {
					usleep(10000);
				}

				removeFilter(pv);
				found = true;
			}
		}
	}

	// then update display
	updateSelectedView();
}
/*
	move ONE (the first) item up for one step
	*/
void SwFilterManager::slotUp()
{
	// change order into selectedFiltersList
	// change order into selectedFiltersList
	// looking for filer plugin
	swPluginView * pv = NULL, *sel=NULL;
	idEditPlugin = -1;
	int id=0, fid = -1;
	if(!selectedFilterColl->isEmpty())
	for(pv=selectedFilterColl->first(); pv && idEditPlugin<0; pv=selectedFilterColl->next())
	{
		if(  selectedListView->isSelected(pv->selItem) )
		{
			sel = pv;
			idEditPlugin = id;
			fid = id-1;
#ifdef __SWPLUGIN_MANAGER__
			printf("Up filter '%s' at %d\n", pv->filter->funcList[pv->indexFunction].name, fid);
#endif
		}
		id++;
	}
	if(idEditPlugin<0 || fid<0) {
		fprintf(stderr, "Up filter: cannot find or mount filter.\n");
		return;
	}


	// remove then insert at next place
	selectedFilterColl->remove(sel);
	selectedFilterColl->insert(fid, sel);
	idEditPlugin = fid;
	updateSelectedView();

	printf("UP!!!\n");
	emit selectedFilterChanged();

}

/*
	move ONE (the first) item down for one step
	*/
void SwFilterManager::slotDown()
{
	// change order into selectedFiltersList
	// looking for filer plugin
	swPluginView * pv = NULL, *sel = NULL;
	idEditPlugin = -1;
	uint id=0, fid = 0;
	if(!selectedFilterColl->isEmpty())
	for(pv=selectedFilterColl->first(); pv && idEditPlugin<0; pv=selectedFilterColl->next())
	{
		if(  selectedListView->isSelected(pv->selItem) )
		{
			sel = pv;
			fid = id+1;
#ifdef __SWPLUGIN_MANAGER__
			printf("Down filter '%s' \n", pv->filter->funcList[pv->indexFunction].name);
#endif
		}
		id++;
	}
	if(!idEditPlugin<0 || fid>=selectedFilterColl->count() ) {
		fprintf(stderr, "Down filter: cannot find filter.\n");
		return;
	}

	// remove then insert at next place
	selectedFilterColl->remove(sel);
	selectedFilterColl->insert(fid, sel);
	idEditPlugin = fid;
	updateSelectedView();

	printf("DOWN!!!\n");
	emit selectedFilterChanged();
}

void SwFilterManager::simpleClic(Q3ListViewItem * item)
{
	fprintf(stderr, "FilterManager::%s:%d : selected item : %p\n", __func__, __LINE__, item);
	// look for selected item
	if(selectedFilterColl->isEmpty()) return;
	swPluginView * pv;

	// enabled by default
	bEdit->setEnabled(true);

	for(pv=selectedFilterColl->first(); pv ; pv=selectedFilterColl->next())
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



void SwFilterManager::doubleClic(Q3ListViewItem * item)
{
	// look for selected item
	swPluginView * pv;
	int id=0;
	idViewPlugin = -1;
	if(!selectedFilterColl->isEmpty())
	for(pv=selectedFilterColl->first(); pv && idViewPlugin<0; pv=selectedFilterColl->next())
	{
		if(pv->selItem == item) {
			idViewPlugin = id;
#ifdef __SWPLUGIN_MANAGER__
			printf("SELECT FILTER '%s' as end viewing filter\n", pv->filter->funcList[pv->indexFunction].name);
#endif
		}
		id++;
	}
	updateSelectedView();

	emit selectedFilterChanged();
}


/* slot for editing the parameters of a filter = aotogenerated interface
*/
void SwFilterManager::slotEdit()
{
	// ---- look for filer plugin ----
	swPluginView * pv = NULL, *pEditPlugin = NULL;
	idEditPlugin = -1;
	int id=0;
	if(!selectedFilterColl->isEmpty()) {
		for(pv=selectedFilterColl->first(); pv && idEditPlugin<0; pv=selectedFilterColl->next())
		{
			if(  selectedListView->isSelected(pv->selItem) )
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
					fprintf(stderr, "[SwFilters] %s:%d insert item [%d/%d]='%s'\n",
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

void SwFilterManager::slotTime() {
	// ---- look for filer plugin ----
	swPluginView * pv = NULL, *pEditPlugin = NULL;
	idEditPlugin = -1;
	int id=0;
	if(!selectedFilterColl->isEmpty())
	for(pv=selectedFilterColl->first(); pv && idEditPlugin<0; pv=selectedFilterColl->next())
	{
		if(  selectedListView->isSelected(pv->selItem) )
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
		fprintf(stderr, "[SwFiltersMngr]::%s:%d : func is null !\n", __func__, __LINE__);
	} else {
		fprintf(stderr, "[SwFiltersMngr]::%s:%d : create time for func '%s'\n", __func__, __LINE__, func->name);
		pEditPlugin->mwPluginTime = new SwTimeWidget(pWorkspace, func->name, 0);

		if(pWorkspace) {
			pWorkspace->addWindow((QWidget *)pEditPlugin->mwPluginTime);
		}
		pEditPlugin->mwPluginTime->show();
	}

}

void SwFilterManager::applyEditParameters()
{
	swPluginView * pEditPlugin = selectedFilterColl->at(idEditPlugin);
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

void SwFilterManager::cancelEditParameters()
{


}




void SwFilterManager::slotDisable()
{
	// looking for filer plugin
	swPluginView * pv = NULL, *sel = NULL;
	idEditPlugin = -1;

	int id=0;
	if(!selectedFilterColl->isEmpty())
	for(pv=selectedFilterColl->first(); pv && idEditPlugin<0; pv=selectedFilterColl->next())
	{
		if(  selectedListView->isSelected(pv->selItem) )
		{
			idEditPlugin = id;
			sel = pv;
#ifdef __SWPLUGIN_MANAGER__
			printf("Disable filter '%s' \n", pv->filter->funcList[pv->indexFunction].name);
#endif
		}
		id++;
	}
	if(idEditPlugin<0) {
		fprintf(stderr, "Disable filter: cannot find filter.\n");
		return;
	}

	sel->enabled = !sel->enabled;
	updateSelectedView();

	printf("ENABLE/DISABLE!!!\n");
	emit selectedFilterChanged();
}





/* slotSave : saves filters configuration into a script file for later opening
Format is simple : exec name and function index are needed, then parameters,
	which are simply read from plugin.
*/
#define FILTERLIST_EXTENSION ".flist"

void SwFilterManager::slotSave()
{
	printf("SwFilterManager::slotSave()...\n");
	if(selectedFilterColl->isEmpty()) {
		bSave->setDown(false);
		return;
	}
	QString fileName = Q3FileDialog::getSaveFileName(0,
		"Filter list (*.flist)",
		this);
	if (!fileName.isEmpty())
	{
		char fname[256];
		strcpy(fname, (char *)fileName.latin1());
		if(!strstr(fname, FILTERLIST_EXTENSION))
			strcat(fname, FILTERLIST_EXTENSION);
		saveFilterList(fname);
	}
	bSave->setDown(false);

}

void SwFilterManager::saveFilterList(char * filename)
{
	if(selectedFilterColl->isEmpty())
		 return;
	printf("Save filter list into file '%s'\n", filename);

	// process each image
	swPluginView * pv;

	FILE * f = fopen(filename, "w");
	if(!f)
	{
		int errnum = errno;
		fprintf(stderr, "SwFilters::%s:%d : error while opening file '%s' : %d='%s'\n",
				__func__,__LINE__,
				filename, errnum, strerror(errnum)
				);
		return;
	}

	for(pv = selectedFilterColl->first(); pv; pv = selectedFilterColl->next()) {
		// process
#ifdef __SWPLUGIN_DEBUG__
		fprintf(stderr, "FILTERMANAGER: saving filter %s\n",
				pv->filter->funcList[pv->indexFunction].name);
#endif
		fprintf(f, "%s\t%d\n",
				pv->filter->exec_name,
				pv->indexFunction);
		// --- read parameters
		// send request and wait for answer
		pv->filter->sendRequest(SWFRAME_ASKFUNCTIONDESC);

		// then save reply string
		char txt[256];
		if(pv->filter->pipeR) {
			fgets(txt, 256, pv->filter->pipeR);
#ifdef __SWPLUGIN_DEBUG__
			fprintf(stderr, "slotSave : read answer '%s'\n", txt);
#endif
			fprintf(f, "%s", txt);
		}
	}

	fclose(f);
}


/* read configuration file for filters order
*/
void SwFilterManager::slotLoad()
{
	printf("SwFilterManager::slotLoad()...\n");
	QString fileName = Q3FileDialog::getOpenFileName(0,
		tr("Filter sequence (*.flist)"),
		this );
	if (!fileName.isEmpty())
	{
		char fname[256];
		strcpy(fname, (char *)fileName.latin1());
		if(!strstr(fname, FILTERLIST_EXTENSION)) {
			strcat(fname, FILTERLIST_EXTENSION);
		}

		loadFilterList(fname);
	}

	bLoad->setDown(false);
}

int SwFilterManager::loadFilterList(char * filename)
{
	fprintf(stderr, "[SwFilterMngr]::%s:%d : Loading file '%s'...\n", __func__, __LINE__, filename);
	// process each image

	FILE * f = fopen(filename, "r");
	if(!f) {
		fprintf(stderr, "[SwFilterMngr]::%s:%d : cannot load file '%s'\n", __func__, __LINE__, filename);
		return -1;
	}

	if(mPluginSequenceFile != filename) {
		strcpy(mPluginSequenceFile, filename);
	}

	//
	swPluginView * pv;
	char line[512];
	while(!feof(f))
	{
		// read exec name and function index
		fgets(line, 512, f);

		// check if filter is available
		swPluginView * foundPV = NULL;
		if(!filterColl->isEmpty())
		{
			for(pv = filterColl->first(); pv && !foundPV; pv=filterColl->next()) {
				if(pv->filter) {
					if(strncmp(pv->filter->exec_name, line, strlen((pv->filter->exec_name)))==0) {
						foundPV = pv;
					}
				} else {
					fprintf(stderr, "[SwFilterMngr]::%s:%d : no filter for pv=%p\n", __func__, __LINE__, pv);
				}
			}
		}

		int id = -1;
		if(!foundPV) {
			fprintf(stderr, "[SwFilterMngr]::%s:%d : Filter exec '%s' seems not to be unknown.\n", __func__, __LINE__, line);
		} else {
			// read function index
			char * pid = nextTab(line);
			if(pid)
			{
				sscanf(pid, "%d", &id);
				if(id >= foundPV->filter->nbfunctions()) {
#ifdef __SWPLUGIN_DEBUG__
					fprintf(stderr, "slotLoad: read function index for filter from exec '%s' is superior to function number %d\n",
						foundPV->filter->exec_name, foundPV->filter->nbfunctions());
#endif
					id = -1;
				}
			}
			else {
				fprintf(stderr, "[SwFilters]::%s:%d: Cannot read function index for filter from exec '%s'\n",
						__func__, __LINE__,
						foundPV->filter->exec_name);
			}
		}

		// read parameters
		fgets(line, 512, f);

		// after checking load filter
		if(id>=0 && foundPV)
		{
			swPluginView * newPV = new swPluginView;
			memset(newPV, 0, sizeof(swPluginView));

			newPV->filter = new SwFilter( foundPV->filter->exec_name, true );// keep it alive
			connect(newPV->filter, SIGNAL(signalDied(int)), this, SLOT(slotFilterDied(int)));

			newPV->indexFunction = id;
			newPV->enabled = true;

			// No widget at start
			newPV->mwPluginEdit = NULL;
			newPV->mwPluginTime = NULL;
			newPV->selItem = NULL;

			// add filter to collection
			addFilter(newPV, -1);

#ifdef __SWPLUGIN_DEBUG__
			fprintf(stderr, "[SwFilterMng]::%s:%d : Loaded filter '%s' \n", __func__, __LINE__,
					newPV->filter->funcList[newPV->indexFunction].name);
#endif

			// send parameters
			if(newPV->filter->pipeW) {
				fprintf(newPV->filter->pipeW, "%s", line);
			}
		}
	}


	fclose(f);

	updateSelectedView();

	return 0;
}








/******************************************************************
IMAGE PROCESSING  SECTION
*/




void SwFilterManager::processImage(swImageStruct * image)
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

	if(selectedFilterColl->isEmpty()) {
		return;
	}
	// process each image

	int step = 0;
	int ret = 1;

	if(!lockProcess) {
		lockProcess = true;

		swPluginView * pv;
		swImageStruct *im1 = image, *im2 = imageTmp;

		ret = 1;
		int id=0;

		if(!selectedFilterColl->isEmpty())
		{
			for(pv = selectedFilterColl->first(); pv && ret && id<=idViewPlugin; pv = selectedFilterColl->next()) {
				// process
				if( pv->enabled ) {
#ifdef __SWPLUGIN_DEBUG_
					fprintf(stderr, "FILTERMANAGER: processing filter func[%d]=%s\n",
							pv->indexFunction,
							pv->filter->funcList[pv->indexFunction].name);
#endif
					ret = pv->filter->processFunction(pv->indexFunction, im1, im2, 2000 );
					if(ret)
					{
						step++;

						// invert buffers
						swImageStruct *imTmp = im2;
						im2 = im1;
						im1 = imTmp;

						// time statistics
						if( pv->mwPluginTime ) {
							pv->mwPluginTime->setTimeUS( imTmp->deltaTus );
						}
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
	}

}


void SwFilterManager::closeEvent(QCloseEvent*)
{
	hide();
}
void SwFilterManager::showWindow()
{
	if(isHidden())
		show();
	else
		setFocus();
}










/**************************************************************************
 *                                                                        *
 *                                                                        *
 *                             SWFILTER                                   *
 *                                                                        *
 *                                                                        *
 **************************************************************************/
SwFilter::SwFilter(char * filename, bool keepAlive)
{
	init();

	// store filename
	exec_name = new char [ strlen(filename)+2 ];
	strcpy(exec_name, filename);
	exec_name[ strlen(filename) ]='\0';



	// --- reads plugin properties
	swAllocateFrame(&frame, 1024);

	if(!loadChildProcess()) {
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
	if(!keepAlive) {
		// unload process
		unloadChildProcess();
	}
}



void SwFilter::init()
{
	exec_name = NULL;
	category = NULL;
	subcategory = NULL;

	comLock = false;
	buffer = new char [BUFFER_SIZE];

	// functions
	funcList = NULL;
	nb_func = 0;
	childpid = 0;

	// pipes initialisation
	pipeIn[0]  = 0;
	pipeIn[1]  = 0;
	pipeOut[0] = 0;
	pipeOut[1] = 0;
	pipeR = NULL;
	pipeW = NULL;
	statusOpen = false;
}

SwFilter::~SwFilter()
{
	destroy();
}

int SwFilter::destroy() {
	fprintf(stderr, "[SwFilter]::%s:%d : DESTROYing SwFilter %p='%s'...\n",
			__func__, __LINE__, this, exec_name);


	if(!exec_name)
		return 0;
	if(exec_name[0] == '\0')
		return 0;

	if(statusOpen) {
		unloadChildProcess();
	}

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

	fprintf(stderr, "[SwFilter]::%s:%d : DESTROYed SwFilter %p.\n", __func__, __LINE__, this);

	return 1;
}

extern int registerChildSig(int pid, SwFilter * filter);
extern int removeChildSig(int pid);

// when pipe is broken
int SwFilter::forceCloseConnection() {
	fprintf(stderr, "SwFilter::forceCloseConnection : closing write pipe...\n");
	fflush(stderr);
//	if(pipeW)
//		fclose(pipeW);
	pipeW = NULL;

	fprintf(stderr, "SwFilter::forceCloseConnection : closing read pipe to child %d...\n",childpid );
	fflush(stderr);
	//if(pipeR)
	//	fclose(pipeR);
	pipeR = NULL;


	removeChildSig(childpid);
	statusOpen = false;
	emit signalDied(childpid);

	childpid = 0;
	return 1;
}

/* Function waitForAnswer. Wait for an answer from plugin then treat it
	@author CSE
*/
int SwFilter::waitForAnswer(int timeout)
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
int SwFilter::loadChildProcess()
{
	if(!exec_name) return 0;

	// Go to this directory
	char path[512]="";
	sprintf(path, exec_name);
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

	fprintf(stderr, "SwFilter::%s:%d : file='%s' => path='%s'\n", __func__, __LINE__,
			exec_name, path
			);


	// fork and
	// Commmunication processing
	if( pipe( pipeOut ) == -1 || pipe( pipeIn ) == -1)
	{
		perror("Error in pipe()");
	}
	else
	{
		childpid = fork();
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
			printf("PARENT PROCESS ====>\n");
			close( pipeIn[1] );

			// set non blocking
			//fcntl(pipeIn[0], F_SETFL, fcntl(pipeIn[0], F_GETFL) | O_NONBLOCK);

			pipeR = fdopen( pipeIn[0], "r");

			if( pipeR == NULL)
			{
				perror("Parent C2P fdopen() error");
			}

			// Parent to Child
			close( pipeOut[0] );

			// set non blocking
			//fcntl(pipeOut[1], F_SETFL, fcntl(pipeOut[1], F_GETFL) | O_NONBLOCK);
			pipeW = fdopen( pipeOut[1], "w");
			if( pipeW == NULL)
			{
				perror("Parent C2P fdopen() error");
				return 0;
			}
			statusOpen = true;

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

void SwFilter::setReadFlag(int b)
{
	readFlag = (b?true:false);

}


// Unload child process
int SwFilter::unloadChildProcess()
{
	if(!statusOpen)
		return 0;

	removeChildSig(childpid);

	// send quit message
//#ifdef __SWPLUGIN_MANAGER__
	fprintf(stderr, "unloadChildProcess : Sending quit message to child '%s'\n", exec_name);
	fflush(stderr);
//#endif
	if(pipeR) {
		fclose(pipeR);
	}

	sendRequest(SWFRAME_QUIT);
//	if(pipeW)
//		fclose(pipeW);

	statusOpen = false;
	usleep(100000);
//#ifdef __SWPLUGIN_MANAGER__
	fprintf(stderr, "Killing child '%s'\n", exec_name);
	fflush(stderr);
//#endif


	while(waitpid(childpid, NULL, WNOHANG) >0) {
		fprintf(stderr, "waitpid !\n");
	}

	childpid = 0;

	return 1;
}

// Returns functions number.
int SwFilter::nbfunctions()
{
	return nb_func;
}

// Wait until comLock flag came to false.
bool SwFilter::waitForUnlock(int timeout_ms)
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
void SwFilter::sendRequest(char * req)
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

int SwFilter::processFunction(int indexFunction, void * data_in, void * data_out, int timeout_ms)
{
	// wait for unlock on stdin/out
	if(waitForUnlock(200)) {

		comLock = true; // lock
		int ret = 0;
		if(pipeW) {
			ret = swSendImage(indexFunction, &frame, swImage, data_in, pipeW);
		}

		// read image from pipeR
		if(ret) {
			if(pipeR) {
				ret = swReceiveImage(data_out, pipeR, timeout_ms);
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
int SwFilter::treatFrame(char *frame, int framesize)
{
	if(framesize< (int)(strlen(SWFRAME_HEADER)+2+strlen(SWFRAME_END)))
		return 0;

	// Decompose frame
	// First arg =? SWFRAME_HEADER ??
	char * header = strstr(frame, SWFRAME_HEADER );
	if( ! header )
	{
		fprintf(stderr, "SwFilter: invalid header from child.\n");
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
		printf("\tReceived category = '%s'\tsub-category='%s'",
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
