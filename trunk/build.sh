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


if [ ! -f /usr/local/lib/libSwPluginCore.so ]; then
	# Install lib if user is root
	WHOAMI=$(whoami)
	if [ "$WHOAMI" == "root" ]; then
		echo "Your are root, so we install the lib now"
		make install	
	else
		echo "Please run './build install' as root"
	fi

fi
cd ..

echo " + building Legacy GUI..."
cd main
$QMAKE piaf.pro && make $@ || print_install 
cd ..


echo " + building Workflow GUI..."
cd workflow
$QMAKE && make $@ || print_install
cd ..


echo " + building plugins..."

cd plugins/vision/
./build_all.sh $@

echo " + building Colibri-console GUI..."
cd colibri-console
$QMAKE && make $@ || print_install
cd ..


echo " + building Colibri GUI..."
cd colibri
$QMAKE && make $@ || print_install
cd ..


if [ ! -n "$1" ]; then
	echo "Build done. Run ./install.sh as root for installation"
fi

echo "Done. Bye"

