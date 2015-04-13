# Goals #

Piaf's goal is to provide a tool for developing computer vision algorithms, easily
and without having to care about acquisition of pictures, movies or live video devices...

Piaf provides the graphical reading and displays of "image sources" (still
pictures, movies or live input from devices), AND a tools for adding your own plugins.

![http://piaf.googlecode.com/svn/wiki/screenshots/Piaf-global.png](http://piaf.googlecode.com/svn/wiki/screenshots/Piaf-global.png)

Each "image" source can be displayed, processed by plugins ...
Thus you can develop your own image processing algorithm, Piaf does the remaining functions.
No need to be a senior developer to add your own plugins.

Just do the image processing, Piaf does the rest.

# Features #

Piaf is a toolbox, composed by:
  * An interface to manage plugins: loading, declaration, image exchanges...
  * A few sample plugins
  * A main user interface (GUI) for displaying images, live video inputs and movies, and process them through the plugin manager
  * Two more tools :
    * A batch processor for processing several files (images or movies)
    * colibri: A lightweight player GUI for a sequence of plugins, supporting live cameras, still images and movies
    * colibri-console: an even lighter app to play sequences, acces live cameras (2D or 3D), still images or movies. Soon available for BeagleBoard/Bone.

## GUIs ##

Released under GPL:
  * support: still images, standard movie files, live video inputs (V4L/V4L2, ieee1394, GigEVision)
  * player/display : zoom, palettes, ...
  * plugins manager which :
    * add/remove plugins
    * sequence plugins operations,
    * generate parameters windows for every function
    * measure processing time

## Plugins ##

The plugin principles have been decided to be **very** simple for researchers, not for expert developpers. See [Plugins](Plugins.md)

  * each plugin is a separated processus/application, communicating with Piaf through pipes ;
  * each plugin can export several functions, one plugin executes only one function
  * plugins can :
    * declare parameters, exported into Piaf's GUI
    * crash without restarting piaf
    * use any library or ressources : files, ... (excluding stdout/stdin)
    * be debugged with gdb, profiled by valgrind, ...
  * processed by plugins : input -> plugin 1 -> plugin 2 ...
  * the interface library is released under LGPL : you can keep your intellectual property on your algorithms

Examples of plugins are provided for simple operations : OpenCV basic tools (morphology, histograms, ...), FFTW (blur), ... See HowToWritePlugins

# Project status #

Status:
  * Linux: beta (for a long time)
  * MacOS X : alpha, just ported and running with lots of bugs

Dependencies:
  * GUI/acquisition : Qt4, OpenCV, FFMPEG via OpenCV
  * Plugins : no dependency