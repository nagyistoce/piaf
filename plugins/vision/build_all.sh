#!/bin/bash

echo "Building all sources..."
for src in opencv_*.cpp ;
do
	export SRCNAME=${src%.cpp}
	echo " + building OpenCV based $src..."
	qmake opencv_template.pro && make $@ || exit 0
done

export SRCNAME=example
echo " + building $SRCNAME"
qmake fftw_template.pro && make $@ || exit 0

export SRCNAME=outoffocus
echo " + building $SRCNAME"
qmake fftw_template.pro && make $@ || exit 0

echo "Done"
