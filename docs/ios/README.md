# iPadOS port baseline

The audit of Android code reuse, shared mobile behavior, and required iOS
platform boundaries is recorded in the
[Android reuse and platform-boundary audit](android-reuse-audit.md).

## Validated matrix

| Component | Pinned value |
|---|---|
| Krita base revision | `7173825999953623d28777a163a65b42a3f26f0a` |
| Host | Apple Silicon macOS |
| Nix | 2.31 or newer |
| Xcode | 26.6 |
| iPhoneOS SDK | 26.5 |
| Qt | 6.11.1 |
| KDE Frameworks/ECM | 6.28.0 |
| Deployment target | iPadOS 17.0 |
| Device architecture | arm64 |

The exact executable host and SDK checks live in `packaging/ios/versions.env`.
Dependency sources are selected by the locked nixpkgs revision and exposed as
flake package outputs. The current 58-package target dependency closure is
fixed as granular Nix derivations which use the validated external Xcode SDK
while keeping the SDK external to the Nix store. See
`docs/ios/adr/0002-nix-target-derivations.md`.

## Build boundary

Nix pins host build tools and open-source target dependencies and owns the
cacheable target build recipes. Xcode supplies Apple Clang and the proprietary
SDK. AltStore/AltServer can perform local development signing and device
installation; credentials remain in the local signing environment. See
`docs/ios/adr/0001-nix-xcode-boundary.md` and
`docs/ios/adr/0002-nix-target-derivations.md`.

The validated Darwin daemon uses `sandbox = true` and
`sandbox-fallback = false`. `/Applications/Xcode.app` stays out of
`sandbox-paths` and is visible only to derivations that explicitly declare it
through `__impureHostDeps`. Target recipes read version identity directly from
Xcode's plists. Host-side build and deployment scripts invoke `xcodebuild`
where needed.

## Build the pinned target dependency closure

The complete aggregate contains 42 base C/C++ packages; QtBase,
QtSvg, Qt5Compat, and QuaZip; nine KF6 packages; and the Poppler, LibKDCraw, and
KSeExpr feature packages. Build the aggregate and its full consumer proof with:

```sh
nix build .#ios-dependencies --no-link
nix build .#kf6-consumer-check --no-link
```

Their derivations check the complete Xcode/SDK/compiler contract and validate
every member of the resulting static archives. The libpng check also builds a
small iOS consumer through `PNG::PNG` and verifies the transitive zlib package.
The FreeType package requires both target packages, fixes its optional feature
set, and adds explicit CMake dependency discovery for them. Its direct-only
consumer check links all three archives through `Freetype::Freetype` alone and
verifies that the common builder expands FreeType's propagated zlib/libpng
closure into the target CMake roots. The `.#ios-dependencies` output is the
complete open-source dependency closure for the current iPad profile. HarfBuzz
additionally verifies its FreeType bridge and
portable CoreText framework export. Fontconfig uses the same sandbox and target
closure contract through a separate common Autotools builder; its host probes
use a Nix compiler with the iPhoneOS SDK removed. A pkg-config consumer forces
Fontconfig's XML, FreeType, and generated-configuration paths into a five-
archive iOS link. Expat, Little CMS, Eigen, xsimd, and libunibreak each build a
small target consumer for their CMake package contract. The xsimd proof compiles
an arm64 SIMD batch through its exported header-only target. The libunibreak
proof follows Krita's `Findlibunibreak.cmake` path and links its UTF-8 line-break
API. The libjpeg-turbo proof links its JPEG and TurboJPEG static exports in
separate consumers and requires arm64 NEON objects. Exiv2 fixes its audited
library-only feature contract and verifies the installed static target through
an in-memory JPEG/Exif and character-conversion compile/link probe plus its
transitive zlib archive. Its export carries the SDK-portable `-liconv` link
item. Boost is a pure header derivation:
its package output is source-keyed but deliberately independent of Xcode, while
its consumer still compiles representative APIs with the pinned Apple toolchain.
Immer and Zug use the same pure path, fix their manifest versions in relocatable
CMake metadata, implement standard same-major range matching, and export the
C++14 requirement. Their consumers check rejected, accepted, and exact version
requests. Zug additionally uses its C++17
`std::variant` skip path entirely through the standard library.
Lager corrects upstream's stale package version and completes its public
contract. Its pure package propagates the Boost and Zug headers required by Krita's
state/cursor/watch/store APIs, exports C++17 through the installed plain `lager`
target. Debugger-only Immer/Cereal support belongs to optional headers outside
that core target. A consumer that directly depends on and links only Lager compiles
those real APIs for arm64 iOS, proving that the two pure dependencies survive a
standalone cache restore.

