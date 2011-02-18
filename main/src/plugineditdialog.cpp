#include "inc/plugineditdialog.h"
#include "ui_plugineditdialog.h"
#include <QFileDialog>
#include <QStringList>
#include <errno.h>

PluginEditDialog::PluginEditDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PluginEditDialog)
{
    ui->setupUi(this);

	lastPluginDir = QString("/home");
	char home[512]="/usr/local/piaf";

	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
		lastPluginDir = QString(home);
	}

//	pluginListView->setColumnWidth(0, 200);

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
				if(!cmd.isNull()
					&& !cmd.isEmpty()) {

					QFileInfo fi(cmd);
					lastPluginDir = fi.absoluteFilePath();

					itCmd++;
					QString val = *itCmd;
					cmd = cmd.stripWhiteSpace();

					if(!(cmd.isEmpty() || cmd[0] == '#' || cmd[0] == '\n'))
					{
						QListWidgetItem * item = new QListWidgetItem(cmd, ui->pluginListWidget);
						val = val.stripWhiteSpace();
						fprintf(stderr, "plugins : '%s' : '%s'\n",
								cmd.latin1(), val.latin1());
						if(!val.isEmpty()) {
							if(pixIcon.load(val)) {
								item->setIcon(pixIcon);
							}
						}

					}
				}
			}
			file.close();
		}
	} else {
		fprintf(stderr, "Cannot read file '%s' \n", home);
	}

}

PluginEditDialog::~PluginEditDialog()
{
    delete ui;
}

void PluginEditDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void PluginEditDialog::on_addButton_clicked()
{
	QStringList files = QFileDialog::getOpenFileNames(NULL,
													  tr("Choose one or more plugin(s) to add"),
													  lastPluginDir,
													  tr("*"));

	QStringList list = files;
	QStringList::Iterator it = list.begin();
	while( it != list.end() ) {
		// Store last plugin dir
		QFileInfo fi(*it);
		if(fi.exists() && fi.isExecutable()) {
			lastPluginDir = fi.dirPath(TRUE);

			// check if it is not already listed
			QList<QListWidgetItem *> found = ui->pluginListWidget->findItems(*it, Qt::MatchExactly);
			if(found.isEmpty())
			{
				/*QListViewItem * item = */
				new QListWidgetItem(*it, ui->pluginListWidget);
			}
		}
		++it;
	}

}


void PluginEditDialog::on_iconButton_clicked()
{

}

void PluginEditDialog::on_delButton_clicked()
{
	bool empty = false;
	int idx = 0;
	QList<QListWidgetItem *> selItems = ui->pluginListWidget->selectedItems();
	QList<QListWidgetItem *>::iterator it;
	for	(it = selItems.begin(); it != selItems.end(); ++it)
	{
		QListWidgetItem * item = *it;
		fprintf(stderr, "\tDelete item %p='%s'\n",
				item, item->text().toAscii().data());
		ui->pluginListWidget->takeItem( ui->pluginListWidget->row(item) );
	}
}


void PluginEditDialog::on_saveButton_clicked()
{
	char home[512]="/usr/local/piaf";

	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
	}
	strcat(home, "/.piaf_plugins");
	// saving
	FILE * f = fopen(home, "w");
	if(f)
	{
		for(int idx = 0; idx < ui->pluginListWidget->count(); idx++)
		{
			QListWidgetItem * item = ui->pluginListWidget->item(idx);
			if(!item->text().isEmpty()) {
				if(item->icon().isNull())
					fprintf(f, "%s\t\n", item->text().toAscii().data());
				else
					fprintf(f, "%s\t\n", item->text().toAscii().data());
/// \todo : fixme 	fprintf(f, "%s\t%s\n", item->text(0).latin1(), item->text(1).latin1());
			}
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
