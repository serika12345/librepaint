{
  fribidi,
  lib,
  mkIOSMesonPackage,
  packageSpec,
  pkg-config,
  toolchain,
}:

assert lib.assertMsg (
  fribidi.version == packageSpec.version
) "FriBidi source version ${fribidi.version} does not match iOS manifest ${packageSpec.version}";
assert lib.assertMsg (
  packageSpec.build_system == "meson"
) "FriBidi iOS manifest build system must be Meson";
assert lib.assertMsg (
  packageSpec.dependencies == [ ]
) "FriBidi must remain independent of other target packages";
assert lib.assertMsg (
  packageSpec.meson_args == [
    "-Ddeprecated=true"
    "-Ddocs=false"
    "-Dbin=false"
    "-Dtests=false"
  ]
) "FriBidi iOS Meson feature contract changed";

mkIOSMesonPackage {
  pname = "fribidi-ios";
  inherit (packageSpec) version;
  src = fribidi.src;

  mesonFlags = packageSpec.meson_args;
  nativeInstallCheckInputs = [ pkg-config ];

  requiredPaths = [
    "include/fribidi/fribidi.h"
    "include/fribidi/fribidi-brackets.h"
    "include/fribidi/fribidi-config.h"
    "include/fribidi/fribidi-unicode-version.h"
    "lib/libfribidi.a"
    "lib/pkgconfig/fribidi.pc"
  ];

  staticArchives = [ "lib/libfribidi.a" ];

  postBuild = ''
    native_generator_count=0
    while IFS= read -r -d "" generator; do
      native_generator_count=$((native_generator_count + 1))
      test "$(${toolchain.lipo} -archs "$generator")" = "${toolchain.architecture}"
      generator_metadata="$(${toolchain.vtool} -show-build "$generator")"
      if ! grep -Eq 'platform[[:space:]]+MACOS([[:space:]]|$)' <<<"$generator_metadata"; then
        echo "error: FriBidi generator is not a native macOS executable: $generator" >&2
        exit 1
      fi
    done < <(find build/gen.tab -maxdepth 1 -type f -name 'gen-*' -print0)
    if test "$native_generator_count" -ne 7; then
      echo "error: built $native_generator_count native FriBidi generators; expected 7" >&2
      exit 1
    fi
  '';

  postInstallCheck = ''
    if test -d "$out/bin"; then
      echo "error: target-only FriBidi output contains command-line tools" >&2
      exit 1
    fi
    if test -d "$out/share/man"; then
      echo "error: target-only FriBidi output contains manual pages" >&2
      exit 1
    fi
    unexpected_library="$(find "$out/lib" -maxdepth 1 -type f \( \
      -name '*.a' ! -name 'libfribidi.a' -o \
      -name '*.dylib' -o -name '*.so' -o -name '*.so.*' -o -name '*.la' \
    \) -print -quit)"
    if test -n "$unexpected_library"; then
      echo "error: target-only FriBidi output contains an unexpected library: $unexpected_library" >&2
      exit 1
    fi

    config_header="$out/include/fribidi/fribidi-config.h"
    unicode_header="$out/include/fribidi/fribidi-unicode-version.h"
    public_header="$out/include/fribidi/fribidi.h"
    grep -Eq '^#define[[:space:]]+FRIBIDI_VERSION[[:space:]]+"${lib.escapeRegex packageSpec.version}"([[:space:]]|$)' "$config_header"
    grep -Eq '^#define[[:space:]]+FRIBIDI_INTERFACE_VERSION[[:space:]]+4([[:space:]]|$)' "$config_header"
    grep -Eq '^#define[[:space:]]+FRIBIDI_SIZEOF_INT[[:space:]]+4([[:space:]]|$)' "$config_header"
    grep -Eq '^#define[[:space:]]+FRIBIDI_UNICODE_VERSION[[:space:]]+"16\.0\.0"([[:space:]]|$)' "$unicode_header"
    grep -Fq '# include "fribidi-deprecated.h"' "$public_header"

    archive="$out/lib/libfribidi.a"
    member_count="$(${toolchain.ar} -t "$archive" | grep -v '^__.SYMDEF' | wc -l | tr -d ' ')"
    if test "$member_count" -ne 18; then
      echo "error: libfribidi.a contains $member_count objects; expected the pinned 18-object runtime" >&2
      exit 1
    fi

    exported_symbols="$(${toolchain.toolchainDir}/nm -gU "$archive")"
    for symbol in \
      _fribidi_get_bidi_types \
      _fribidi_get_bracket_types \
      _fribidi_get_par_embedding_levels_ex \
      _fribidi_set_mirroring; do
      if ! grep -Eq "[[:space:]]T[[:space:]]+$symbol$" <<<"$exported_symbols"; then
        echo "error: libfribidi.a omits required public symbol $symbol" >&2
        exit 1
      fi
    done

    export PKG_CONFIG_PATH=
    export PKG_CONFIG_DIR=
    export PKG_CONFIG_LIBDIR="$out/lib/pkgconfig"
    export PKG_CONFIG_SYSROOT_DIR=
    test "$(pkg-config --modversion fribidi)" = "${packageSpec.version}"
    pkg_cflags="$(pkg-config --cflags fribidi)"
    pkg_libs="$(pkg-config --static --libs fribidi)"
    grep -Fq -- "-I$out/include/fribidi" <<<"$pkg_cflags"
    grep -Fq -- "-L$out/lib" <<<"$pkg_libs"
    grep -Eq -- '(^|[[:space:]])-lfribidi([[:space:]]|$)' <<<"$pkg_libs"
  '';

  meta = {
    description = "Minimal static GNU FriBidi runtime for Krita on iPadOS";
    license = lib.licenses.lgpl21Plus;
  };
}
