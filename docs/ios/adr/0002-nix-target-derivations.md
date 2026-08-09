# ADR 0002: Build iOS target artifacts as granular Nix derivations

- Status: accepted
- Date: 2026-08-03

## Context

The first iPadOS build pipeline pinned sources and host tools with Nix but
installed all target libraries into mutable prefixes under `build-ios/`. Its
fingerprints avoided many local rebuilds. Granular Nix outputs add binary-cache
substitution and recovery after local garbage collection while limiting the
rebuild boundary to each changed package.

The proprietary iPhoneOS SDK and Apple toolchain must remain external. Their
identity participates in every target derivation key, giving each validated SDK
its own cache key.

## Decision

1. Each open-source iOS library is built into an independent Nix store path.
   Sources, patches, flags, and direct target dependencies are package-local.
2. Each package installs into its own store path; `symlinkJoin` creates the
   aggregated prefixes.
3. The exact Xcode, Xcode build, SDK, SDK build, Apple Clang, deployment target,
   and architecture form a checked toolchain contract and are derivation
   inputs.
4. Xcode remains outside the Nix store. A target derivation reads only the
   validated Xcode application and SDK. The pre-configuration gate requires an
   exact match with the toolchain contract.
5. All Apple static archives are created with `ZERO_AR_DATE=1` and normalized
   with deterministic `ranlib`. Every archive member is checked for arm64,
   platform IOS, minimum OS, SDK version, and duplicate member names.
6. The output reference scan must return zero matches for the Xcode installation
   and temporary build directories.
7. Target store paths are published only to a private Nix binary cache. Shared
   caches require a Nix cache signature; this is unrelated to Apple application
   signing. Cache private keys and Apple credentials stay outside the
   repository.
8. Local GC roots retain both the current target aggregate and its cache-
   deployment closure, including derivations, sources, and existing build-time-
   only outputs. GC is skipped when that closure cannot be refreshed without a
   target build.
9. AltStore signing, USB device installation, launch, and device log collection
   stay outside Nix because they mutate external state and require credentials.
10. The script-driven build remains available until equivalent Nix packages and
   probes have been validated. Migration proceeds package-by-package after each
   probe passes.

## Xcode sandbox boundary

The validated host runs the Darwin Nix daemon with sandboxing enabled and
fallback disabled. Xcode stays out of `sandbox-paths` and is the only
project-specific addition to the administrator-controlled impure host
dependency allowlist:

```nix
nix.settings.sandbox = true;
nix.settings.sandbox-fallback = false;
nix.settings.extra-allowed-impure-host-deps = [
  "/Applications/Xcode.app"
];
```

Using the `extra-` setting preserves Nix's required Darwin defaults. The common
iOS builder declares `toolchain.impureHostDeps` through `__impureHostDeps`, so
only those derivations can see Xcode. A control derivation that omits the
declaration confirmed the absence of `/Applications/Xcode.app` from its build
sandbox.

Target derivations read the canonical Xcode and iPhoneOS platform XML plists
directly for version and build identity. `xcodebuild` is reserved for host-side
configure, bundling, signing, and installation scripts: its IDE/DVT startup
initializes file watchers and crashes when the Nix sandbox denies unrelated
Mach services and host paths. Apple Clang and the SDK work with the existing
strict host allowlist.

The common builders reserve their phase, fixed-output, network, and impure-host
attributes, enforcing that boundary for every package recipe. The checked
toolchain identity takes precedence over package-specific passthru data, and
every compiled member of a propagated target dependency closure carries the
same identity. Pure header packages use a builder keyed by source and header
dependencies and carry `iosTargetIndependent = true`.
When one pure header package depends on another, that builder validates the
complete closure as toolchain-independent and writes the direct dependencies to
`nix-support/propagated-build-inputs`; this is explicit because its minimal
phase list intentionally omits stdenv fixup. The resulting package remains
Xcode-independent while binary-cache copies retain its required headers.
Target builders accept a dependency only when it either has that marker and no
toolchain identity, or carries the exact current toolchain identity. Header
trees reject symlinks, special files, compiled artifacts, and mutations by
package check hooks before they can enter a target closure. Autoconf cache
entries are validated strings with a configure-only export path. Autotools and
Meson pkg-config lookups use only declared target closures, with a private empty
directory as the search
root when a package has no target dependency. Meson's CMake prefix similarly
contains only the declared target closure, while native pkg-config and CMake
search paths are empty. Its `nofallback` wrap mode rejects dependency fallback.
The Meson cross file deliberately leaves `sys_root` unset. Its separate native
machine file invokes the Nix compiler with both `SDKROOT` and `DEVELOPER_DIR`
removed from the compiler commands, while target compilation and linking remain
fixed to the validated Apple compiler and iPhoneOS SDK.

The administrator-controlled nix-darwin configuration installs the restricted
daemon settings; clients and flakes consume that policy. On
2026-08-03 the active policy was verified with `nix config show`, followed by:

