/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/
#include <stdlib.h>
#include <qfile.h>
#include <q3filedialog.h>
#include <qdir.h>

void PiafConfigurationDialog::init()
{
	char home[512] = "/home";
	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
	}
    
    homeDir = QString(home);
    
    // Read configuration directories
    QFile file;
	file.setName(homeDir + "/.piafrc");
    // create directories
    measureDir = homeDir + "/Measures";
    imageDir = homeDir + "/Images";
    movieDir = homeDir + "/Movies";
    saveAtExit = true;
    
	if(file.exists()) {
		if(file.open(QIODevice::ReadOnly)) {
			QString str;
			while(!file.atEnd()) {
			//while(file.readLine(str, 512) >= 0) {
				str = file.readLine();

				// split
				QStringList lst( QStringList::split( "\t", str ) );
				QStringList::Iterator itCmd = lst.begin();
				QString cmd = *itCmd;
				if(!cmd.isNull()) {
					itCmd++;
					QString val = *itCmd;
					fprintf(stderr, ".piafrc : '%s' : '%s'\n",
						cmd.latin1(), val.latin1());
					if(!val.isNull()) {
						if( cmd.contains("MeasureDir" ))
							measureDir = val;
						if( cmd.contains("ImageDir" ))
							imageDir = val;
						if( cmd.contains("MovieDir" ))
							movieDir = val;
						if( cmd.contains("SaveOnExit")) {
							saveAtExit = val.contains("true", FALSE);
						}
					}
				}
			}
		}
	} else {
		fprintf(stderr, "Cannot read file $HOME/.piafrc \n");
	}

	// set values
	defaultMeasureLineEdit->setText(measureDir);
	defaultImageLineEdit->setText(imageDir);
	defaultMovieLineEdit->setText(movieDir);
	saveAtExitCheckBox->setChecked(saveAtExit);    
}

void PiafConfigurationDialog::slotImageChanged( const QString &str )
{
    QDir dir(str);
    if(dir.exists()) {
	imageDir = str;
		buttonOk->setEnabled(true);
    }
    else {
		buttonOk->setEnabled(false);
    }
}


void PiafConfigurationDialog::slotMeasureChanged( const QString &str )
{
    QDir dir(str);
    if(dir.exists()) {
	measureDir = str;
		buttonOk->setEnabled(true);
    }
    else {
		buttonOk->setEnabled(false);
    }
    
}


void PiafConfigurationDialog::slotMovieChanged( const QString &str )
{
    QDir dir(str);
    if(dir.exists()) {
	movieDir = str;
	buttonOk->setEnabled(true);
    }
    else {
	buttonOk->setEnabled(false);
    }

}


void PiafConfigurationDialog::slotImagePushed()
{
	QString s = Q3FileDialog::getExistingDirectory(
		imageDir,
		NULL,
		"get existing directory",
		tr("Choose a directory for images"),
		TRUE, TRUE );
	if(!s.isNull()) {
		imageDir = s;
		defaultImageLineEdit->setText(imageDir);
		buttonOk->setEnabled(true);
	}
}


void PiafConfigurationDialog::slotMoviePushed() {
 	QString s = Q3FileDialog::getExistingDirectory(
		movieDir,
		NULL,
		"get existing directory",
		tr("Choose a directory for movies"),
		TRUE, TRUE );
	if(!s.isNull()) {
		movieDir = s;
		defaultMovieLineEdit->setText(movieDir);
		buttonOk->setEnabled(true);
	}
}


void PiafConfigurationDialog::slotMeasurePushed()
{
 	QString s = Q3FileDialog::getExistingDirectory(
		measureDir,
		NULL,
		"get existing directory",
		tr("Choose a directory for measures"),
		TRUE, TRUE );
	if(!s.isNull()) {
		measureDir = s;
		defaultMeasureLineEdit->setText(measureDir);
		buttonOk->setEnabled(true);
	}
}




void PiafConfigurationDialog::slotSaveAtExit( bool on )
{
    saveAtExit = on;
	buttonOk->setEnabled(true);
}

void PiafConfigurationDialog::accept() {
	// Save file
	char filename[512];
	sprintf(filename, "%s/.piafrc", homeDir.latin1());
	
	FILE * f = fopen(filename, "w");
	if(f) {
		fprintf(f, "MeasureDir:\t%s\n", measureDir.latin1());
		fprintf(f, "ImageDir:\t%s\n", imageDir.latin1());
		fprintf(f, "MovieDir:\t%s\n", movieDir.latin1());
		fprintf(f, "SaveOnExit:\t%s\n", (saveAtExit ? "true" : "false"));
		fclose(f);
	} else {
		fprintf(stderr, "Cannot open file '%s' for writting\n", filename);
	}
	
	close();
}
