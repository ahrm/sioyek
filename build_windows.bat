set BUILD_TYPE=Release
set BUILD_TYPELC=release
set PORTABLE=0

:: Parse command line arguments
for %%a in (%*) do (
    if /i "%%a"=="debug" set BUILD_TYPE=Debug && set BUILD_TYPELC=debug
    if /i "%%a"=="portable" set PORTABLE=1
)

:: Build MuPDF
cd mupdf\platform\win32\
msbuild -maxcpucount mupdf.sln /m /property:Configuration=%BUILD_TYPE% /property:MultiProcessorCompilation=true
cd ..\..\..

:: Build Zlib
cd zlib
nmake -f win32/makefile.msc
cd ..

:: Set portable preprocessor flag dynamically based on %PORTABLE%
set QMAKE_DEFINES="DEFINES+=NON_PORTABLE"
if "%PORTABLE%"=="1" set QMAKE_DEFINES=

:: Generate Visual Studio project via qmake
qmake -tp vc %QMAKE_DEFINES% "CONFIG+=%BUILD_TYPELC%" pdf_viewer_build_config.pro

:: Build Sioyek executable
msbuild -maxcpucount sioyek.vcxproj /m /property:Configuration=%BUILD_TYPE%

:: Clean and recreate output staging directory
if exist sioyek-release-windows rmdir /s /q sioyek-release-windows
mkdir sioyek-release-windows

cp %BUILD_TYPELC%\sioyek.exe sioyek-release-windows\sioyek.exe
cp pdf_viewer\keys.config sioyek-release-windows\keys.config
cp pdf_viewer\prefs.config sioyek-release-windows\prefs.config
cp -r pdf_viewer\shaders sioyek-release-windows\shaders
cp tutorial.pdf sioyek-release-windows\tutorial.pdf

:: Deploy Qt dependencies
windeployqt --qmldir ./pdf_viewer/touchui --%BUILD_TYPELC% sioyek-release-windows\sioyek.exe

:: Copy Runtime DLLs if present
if exist windows_runtime\vcruntime140.dll copy /y windows_runtime\vcruntime140.dll sioyek-release-windows\
if exist windows_runtime\vcruntime140_1.dll copy /y windows_runtime\vcruntime140_1.dll sioyek-release-windows\

:: Copy OpenSSL 3 DLLs if present
if exist openssl_build\bin\libssl-3-x64.dll copy /y openssl_build\bin\libssl-3-x64.dll sioyek-release-windows\
if exist openssl_build\bin\libcrypto-3-x64.dll copy /y openssl_build\bin\libcrypto-3-x64.dll sioyek-release-windows\

if "%PORTABLE%"=="1" (
    cp pdf_viewer\keys_user.config sioyek-release-windows\keys_user.config
    cp pdf_viewer\prefs_user.config sioyek-release-windows\prefs_user.config
    7z a sioyek-release-windows-portable.zip sioyek-release-windows
) else (
    7z a sioyek-release-windows.zip sioyek-release-windows
)
