Android port of [Brogue](https://sites.google.com/site/broguegame/) using SDL2
# Installation
You can download apks from [here](https://github.com/bilgincoskun/brogue-android-port/releases) 

Currently there are 3 apks use different versions of Brogue which can be installed side by side.

* 1.7.5 with bug fixes from [here](https://github.com/flend/brogue-windows/) (Suggested version)

* Original v1.7.5 version

* Original 1.7.4 version

Note that apks require two additional permissions which both are optional:

* Internet Access: This permission is to check updates. You can disable internet access by disabling updates.

* Write Access: This permission is to write /sdcard/Brogue. For Android 6.0+ if you do not grant it at runtime it will write to default data folder under Android/data.

#Updates

By default the app check if there is a new version when started. You can disable it via **check_update** config. You can also set the interval of checks via **check_update_interval**

#App Folder

All versions will try to use /sdcard/Brogue. If write permission is not granted, they will use /sdcard/Android/org.brogue.brogue.[version suffix]/files/ instead. Note that since all save files etc. is written under a folder specific to the that version even multiple versions are installed they will not override others' files under /sdcard/Brogue.

# User Input

## Mouse Input

First touch selects the cell and second touch on the same cell clicks there.

## Zooming

The game grid can be zoomed in/out via pinch gesture. You can also toggle on/off zoom by tapping with two fingers.

## On-screen D-pad
D-pad is enabled by default to help with more precise control.

There are two mods of D-pad: movement mode (default) and selection mode. You can change between modes via long pressing d-pad.

### Movement Mode

In this mode movement keys directly moves the PC. The square sends Enter key (makes the D-pad mode temporarily selection mode). One exception is that the square sends Space key instead of Enter key when the game is playing a record. D-pad is yellow.

### Selection Mode

In selection mode direction keys moves the cursor and the square clicks the selected cell. D-pad is white.

## Keyboard Input

### Escape Key

There are two ways to send escape key:

* Pressing Back Button

* Pressing Bottom Left Corner of the Screen

### Enter Key

Top left corner of the screen is reserved for enter key (when It is not playing a record)

### Space Key (in Playback Mode)

Top left corner sends space key instead of enter key when It is playing a record.

### Control Key

Three finger touch. Note that it is implemented to enter a custom seed.

### Virtual Keyboard

Except corners left edge of the screen is reserved for opening virtual keyboard.
# Configuration File

When the application started it reads the configuration file in **[App Folder]/settings.txt** .  You can use this file for customizing the behaviour of the application.

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
| custom_screen_width | Integer | 0 | When present and value is not 0 it sets the screen width.Disables custom_cell_width when active |
| custom_screen_height | Integer | 0 | When present and value is not 0 it sets the screen height.Disables custom_cell_height when active |
| force_portrait | Boolean | 0 | By default the app will start in landscape mode. If this option enabled it will force application to use portrait mode |
| dynamic_colors | Boolean | 1 | Dynamic colors on water,lava etc.  Disabling will lower the power consumption |
| filter_mode | Integer | 2 | Selects the algorithm for anti-aliasing: 0 uses nearest pixel sampling, 1 uses linear filtering and 2 uses anistropic filtering. If you get graphical glitches or performance issues, try to lower the value. |
| double_tap_lock | Boolean | 1 | When enabled it will ignore the missclicks within **double_tap_interval** range |
| double_tap_interval | Integer | 500 | Milliseconds.The maximum time between two taps to acknowledge it as a double tap. Value is between 100 and 100000 |
| dpad_enabled | Boolean | 1 | Enable on-screen d-pad |
| dpad_width | Integer | 0 | When it is not 0 it sets the width of the d-pad square. Otherwise width is set to fit into left panel |
| dpad_x_pos | Integer | 0 | When it is not 0 it sets the x position of the top left corner of the d-pad square. Otherwise position is set to bottom of the left panel |
| dpad_y_pos | Integer | 0 | When it is not 0 it sets the y position of the top left corner of the d-pad square. Otherwise position is set to bottom of the left panel |
| allow_dpad_mode_change | Boolean | 1 | Allow to change d-pad mode by long pressing |
| default_dpad_mode | Boolean | 1 | Decides if the mode of d-pad at the start of the game will be selection mode (0) or movement mode (1) |
| dpad_transparency | Integer | 75 |  Value is between 0 (non-visible) and 255 (opaque) |
| long_press_interval | Integer | 750 | Milliseconds.The minimum time between press and release to acknowledge it as a long press. Value is between 100 and 100000 |
| keyboard_always_on | Boolean | 0 | Opens the keyboard at the start of the game and prevents it from closing when touch input is occured. Note that playing back button still closes the keyboard. |
| zoom_mode | Integer | 1 | 0 disables zoom, 1 zooms to player character (default), 2 zooms to cursor (falls back to zoom_mode 1 if the cursor is not present on the screen)|
| init_zoom_toggle | Boolean | 0 | Start the game as zoomed |
| init_zoom | Decimal | 2.0 | Default zooming at the start of the game. Value is between 1.0 and 10.0 |
| max_zoom | Decimal | 4.0 | Maximum allowed zoom level. Value is between 1.0 and 10.0. It is overriden by init_zoom if init_zoom is bigger than max_zoom|
| smart_zoom | Boolean | 1 | Zoom out when a menu, logs or a confirmation dialog open or left panel is tapped |
| check_update | Boolean | 1 | Check updates when the app starts |
| check_update_interval | Integer | 1 | How many days should pass before checking again. With 0, the app checks update each time it starts |
# Saves,Recordings and Highscore

These files are stored in **[App Folder]/\[Brogue Version]** which is accessible by the user.

# Fonts
Default font  is modified [Dejavu Sans Mono](https://dejavu-fonts.github.io/) with missing characters added/modified from Dejavu Sans.

You can use custom fonts by copying them to **[App Folder]/custom.ttf** .

However fonts must be:

* Edited according to draw_glyph function in main.c

* Monospace
