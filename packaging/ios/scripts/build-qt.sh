#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "usage: $0 <device|simulator> [--clean]" >&2
    exit 2
}

if (( $# < 1 || $# > 2 )); then
    usage
fi

mode="$1"
clean=0
if (( $# == 2 )); then
    [[ "$2" == "--clean" ]] || usage
    clean=1
fi

case "$mode" in
    device)
        platform=DEVICE
        sdk=iphoneos
        ;;
    simulator)
        platform=SIMULATOR
        sdk=iphonesimulator
        ;;
    *)
        usage
        ;;
esac

repo_root="$(git rev-parse --show-toplevel)"
ios_dir="$repo_root/packaging/ios"
target_root="$repo_root/build-ios/qt/$mode"
source_root="$repo_root/build-ios/qt/sources"
prefix="$target_root/prefix"
dependency_prefix="$repo_root/build-ios/deps/$mode/prefix"
stamp="$target_root/build-input.sha256"

case "$target_root" in
    "$repo_root/build-ios/qt/"*) ;;
    *)
        echo "error: unsafe Qt target root: $target_root" >&2
        exit 1
        ;;
esac

make_tree_writable() {
    local tree="$1"
    [[ -d "$tree" ]] || return
    find "$tree" -type d -exec chmod u+rwx {} +
    find "$tree" -type f -exec chmod u+rw {} +
}

if (( clean )); then
    if [[ -e "$target_root" ]]; then
        make_tree_writable "$target_root"
        rm -rf -- "$target_root"
    fi
fi

"$ios_dir/scripts/check-host.sh"

if [[ ! -f "$dependency_prefix/lib/libz.a" || ! -f "$dependency_prefix/lib/libharfbuzz.a" ]]; then
    echo "error: core $mode dependencies are missing; run build-dependencies.sh first" >&2
    exit 1
fi

resolve_output() {
    (cd "$repo_root" && nix build --no-link --print-out-paths ".#$1") | tail -n 1
}

host_qt="$(resolve_output host-qtbase)"
qtbase_source="$(resolve_output source-qtbase)"
qtsvg_source="$(resolve_output source-qtsvg)"
qt5compat_source="$(resolve_output source-qt5compat)"

build_input="$({
    printf '%s\n' \
        "builder=2" \
        "mode=$mode" \
        "host_qt=$host_qt" \
        "qtbase_source=$qtbase_source" \
        "qtsvg_source=$qtsvg_source" \
        "qt5compat_source=$qt5compat_source"
    xcodebuild -version
    xcrun --sdk "$sdk" --show-sdk-version
    shasum -a 256 "$ios_dir/versions.env" "$ios_dir/scripts/build-qt.sh"
} | shasum -a 256 | awk '{print $1}')"

if [[ -f "$stamp" && "$(<"$stamp")" == "$build_input" ]]; then
    echo "skip Qt: fingerprint matches"
