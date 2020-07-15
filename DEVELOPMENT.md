# Building

Before building it, run **./scripts/setup_project.sh** to download and copy necessary files into the project.

These files are:

* SDL2

* SDL2_ttf

* platform independent files of Brogue

After that you can use gradlew script to build it as usual.

The default version is 1.7.5 with bug fixes. To build a different version change to a different branch in brogue-files folder.

There is also scripts/build_release_apks.py to build and sign all versions. When executed this script will ask key file information and version name and tag the repository. If there is a sign_info.json file in the project of the root, the script will only prompt missing signing information in this file. These parameters are "path" (key store location), "ks_pw" (key store password), "alias" (key alias) and "key_pw" (key password).

Note that both of the scripts are assumed to be run at root folder of the project.

# Porting New Brogue Variants

Fork Brogue code from [here](https://github.com/bilgincoskun/game-logic-for-brogue-android-port)

Change files under src/brogue and include with your variants' **(Do not change src/platform)**

Make sure it compiles and run

Change BROGUE_VERSION_STRING in Rogue.h with something does not collide with other variants

Edit version_name file with your variant name and version

Make a PR

