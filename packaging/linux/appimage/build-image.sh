#!/bin/bash
#
#  SPDX-License-Identifier: GPL-3.0-or-later
#

# Halt on errors and be verbose about what we are doing
set -e
set -x

fail()
{
  echo "ERROR: $*" >&2
  exit 2
}

require_command()
{
  if ! command -v "$1" >/dev/null 2>&1; then
    fail "Required command was not found: $1"
  fi
}

require_directory()
{
  if [ ! -d "$1" ]; then
    fail "Required directory is missing: $1"
  fi
}

require_match()
{
  if ! compgen -G "$1" >/dev/null; then
    fail "Required dependency files are missing: $1"
  fi
}

if [ "$#" -ne 2 ]; then
  fail "Usage: LIBREPAINT_DEPS_PATH=/absolute/dependency/prefix $0 BUILD_PREFIX SOURCE_DIR"
fi

# Read in our parameters. Dependencies are an explicit, prebuilt input; this
# script never downloads or builds them.
export BUILD_PREFIX="$1"
export KRITA_SOURCES="$2"
export DEPS_INSTALL_PREFIX="${LIBREPAINT_DEPS_PATH:-${KRITA_DEPS_PATH:-}}"

if [ -z "$DEPS_INSTALL_PREFIX" ]; then
  fail "Set LIBREPAINT_DEPS_PATH to a prebuilt AppImage dependency prefix (KRITA_DEPS_PATH is accepted for compatibility)."
fi
case "$BUILD_PREFIX" in
  /*) ;;
  *) fail "BUILD_PREFIX must be an absolute path: $BUILD_PREFIX" ;;
esac
case "$KRITA_SOURCES" in
  /*) ;;
  *) fail "SOURCE_DIR must be an absolute path: $KRITA_SOURCES" ;;
esac
case "$DEPS_INSTALL_PREFIX" in
  /*) ;;
  *) fail "The dependency prefix must be an absolute path: $DEPS_INSTALL_PREFIX" ;;
esac
if [ ! -d "$BUILD_PREFIX" ]; then
  fail "Build prefix does not exist: $BUILD_PREFIX (run build-krita.sh first)."
fi
if [ ! -d "$DEPS_INSTALL_PREFIX" ]; then
  fail "Dependency prefix does not exist: $DEPS_INSTALL_PREFIX"
fi
if [ ! -f "$KRITA_SOURCES/CMakeLists.txt" ]; then
  fail "Source directory does not contain CMakeLists.txt: $KRITA_SOURCES"
fi
if [ ! -x "$DEPS_INSTALL_PREFIX/bin/qtpaths" ]; then
  fail "Dependency prefix is missing bin/qtpaths: $DEPS_INSTALL_PREFIX"
fi
if [ ! -d "$DEPS_INSTALL_PREFIX/lib" ] || [ ! -d "$DEPS_INSTALL_PREFIX/share" ]; then
  fail "Dependency prefix must contain lib/ and share/: $DEPS_INSTALL_PREFIX"
fi

# Save some frequently referenced locations in variables for ease of use / updating
export APPDIR="${KRITA_APPDIR_PATH:-$BUILD_PREFIX/krita.appdir}"
export PLUGINS="$APPDIR/usr/lib/kritaplugins/"

# qjsonparser, used to add metadata to the plugins needs to work in a en_US.UTF-8 environment.
# That's not always the case, so make sure it is
export LC_ALL=en_US.UTF-8
export LANG=en_us.UTF-8

export BUILD_DIR="${KRITA_BUILD_PATH:-$BUILD_PREFIX/krita-build/}"

case "$APPDIR" in
  /*) ;;
  *) fail "The AppDir path must be absolute: $APPDIR" ;;
esac
case "$BUILD_DIR" in
  /*) ;;
  *) fail "The build directory must be absolute: $BUILD_DIR" ;;
esac

# Prefer tools shipped in the dependency prefix, including linuxdeployqt when
# it is part of the prebuilt AppImage tool set.
export PATH="$DEPS_INSTALL_PREFIX/bin/:$PATH"
if [ -d "$DEPS_INSTALL_PREFIX/appimage-tools/bin" ]; then
  export PATH="$DEPS_INSTALL_PREFIX/appimage-tools/bin/:$PATH"
fi

for required_command in cmake dpkg find gcc git linuxdeployqt patchelf realpath rsync; do
  require_command "$required_command"
done
if [ -n "${STRIP_APPIMAGE:-}" ]; then
  require_command mktemp
  require_command strip
fi

# Setup variables needed to help everything find what we built
ARCH=$(dpkg --print-architecture)
TRIPLET=$(gcc -dumpmachine)
export LD_LIBRARY_PATH="$DEPS_INSTALL_PREFIX/lib/:$DEPS_INSTALL_PREFIX/lib/$TRIPLET/:$APPDIR/usr/lib/:${LD_LIBRARY_PATH:-}"
export PKG_CONFIG_PATH="$DEPS_INSTALL_PREFIX/share/pkgconfig/:$DEPS_INSTALL_PREFIX/lib/pkgconfig/:/usr/lib/pkgconfig/:${PKG_CONFIG_PATH:-}"
export CMAKE_PREFIX_PATH="$DEPS_INSTALL_PREFIX:${CMAKE_PREFIX_PATH:-}"

QT_VERSION=$("$DEPS_INSTALL_PREFIX/bin/qtpaths" --qt-version)
export QT_VERSION_MAJOR=${QT_VERSION%%.*}
if [ "$QT_VERSION_MAJOR" != "5" ] && [ "$QT_VERSION_MAJOR" != "6" ]; then
  fail "Could not determine the Qt major version from $DEPS_INSTALL_PREFIX/bin/qtpaths."
fi

# https://docs.python.org/3.10/using/cmdline.html#envvar-PYTHONHOME
if [ -d "$DEPS_INSTALL_PREFIX/sip" ] ; then
export PYTHONPATH="$DEPS_INSTALL_PREFIX/sip"
fi
export PYTHONHOME="$DEPS_INSTALL_PREFIX"

# Detect Python version; for example 'python3.13'.
PYTHON_DIR=$(find "$DEPS_INSTALL_PREFIX/lib" -maxdepth 1 -name 'python*' -type d -print -quit)
if [ -z "$PYTHON_DIR" ]; then
  fail "Dependency prefix is missing its Python library directory: $DEPS_INSTALL_PREFIX/lib/python*"
fi
export PYTHON_VER=${PYTHON_DIR##*/}
if [ -d "$DEPS_INSTALL_PREFIX/lib/$PYTHON_VER/site-packages/PyQt5/" ]; then
  export PYQT_VER=PyQt5
