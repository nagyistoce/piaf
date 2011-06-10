#!/bin/bash
# Build a piaf source package
PIAFDIR=piaf-`date +%Y%m%d`
echo "Copying piaf in $PIAFDIR"
mkdir /tmp/"$PIAFDIR"
#cp -r . "$PIAFDIR"

cd /tmp/
echo "Checkout of piaf SVN..."
svn checkout http://piaf.googlecode.com/svn/trunk/ "$PIAFDIR"

cd "$PIAFDIR"

echo "Cleaning builds..."
echo " + cleaning all built..."
./build.sh clean

echo " + cleaning remaining files after make clean ..."
find . -name "Makefile*" | xargs -n 1 rm -f 
find . -name ".obj*" | xargs -n 1 rm -fr
find . -name ".moc*" | xargs -n 1 rm -fr
find . -name "*.pro.user" | xargs -n 1 rm -f
find . -name "*.qm" | xargs -n 1 rm -f
find . -name "*build-desktop" | xargs -n 1 rm -fr
find . -name "*.log" | xargs -n 1 rm -f
find . -name "*.swp*" | xargs -n 1 rm -f


# Delete builds on MacOS X
find . -name "*.app" | xargs -n 1 rm -fr

echo "Cleaning SVN directories..."
find . -name ".svn" -print | xargs -n 1 rm -fr
find . -name "*svn-commit*" -print | xargs -n 1 rm -f

find . -name "*.o" -print | xargs -n 1 rm -f
find . -name "*.so*" -print | xargs -n 1 rm -f

cd ../
tar jcvf $PIAFDIR.tar.bz2 $PIAFDIR

