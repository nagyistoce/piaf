#!/bin/bash
echo "Preparing installation..."
echo
if [ "$(whoami)" == "root" ] ; then
	echo 'Creating directory /usr/local/piaf...'

	if [ ! -d /usr/local/piaf ]; then 
		mkdir /usr/local/piaf
	fi

	echo 'Copying directory images to /usr/local/piaf...'
	cp -r main/images /usr/local/piaf
	echo "You may want to do chmod 777 /usr/local/piaf/images/pixmaps/ to enable user to update the icons, ..."
	echo
	echo 'Creating directory /etc/piaf...'
	mkdir /etc/piaf
	chmod 777 /etc/piaf
	echo 'Done.'
else
	echo "Warning : You must run install.sh as root to create local install directories."
fi

echo
echo "Installing piaf..."
./build.sh install || echo "Installation failed"

echo "  running ldconfig"
ldconfig

echo "Done."

