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



} t_workflow_settings ;

#ifdef EMAMAINWINDOW_CPP
t_workflow_settings m_workflow_settings;
#else
extern t_workflow_settings m_workflow_settings;
#endif

#endif // PIAFWORKFLOWSETTINGS_H
