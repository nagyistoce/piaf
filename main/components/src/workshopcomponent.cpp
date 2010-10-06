#include "workshopcomponent.h"

WorkshopComponent::WorkshopComponent(QString label, int type)
{ 
	swcLabel = label;
	swcType = type;
	
	initDefaults();
	
	swcModified = true;
	
}

void WorkshopComponent::initDefaults()
{
	// default class set to UNDEFINED
	swcClass = CLASS_UNDEFINED;
	swcShortLabel = swcLabel.left(3);
	swcShortLabel += ".";
	swcDate.currentDate();
	swcValidity = false;
    swcPathName = QString::null;
    swcFileName = QString::null;
	
}

int WorkshopComponent::setComponentClass(int newClass)
{
	if(newClass>=NB_COMPONENT_CLASS)
		return -1;
	
	swcClass = newClass;
	
	return swcClass;
}

void WorkshopComponent::selectPathName()
{
	
	QString newPath;
	QDir dir(swcPathName);
	
	switch(swcClass)
	{
		default:
			break;
		
		case CLASS_MEASURE:
			newPath = Q3FileDialog::getSaveFileName(dir.absPath(), tr(" Measures (*.meas)"),NULL,
				"save measure dialog", tr("Choose a filename to save as"));
	        if(!newPath.isEmpty())
			{
				char fname[256];
				strcpy(fname,(char*)newPath.latin1());
				if(!strstr(fname,".meas"))
					strcat(fname,".meas");
				QString string(fname);
				swcPathName=string;
			}
			break;
	
			
		case CLASS_IMAGE:
			newPath= Q3FileDialog::getSaveFileName(dir.absPath(),  tr(" Images (*.png *.jpg)"),NULL,"save image dialog",
				tr("Choose a filename to save as"));
	        if(!newPath.isEmpty())
			{
				char fname[256];
				strcpy(fname,(char*)newPath.latin1());
				if(!strstr(fname,".png") && !strstr(fname,".jpg")) 
					strcat(fname,".png");
				QString string(fname);
				swcPathName=string;
			}
			break;
			
		case CLASS_VIDEO:
			newPath= Q3FileDialog::getSaveFileName(dir.absPath(),  tr(" Movies (*.avi *.mpg *.wmv)"),NULL,"save movie dialog",
				tr("Choose a filename to save as"));
	        if(!newPath.isEmpty())
			{
				char fname[256];
				strcpy(fname,(char*)newPath.latin1());
				if(!strstr(fname,".avi") && !strstr(fname,".mpg"))
					strcat(fname,".avi");
				QString string(fname);
				swcPathName=string;
			}

			break;
	}
	//if((swcPathName.isNull())||(!dir.exists()))

		//newPath = QFileDialog::getExistingDirectory("/home", 0, "get existing directory", "Choose a directory", TRUE);
	/*	newPath = QFileDialog::getSaveFileName(
                    "/home",
                    "Images (*.png)",
                    NULL,
                    "save file dialog"
                    "Choose a filename to save under" );
	*/
       /* newPath = savedialog->getSaveFileName(
                    "/home",
                    "Images (*.png)",
                    NULL,
                    "save file dialog"
                    "Choose a filename to save under" );
  		
	*/
	
	/*
	else
		
	    //newPath = QFileDialog::getExistingDirectory(swcPathName, 0, "get existing directory", "Choose a directory", TRUE);
		newPath = QFileDialog::getSaveFileName(
                    swcPathName,
                    "Images (*.png)",
                    NULL,
                    "save file dialog"
                    "Choose a filename to save under" );
	    newPath = savedialog->getSaveFileName(
                    swcPathName,
                    "Images (*.png)",
                    NULL,
                    "save file dialog"
                    "Choose a filename to save under" );
  
	*/
	// if(!newPath.isNull())
	//	swcPathName = newPath;
	
}
