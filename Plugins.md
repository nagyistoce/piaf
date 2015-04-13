# Introduction #

![http://piaf.googlecode.com/svn/wiki/screenshots/Piaf-global.png](http://piaf.googlecode.com/svn/wiki/screenshots/Piaf-global.png)

Piaf's plugins are simple processus which do one or more image processing functions. It's easy to create your own plugin for specific processings or to integrate computer vision libraries' function as plugins.

# First try examples #

  * open a source (still image, movie, webcam...)
  * launch the plugin manager with icon ![http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/IconFilters.png](http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/IconFilters.png)

![http://piaf.googlecode.com/svn/wiki/screenshots/PIAF-Plugin-manager.png](http://piaf.googlecode.com/svn/wiki/screenshots/PIAF-Plugin-manager.png)

  * On left column, all the available plugins are displayed
  * in central column, the actions: add ![http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/go-next.png](http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/go-next.png), remove ![http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/go-previous.png](http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/go-previous.png), edit params ![http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/configure.png](http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/configure.png), disable ![http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/network-disconnect16.png](http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/network-disconnect16.png), measure time ![http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/chronometer.png](http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/chronometer.png), move up/down ![http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/go-up.png](http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/go-up.png) / ![http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/go-down.png](http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/go-down.png)...
  * On right column, the activated plugins



  * Choose a plugin (for ex: _"Contour">"Canny"_, and load it with button ![http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/go-next.png](http://piaf.googlecode.com/svn/trunk/main/images/pixmaps/go-next.png) : the image is processsed and the output is displayed

![http://piaf.googlecode.com/svn/wiki/screenshots/PIAF-Lena-Canny.png](http://piaf.googlecode.com/svn/wiki/screenshots/PIAF-Lena-Canny.png)

  * Click on the parameters button: a parameters window is generated from the exported parameters in plugin. Modify the parameters and apply.

![http://piaf.googlecode.com/svn/wiki/screenshots/PIAF-Lena-Canny-params.png](http://piaf.googlecode.com/svn/wiki/screenshots/PIAF-Lena-Canny-params.png)

  * Add another plugin (for ex: "Morphology">"Dilate"), see the result

![http://piaf.googlecode.com/svn/wiki/screenshots/PIAF-Lena-Canny-Dilate.png](http://piaf.googlecode.com/svn/wiki/screenshots/PIAF-Lena-Canny-Dilate.png)


# Principles of plugins #

The plugin principles have been decided to be **very** simple for researchers, not for expert developpers.

By now, the plugins are compiled with _g++_ and the API is written in C/C++.
  * each plugin communicated with Piaf through stdin/stdout redirections in pipes. The _libSwPluginCore_ is provided (C++). You just need to link with it and to create one instance of communication class.
  * each plugin is a separated processus/application
> > => this way, plugin can :
    * crash without crashing Piaf
    * be debuggued:
      * load the plugin in piaf, ex: my\_plugin,
      * in a terminal, get its PID : `ps aux | grep my_plugin` => PID=14323
      * hang on processus : `gdb ./my_plugin 14323`
`(gdb) [...]`
      * continue execution of plugin
`(gdb) c`
`...`
      * be profiled with valgrind : launch piaf with command :
`valgrind --trace-children=yes ./piaf`

> Then load you plugin, it'll be very slow, but profiled ;)


  * each plugin can export several functions, one plugin executes only one function
> Thus the global variables are dedicated to one single function.
> So you can develop from the "fast & dirty" method to advanced object methods.

  * plugins can :
> - declare parameters, exported into Piaf's GUI

> - crash without restarting piaf

> - use any library or ressources : files, ... (excluding stdout/stdin)

> - be debugged with gdb, profiled by valgrind, ...

  * processed by plugins : input -> plugin 1 -> plugin 2 ...

  * interface library is released under LGPL : you can keep your intellectual property on your algorithms

# Write your own plugin #

Easiest way is to edit one of the examples.

Examples of plugins are provided for simple operations : OpenCV basic tools (morphology, histograms, ...), FFTW (blur), ...

See HowToWritePlugins