libintl builds only GNU gettext's `gettext-runtime/intl` subtree. Its cross
answers and feature settings are part of the manifest, and its target closure
contains `libintl.h` and `libintl.a`; gettext tools remain host build inputs.
A direct-only consumer resolves CMake's `Intl::Intl`, calls the
gettext/domain/plural APIs, and links the SDK's
portable iconv and CoreFoundation interfaces into an arm64 iOS executable. At
that migration checkpoint, the 17-package aggregate and its complete 18-path
closure were restored into an empty local store solely from the binary cache.

FriBidi 1.0.16 is the first package using the common Meson target builder.
Separate native and cross machine files keep its seven table generators on the
Nix-provided arm64 macOS compiler while fixing the runtime to Apple Clang and
the iPhoneOS SDK. The declared target closure is the sole dependency lookup
source. The target output comprises the public headers, static
`libfribidi.a`, and `fribidi.pc`; command-line tools, manuals, and shared
libraries stay outside the target output. All 18 archive members and the
enabled deprecated-interface contract are checked. A direct-only consumer
resolves Krita's existing `FindFriBidi.cmake` module and calls the same
bidi-type, bracket, and paragraph-embedding APIs as the bundled Raqm
implementation. The package and consumer rebuild deterministically. At the
next migration checkpoint, the resulting
18-package aggregate and its complete 19-path target closure were restored into
an empty local store solely from the binary cache.

All dependency packages in the current profile are represented by the Nix
aggregate. Krita itself and the deterministic unsigned IPA are separate final
derivations on top of that aggregate. Build rooted outputs with:

```sh
nix build .#librepaint-ios-app \
  --out-link build-ios/nix-results/librepaint-ios-app
nix build .#librepaint-ios-ipa \
  --out-link build-ios/nix-results/librepaint-ios-ipa
```

The artifacts are
`build-ios/nix-results/librepaint-ios-app/LibrePaint.app` and
`build-ios/nix-results/librepaint-ios-ipa/LibrePaint-iOS-unsigned.ipa`. The app
derivation builds the 50-target initial static-plugin profile, installs the
runtime resource tree into the bundle, and rejects the wrong architecture,
Apple platform, deployment target, SDK, bundle metadata, signing state, or a
temporary build/Xcode path leak. The IPA derivation normalizes timestamps and
entry order as well as portable bundle permissions. Both the reproducible IPA
and the incremental deployment path stage writable copies with directories at
`0755`, data files at `0644`, and the main executable at `0755`. Completed
archives reject symlinks, special files, extra metadata, non-Unix types or
modes, unsafe names, stage/archive inventory differences, and the DOS
read-only bit before they can reach an importer. These rules share a lightweight
success/failure regression check. The Nix Store application remains
immutable and unsigned.

Source filtering separates Nix expressions, generated outputs, port
documentation, and `TODO.md` from the Krita compilation input. Changes to those
files rebuild the affected recipe or IPA layer while reusing the Krita
compilation output; Krita/CMake source changes rebuild the app. The legacy
`build-ios/` builders remain available for device deployment during the
transition. The Nix app and IPA derivations produce the unsigned release
artifacts.

To force source-build validation:

```sh
nix build .#ios-dependencies --no-link --no-substitute
nix build .#kf6-consumer-check --no-link --no-substitute
```

## Start a development shell

In a normal terminal:

```sh
nix develop
packaging/ios/scripts/check-host.sh
```

The host check requires sandboxing, disabled fallback, the Xcode allowlist
entry, and Xcode exposure limited to declared impure host dependencies.

In a restricted environment where the user cache is not writable:

```sh
XDG_CACHE_HOME="$PWD/.cache/nix" nix develop
```

