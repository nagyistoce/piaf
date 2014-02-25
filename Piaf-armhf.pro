# .Pro for components built in arm version
# Author : C. Seyve 
# Copyrights Christophe Seyve - 2002-2014
TEMPLATE=subdirs
message("building piaf embedded on ARMhf from subdirs")

OBJECTS_DIR = .obj-arm
MOC_DIR = .moc-arm

SUBDIRS = piaflib \
	colibri-console