elif [ -d "$DEPS_INSTALL_PREFIX/lib/$PYTHON_VER/site-packages/PyQt6/" ]; then
  export PYQT_VER=PyQt6
else
  fail "Dependency prefix is missing PyQt5 or PyQt6 for $PYTHON_VER."
fi

for required_source_file in \
  "$KRITA_SOURCES/packaging/linux/appimage/override_compiler.sh.inc" \
  "$KRITA_SOURCES/packaging/linux/appimage/krita-apprun/CMakeLists.txt" \
  "$KRITA_SOURCES/packaging/linux/appimage/krita-apprun/main.c"; do
  if [ ! -f "$required_source_file" ]; then
    fail "Required AppImage source file is missing: $required_source_file"
  fi
done

for required_file in \
  "$BUILD_DIR/libs/version/kritaversion.h" \
  "$APPDIR/usr/bin/krita" \
  "$APPDIR/usr/share/applications/org.kde.krita.desktop" \
  "$APPDIR/usr/share/metainfo/org.kde.krita.appdata.xml" \
  "$APPDIR/usr/share/icons/hicolor/256x256/apps/krita.png"; do
  if [ ! -f "$required_file" ]; then
    fail "Required build output is missing: $required_file (run build-krita.sh first)."
  fi
done

for required_directory in \
  "$APPDIR/usr/lib/kritaplugins" \
  "$DEPS_INSTALL_PREFIX/lib/$PYTHON_VER/lib-dynload" \
  "$DEPS_INSTALL_PREFIX/lib/mlt-7" \
  "$DEPS_INSTALL_PREFIX/share/locale" \
  "$DEPS_INSTALL_PREFIX/share/mime" \
  "$DEPS_INSTALL_PREFIX/share/mlt-7" \
  "$DEPS_INSTALL_PREFIX/translations"; do
  require_directory "$required_directory"
