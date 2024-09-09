#!/usr/bin/env bash
set -e

if [ -z ${MAKE_PARALLEL+x} ]; then export MAKE_PARALLEL=$(nproc); else echo "MAKE_PARALLEL defined"; fi
echo "MAKE_PARALLEL set to $MAKE_PARALLEL"

# download linuxdeployqt if not exists
if [[ ! -f linuxdeployqt-continuous-x86_64.AppImage ]]; then
	wget -q https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
	chmod +x linuxdeployqt-continuous-x86_64.AppImage
fi

cd mupdf
make USE_SYSTEM_HARFBUZZ=yes
cd ..

if [ -z ${QMAKE+x} ]; then
    QMAKE=qmake
fi

if [[ $1 == console ]]; then
	$QMAKE "CONFIG+=linux_app_image console" pdf_viewer_build_config.pro
else
	$QMAKE "CONFIG+=linux_app_image" pdf_viewer_build_config.pro
fi

rm -rf sioyek-release 2> /dev/null
make install INSTALL_ROOT=sioyek-release -j$MAKE_PARALLEL

cp pdf_viewer/prefs.config sioyek-release/usr/bin/prefs.config
cp pdf_viewer/prefs_user.config sioyek-release/usr/share/prefs_user.config
cp pdf_viewer/keys.config sioyek-release/usr/bin/keys.config
cp pdf_viewer/keys_user.config sioyek-release/usr/share/keys_user.config
cp -r pdf_viewer/shaders sioyek-release/usr/bin/shaders
cp tutorial.pdf sioyek-release/usr/bin/tutorial.pdf

#./linuxdeployqt-continuous-x86_64.AppImage --appdir sioyek-release --plugin qt
./linuxdeployqt-continuous-x86_64.AppImage ./sioyek-release/usr/share/applications/sioyek.desktop -appimage


mv Sioyek-* Sioyek-x86_64.AppImage
zip sioyek-release-linux.zip Sioyek-x86_64.AppImage