## Build the smoke application

The smoke application validates Objective-C++, UIKit, the selected SDK,
deployment target, bundle generation, and target platform metadata. It produces
an unsigned inspection bundle; device signing and installation belong to the
deployment workflow.

```sh
nix develop .#librepaint-ios --command packaging/ios/scripts/build-smoke.sh device
nix develop .#librepaint-ios --command packaging/ios/scripts/build-smoke.sh simulator
```

For normal device development, use the source-independent Nix environment and
its persistent Ninja tree. The first configuration needs one explicit
baseline build:

```sh
packaging/ios/scripts/build-librepaint-incremental.sh bootstrap
```

After that, inspect and execute only the affected Ninja steps:

```sh
packaging/ios/scripts/build-librepaint-incremental.sh plan
packaging/ios/scripts/build-librepaint-incremental.sh build
```

The exact output directory is printed by `build-librepaint-incremental.sh path`.
Normal builds refuse more than 200 planned steps, making an accidental broad
rebuild visible before compilation begins. The pure
`nix build .#librepaint-ios-ipa` path is the clean checkpoint/release gate. The
incremental helper handles source iteration. The iPadOS feature profile links
the required Krita plugins statically; Python/PyQt, PrintSupport,
process-launched FFmpeg features, and the updater remain outside this profile.

## Install the current build with AltStore

With AltServer running and AltStore installed on a connected iPad, one command
incrementally builds, validates, packages, installs, launches, and collects the
LibrePaint startup log:

```sh
packaging/ios/scripts/build-librepaint-incremental.sh deploy [device-id]
```

Bare `deploy-altstore.sh` delegates to the same guarded workflow. Its
`--skip-build` form is an internal handoff that requires the exact build
directory selected by the helper. See `docs/ios/altstore-deployment.md` for
prerequisites, validations, outputs, and the physical-device result.

## Build M2 dependencies

Pass a package name to build one dependency and its transitive prerequisites.
With only the platform argument, the script builds every dependency currently
present in the manifest:

```sh
nix develop .#librepaint-ios --command packaging/ios/scripts/build-dependencies.sh device harfbuzz
nix develop .#librepaint-ios --command packaging/ios/scripts/build-dependencies.sh device
```

Device and Simulator use separate source-independent prefixes. Every installed
static archive is checked member-by-member for architecture and Apple platform
metadata. A stale or host archive is rejected before its build stamp is written.

Link the completed core subset into a single unsigned iOS application:

```sh
nix develop .#librepaint-ios --command packaging/ios/scripts/probe-dependencies.sh device
```

The dependency graph and package-specific options are defined in
`packaging/ios/deps/dependencies.json`. See `docs/ios/validation-m2.md` for the
current validated subset and known limitations.

## Build Qt and Qt-dependent libraries

Qt is built statically from the locked Qt 6.11.1 sources. Build the core
dependencies first, then Qt, then rerun the dependency builder so it can add
Qt-dependent packages such as QuaZip:

```sh
nix develop .#librepaint-ios --command packaging/ios/scripts/build-dependencies.sh device
nix develop .#librepaint-ios --command packaging/ios/scripts/build-qt.sh device
nix develop .#librepaint-ios --command packaging/ios/scripts/build-dependencies.sh device
nix develop .#librepaint-ios --command packaging/ios/scripts/probe-qt.sh device
```

`build-qt.sh` fingerprints the locked source outputs, Xcode/SDK matrix, and
build recipe. A matching build is reused and all installed archives are still
revalidated. If an input changes, rebuild the isolated Qt target directory:

```sh
nix develop .#librepaint-ios --command packaging/ios/scripts/build-qt.sh device --clean
```

The Qt probe links Core, Gui, Widgets, Xml, Network, Svg, Concurrent, Sql,
OpenGL, OpenGLWidgets, Core5Compat, the iOS platform plugin, static support
plugins, and QuaZip into one unsigned iOS application. PrintSupport is
explicitly disabled and is rejected as a required Krita dependency.

## Build KDE Frameworks

Build the locked ECM/KF6 subset after the target dependencies and Qt. The
builder also creates the macOS `kconfig_compiler_kf6` needed while cross
compiling. The iOS output contains the target libraries, while
`kconfig_compiler_kf6` runs on the macOS host.

