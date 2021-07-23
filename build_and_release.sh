mkdir -p linux-deploy-binaries
cd linux-deploy-binaries
curl -L https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage --output linuxdeploy-x86_64.AppImage
curl -L https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage --output linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-plugin-qt-x86_64.AppImage
cd ..

cd mupdf
make USE_SYSTEM_HARFBUZZ=yes
cd ..
qmake pdf_viewer_build_config.pro
rm -r AppDir 2> /dev/null
make install INSTALL_ROOT=sioyek-release

./linux-deploy-binaries/linuxdeploy-x86_64.AppImage --appdir sioyek-release --plugin qt

cp pdf_viewer/prefs.config sioyek-release/usr/bin/prefs.config
cp pdf_viewer/prefs_user.config sioyek-release/usr/bin/prefs_user.config
cp pdf_viewer/keys.config sioyek-release/usr/bin/keys.config
cp pdf_viewer/keys_user.config sioyek-release/usr/bin/keys_user.config
cp -r pdf_viewer/shaders sioyek-release/usr/bin/shaders
cp tutorial.pdf sioyek-release/usr/bin/tutorial.pdf

zip -r sioyek-release.zip sioyek-release
