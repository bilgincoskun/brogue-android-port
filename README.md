Android port of [Brogue](https://sites.google.com/site/broguegame/) using SDL2

You can download apk from [here](https://github.com/bilgincoskun/brogue-android-port/releases) 
# Building

Before building it, run **setup_project.sh** to download and copy necessary files into the project.

These files are:

* SDL2

* SDL2_ttf

* platform independent files of Brogue

After that you can use gradlew script to build it as usual.

# User Input

## Mouse Input

There are two ways to emulate mouse input:

* Two short presses:

First press selects the cell and second press on the same cell clicks there.

* One long press:

Long press on the same cell both selects and clicks the it.

## Keyboard Input

### Escape Key

There are two ways to send escape key:

* Pressing Back Button

* Pressing Bottom Left Corner of the Screen

### Enter Key

Top left corner of the screen is reserved for enter key

### Control Key

Three finger touch. Note that it is implemented to enter a custom seed.

### Virtual Keyboard

Except corners left of the screen is reserved for opening virtual keyboard.


# Saves,Recordings and Highscore

These files are stored in **/sdcard/Android/org.brogue.brogue/files/** which is accessible by the user.

# Fonts
Default font  is modified [Dejavu Sans Mono](https://dejavu-fonts.github.io/) with missing characters added/modified from Dejavu Sans.

You can use custom fonts by copying them to **/sdcard/Android/org.brogue.brogue/files/custom.ttf** . 

However fonts must be:

* Edited according to draw_letter function in main.c

* Monospace
