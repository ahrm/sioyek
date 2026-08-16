cd mupdf\platform\win32\
msbuild -maxcpucount mupdf.sln /m /property:Configuration=Debug /property:MultiProcessorCompilation=true
msbuild -maxcpucount mupdf.sln /m /property:Configuration=Release /property:MultiProcessorCompilation=true
cd ..\..\..

cd zlib
nmake -f win32/makefile.msc
cd ..

qmake -tp vc "DEFINES+=NON_PORTABLE" "CONFIG+=release" pdf_viewer_build_config.pro

msbuild -maxcpucount sioyek.vcxproj /m /property:Configuration=Release
rm -r sioyek-release-windows 2> NUL
mkdir sioyek-release-windows
cp release\sioyek.exe sioyek-release-windows\sioyek.exe
cp pdf_viewer\keys.config sioyek-release-windows\keys.config
cp pdf_viewer\prefs.config sioyek-release-windows\prefs.config
cp -r pdf_viewer\shaders sioyek-release-windows\shaders
cp tutorial.pdf sioyek-release-windows\tutorial.pdf

windeployqt --qmldir ./pdf_viewer/touchui --release sioyek-release-windows\sioyek.exe
REM windeployqt sioyek-release-windows\sioyek.exe

REM Overwrite Qt's default vcruntime with the secure ones pulled from System32
cp windows_runtime\vcruntime140.dll sioyek-release-windows\vcruntime140.dll
cp windows_runtime\vcruntime140_1.dll sioyek-release-windows\vcruntime140_1.dll

REM Feed Qt6 the OpenSSL 3 DLLs it expects to find at runtime
cp openssl_build\bin\libssl-3-x64.dll sioyek-release-windows\libssl-3-x64.dll
cp openssl_build\bin\libcrypto-3-x64.dll sioyek-release-windows\libcrypto-3-x64.dll

if %1 == portable (
    cp pdf_viewer\keys_user.config sioyek-release-windows\keys_user.config
    cp pdf_viewer\prefs_user.config sioyek-release-windows\prefs_user.config
    7z a sioyek-release-windows-portable.zip sioyek-release-windows

) else (
    7z a sioyek-release-windows.zip sioyek-release-windows
)