done
if [ ! -d "$DEPS_INSTALL_PREFIX/share/kf5" ] && [ ! -d "$DEPS_INSTALL_PREFIX/share/kf6" ]; then
  fail "Dependency prefix must contain share/kf5/ or share/kf6/: $DEPS_INSTALL_PREFIX"
fi

for required_pattern in \
  "$APPDIR/usr/lib/kritaplugins/*.so*" \
  "$DEPS_INSTALL_PREFIX/bin/ff*" \
  "$DEPS_INSTALL_PREFIX/lib/libav*.s*" \
  "$DEPS_INSTALL_PREFIX/lib/libfontconfig.so.1*" \
  "$DEPS_INSTALL_PREFIX/lib/libfreetype.so.6*" \
  "$DEPS_INSTALL_PREFIX/lib/libfribidi.so.0*" \
  "$DEPS_INSTALL_PREFIX/lib/libharfbuzz.so.0*" \
  "$DEPS_INSTALL_PREFIX/lib/libmlt*.so*" \
  "$DEPS_INSTALL_PREFIX/lib/libsw*.s*"; do
  require_match "$required_pattern"
done

SYSTEM_LIBSTDCPP_DIR="/usr/lib/$TRIPLET"
shopt -s nullglob
SYSTEM_LIBSTDCPP_FILES=("$SYSTEM_LIBSTDCPP_DIR"/libstdc++.so.6.*)
shopt -u nullglob
if [ ! -d "$APPDIR/usr/libstdcpp-fallback" ] && [ "${#SYSTEM_LIBSTDCPP_FILES[@]}" -eq 0 ]; then
  fail "No libstdc++.so.6.* fallback was found under $SYSTEM_LIBSTDCPP_DIR."
fi

source "${KRITA_SOURCES}/packaging/linux/appimage/override_compiler.sh.inc"

if [[ $ARCH == "arm64" ]]; then
  APPIMAGE_ARCHITECTURE="aarch64"
elif [[ $ARCH == "amd64" ]]; then
  APPIMAGE_ARCHITECTURE="x86_64"
else
  APPIMAGE_ARCHITECTURE=$ARCH
fi

# Step 0: Find the locally built version and give the AppImage a unique name
cd "$BUILD_DIR"

KRITA_VERSION=$(grep "#define KRITA_VERSION_STRING" libs/version/kritaversion.h | cut -d '"' -f 2)
cd "$KRITA_SOURCES"

VERSION_TYPE="development"
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    GIT_REVISION=$(git rev-parse --short HEAD)
    export VERSION=$KRITA_VERSION-$GIT_REVISION
else
    export VERSION=$KRITA_VERSION
fi
NEW_APPIMAGE_NAME="LibrePaint-${VERSION}-${APPIMAGE_ARCHITECTURE}.AppImage"

# Switch over to our build prefix
cd "$BUILD_PREFIX"

#
# Now we can get the process started!
#

# Step 0: place the translations where ki18n and Qt look for them
#
# In Qt5 version of Krita we used to patch ki18n to locate the translations
# in QStandardPaths::AppLocalDataLocation, hence all the translations had to
# be moved into the Krita-local directory. In Qt6 we dropped this patch, so
# all the translations are now searched in QStandardPaths::GenericDataLocation

if [ "$QT_VERSION_MAJOR" = "5" ] ; then
    if [ -d "$APPDIR/usr/share/locale" ] ; then
        rsync -prul "$APPDIR/usr/share/locale" "$APPDIR/usr/share/krita"
        rm -rf "$APPDIR/usr/share/locale"
    fi
    rsync -prul "$DEPS_INSTALL_PREFIX/share/locale" "$APPDIR/usr/share/krita"
else
    rsync -prul "$DEPS_INSTALL_PREFIX/share/locale" "$APPDIR/usr/share/"
