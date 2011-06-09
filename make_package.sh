#!/bin/bash
# Build a piaf source package
PIAFDIR=/tmp/piaf-`date +%Y%m%d`
echo "Copying piaf in $PIAFDIR"
mkdir "$PIAFDIR"
cp -r . "$PIAFDIR"
cd "$PIAFDIR"

echo "Cleaning builds..."
echo " + cleaning main..."
cd main/
qmake piaf.pro && make clean
qmake piaf-lib.pro && make clean
rm -f Makefile piaf 
rm -fr obj moc .obj .moc *.pro.user *.qm
cd ../

echo " + cleaning plugins ..."
cd plugins/vision
./build_all.sh clean
rm -f Makefile
cd ../..

echo "Cleaning SVN directories..."
find . -name ".svn" -print | xargs -n 1 rm -fr
find . -name "svn-commit*" -print | xargs -n 1 rm -fr

find . -name "*.o" -print | xargs -n 1 rm -f
find . -name "*.so*" -print | xargs -n 1 rm -f

cd ../
tar jcvf $PIAFDIR.tar.bz2 $PIAFDIR
