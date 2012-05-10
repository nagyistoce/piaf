#!/bin/bash
echo "Building piaf..."


print_install() {
	echo "Build failed. Missing packages ? Please use ./prepare_debian.sh to install needed packages for compilation (for ubuntu/debian)..."
	exit 0
}

QMAKE=qmake
if [ -f /Developer/Tools/Qt/qmake ]; then 
	QMAKE="/Developer/Tools/Qt/qmake -r -spec macx-g++ CONFIG+=release "
fi

cd piaflib
echo " + building plugins library..."
$QMAKE piaf-lib.pro && make $@ || print_install 
cd ..

echo " + building Legacy GUI..."
cd main
$QMAKE piaf.pro && make $@ || print_install 
cd ..

echo " + building Colibri GUI..."
cd colibri
$QMAKE && make $@ || print_install
cd ..

echo " + building Workflow GUI..."
cd workflow
$QMAKE && make $@ || print_install
cd ..


echo "Building plugins..."

cd plugins/vision/
./build_all.sh $@

if [ ! -n "$1" ]; then
	echo "Build done. Run ./install.sh as root for installation"
fi

echo "Done. Bye"

