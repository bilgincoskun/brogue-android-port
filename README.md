Android port of [Brogue](https://sites.google.com/site/broguegame/) using SDL2
# Installation
You can find the latest release in [here](https://github.com/bilgincoskun/brogue-android-port/releases/latest).

Under Assets section there are files with apk extension (each for a different Brogue version).

You can install the game by downloading the preferred version to your Android device and running it.

Currently [Community Edition](https://github.com/tmewett/BrogueCE) is supported.

Note that apks require two additional permissions which both are optional:

* Internet Access: This permission is to check updates. You can disable internet access by disabling updates.

* Write Access: This permission is to write /sdcard/Brogue. For Android 6.0+ if you do not grant it at runtime it will write to default data folder under Android/data.

# Updates

By default the app check if there is a new version when started. You can disable it via **check_update** config. You can also set the interval of checks via **check_update_interval** . If you want to be asked before update check **ask_for_update_check** .

# App Folder

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
# Settings
You can access settings from title menu. The explanations of the configurations can be found under the **Configuration File** section.
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
| custom_cell_width | Decimal | 0 | When present and value is not 0 it sets the cell width |
| custom_cell_height | Decimal | 0 | When present and value is not 0 it sets the cell height |
| custom_screen_width | Integer | 0 | When present and value is not 0 it sets the screen width.Disables custom_cell_width when active |
| custom_screen_height | Integer | 0 | When present and value is not 0 it sets the screen height.Disables custom_cell_height when active |
| force_portrait | Boolean | 0 | By default the app will start in landscape mode. If this option enabled it will force application to use portrait mode |
| dynamic_colors | Boolean | 1 | Dynamic colors on water,lava etc.  Disabling will lower the power consumption |
| filter_mode | Integer | 2 | Selects the algorithm for anti-aliasing: 0 uses nearest pixel sampling, 1 uses linear filtering and 2 uses anistropic filtering. If you get graphical glitches or performance issues, try to lower the value |
| default_graphics_mode | Integer | 0 | 0 enables text-only, 1 enables tiles and 2 enables hybrid graphics by default |
| tiles_animation | Boolean | 1 | Enables tiles animation. Disabling will lower the power consumption |
| blend_full_tiles | Boolean | 1 | Tries to blend full tiles like walls with adjacent ones. May result in blurrier tiles |
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
| keyboard_visibility | Integer | 1 | 0 disables virtual keyboard completely, 1 shows the keyboard on demand and 2 opens the keyboard at the start of the game and prevents it from closing when touch input is occured. Note that pressing back button still closes the keyboard |
| zoom_mode | Integer | 1 | 0 disables zoom, 1 zooms to player character (default), 2 zooms to cursor (falls back to zoom_mode 1 if the cursor is not present on the screen) |
| init_zoom_toggle | Boolean | 0 | Start the game as zoomed |
| init_zoom | Decimal | 2.0 | Default zooming at the start of the game. Value is between 1.0 and 10.0 |
| max_zoom | Decimal | 4.0 | Maximum allowed zoom level. Value is between 1.0 and 10.0. It is overriden by init_zoom if init_zoom is bigger than max_zoom |
| smart_zoom | Boolean | 1 | Zoom out when a menu, logs or a confirmation dialog open or an item in left panel is tapped except left edge (when left_panel_smart zoom is enabled) |
| left_panel_smart_zoom | Boolean | 1 | Enable smart zoom for left panel |
| check_update | Boolean | 1 | Check updates when the app starts |
| check_update_interval | Integer | 1 | How many days should pass before checking again. With 0, the app checks update each time it starts |
| ask_for_update_check | Boolean | 0 | Ask before checking update |
# Saves,Recordings and Highscore

These files are stored in **[App Folder]/\[Brogue Version]** which is accessible by the user.

# Graphics
By default the port uses ASCII. To always use tileset, you can use default_graphics_mode settting. You can also use in-game menu to enable tiles. Note that changes from in-game menu setting is not persistent.

Default graphics use modified [Dejavu Sans Mono](https://dejavu-fonts.github.io/) with missing characters added/modified from Dejavu Sans for the text. For the tiles, tileset based on [Oryx Design Lab Tiles](https://www.oryxdesignlab.com/news/2018/11/8/brogue-tiles-v175-is-released) is used with some additions/changes.

You can use custom fonts by copying them to **[App Folder]/custom.ttf** .

However fonts must be:

* Edited according to draw_glyph function in main.c

* Monospace

Additionally:

* If the font have animated tiles then the offset between the first frame and the second one should be 256

* the font should define a boundary box in glyph 139 for full tiles