else
    if [[ -e "$prefix" && ! -f "$stamp" ]]; then
        echo "error: unstamped Qt prefix exists; rerun with --clean" >&2
        exit 1
    fi
    if [[ -f "$stamp" && "$(<"$stamp")" != "$build_input" ]]; then
        echo "error: Qt build inputs changed; rerun with --clean" >&2
        exit 1
    fi

    mkdir -p "$source_root" "$target_root"

    materialize_source() {
        local name="$1"
        local source="$2"
        local destination="$source_root/$name-6.11.3"
        local marker="$source_root/$name-6.11.3.source"

        if [[ -d "$destination" && -f "$marker" && "$(<"$marker")" == "$source" ]]; then
            printf '%s\n' "$destination"
            return
        fi

        case "$destination" in
            "$source_root/"*) ;;
            *)
                echo "error: unsafe Qt source destination: $destination" >&2
                exit 1
                ;;
        esac
        if [[ -e "$destination" ]]; then
            make_tree_writable "$destination"
            rm -rf -- "$destination"
        fi
        mkdir -p "$destination"
        if [[ -d "$source" ]]; then
            cp -R "$source/." "$destination"
        else
            tar -xf "$source" -C "$destination" --strip-components=1
        fi
        make_tree_writable "$destination"
        printf '%s\n' "$source" >"$marker"
        printf '%s\n' "$destination"
    }

    qtbase_source_dir="$(materialize_source qtbase "$qtbase_source")"
    qtsvg_source_dir="$(materialize_source qtsvg "$qtsvg_source")"
    qt5compat_source_dir="$(materialize_source qt5compat "$qt5compat_source")"

    qtbase_build="$target_root/qtbase-build"
    mkdir -p "$qtbase_build"
    (
        cd "$qtbase_build"
        "$qtbase_source_dir/configure" \
            -release \
            -static \
            -no-framework \
            -platform macx-ios-clang \
            -sdk "$sdk" \
            -qt-host-path "$host_qt" \
            -prefix "$prefix" \
            -nomake examples \
            -nomake tests \
            -no-dbus \
            -no-feature-printsupport \
            -no-feature-printer \
            -no-feature-printdialog \
            -no-feature-printpreviewdialog \
            -no-feature-printpreviewwidget \
            -no-feature-cups \
            -no-feature-glib \
            -no-feature-icu \
            -no-feature-openssl \
            -no-feature-sctp \
            -no-feature-libproxy \
            -no-feature-brotli \
            -system-zlib \
            -system-libpng \
            -system-freetype \
            -system-harfbuzz \
            -qt-libjpeg \
            -qt-pcre \
            -qt-doubleconversion \
            -qt-sqlite \
            -sql-sqlite \
            -no-sql-db2 \
            -no-sql-ibase \
            -no-sql-mysql \
            -no-sql-oci \
            -no-sql-odbc \
            -no-sql-psql \
            -no-sql-mimer \
            -- \
            -DCMAKE_OSX_ARCHITECTURES=arm64 \
            -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 \
            -DCMAKE_PREFIX_PATH="$dependency_prefix" \
            -DCMAKE_FIND_ROOT_PATH="$dependency_prefix" \
            '-DCMAKE_IGNORE_PREFIX_PATH=/usr/local;/opt/homebrew' \
            -DQT_BUILD_TOOLS_WHEN_CROSSCOMPILING=OFF
    )
    cmake --build "$qtbase_build" --parallel
    cmake --install "$qtbase_build"

    build_module() {
        local name="$1"
        local source="$2"
        local build_dir="$target_root/$name-build"
        mkdir -p "$build_dir"
        (
            cd "$build_dir"
            "$prefix/bin/qt-configure-module" "$source" -- \
                -DQT_BUILD_TESTS=OFF \
                -DQT_BUILD_EXAMPLES=OFF \
                -DCMAKE_PREFIX_PATH="$prefix;$dependency_prefix" \
                -DCMAKE_FIND_ROOT_PATH="$prefix;$dependency_prefix" \
                '-DCMAKE_IGNORE_PREFIX_PATH=/usr/local;/opt/homebrew'
        )
        cmake --build "$build_dir" --parallel
        cmake --install "$build_dir"
    }

    build_module qtsvg "$qtsvg_source_dir"
    build_module qt5compat "$qt5compat_source_dir"
    printf '%s\n' "$build_input" >"$stamp"
fi

required_libraries=(
    libQt6Core.a
    libQt6Concurrent.a
    libQt6Core5Compat.a
    libQt6Gui.a
    libQt6Network.a
    libQt6OpenGL.a
    libQt6OpenGLWidgets.a
    libQt6Sql.a
    libQt6Svg.a
    libQt6SvgWidgets.a
    libQt6Widgets.a
    libQt6Xml.a
)
for library in "${required_libraries[@]}"; do
    if [[ ! -f "$prefix/lib/$library" ]]; then
        echo "error: Qt did not install $library" >&2
        exit 1
    fi
done

archive_count=0
while IFS= read -r archive; do
    "$ios_dir/scripts/inspect-apple-archive.sh" "$mode" "$archive"
    archive_count=$((archive_count + 1))
done < <(find "$prefix" -type f -name '*.a' -print | sort)

echo "Qt prefix: $prefix"
echo "validated Qt archives: $archive_count"
