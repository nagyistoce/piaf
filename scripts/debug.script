#!/bin/bash

cat $1 | sed '/g++/d' | sed '/ has virtual functions/d' | sed '/Wmissing-prototypes/d'  | sed '/mkdir/d' | sed '/so.0.1.0/d' | sed '/toto/d' | sed '/qt3/d' | sed '/`y/d' | sed '/`x/d' | sed '/`parent/d' | sed '/`width/d' | sed '/`height/d' | sed '/`pos/d' | sed '/`name/d' | sed '/`font/d' | less

