#!/usr/bin/env bash

if [ -z ${MAKE_PARALLEL+x} ]; then export MAKE_PARALLEL=1; else echo "MAKE_PARALLEL defined"; fi
echo "MAKE_PARALLEL set to $MAKE_PARALLEL"

# download linuxdeployqt if not exists
if [[ ! -f linuxdeployqt-continuous-x86_64.AppImage ]]; then
	wget -q https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
	chmod +x linuxdeployqt-continuous-x86_64.AppImage
fi

cd mupdf
make USE_SYSTEM_HARFBUZZ=yes
cd ..

if [[ $1 == portable ]]; then
	qmake "CONFIG+=linux_app_image" pdf_viewer_build_config.pro
else
	qmake "CONFIG+=linux_app_image non_portable" pdf_viewer_build_config.pro
fi

rm -r sioyek-release 2> /dev/null
make install INSTALL_ROOT=sioyek-release -j$MAKE_PARALLEL

if [[ $1 == portable ]]; then
	cp pdf_viewer/prefs.config sioyek-release/usr/bin/prefs.config
	cp pdf_viewer/prefs_user.config sioyek-release/usr/bin/prefs_user.config
	cp pdf_viewer/keys.config sioyek-release/usr/bin/keys.config
	cp pdf_viewer/keys_user.config sioyek-release/usr/bin/keys_user.config
	cp -r pdf_viewer/shaders sioyek-release/usr/bin/shaders
	cp tutorial.pdf sioyek-release/usr/bin/tutorial.pdf
else
	cp pdf_viewer/prefs.config sioyek-release/usr/bin/prefs.config
	cp pdf_viewer/prefs_user.config sioyek-release/usr/share/prefs_user.config
	cp pdf_viewer/keys.config sioyek-release/usr/bin/keys.config
	cp pdf_viewer/keys_user.config sioyek-release/usr/share/keys_user.config
	cp -r pdf_viewer/shaders sioyek-release/usr/bin/shaders
	cp tutorial.pdf sioyek-release/usr/bin/tutorial.pdf
fi

#./linuxdeployqt-continuous-x86_64.AppImage --appdir sioyek-release --plugin qt
./linuxdeployqt-continuous-x86_64.AppImage ./sioyek-release/usr/share/applications/sioyek.desktop -appimage


if [[ $1 == portable ]]; then
	rm -r Sioyek-x86_64.AppImage.config
	rm Sioyek-x86_64.AppImage
	mv Sioyek-* Sioyek-x86_64.AppImage
	mkdir -p Sioyek-x86_64.AppImage.config/.local/share/Sioyek
	cp tutorial.pdf Sioyek-x86_64.AppImage.config/.local/share/Sioyek/tutorial.pdf
	zip -r sioyek-release-linux-portable.zip Sioyek-x86_64.AppImage.config Sioyek-x86_64.AppImage
else
	mv Sioyek-* Sioyek-x86_64.AppImage
	zip sioyek-release-linux.zip Sioyek-x86_64.AppImage
fi