fi

# Step 1: Copy over all necessary resources required by dependencies or libraries that are missed by linuxdeployqt

if [ -d "$DEPS_INSTALL_PREFIX/share/kf5" ]; then
    cp -r "$DEPS_INSTALL_PREFIX/share/kf5" "$APPDIR/usr/share"
else
    cp -r "$DEPS_INSTALL_PREFIX/share/kf6" "$APPDIR/usr/share"
fi
cp -r "$DEPS_INSTALL_PREFIX/share/mime" "$APPDIR/usr/share"
cp -r "$DEPS_INSTALL_PREFIX/lib/$PYTHON_VER" "$APPDIR/usr/lib"
if [ -d "$DEPS_INSTALL_PREFIX/share/sip" ] ; then
cp -r "$DEPS_INSTALL_PREFIX/share/sip" "$APPDIR/usr/share"
fi

cp -r "$DEPS_INSTALL_PREFIX/translations" "$APPDIR/usr/"

if [ ! -d "$APPDIR/usr/libstdcpp-fallback/" ] ; then
    mkdir -p "$APPDIR/usr/libstdcpp-fallback/"
    cd "$APPDIR/usr/libstdcpp-fallback/"
    cp "${SYSTEM_LIBSTDCPP_FILES[@]}" ./
    ln -s "$(basename "${SYSTEM_LIBSTDCPP_FILES[0]}")" libstdc++.so.6
fi

mkdir "$BUILD_PREFIX/krita-apprun-build"
(
    cd "$BUILD_PREFIX/krita-apprun-build"
    cmake -DCMAKE_BUILD_TYPE=Release "$KRITA_SOURCES/packaging/linux/appimage/krita-apprun/"
    cmake --build .
    cp AppRun "$APPDIR"
)
rm -rf "$BUILD_PREFIX/krita-apprun-build"

if [ -d "$APPDIR/usr/lib/$PYTHON_VER/site-packages" ]; then
    rm -rf "$APPDIR/usr/lib/$PYTHON_VER/site-packages"/packaging*
    rm -rf "$APPDIR/usr/lib/$PYTHON_VER/site-packages"/pip*
    rm -rf "$APPDIR/usr/lib/$PYTHON_VER/site-packages"/pyparsing*
    rm -rf "$APPDIR/usr/lib/$PYTHON_VER/site-packages"/PyQt_builder*
    rm -rf "$APPDIR/usr/lib/$PYTHON_VER/site-packages"/setuptools*
    rm -rf "$APPDIR/usr/lib/$PYTHON_VER/site-packages"/sip*
    rm -rf "$APPDIR/usr/lib/$PYTHON_VER/site-packages"/toml*
    rm -rf "$APPDIR/usr/lib/$PYTHON_VER/site-packages/easy-install.pth"
fi

## Font related deps are explicitly ignored by AppImage build script,
## so we should copy them manually
cp -av --preserve=links "$DEPS_INSTALL_PREFIX/lib"/libfontconfig.so.1* "$APPDIR/usr/lib/"
cp -av --preserve=links "$DEPS_INSTALL_PREFIX/lib"/libharfbuzz.so.0* "$APPDIR/usr/lib/"
cp -av --preserve=links "$DEPS_INSTALL_PREFIX/lib"/libfribidi.so.0* "$APPDIR/usr/lib/"
cp -av --preserve=links "$DEPS_INSTALL_PREFIX/lib"/libfreetype.so.6* "$APPDIR/usr/lib/"

## == MLT Dependencies and Resources ==
cp -r "$DEPS_INSTALL_PREFIX/share/mlt-7" "$APPDIR/usr/share/mlt-7"
cp -r "$DEPS_INSTALL_PREFIX/lib/mlt-7" "$APPDIR/usr/lib/mlt-7"
cp -av --preserve=links "$DEPS_INSTALL_PREFIX/lib"/libmlt*.so* "$APPDIR/usr/lib/"

MLT_BINARIES=()
for BIN in "$APPDIR/usr/lib"/libmlt*.so*; do
  MLT_BINARIES+=("-executable=$BIN")
done

