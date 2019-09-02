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

Except corners left edge of the screen is reserved for opening virtual keyboard.

# Configuration File

When the application started it checks if configuration file exists in **/sdcard/Android/org.brogue.brogue/files/settings.conf**. You can use this file for customizing the behaviour of application.

The syntax of this file is very simple,name value pairs separated with a space in every line:
[NAME1] [VALUE1]<br/>
[NAME2] [VALUE2]<br/>
[NAME3] [VALUE3]<br/>
...

Note that currently the application does not handle erroneous syntax so make sure that configuration is correct.

You can see the supported configurations below:

| Name | Value Type | Default Value | |
| --- | --- | --- | --- |
| custom_cell_width | Integer | 0 | When present and value is not 0 it sets the cell width |
| custom_cell_height | Integer | 0 | When present and value is not 0 it sets the cell height |
| force_portrait | Boolean | 0 | By default the app will start in landscape mode. If this option enabled it will force application to use portrait mode |
| dynamic_colors | Boolean | 1 | Dynamic colors on water,lava etc.  Disabling will lower the power consumption |
| double_tap_lock | Boolean | 1 | When enabled it will ignore the missclicks within **double_tap_interval** range |
| double_tap_interval | Integer | 500 | The maximum time between two taps to acknowledge it as double tap |

# Saves,Recordings and Highscore

These files are stored in **/sdcard/Android/org.brogue.brogue/files/** which is accessible by the user.

# Fonts
Default font  is modified [Dejavu Sans Mono](https://dejavu-fonts.github.io/) with missing characters added/modified from Dejavu Sans.

You can use custom fonts by copying them to **/sdcard/Android/org.brogue.brogue/files/custom.ttf** . 

However fonts must be:

* Edited according to draw_letter function in main.c

* Monospace
