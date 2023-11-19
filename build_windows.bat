cd mupdf\platform\win32\
msbuild mupdf.sln /property:Configuration=Debug
msbuild mupdf.sln /property:Configuration=Release
cd ..\..\..

cd zlib
nmake -f win32/makefile.msc
cd ..

if %1 == portable (
    qmake -tp vc pdf_viewer_build_config.pro
) else (
    qmake -tp vc "DEFINES+=NON_PORTABLE" pdf_viewer_build_config.pro
)

msbuild -maxcpucount sioyek.vcxproj /property:Configuration=Release
rmdir /S sioyek-release-windows
mkdir sioyek-release-windows
copy release\sioyek.exe sioyek-release-windows\sioyek.exe
copy pdf_viewer\keys.config sioyek-release-windows\keys.config
copy pdf_viewer\prefs.config sioyek-release-windows\prefs.config
xcopy /E /I pdf_viewer\shaders sioyek-release-windows\shaders\
copy tutorial.pdf sioyek-release-windows\tutorial.pdf
windeployqt sioyek-release-windows\sioyek.exe
copy windows_runtime\vcruntime140_1.dll sioyek-release-windows\vcruntime140_1.dll
copy windows_runtime\libssl-1_1-x64.dll sioyek-release-windows\libssl-1_1-x64.dll
copy windows_runtime\libcrypto-1_1-x64.dll sioyek-release-windows\libcrypto-1_1-x64.dll

if %1 == portable (
    copy pdf_viewer\keys_user.config sioyek-release-windows\keys_user.config
    copy pdf_viewer\prefs_user.config sioyek-release-windows\prefs_user.config
    7z a sioyek-release-windows-portable.zip sioyek-release-windows

) else (
    7z a sioyek-release-windows.zip sioyek-release-windows
)
