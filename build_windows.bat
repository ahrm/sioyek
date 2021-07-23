cd mupdf\platform\win32\
msbuild mupdf.sln /property:Configuration=Release
cd ..\..\..
qmake -tp vs pdf_viewer_build_config.pro
msbuild sioyek.vcxproj /property:Configuration=Release
rm -r sioyek-release-windows 2> NUL
mkdir sioyek-release-windows
cp release\sioyek.exe sioyek-release-windows\sioyek.exe
cp pdf_viewer\keys.config sioyek-release-windows\keys.config
cp pdf_viewer\keys_user.config sioyek-release-windows\keys_user.config
cp pdf_viewer\prefs.config sioyek-release-windows\prefs.config
cp pdf_viewer\prefs_user.config sioyek-release-windows\prefs_user.config
cp -r pdf_viewer\shaders sioyek-release-windows\shaders
zip -r sioyek-release-windows.zip sioyek-release-windows

