#ifndef PIAFWORKFLOWSETTINGS_H
#define PIAFWORKFLOWSETTINGS_H

#include "workflowtypes.h"
#include <QList>

/** @brief Workflow interface settings storage */
typedef struct {

	/// Default measure directory to save measures;
	QString defaultMeasureDir;

	/// Default image directory for saving snapshots
	QString defaultImageDir;

	/// Default movie directory for saving capture sequences
	QString defaultMovieDir;

	/// Last sequence directory for saving plugins sequences
	QString defaultSequenceDir;



	/// List of input directories
	QList<t_folder *> directoryList;

	/// list of collections
	QList<EmaCollection *> collectionList;

	// ======== DEVICES =========
	/// List of devices
	QList<t_device *> devicesList;
	int maxV4L2;///< Max index for search
	int maxOpenCV;///< Max index for search
	int maxOpenNI;///< Max index for search
	int maxFreenect;///< Max index for search

	// ======== GUI Settings ========
	int leftTabWidget_index;	///< Index of last opened tab in left tabwigdet (Folder, Collections, Device...)
	int rightTabWidget_index;	///< Index of last opened tab in right tabwigdet (Info, Image, Plugins...)

} t_workflow_settings ;

#ifdef EMAMAINWINDOW_CPP
t_workflow_settings g_workflow_settings;
#else
extern t_workflow_settings g_workflow_settings;
#endif

#endif // PIAFWORKFLOWSETTINGS_H
