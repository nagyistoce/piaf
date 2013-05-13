#!/bin/bash
echo "Checkout piaf in /tmp/..."
cd /tmp
svn checkout http://piaf.googlecode.com/svn/trunk/ piaf
cd piaf/

echo "Building with fakeroot..."
dpkg-buildpackage -uc -us -rfakeroot

