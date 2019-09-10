#!/usr/bin/env bash
echo "This script will download all necessasry files and setup the project"
tmp_dir=$(mktemp -d)
cur_dir=$(pwd)
mkdir -p "$tmp_dir"
cd "$tmp_dir"

echo "Downloading SDL2"
wget -qO- https://www.libsdl.org/release/SDL2-2.0.10.tar.gz | tar xz
mv SDL2* SDL

echo "Downloading SDL2_ttf"
wget -qO- https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.15.tar.gz | tar xz
mv SDL2_ttf* SDL_ttf

mv SDL/android-project/app/src/main/java/org/libsdl "$cur_dir"/app/src/main/java/org/
mv SDL "$cur_dir"/app/jni
mv SDL_ttf "$cur_dir"/app/jni

cd -
rm -rf "$tmp_dir"

echo "Downloading Brogue Files"
#You can also use original distribution but dont forget to move necessary .h files and .c files to header and src/brogue folders
git clone https://github.com/bilgincoskun/brogue-libtcod-sdl2-fix brogue-files -b 175-with-fixes -q

echo "Setting Symlinks for Brogue Code"
ln -sf ../../../brogue-files/include ./app/jni/src
ln -sf ../../../brogue-files/src/brogue ./app/jni/src
