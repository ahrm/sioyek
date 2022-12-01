#!/usr/bin/env bash

# Compile mupdf
cd mupdf
make USE_SYSTEM_HARFBUZZ=yes
cd ..

# Compile sioyek
if [ -f "/usr/bin/qmake-qt5" ]; 
then
	QMAKE="/usr/bin/qmake-qt5"
elif [ -f "/usr/bin/qmake" ]; 
then
	QMAKE="/usr/bin/qmake"
else
	QMAKE="qmake"
fi

$QMAKE "CONFIG+=linux_app_image" pdf_viewer_build_config.pro
make

# Copy files in build/ subdirectory
rm -r build 2> /dev/null
mkdir build
mv sioyek build/sioyek
cp pdf_viewer/prefs.config build/prefs.config
cp pdf_viewer/prefs_user.config build/prefs_user.config
cp pdf_viewer/keys.config build/keys.config
cp pdf_viewer/keys_user.config build/keys_user.config
cp -r pdf_viewer/shaders build/shaders
cp tutorial.pdf build/tutorial.pdf
