#!/usr/bin/env bash
set -e

# Compile mupdf
cd mupdf
make USE_SYSTEM_HARFBUZZ=yes -j$(nproc)
cd ..

# set QMAKE if not already defined
if [ -z "$QMAKE" ]; 
then
    if [ -f "/usr/bin/qmake-qt6" ]; 
    then
        QMAKE="/usr/bin/qmake-qt6"
    elif [ -f "/usr/bin/qmake" ]; 
    then
        QMAKE="/usr/bin/qmake"
    else
        QMAKE="qmake"
    fi
fi

$QMAKE "CONFIG+=linux_app_image" pdf_viewer_build_config.pro
make -j$(nproc)

# Copy files in build/ subdirectory
rm -rf build 2> /dev/null
mkdir build
mv sioyek build/sioyek
cp pdf_viewer/prefs.config build/prefs.config
cp pdf_viewer/prefs_user.config build/prefs_user.config
cp pdf_viewer/keys.config build/keys.config
cp pdf_viewer/keys_user.config build/keys_user.config
cp -r pdf_viewer/shaders build/shaders
cp tutorial.pdf build/tutorial.pdf
