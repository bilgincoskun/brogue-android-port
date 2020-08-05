#!/usr/bin/env bash
echo "This script will download all necessary files and setup the project"
tmp_dir=$(mktemp -d)
cur_dir=$(pwd)
mkdir -p "$tmp_dir"
cd "$tmp_dir"
game_folder="game_files"

echo "Downloading SDL2"
wget -qO- https://www.libsdl.org/release/SDL2-2.0.12.tar.gz | tar xz
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
git clone https://github.com/bilgincoskun/game-logic-for-brogue-android-port $game_folder -q
git -C $game_folder pull --all
for b in $(git -C $game_folder branch -r)
do
    git -C $game_folder checkout --track $b  &> /dev/null
done

echo "Setting Symlinks for Brogue Code"
ln -sf ../../../$game_folder/include ./app/jni/src
ln -sf ../../../$game_folder/src/brogue ./app/jni/src
