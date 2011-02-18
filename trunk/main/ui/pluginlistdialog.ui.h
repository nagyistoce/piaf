/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

#include <qfile.h>
#include <qfileinfo.h>
#include <q3filedialog.h>
#include <qdir.h>
//Added by qt3to4:
#include <QPixmap>
#include <stdlib.h>
#include <errno.h>

void PluginListDialog::slotAddPlugin()
{
	QStringList files = Q3FileDialog::getOpenFileNames(
			"",
			lastPluginDir,
			this,
			tr("Add plugin dialog"),
			tr("Choose one or more plugin(s)"));

	QStringList list = files;
    QStringList::Iterator it = list.begin();
    while( it != list.end() ) {
		// Store last plugin dir
		QFileInfo fi(*it);
		if(fi.exists()) {
			lastPluginDir = fi.dirPath(TRUE);
			/*QListViewItem * item = */
			new Q3ListViewItem(pluginListView, *it , "");

			++it;
		}
	}
}


void PluginListDialog::slotDelPlugin()
{

	// Delete all selected items
	if(!selected) return;
    
	pluginListView->removeItem(selected);
	
	selected = NULL;
}


void PluginListDialog::slotIconPlugin()
{
	if(!selected) return;

	QFileInfo fInfo(selected->text(0));	
	QString s = Q3FileDialog::getOpenFileName(
			fInfo.dirPath(TRUE),
			"",
			this,
			tr("open icon dialog"),
			tr("Choose an icon" ));
	if(!s.isNull()) {
		QPixmap pixIcon;
		if(pixIcon.load(s)) {
			selected->setPixmap(0, pixIcon);
			selected->setText(1, s);
		}
	}
}


void PluginListDialog::slotSelectPlugin( Q3ListViewItem * item )
{	
	selected = item;
}


void PluginListDialog::accept()
{
	char home[512]="/usr/local/piaf";
	
	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
	}
	strcat(home, "/.piaf_plugins");
    // saving
	FILE * f = fopen(home, "w");
	if(f) {
		Q3ListViewItemIterator it( pluginListView );
		while ( it.current() ) {
			Q3ListViewItem * item = *it;
			if(!item->text(0).isEmpty()) {
				if(item->text(1).isNull())
					fprintf(f, "%s\t\n", item->text(0).latin1());
				else
					fprintf(f, "%s\t%s\n", item->text(0).latin1(), item->text(1).latin1());
			}
			++it;
		}
		fclose(f);
	}
	else {
		int errnum = errno;
		fprintf(stderr, "[%s] %s:%d : Plugin list : Cannot open file '%s' for writing ! err=%d='%s'\n",
				__FILE__, __func__, __LINE__,
				home, errnum, strerror(errnum));
	}
	
    close();
}


void PluginListDialog::init()
{
	lastPluginDir = QString("/home");
	char home[512]="/usr/local/piaf";
	
	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
		lastPluginDir = QString(home);
	}
	
    selected = NULL;
    pluginListView->setColumnWidth(0, 200);
    
    // load plugin list
    QFile file;
    strcat(home, "/.piaf_plugins");
    file.setName(home);
    
    if(file.exists()) {
		if(file.open(QIODevice::ReadOnly)) {
			char str[512];
			QPixmap pixIcon;
			while(file.readLine(str, 512) >= 0) {
				// split
				QStringList lst( QStringList::split( "\t", str ) );
				QStringList::Iterator itCmd = lst.begin();
				QString cmd = *itCmd;
				if(!cmd.isNull()) 
					if(!cmd.isEmpty()) {
					itCmd++;
					QString val = *itCmd;
					cmd = cmd.stripWhiteSpace();

					Q3ListViewItem * item = new Q3ListViewItem(pluginListView, 
															   cmd, "");
					val = val.stripWhiteSpace();
					fprintf(stderr, "plugins : '%s' : '%s'\n",
							cmd.latin1(), val.latin1());
					if(!val.isNull()) {
						item->setText(1, val);
						if(pixIcon.load(val)) {
							item->setPixmap(0, pixIcon);
						}
					}
					
					if(cmd.isEmpty() || cmd[0] == '#' || cmd[0] == '\n') {
						item->setVisible(FALSE);
					}
					
				}
			}
			file.close();
		}
	} else {
		fprintf(stderr, "Cannot read file '%s' \n", home);
	}

}
