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

if [ "$#" -ne 2 ]; then
    fail "Usage: LIBREPAINT_DEPS_PATH=/absolute/dependency/prefix $0 BUILD_PREFIX SOURCE_DIR"
fi

# Read in our parameters. The dependency prefix must be supplied explicitly;
# this repository no longer carries a downloader or dependency builder.
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
if [ ! -f "$KRITA_SOURCES/packaging/linux/appimage/override_compiler.sh.inc" ]; then
    fail "AppImage compiler configuration is missing from the source tree."
fi

# qjsonparser, used to add metadata to the plugins needs to work in a en_US.UTF-8 environment.
# That's not always the case, so make sure it is
export LC_ALL=en_US.UTF-8
export LANG=en_us.UTF-8

# Setup variables needed to help everything find what we build
export LD_LIBRARY_PATH="$DEPS_INSTALL_PREFIX/lib:${LD_LIBRARY_PATH:-}"
export PATH="$DEPS_INSTALL_PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="$DEPS_INSTALL_PREFIX/share/pkgconfig:$DEPS_INSTALL_PREFIX/lib/pkgconfig:/usr/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
export CMAKE_PREFIX_PATH="$DEPS_INSTALL_PREFIX:${CMAKE_PREFIX_PATH:-}"
# https://docs.python.org/3.10/using/cmdline.html#envvar-PYTHONHOME
if [ -d "$DEPS_INSTALL_PREFIX/sip" ]; then
    export PYTHONPATH="$DEPS_INSTALL_PREFIX/sip"
fi
export PYTHONHOME="$DEPS_INSTALL_PREFIX"

source "${KRITA_SOURCES}/packaging/linux/appimage/override_compiler.sh.inc"

if ! command -v cmake >/dev/null 2>&1; then
    fail "cmake was not found in the dependency prefix or host PATH."
fi
if ! command -v nproc >/dev/null 2>&1; then
    fail "nproc was not found in the dependency prefix or host PATH."
fi

cd "$KRITA_SOURCES"

BUILD_TYPE="Release"

# Make sure our build directory exists
if [ ! -d "$BUILD_PREFIX/krita-build/" ] ; then
    mkdir -p "$BUILD_PREFIX/krita-build/"
fi

# Now switch to it
cd "$BUILD_PREFIX/krita-build/"

# Determine how many CPUs we have
CPU_COUNT=$(nproc)

if [ $CPU_COUNT -gt 2 ]; then
    let "jobs = ${CPU_COUNT} - 2"
    CPU_COUNT=$jobs
fi

# Configure Krita
cmake "$KRITA_SOURCES" \
    "-DCMAKE_INSTALL_PREFIX:PATH=$BUILD_PREFIX/krita.appdir/usr" \
    -DDEFINE_NO_DEPRECATED=1 \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DFOUNDATION_BUILD=1 \
    -DHIDE_SAFE_ASSERTS=ON \
    -DBUILD_TESTING=FALSE \
    -DKRITA_ENABLE_PCH=off \
    "-DPYQT_SIP_DIR_OVERRIDE=$DEPS_INSTALL_PREFIX/share/sip/" \
    -DHAVE_MEMORY_LEAK_TRACKER=FALSE

# Build and Install Krita (ready for the next phase)
cmake --build . --target install --parallel "$CPU_COUNT"
