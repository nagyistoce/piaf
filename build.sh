#!/bin/bash
echo "Building piaf..."
cd main/

QMAKE=qmake
if [ -f /Developer/Tools/Qt/qmake ]; then 
	QMAKE="/Developer/Tools/Qt/qmake -r -spec macx-g++ CONFIG+=release "
fi

echo " + building plugins library..."
$QMAKE piaf-lib.pro && make $@ || echo "Build failed. Missing packages ? Please use ./prepare.sh to install needed packages for compilation (for ubuntu/debian)..."

echo " + building GUI..."
$QMAKE piaf.pro && make $@ || echo "Build failed. Missing packages ? Please use ./prepare.sh to install needed packages for compilation (for ubuntu/debian)..."
cd ..

echo " + building Colibri GUI..."
cd colibri
$QMAKE && make $@ || echo "Build failed. Missing packages ? Please use ./prepare.sh to install needed packages for compilation (for ubuntu/debian)..."
cd ..

echo " + building Workflow GUI..."
cd workflow
$QMAKE && make $@ || echo "Build failed. Missing packages ? Please use ./prepare.sh to install needed packages for compilation (for ubuntu/debian)..."
cd ..


echo "Building plugins..."

cd plugins/vision/
./build_all.sh $@

if [ ! -n "$1" ]; then
	echo "Build done. Run ./install.sh as root for installation"
fi
