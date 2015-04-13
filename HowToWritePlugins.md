# Introduction #

This page explains step by step how to create your own plugin.

# Examples #

Examples of plugins are provided in _plugins/vision/_
  * OpenCV for basic operations, morphology, feature ... : `opencv_morphology.cpp`, ...
  * FFTW for blurring: `outoffocus.cpp`
  * Example from scratch : `example.cpp`

Build examples with `./build_all.sh`

# Step by step #

A new Piaf plugin needs only a few steps :
  1. Create a `Makefile` to build the executable
  1. Edit the default plugin skeleton to add your own functions
  1. Defines functions and parameters
  1. Implement functions
  1. Add your plugin in Piaf interface

## Create a project ##

The easiest way is to use a `.pro` (since you've already got Qt4 !) for `qmake`.

The `.pro` must at least contains the path of Piaf's headers and the link to library `libSwPlugin` :
```
TEMPLATE        =       app
CONFIG          =       warn_on release thread
LANGUAGE        =       C++
TARGET          =       myplugin

INCLUDEPATH +=          /usr/local/include/SwPlugin/

unix:LIBS += -L/usr/local/lib 
unix:LIBS += -lSwPluginCore

SOURCES = myplugin.cpp
```

Generate your `Makefile` and build :
```
qmake myplugin.pro
```

## Edit the source code ##

The plugin must contain a minimum amount of code which you don't need to edit :
  * Header:
```
#include "SwPluginCore.h"
```

  * An object for communication:
```
SwPluginCore plugin;
```
  * The standard main and a signal handler

Then you need to customize your plugin:
  * Declaration of plugin properties:
```
#define CATEGORY        "Vision"
#define SUBCATEGORY     "MyPlugin"
```

  * Functions' parameters:
```
// circular buffer median
unsigned char median = 5;

swFuncParams median_params[] = {
        {"iteration", swU8, (void *)&median}
};
```
> > The types of parameters are defines in `SwTypes.h`. The types are defined by their name : `sw\` + type, U8 for unsigned on 8 bits. The API handles: `swU8, swS8, swU16, swS16	swU32, swS32, swFloat, swDouble` and `swStringListStruct` for generating a menu in Piaf's GUI.
> > When a function is declared with parameters, the GUI will generate a parameters window automatically for editing those parameters.

  * List of functions:
```
/* swFunctionDescriptor : 
        char * : function name
        int : number of parameters
        swFuncParams : function parameters
        swType : input type
        swType : output type
        void * : procedure
        */
swFunctionDescriptor functions[] = {
        {"Circ. Median", 1,     median_params,          swImage, swImage, &median_func, NULL},
        {"Low update",  1,              low_update_params, swImage, swImage, &low_update, NULL},
        {"Invert",              0,      NULL,                           swImage, swImage, &invert, NULL},
        {"Threshold",   2,      threshold_params,       swImage, swImage, &threshold, NULL},
        {"Lighten",     2,      erode_params,           swImage, swImage, &erode, NULL}
};
int nb_functions = 5;
```

  * Then implement your function: a header is always present, with input and output images
```
// function invert
void invert()
{
        swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
        swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
        unsigned char * imageIn  = (unsigned char *)imIn->buffer;
        unsigned char * imageOut = (unsigned char *)imOut->buffer;
        
        
        for(unsigned long r = 0; r < imIn->buffer_size; r++)
        {
                imageOut[r] = 255 - imageIn[r];
        }
}
```


Then compile your plugin with `make`.

You can try to launch the executable directly, it will wait from commands from stdin.
```
piaf/plugins/vision$ ./outoffocus 
registerCategory...
registerFunctions...
loop...
```

## Add the plugin in Piaf ##

In Piaf's GUI, select menu "Tools" > "Plugins", add you executable's path.

![http://piaf.googlecode.com/svn/wiki/screenshots/PIAF-Plugins-add.png](http://piaf.googlecode.com/svn/wiki/screenshots/PIAF-Plugins-add.png)

Then open our image source, your filter should appear in left column. If not, it may not export its parameters or crash at start.

Load one of the function of your plugin.

![http://piaf.googlecode.com/svn/wiki/screenshots/PIAF-Lena-Invert.png](http://piaf.googlecode.com/svn/wiki/screenshots/PIAF-Lena-Invert.png)

You can now load or unload your plugin, compare with several images opened at the same time ...

## Debug your function ##

Since every function is run as a separated processus, you can use global variables, debug each function ...

_Hint:_ If the plugin crashes at first image:
  * do not process on first loading :
```
void invert()
{
        swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
        swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
        unsigned char * imageIn  = (unsigned char *)imIn->buffer;
        unsigned char * imageOut = (unsigned char *)imOut->buffer;

        static int invert_counter = 0;
        invert_counter++;
        if(invert_counter < 2) { sleep(30); return; }
        // your code ...
}
```

  * in a terminal, get the _pid_ of your process, then use `gdb` to debug it:
```
$ ps aux | grep myplugin
tof      32153  0.0  0.1  22472  3724 pts/10   S+   22:56   0:00 myplugin
$ gdb ./myplugin 32153
GNU gdb (GDB) 7.0.1-debian
Copyright (C) 2009 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
and "show warranty" for details.
This GDB was configured as "i486-linux-gnu".
_[...]_
Program not killed.
(gdb) c
Continuing.
```


> Then it will crash later ;)