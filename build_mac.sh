#!/usr/bin/env bash
set -euo pipefail

if [[ -z ${MAKE_PARALLEL+x} ]]; then
  MAKE_PARALLEL=$(sysctl -n hw.logicalcpu)
fi

if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew is required for the macOS CMake build." >&2
  exit 1
fi

CMAKE=${CMAKE:-cmake}
if ! command -v "$CMAKE" >/dev/null 2>&1; then
  CMAKE="$(brew --prefix cmake)/bin/cmake"
fi

required_formulae=(cmake mupdf qtspeech)
cmake_prefixes=()
for formula in qtbase qtdeclarative qtsvg qtmultimedia qtspeech mupdf; do
  prefix=$(brew --prefix "$formula" 2>/dev/null || true)
  if [[ -z $prefix || ! -d $prefix ]]; then
    echo "Missing Homebrew formula '$formula'. Install dependencies with:" >&2
    echo "  brew install ${required_formulae[*]}" >&2
    exit 1
  fi
  cmake_prefixes+=("$prefix")
done

for formula in qtquicktimeline qtquick3d qtshadertools; do
  prefix=$(brew --prefix "$formula" 2>/dev/null || true)
  if [[ -n $prefix && -d $prefix ]]; then
    cmake_prefixes+=("$prefix")
  fi
done

cmake_prefix_path=$(IFS=';'; printf '%s' "${cmake_prefixes[*]}")
if [[ -n ${CMAKE_PREFIX_PATH:-} ]]; then
  cmake_prefix_path="$cmake_prefix_path;$CMAKE_PREFIX_PATH"
fi

build_dir=${BUILD_DIR:-build}
case "$build_dir" in
  /*) ;;
  *) build_dir="$PWD/$build_dir" ;;
esac

package_dir=${PACKAGE_DIR:-$build_dir/package}
case "$package_dir" in
  /*) ;;
  *) package_dir="$PWD/$package_dir" ;;
esac

release_zip=${RELEASE_ZIP:-sioyek-release-mac.zip}

rm -rf "$package_dir" "$build_dir/sioyek.app" "$build_dir/sioyek.dmg"

"$CMAKE" -S . -B "$build_dir" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$cmake_prefix_path"

"$CMAKE" --build "$build_dir" --parallel "$MAKE_PARALLEL"
"$CMAKE" --install "$build_dir" --prefix "$package_dir"

app_path="$package_dir/sioyek.app"

qtquicktimeline_prefix=$(brew --prefix qtquicktimeline 2>/dev/null || true)
if [[ -n $qtquicktimeline_prefix && -d $qtquicktimeline_prefix/lib ]]; then
  for framework in "$qtquicktimeline_prefix"/lib/*.framework; do
    [[ -d "$framework" ]] || continue
    ditto "$framework" "$app_path/Contents/Frameworks/$(basename "$framework")"
  done
fi

frameworks_dir="$app_path/Contents/Frameworks"
# Normalize install names that macdeployqt leaves pointing at Homebrew.
while IFS= read -r binary; do
  otool_output=$(otool -L "$binary" 2>/dev/null || true)
  [[ -n $otool_output ]] || continue

  if [[ $binary == "$frameworks_dir"/* ]]; then
    relative_path=${binary#"$frameworks_dir"/}
    install_name_tool -id "@rpath/$relative_path" "$binary" 2>/dev/null || true
  fi

  while IFS= read -r line; do
    dependency=${line#"${line%%[![:space:]]*}"}
    dependency=${dependency%% (*}
    case "$dependency" in
      /opt/homebrew/*/lib/*|/usr/local/opt/*/lib/*|/usr/local/Cellar/*/lib/*)
        relative_path=${dependency#*/lib/}
        if [[ -e "$frameworks_dir/$relative_path" ]]; then
          install_name_tool -change "$dependency" "@executable_path/../Frameworks/$relative_path" "$binary"
        fi
        ;;
    esac
  done <<< "$otool_output"
done < <(find "$app_path/Contents" -type f)

# Avoid intermittent CI failures caused by XProtect scanning newly created bundles.
if [[ -n ${GITHUB_ACTIONS:-} ]]; then
  sudo pkill -9 XProtect >/dev/null 2>&1 || true
  while pgrep XProtect >/dev/null; do sleep 3; done
fi

codesign --force --deep --sign - "$app_path"

rm -f "$build_dir/sioyek.dmg" "$release_zip"
hdiutil create -volname sioyek -srcfolder "$app_path" -ov -format UDZO "$build_dir/sioyek.dmg"
ditto -c -k --keepParent "$build_dir/sioyek.dmg" "$release_zip"
