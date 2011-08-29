#!/bin/bash
rm -fr PiafWorkflow.app
/Developer/Tools/Qt/qmake piafworkflow.pro -r -spec macx-g++ CONFIG+=release
make clean && make || exit
/Developer/Tools/Qt/lrelease PiafWorkflow_French.ts
cp PiafWorkflow_French.qm PiafWorkflow.app/Contents/MacOS/

/Developer/Tools/Qt/macdeployqt PiafWorkflow.app

SVNVERSION=`svnversion -n .`
zip -r Piaf-Workflow-MacOSX-"$SVNVERSION".zip PiafWorkflow.app

