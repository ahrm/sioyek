set BUILD_TYPE=Release
set BUILD_TYPELC=release
set PORTABLE=0

for %%a in (%*) do (
    if /i "%%a"=="debug" set BUILD_TYPE=Debug && set BUILD_TYPELC=debug
    if /i "%%a"=="portable" set PORTABLE=1
)

cd mupdf\platform\win32\
msbuild -maxcpucount mupdf.sln /m /property:Configuration=%BUILD_TYPE% /property:MultiProcessorCompilation=true
cd ..\..\..

cd zlib
nmake -f win32/makefile.msc
cd ..

:: Configure qmake dynamically based on our build type
if "%BUILD_TYPE%"=="Debug" (
    qmake -tp vc "DEFINES+=NON_PORTABLE" "CONFIG+=debug" pdf_viewer_build_config.pro
) else (
    qmake -tp vc "DEFINES+=NON_PORTABLE" "CONFIG+=release" pdf_viewer_build_config.pro
)

msbuild -maxcpucount sioyek.vcxproj /m /property:Configuration=%BUILD_TYPE%
rm -r sioyek-release-windows 2> NUL
mkdir sioyek-release-windows
:: Note: MSBuild outputs to a folder named after the configuration (debug or release lower case)
cp %BUILD_TYPELC%\sioyek.exe sioyek-release-windows\sioyek.exe
cp pdf_viewer\keys.config sioyek-release-windows\keys.config
cp pdf_viewer\prefs.config sioyek-release-windows\prefs.config
cp -r pdf_viewer\shaders sioyek-release-windows\shaders
cp tutorial.pdf sioyek-release-windows\tutorial.pdf

windeployqt --qmldir ./pdf_viewer/touchui --%BUILD_TYPELC% sioyek-release-windows\sioyek.exe
REM windeployqt sioyek-release-windows\sioyek.exe

REM Overwrite Qt's default vcruntime with the secure ones pulled from System32
cp windows_runtime\vcruntime140.dll sioyek-release-windows\vcruntime140.dll
cp windows_runtime\vcruntime140_1.dll sioyek-release-windows\vcruntime140_1.dll

REM Feed Qt6 the OpenSSL 3 DLLs it expects to find at runtime
cp openssl_build\bin\libssl-3-x64.dll sioyek-release-windows\libssl-3-x64.dll
cp openssl_build\bin\libcrypto-3-x64.dll sioyek-release-windows\libcrypto-3-x64.dll

if "%PORTABLE%"=="1" (
    cp pdf_viewer\keys_user.config sioyek-release-windows\keys_user.config
    cp pdf_viewer\prefs_user.config sioyek-release-windows\prefs_user.config
    7z a sioyek-release-windows-portable.zip sioyek-release-windows
) else (
    7z a sioyek-release-windows.zip sioyek-release-windows
)
