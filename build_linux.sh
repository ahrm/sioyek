#!/usr/bin/env bash
set -e

if [ -z $1 ];
then
	echo "Defaulting to portable build"
	export MODE="portable"
elif [ $1 == "local" ];
then
	echo "Local build"
	export MODE="local"
elif [ $1 == "portable" ];
then
	echo "Portable build"
	export MODE="portable"
else
	echo "Call with 'local' for a local build, or with 'portable' for a portable build"
	exit
fi

# Get qmake path
if [ -f "/usr/bin/qmake-qt5" ]; 
then
	QMAKE="/usr/bin/qmake-qt5"
elif [ -f "/usr/bin/qmake" ]; 
then
	QMAKE="/usr/bin/qmake"
else
	QMAKE="qmake"
fi

# Clean old build
# make clean
# rm Makefile

# Local build
if [ $MODE == "local" ];
then
	echo "Calling QMake"
	$QMAKE pdf_viewer_build_config.pro
	echo "Calling make"
	make
	echo "Calling make install"
	sudo make install
# Portable build
elif [ $MODE == "portable" ];
then
	# Compile sioyek-shipped version of mupdf
	cd mupdf
	make USE_SYSTEM_HARFBUZZ=yes
	cd ..
	# Configure sioyek build
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
fi

