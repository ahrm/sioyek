#!/usr/bin/env bash
set -e
# prerequisite: brew install qt@5 freeglut mesa harfbuzz

#sys_glut_clfags=`pkg-config --cflags glut gl`
#sys_glut_libs=`pkg-config --libs glut gl`
#sys_harfbuzz_clfags=`pkg-config --cflags harfbuzz`
#sys_harfbuzz_libs=`pkg-config --libs harfbuzz`

if [ -z ${MAKE_PARALLEL+x} ]; then export MAKE_PARALLEL=1; else echo "MAKE_PARALLEL defined"; fi
echo "MAKE_PARALLEL set to $MAKE_PARALLEL"

cd mupdf
#make USE_SYSTEM_HARFBUZZ=yes USE_SYSTEM_GLUT=yes SYS_GLUT_CFLAGS="${sys_glut_clfags}" SYS_GLUT_LIBS="${sys_glut_libs}" SYS_HARFBUZZ_CFLAGS="${sys_harfbuzz_clfags}" SYS_HARFBUZZ_LIBS="${sys_harfbuzz_libs}" -j 4
make -j$MAKE_PARALLEL
cd ..

sed -Ei '' "s/QMAKE_MACOSX_DEPLOYMENT_TARGET.=.[0-9]+/QMAKE_MACOSX_DEPLOYMENT_TARGET = $(sw_vers -productVersion | cut -d. -f1)/" pdf_viewer_build_config.pro

if [[ $1 == portable ]]; then
	qmake pdf_viewer_build_config.pro
else
	qmake "CONFIG+=non_portable" pdf_viewer_build_config.pro
fi

make -j$MAKE_PARALLEL

rm -rf build 2> /dev/null
mkdir build
mv sioyek.app build/
cp -r pdf_viewer/shaders build/sioyek.app/Contents/MacOS/shaders

cp pdf_viewer/prefs.config build/sioyek.app/Contents/MacOS/prefs.config
cp pdf_viewer/prefs_user.config build/sioyek.app/Contents/MacOS/prefs_user.config
cp pdf_viewer/keys.config build/sioyek.app/Contents/MacOS/keys.config
cp pdf_viewer/keys_user.config build/sioyek.app/Contents/MacOS/keys_user.config
cp tutorial.pdf build/sioyek.app/Contents/MacOS/tutorial.pdf

# Capture the current PATH
CURRENT_PATH=$(echo $PATH)

# Define the path to the Info.plist file inside the app bundle
INFO_PLIST="build/sioyek.app/Contents/Info.plist"

# Add LSEnvironment key with PATH to Info.plist
/usr/libexec/PlistBuddy -c "Add :LSEnvironment dict" "$INFO_PLIST" || echo "LSEnvironment already exists"
/usr/libexec/PlistBuddy -c "Add :LSEnvironment:PATH string $CURRENT_PATH" "$INFO_PLIST" || /usr/libexec/PlistBuddy -c "Set :LSEnvironment:PATH $CURRENT_PATH" "$INFO_PLIST"

# Hack is required to avoid race condition in macos in CI
# See https://github.com/actions/runner-images/issues/7522
if [[ -n "$GITHUB_ACTIONS" ]]; then
  echo killing...; sudo pkill -9 XProtect >/dev/null || true;
  echo waiting...; while pgrep XProtect; do sleep 3; done;
fi

sleep 5

# mac deploys with qml currently don't work due to a qt bug
# macdeployqt build/sioyek.app -qmldir=./pdf_viewer/touchui -dmg
macdeployqt build/sioyek.app -dmg

zip -r sioyek-release-mac.zip build/sioyek.dmg
