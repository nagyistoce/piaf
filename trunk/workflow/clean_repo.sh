#!/bin/bash

qmake && make clean 
rm -fr .obj .moc
rm -f ui_* qrc* moc_*
rm -f Makefile *.pro.user
