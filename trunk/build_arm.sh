#!/bin/bash
# BUILD PIAF COMPONENTS FOR ARM
# Prerequisite: qmake profile for ARM

USE_ARM=true
export USE_ARM

echo "============= LAUNCHING qmake -spec linux-armv7a-hf-g++ -r Piaf-armhf.pro ============="
export PATH=/opt/eldk-5.3/armv7a-hf/sysroots/i686-eldk-linux/usr/bin/armv7ahf-vfp-neon-linux-gnueabi:$PATH
qmake -spec linux-armv7a-hf-g++ -r Piaf-armhf.pro
echo
echo "============= BUILDING ============="
make $@

echo 
echo "Done."


