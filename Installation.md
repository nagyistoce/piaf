# Introduction #

This page explains how to install Piaf GUI, library and plugins in Linux or MacOS X

# Requirements #

## Linux ##

You need to install OpenCV and Qt4.

The script `./prepare.sh` in root directory install required packages for Debian/Ubuntu using `apt-get`.
```
sudo ./prepare.sh
```

But maybe you want to install manually or on another kind of package manager.

### Qt4 ###

Easiest way is to use the packages from your distribution. You can download latest version here : http://qt.nokia.com

Check that Qt4's _qmake_ is the one by default. It need to be Qt4's:
```
$ qmake -v
QMake version 2.01a
Using Qt version 4.6.3 in /usr/lib
```

If not, you can edit the build scripts or, easier way, change the qmake link :
```
$ sudo update-alternatives --config qmake
[sudo] password for tof: 
There are 2 choices for the alternative qmake (providing /usr/bin/qmake).

  Selection    Path                Priority   Status
------------------------------------------------------------
* 0            /usr/bin/qmake-qt3   45        auto mode
  1            /usr/bin/qmake-qt3   45        manual mode
  2            /usr/bin/qmake-qt4   40        manual mode

Press enter to keep the current choice[*], or type selection number: 2
update-alternatives: using /usr/bin/qmake-qt4 to provide /usr/bin/qmake (qmake) in manual mode.
```

### OpenCV ###

You need _libcv_, _libcvaux_ and _libhighgui_. You have got 2 solutions:
  1. Use your distribution packages : install a development version of OpenCV with your package manager, for example:
```
apt-get install libcv-dev libcvaux-dev libhighgui-dev
```
  1. Download and install from snapshot or SVN : http://opencv.willowgarage.com/wiki/

You may need to download and install FFMPEG to use _highgui_.

## MacOS X ##

Easiest way is to :
  * install Qt from DMG : http://qt.nokia.com
  * OpenCV from sources : http://opencv.willowgarage.com/wiki/


# Compilation #

You need to compile first the library in order to use plugins. Without plugins, Piaf is just an poor image viewer and video player.

## First: Plugin's library ##

Go to main/ and compile using `piaf-lib.pro`