```sh
nix develop .#librepaint-ios --command packaging/ios/scripts/build-frameworks.sh device
nix develop .#librepaint-ios --command packaging/ios/scripts/probe-frameworks.sh device
```

The framework probe runs the host KConfig generator and links Config,
WidgetsAddons, Codecs, Completion, CoreAddons, GuiAddons, I18n, ItemViews, and
ColorScheme with Qt into one unsigned iOS application. Framework sources,
build options, patches, and dependencies are declared in
`packaging/ios/frameworks/frameworks.json`.

## Build output and logs

- Device and Simulator output: `build-ios/`
- Timestamped command logs: `logs/ios/`
- Plugin inventory: `packaging/ios/manifests/plugins.json`
- Dependency inventory: `packaging/ios/manifests/dependencies.json`

Regenerate the plugin inventory after adding or removing plugin targets:

```sh
python3 packaging/ios/scripts/inventory-plugins.py
```

## Local cache

The local Nix store is the first-level build cache. A GC-independent local Nix
binary cache can be populated with:

```sh
packaging/ios/scripts/publish-nix-cache.sh .#zlib-ios
```

Restore a local cache object after Nix GC:

```sh
packaging/ios/scripts/restore-nix-cache.sh .#zlib-ios
```

Dependency pinning, clean bootstrap, and normal deployment use separate cache
policies. While recipes are being pinned, do not run deployment maintenance or
protect the legacy dev-shell, host-tool, and build-closure roots. Finish and
commit every dependency recipe first. Then run the destructive clean bootstrap
once from a clean repository root:

```sh
packaging/ios/scripts/bootstrap-ios-dependencies.sh --confirm-pinning-complete
```

The bootstrap checks the committed Git flake without building, releases only
the known repository-local legacy GC-root symlinks, runs a full Nix GC, and
builds `.#ios-dependencies` without an intermediate out-link. It roots only the
successfully realised final aggregate. This is intentionally deferred until all
dependency recipes are fixed.

The clean bootstrap passed on 2026-08-03 from commit `e8ba4dc`. It removed
1,808 unrooted store paths (4,627.38 MiB), rebuilt the then-current 31 target
dependencies from the normal committed Git flake, passed the complete KF6 iOS
consumer link, and then created only
`build-ios/nix-roots/ios-dependencies`.

After the clean bootstrap, incremental device deployment passes its exact
CMake/Ninja tree to `maintain-build-cache.sh`. Deployment maintenance extracts
and roots the store inputs already recorded by that graph. This uses the active
build graph directly and keeps edit-and-deploy cycles on one Krita source
snapshot. The exact incremental tree is mandatory. Maintenance runs GC only
below the free-space threshold (or when explicitly forced) and
only after the current graph closure has been protected.

The default repository is the ignored
`build-ios/nix-binary-cache`. Set `KRITA_IOS_NIX_CACHE_URI` to a private,
writable Nix store URI supported by `nix copy`. A non-file destination also
requires `KRITA_IOS_NIX_CACHE_SIGNING_KEY`; keep that private key outside the
repository. Services with a dedicated push protocol use their native client.

Inspect a closure with:

```sh
nix path-info --recursive .#devShells.aarch64-darwin.default
```

For multiple Macs, configure the private cache as a substituter with its trusted
public key in the normal Nix configuration. Do not publish these SDK-derived
artifacts to a public cache, and never cache Apple signing material. The exact
Xcode build, SDK build, and Apple Clang build are derivation inputs, so a
different validated toolchain selects a different cache key.

## Existing platform baseline

- Android has a maintained packaging tree and extensive `Q_OS_ANDROID`
  adaptations. `README.android.md` documents the local configure and APK path;
  its Android-compatible dependency prefix is prepared separately.
- macOS is the closest Apple compilation baseline. iOS uses dedicated
  packaging, RPATH, icon, process, and filesystem policies.
- Development on this host uses the Nix shell, which supplies CMake and Ninja.
- M0 acceptance uses the iOS probes. Full macOS and Android builds use their
  respective prebuilt dependency environments.