```sh
nix build .#zlib-ios .#libpng-ios --no-link --no-substitute
nix build .#zlib-ios .#libpng-ios --no-link --no-substitute --rebuild
nix build .#ios-dependencies --no-link --no-substitute
nix flake check
```

The first command rebuilt both packages from source, including archive checks
and the libpng consumer link probe. The second command matched both existing
outputs, establishing determinism under the enforced sandbox.

## First proof

`zlib-ios` 1.3.2 is the first package using this design. It is built with Xcode
26.6 build 17F113, iPhoneOS SDK 26.5 build 23F81a, Apple Clang 21.0.0 build
2100.1.1.101, deployment target 17.0, and arm64. The following gates passed:

- Nix store build from the locked zlib source;
- all 15 archive members validated as arm64/IOS;
- relative CMake and pkg-config installation metadata;
- retained Xcode paths: 0;
- `nix build --rebuild` produced an identical output;
- copy to and verification from a local Nix binary cache.

The local proof cache uses the explicit local `--no-check-sigs` exception and
is restored by `packaging/ios/scripts/restore-nix-cache.sh`. Shared caches use
normal Nix signature verification.

## Second proof

libpng 1.6.58 is the first package to consume another migrated target
derivation. Its zlib input is propagated into the output closure, and the
installed CMake package is tested by linking a small iOS executable through
`PNG::PNG` and the transitive `ZLIB::ZLIB` target. All 18 archive members,
including the arm64 NEON implementation, passed the same architecture and
platform checks. Forced rebuilds of zlib and libpng matched their existing
outputs, and the two-path closure was restored and recursively verified in an
isolated Nix store from the local binary cache.

## Third proof

FreeType 2.14.3 is the first package with two direct migrated target
dependencies. Its feature contract requires zlib and libpng while disabling
BZip2, HarfBuzz, and Brotli. The generated `ftoption.h` is checked against that
contract, and all 42 object members in the archive pass the iOS metadata gates.
A consumer declares only FreeType as its direct target dependency but discovers
and links all three archives through `Freetype::Freetype` alone. The installed
config adds explicit `ZLIB::ZLIB` and `PNG::PNG` dependency discovery. That
consumer also proves that the common builder recursively adds propagated target
dependencies to CMake's search roots. A forced rebuild
matched the existing output. The three-path
zlib/libpng/FreeType closure was then published to the local binary cache,
restored into a separate temporary Nix store, and verified recursively without
rebuilding.

## Further target proofs

Expat 2.8.2 fixes the XML character, DTD, general-entity, namespace, and context
features and validates both its CMake target and static pkg-config contract.
Little CMS 2.19.1 keeps thread support while suppressing an unnecessary Apple
`libm` lookup that otherwise embeds the external SDK path in its exported CMake
target. Eigen 3.4.1 is header-only, so its proof builds an iOS C++ consumer
through `Eigen3::Eigen` as the validation artifact.

HarfBuzz 13.2.1 consumes only FreeType directly; the common target closure adds
zlib and libpng. Its installed CMake export replaces upstream's raw FreeType
archive and absolute SDK framework paths with `Freetype::Freetype` and portable
CoreFoundation/CoreText/CoreGraphics link items. A direct-only consumer forces
both the FreeType and CoreText bridges into the link. Source and forced rebuilds
matched for all four packages. Their outputs and the four-path HarfBuzz closure
were published to the local cache and restored into isolated stores.

Fontconfig 2.18.2 is the first package using the common Autotools target
builder. Target compilation and pkg-config lookup see only its direct Expat and
FreeType dependencies and their propagated libpng/zlib closure. Build-machine
configure probes use the Nix host compiler with `SDKROOT` removed, while target
objects use the validated Apple compiler and iPhoneOS SDK. The release
`configure` omits expansion of its `AX_FUNC_SNPRINTF` check, so the recipe fixes
the generated script and records the known iOS C99 snprintf/vsnprintf contract
as configure-cache inputs. A second upstream Autotools omission leaves
`fcconffile.c` out of the archive despite exporting `FcConfigFileGenerate`; the
package patches both source and generated Makefile bookkeeping. Its consumer
forces that symbol, the Expat parser, and the FreeType query path into one iOS
link and checks the exact five-archive closure. A forced rebuild matched the
existing output, and the five-path closure was restored into an isolated store
from the local cache.

xsimd 14.3.0 adds a source- and toolchain-keyed header-only package. Its
consumer uses the installed CMake target to compile a real vector batch for the
pinned arm64 iOS target; that executable is the header-only target's contract
proof. libunibreak 7.0 builds through the same repository CMake wrapper used
by the legacy dependency pipeline. Its consumer resolves
`libunibreak::libunibreak` through Krita's find module and links the UTF-8 line-
breaking API, keeping the Nix package aligned with the actual application
discovery path. Source and forced rebuilds matched for both packages. The
updated ten-package aggregate and its complete 11-path runtime closure were
published to the local binary cache, restored into an empty isolated Nix store,
and recursively verified without access to the primary store.

