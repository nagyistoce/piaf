#!/bin/bash
echo "Building piaf..."
cd main/

echo " + building plugins library..."
qmake piaf-lib.pro && make $@ || echo "Build failed. Missing packages ? Please use ./prepare.sh to install needed packages for compilation (for ubuntu/debian)..."

echo " + building GUI..."
qmake piaf.pro && make $@ || echo "Build failed. Missing packages ? Please use ./prepare.sh to install needed packages for compilation (for ubuntu/debian)..."
cd ..

echo " + building Colibri GUI..."
cd colibri
qmake && make $@ || echo "Build failed. Missing packages ? Please use ./prepare.sh to install needed packages for compilation (for ubuntu/debian)..."
cd ..

echo "Building plugins..."

cd plugins/vision/
./build_all.sh $@

echo "Build done. Run ./install.sh as root for installation"

