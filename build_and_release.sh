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
qmake pdf_viewer_linux.pro
rm -r AppDir 2> /dev/null
make install INSTALL_ROOT=AppDir

./linux-deploy-binaries/linuxdeploy-x86_64.AppImage --appdir AppDir --plugin qt

cp pdf_viewer/prefs.config AppDir/prefs.config
cp pdf_viewer/prefs_user.config AppDir/prefs_user.config
cp pdf_viewer/keys.config AppDir/keys.config
cp pdf_viewer/keys_user.config AppDir/keys_user.config
cp -r pdf_viewer/shaders AppDir/shaders
cp tutorial.pdf AppDir/tutorial.pdf