libjpeg-turbo 3.1.4.1 fixes its static JPEG and TurboJPEG outputs, requires the
arm64 NEON implementation, and pins the embedded build identity to the existing
iOS dependency baseline date `19800101`. Two separate consumers use the
installed `libjpeg-turbo::jpeg-static` and
`libjpeg-turbo::turbojpeg-static` targets, making the proof independent of
archive order despite overlapping codec objects. Krita's active iOS JPEG file
plugin continues to use Qt's bundled JPEG implementation; the migrated external
codec remains an independently validated dependency. Its source and forced
rebuilds matched, and the resulting 12-path aggregate closure
was restored and verified in an empty store from the local cache.

Exiv2 0.28.8 pins its audited library-only feature contract. It keeps the
SDK-provided Iconv implementation, and the static CMake and pkg-config metadata
carry the portable `-liconv` item required by final iOS links. Their reference
scan reports zero host and SDK paths. Exiv2 propagates only the migrated zlib
target as a store dependency. Its consumer compiles and links the
creation and reopening of an in-memory JPEG with Exif metadata plus the public
character-conversion API through `Exiv2::exiv2lib`. Every archive member and the
resulting executable use the pinned arm64 iOS target. The resulting 12-package
aggregate and its complete 13-path closure were restored into an empty store
from the local cache.

Boost 1.89.0 is the first package built by the pure header path. It copies the
locked upstream headers and generates the same relocatable `Boost::headers`,
`Boost::boost`, and `Boost::disable_autolinking` CMake contract used by the
legacy prefix. The package output's reference scan reports zero Xcode, SDK,
toolchain identity, and other Nix store paths. A separate pinned-toolchain
consumer compiles real Boost.MP11 and circular-buffer APIs for arm64 iOS. The resulting 13-package
aggregate and its complete 14-path target closure were restored into an empty
store from the local cache.

Immer 0.9.1 and Zug 0.1.2 also use the pure header path. Their generated
package metadata corrects the older versions declared by upstream CMake,
retains Krita's plain `immer` and `zug` target names, and exports their C++14
minimum. Separate consumers exercise rejected and accepted same-major ranges,
resolve the exact versions, and compile real APIs. The Zug proof uses a
filtering transducer under C++17 with the standard library's `std::variant`
implementing its skip state. The resulting 15-package
aggregate and its complete 16-path target closure were restored into an empty
store from the local cache.

Lager 0.1.3 extends the pure header path with target dependencies. The generated
package metadata replaces upstream's project version 0.1.0, adds a version file,
and completes the plain `lager` target with C++17 plus the Boost and Zug targets
used directly by Lager's core state, cursor, watch, lens, and store headers.
Immer remains a separate aggregate package because its use is confined to optional
debugger/cereal headers outside Krita's active API set. This keeps the reverse
rebuild graph aligned with actual dependencies. A consumer with only
`lager-ios` as its direct Nix dependency compiles those APIs and a
manual-event-loop store for arm64 iOS. The resulting 16-package aggregate has a
complete 17-path target closure.

libintl 1.0 is built from only the `gettext-runtime/intl` subtree of the locked
GNU gettext source. The manifest fixes the cross-compile answers and excludes
Java, C#, OpenMP, Emacs, libiconv-prefix, and ncurses-prefix integration. Its
target output contains only `libintl.h` and the static `libintl.a`; native
gettext and Perl remain build-time tools outside the output closure. A
direct-only CMake consumer resolves the exact Intl version, calls the
translation, domain, and plural APIs, and
explicitly links the iOS SDK's
iconv implementation and CoreFoundation. The package and consumer rebuild
deterministically, while the resulting 17-package aggregate and complete
18-path target closure restore into an empty local store from the cache.

FriBidi 1.0.16 is the first package using the common Meson target builder. Its
seven `gen-*` build executables are verified as arm64 macOS binaries, while all
18 members of `libfribidi.a` are verified against the pinned arm64 iOS
toolchain contract. The locked feature set keeps deprecated interfaces enabled
and disables documentation, command-line binaries, and tests; install checks
also reject manuals and dynamic libraries. A direct-only consumer resolves the
exact version through Krita's existing `FindFriBidi.cmake` module and calls the
three FriBidi APIs used by bundled Raqm. The package and consumer rebuild
deterministically, and the resulting 18-package aggregate and complete 19-path
target closure restore into an empty local store from the cache.

## Consequences

- Normal Krita source edits preserve the existing target dependency outputs.
- A package source, patch, recipe, flag, direct dependency, or toolchain
  contract change rebuilds that package and its reverse dependencies.
- Static consumers intentionally rebuild when an input library store path
  changes; bypassing that invalidation would be unsafe.
- A matching private-cache object avoids compilation even after local GC or on
  another compatible Apple Silicon Mac.
- Fully reproducing a cache miss still requires the validated proprietary
  Xcode installation.