for BIN in "$APPDIR/usr/lib/mlt-7"/*.so*; do
  MLT_BINARIES+=("-executable=$BIN")
done

## == FFMPEG Dependencies and Resources ==
cp -av --preserve=links "$DEPS_INSTALL_PREFIX/lib"/libav*.s* "$APPDIR/usr/lib/"
## TODO: remove (libpostproc is disabled in our builds of ffmpeg)
##cp -av --preserve=links $DEPS_INSTALL_PREFIX/lib/libpostproc*.s* $APPDIR/usr/lib/
cp -av --preserve=links "$DEPS_INSTALL_PREFIX/lib"/libsw*.s* "$APPDIR/usr/lib/"
cp -av "$DEPS_INSTALL_PREFIX/bin"/ff* "$APPDIR/usr/bin/"

FFMPEG_BINARIES=()

for BIN in "$APPDIR/usr/bin"/ff*; do
  FFMPEG_BINARIES+=("-executable=$BIN")
done

for BIN in "$APPDIR/usr/lib"/libav*.s*; do
  FFMPEG_BINARIES+=("-executable=$BIN")
done;

## TODO: remove (libpostproc is disabled in our builds of ffmpeg)
#for BIN in $APPDIR/usr/lib/libpostproc*; do
#  FFMPEG_BINARIES="${FFMPEG_BINARIES} -executable=${BIN}"
#done;

for BIN in "$APPDIR/usr/lib"/libsw*.s*; do
  FFMPEG_BINARIES+=("-executable=$BIN")
done;

# Step 2: Relocate binaries from the architecture specific directory as required for Appimages
if [[ -d "$APPDIR/usr/lib/$TRIPLET" ]] ; then
  rsync -prul "$APPDIR/usr/lib/$TRIPLET/" "$APPDIR/usr/lib/"
  rm -rf "$APPDIR/usr/lib/$TRIPLET/"
fi

# Step 3: Update the rpath in the various plugins we have to make sure they'll be loadable in an Appimage context
for lib in "$PLUGINS"/*.so*; do
  patchelf --set-rpath '$ORIGIN/..' "$lib";
done

if [ -d "$APPDIR/usr/lib/$PYTHON_VER/site-packages/$PYQT_VER/" ] ; then
  for lib in "$APPDIR/usr/lib/$PYTHON_VER/site-packages/$PYQT_VER"/*.so*; do
    patchelf --set-rpath '$ORIGIN/../..' "$lib";
  done
fi

for lib in "$APPDIR/usr/lib/$PYTHON_VER/lib-dynload"/*.so*; do
  patchelf --set-rpath '$ORIGIN/../..' "$lib";
done

if [ -f "$APPDIR/usr/lib/krita-python-libs/PyKrita/krita.so" ]; then
  patchelf --set-rpath '$ORIGIN/../..' "$APPDIR/usr/lib/krita-python-libs/PyKrita/krita.so"
else
  echo "WARNING: not found $APPDIR/usr/lib/krita-python-libs/PyKrita/krita.so, skipping..."
fi

if [ -f "$APPDIR/usr/lib/$PYTHON_VER/site-packages/$PYQT_VER/sip.so" ] ; then
patchelf --set-rpath '$ORIGIN/../..' "$APPDIR/usr/lib/$PYTHON_VER/site-packages/$PYQT_VER/sip.so"
fi

# Step 4: Add local build version metadata

cd "$KRITA_SOURCES"

DATE=$(git log -1 --format="%ct" | xargs -I{} date -d @{} +%Y-%m-%d)
if [ "$DATE" = "" ] ; then
        DATE=$(date +%Y-%m-%d)
fi

sed -e "s|<release version=\"\" date=\"\" />|<release version=\"$VERSION\" date=\"$DATE\" type=\"$VERSION_TYPE\"/>|" -i "$APPDIR/usr/share/metainfo/org.kde.krita.appdata.xml"

# Return to our build root
cd "$BUILD_PREFIX"

# place the icon where linuxdeployqt seems to expect it
find "$APPDIR" -name krita.png
cp "$APPDIR/usr/share/icons/hicolor/256x256/apps/krita.png" "$APPDIR"
ls "$APPDIR"

if [ -n "$STRIP_APPIMAGE" ]; then
    # strip debugging information
    function find-elf-files {
        # * python libraries are not strippable (strip fails with error)
        # * AppImage packages should not be stripped, because it breaks them
        find "$1" -type f -name "*" -not -name "*.o" -not -path "*/python3.*/*" -not -name "libstdc++.so.6.*" -exec sh -c '
            case "$(head -n 1 "$1")" in
            ?ELF*) exit 0;;
            esac
            exit 1
            ' sh {} \; -print
    }

    TEMPFILE=$(mktemp)
    find-elf-files "$APPDIR" > "$TEMPFILE"

    while IFS= read -r i; do
        strip -v --strip-unneeded --strip-debug "$i"
    done < "$TEMPFILE"

    rm -f "$TEMPFILE"
fi

EXTRA_PLUGINS_LIST="$PLUGINS"
if [ -f "$APPDIR/usr/lib/krita-python-libs/PyKrita/krita.so" ]; then
  EXTRA_PLUGINS_LIST="$EXTRA_PLUGINS_LIST,$APPDIR/usr/lib/krita-python-libs/PyKrita/krita.so"
fi

WAYLAND_PLATFORM_PLUGIN_NAME=
# Qt6.8 names the plugin file as libqwayland-generic.so
if [ -f "$DEPS_INSTALL_PREFIX/plugins/platforms/libqwayland-generic.so" ]; then
  WAYLAND_PLATFORM_PLUGIN_NAME="platforms/libqwayland-generic.so"
fi
# Qt6.10 names the plugin file as libqwayland.so
if [ -f "$DEPS_INSTALL_PREFIX/plugins/platforms/libqwayland.so" ]; then
  WAYLAND_PLATFORM_PLUGIN_NAME="platforms/libqwayland.so"
fi

if [ -n "$WAYLAND_PLATFORM_PLUGIN_NAME" ]; then
  EXTRA_PLATFORM_PLUGINS="$WAYLAND_PLATFORM_PLUGIN_NAME,wayland-shell-integration/libxdg-shell.so,wayland-graphics-integration-client/libqt-plugin-wayland-egl.so"
  EXTRA_PLUGINS_LIST="$EXTRA_PLUGINS_LIST,$EXTRA_PLATFORM_PLUGINS"
fi

if [ -d "$DEPS_INSTALL_PREFIX/plugins/wayland-decoration-client" ]; then
  for plugin in "$DEPS_INSTALL_PREFIX/plugins/wayland-decoration-client"/*.so; do
    echo "Adding client-side-decoration plugin: $plugin"
    relative_plugin=$(realpath -m --relative-to="$DEPS_INSTALL_PREFIX/plugins" "$plugin")
    EXTRA_PLUGINS_LIST="$EXTRA_PLUGINS_LIST,$relative_plugin"
  done
fi

EXTRA_RUNTIME_ARGUMENT=()

RUNTIME_FILE="$DEPS_INSTALL_PREFIX/appimage-tools/share/runtime-$APPIMAGE_ARCHITECTURE"
if [ -f "$RUNTIME_FILE" ]; then
  EXTRA_RUNTIME_ARGUMENT=("-runtime-file=$RUNTIME_FILE")
fi

# Step 4: Build the image!!!
linuxdeployqt "$APPDIR/usr/share/applications/org.kde.krita.desktop" \
  "-executable=$APPDIR/usr/bin/krita" \
  "${MLT_BINARIES[@]}" \
  "${FFMPEG_BINARIES[@]}" \
  "-qmldir=$KRITA_SOURCES/plugins/dockers/textproperties" \
  -bundle-non-qt-libs \
  "-extra-plugins=$EXTRA_PLUGINS_LIST" \
  "${EXTRA_RUNTIME_ARGUMENT[@]}" \
  -appimage || (echo "failed with exit code $?"; exit 1)

# No update endpoint is embedded. LibrePaint has no independent AppImage update
# service, and the upstream project's update feeds must not be reused.
test -f "${NEW_APPIMAGE_NAME}"
