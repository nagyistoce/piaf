#!/bin/bash
echo "Install required packages for Debian packages..."
if [ "$(whoami)" == "root" ] ; then
	# Create directories
	mkdir /usr/local/piaf
	mkdir /usr/local/piaf/plugins
	chmod 777 /usr/local/piaf/plugins

	# Install packages 
	apt-get install -y libqt4-dev 
	apt-get install -y libqt3-compat 
	apt-get install -y libavcodec-dev libavformat-dev libavutils-dev 
        apt-get install -y libswscale-dev 

	apt-get install -y libcv-dev libcvaux-dev libhighgui-dev

	apt-get install -y libfftw3-dev sfftw-dev

	# For workflow, use exiv2 to read EXIF data
	apt-get install -y libexiv2-dev
else
	echo "Yoo need to be root to install packages"
fi


