#!/bin/bash
rm -fr Piaf.app
/Developer/Tools/Qt/qmake piaf.pro -r -spec macx-g++ CONFIG+=release
make clean && make || exit
/Developer/Tools/Qt/lrelease piaf_French.ts
cp piaf_French.qm Piaf.app/Contents/MacOS/

/Developer/Tools/Qt/macdeployqt Piaf.app

SVNVERSION=`svnversion -n .`
zip -r Piaf-MacOSX-"$SVNVERSION".zip Piaf.app

