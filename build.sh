#!/bin/bash
# Build all piaf applications and vision plugins
echo "Building piaf..."


# Print the message to 
print_install() {
	echo "Build failed. Are there missing packages? If so, please use ./prepare_debian.sh to install needed packages for compilation (for ubuntu/debian)..."
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

cd main/
echo " + building Legacy GUI in `pwd` ..."
$QMAKE piaf.pro && make $@ || print_install 
cd ..


echo " + building Workflow GUI..."
cd workflow
$QMAKE && make $@ || print_install
cd ..


echo
cd plugins/vision/
echo " + building vision plugins in `pwd` ..."
./build_all.sh $@
cd ../../


echo
cd colibri-console
echo " + building Colibri-console GUI in `pwd` ..."
$QMAKE && make $@ || print_install
cd ..

echo
cd colibri
echo " + building Colibri GUI in `pwd` ..."
$QMAKE && make $@ || print_install
cd ..


if [ ! -n "$1" ]; then
	echo "Build done. Run ./install.sh as root for installation"
fi

echo "Done. Bye"

