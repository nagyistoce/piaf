#!/bin/bash
echo "Building piaf..."
cd main/
echo " + building plugins library..."
qmake-qt4 piaf-lib.pro && make && make install || exit
echo " + building GUI..."
qmake-qt4 piaf.pro && make && make install || exit

echo "Building plugins..."
cd ../plugins/vision/
./build_all.sh

echo "Done."

