#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# --- Configuration ---
INSTALL_DIR="/Applications"
CLI_WRAPPER="/opt/homebrew/bin/sioyek"
QT_PATH="/opt/homebrew/opt/qt/bin"
MAKE_PARALLEL="${MAKE_PARALLEL:-$(sysctl -n hw.logicalcpu 2>/dev/null || echo 4)}"

export PATH="$QT_PATH:$PATH"

# --- Clean mode ---
if [[ "$1" == "--clean" ]]; then
    echo "Cleaning build artifacts..."
    (cd mupdf && make clean 2>/dev/null || true)
    make clean 2>/dev/null || true
    rm -rf build
    echo "Clean complete."
    if [[ "$2" != "--build" ]]; then
        exit 0
    fi
fi

echo "Building sioyek (parallel=$MAKE_PARALLEL)..."

# --- Build mupdf ---
echo "==> Building mupdf..."
cd mupdf
make HAVE_GLUT=no -j"$MAKE_PARALLEL"
cd ..

# --- Update deployment target ---
sed -Ei '' "s/QMAKE_MACOSX_DEPLOYMENT_TARGET.=.[0-9]+/QMAKE_MACOSX_DEPLOYMENT_TARGET = $(sw_vers -productVersion | cut -d. -f1)/" pdf_viewer_build_config.pro

# --- Build sioyek ---
echo "==> Running qmake6..."
qmake6 "CONFIG+=non_portable" pdf_viewer_build_config.pro

echo "==> Building sioyek..."
make -j"$MAKE_PARALLEL"

# --- Package app bundle ---
echo "==> Packaging app bundle..."
rm -rf build 2>/dev/null
mkdir build
mv sioyek.app build/
cp -r pdf_viewer/shaders build/sioyek.app/Contents/MacOS/shaders
cp pdf_viewer/prefs.config build/sioyek.app/Contents/MacOS/prefs.config
cp pdf_viewer/prefs_user.config build/sioyek.app/Contents/MacOS/prefs_user.config
cp pdf_viewer/keys.config build/sioyek.app/Contents/MacOS/keys.config
cp pdf_viewer/keys_user.config build/sioyek.app/Contents/MacOS/keys_user.config
cp tutorial.pdf build/sioyek.app/Contents/MacOS/tutorial.pdf

# --- Patch Info.plist with PATH ---
INFO_PLIST="build/sioyek.app/Contents/Info.plist"
CURRENT_PATH="$PATH"
/usr/libexec/PlistBuddy -c "Add :LSEnvironment dict" "$INFO_PLIST" 2>/dev/null || true
/usr/libexec/PlistBuddy -c "Add :LSEnvironment:PATH string $CURRENT_PATH" "$INFO_PLIST" 2>/dev/null || \
    /usr/libexec/PlistBuddy -c "Set :LSEnvironment:PATH $CURRENT_PATH" "$INFO_PLIST"

# --- Deploy Qt frameworks ---
echo "==> Running macdeployqt..."
macdeployqt build/sioyek.app

# --- Sign ---
echo "==> Signing..."
codesign --force --deep --sign - build/sioyek.app

# --- Install ---
echo "==> Installing to $INSTALL_DIR..."
rm -rf "$INSTALL_DIR/sioyek.app"
cp -R build/sioyek.app "$INSTALL_DIR/sioyek.app"
xattr -cr "$INSTALL_DIR/sioyek.app"

# --- CLI wrapper ---
echo "==> Writing CLI wrapper to $CLI_WRAPPER..."
cat > "$CLI_WRAPPER" << 'WRAPPER'
#!/bin/sh
exec /Applications/sioyek.app/Contents/MacOS/sioyek "$@"
WRAPPER
chmod +x "$CLI_WRAPPER"

echo ""
echo "Done! sioyek installed to $INSTALL_DIR/sioyek.app"
echo "CLI available at: $CLI_WRAPPER"
