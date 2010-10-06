#!/bin/bash

echo "Building all sources..."
for src in opencv_*.cpp ; do export SRCNAME=${src%.cpp} ; qmake opencv_template.pro && make $@ ; done

export SRCNAME=example
qmake fftw_template.pro && make

export SRCNAME=outoffocus
qmake fftw_template.pro && make 

