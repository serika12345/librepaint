# M2 validation record

Date: 2026-08-02

M2 is complete for the selected initial feature set. The core dependency set,
Qt, QuaZip, ECM, and the required KDE Frameworks have been compiled and linked
into unsigned iOS/arm64 probe applications.

## Build boundary

The locked nixpkgs revision selects and fetches dependency source derivations
and provides host tools. Each target library is then compiled outside a Nix
derivation with Apple Clang and the validated Xcode SDK. The proprietary SDK
stays in Xcode, and generated outputs are checked for zero retained SDK absolute
paths. `flake.lock` pins the open-source inputs.

The standard nixpkgs `arm64-apple-ios` cross package set currently imports
Xcode 12.3 into the Nix store. This port uses the custom ADR 0001 boundary to
retain the pinned Xcode 26.6 and SDK 26.5 matrix.

## Validated target libraries

| Dependency | Version | Result |
|---|---:|---|
| zlib | 1.3.2 | static arm64/IOS archive |
| libpng | 1.6.58 | static arm64/IOS archive |
| libjpeg-turbo | 3.1.4.1 | two static arm64/IOS archives |
| Expat | 2.8.2 | static arm64/IOS archive |
| Boost | 1.89.0 | headers installed |
| Immer | 0.9.1 | headers installed |
| Zug | 0.1.2 | headers installed |
| Lager | 0.1.3 | headers installed |
| Eigen | 3.4.1 | headers installed |
| xsimd | 14.3.0 | headers installed |
| LCMS2 | 2.19.1 | static arm64/IOS archive |
| Exiv2 | 0.28.8 | static arm64/IOS archive |
| FreeType | 2.14.3 | static arm64/IOS archive |
| HarfBuzz | 13.2.1 | static arm64/IOS archive |
| Fontconfig | 2.18.2 | static arm64/IOS archive |
| FriBidi | 1.0.16 | static arm64/IOS archive |
| libunibreak | 7.0 | static arm64/IOS archive |
| GNU libintl | 1.0 | static arm64/IOS archive |
| Qt | 6.11.1 | 39 static arm64/IOS archives |
| QuaZip | 1.5 | static arm64/IOS archive |
| ECM | 6.28.0 | host-executed CMake modules installed in target prefix |
| KF6 Config | 6.28.0 | two static arm64/IOS archives |
| KF6 WidgetsAddons | 6.28.0 | static arm64/IOS archive |
| KF6 Codecs | 6.28.0 | static arm64/IOS archive |
| KF6 Completion | 6.28.0 | static arm64/IOS archive |
| KF6 CoreAddons | 6.28.0 | static arm64/IOS archive |
| KF6 GuiAddons | 6.28.0 | static arm64/IOS archive |
| KF6 I18n | 6.28.0 | static arm64/IOS archive |
| KF6 ItemViews | 6.28.0 | static arm64/IOS archive |
| KF6 ColorScheme | 6.28.0 | static arm64/IOS archive |

WebP and TIFF remain conditional P1 additions. OpenEXR, HEIF, JPEG XL, RAW,
and Poppler remain explicitly deferred P2 dependencies.

## Commands and results

```sh
nix develop --command packaging/ios/scripts/build-dependencies.sh device
nix develop --command packaging/ios/scripts/build-qt.sh device
nix develop --command packaging/ios/scripts/build-dependencies.sh device
nix develop --command packaging/ios/scripts/probe-dependencies.sh device
nix develop --command packaging/ios/scripts/probe-qt.sh device
nix develop --command packaging/ios/scripts/build-frameworks.sh device
nix develop --command packaging/ios/scripts/probe-frameworks.sh device
```

The dependency probe compiled and linked the non-Qt C/C++ target libraries into
one Mach-O application with these properties:

- architecture: arm64
- platform: IOS
- minimum OS: 17.0
- SDK: 26.5
- unresolved symbols: 0

The Qt/QuaZip probe additionally linked Qt Core, Gui, Widgets, Xml, Network,
Svg, Concurrent, Sql, OpenGL, OpenGLWidgets, Core5Compat, QuaZip, and the static
iOS platform and support plugins. Its executable has the same arm64/IOS/minimum
OS/SDK metadata. Qt's configuration summary reports `Qt PrintSupport ... no`;
the installed PrintSupport archive count is 0.

The framework probe used the macOS `kconfig_compiler_kf6` to generate target
C++ sources from a `.kcfg` file, then linked every selected KF6 target and its
static resources into one application. The resulting executable is arm64,
platform IOS, minimum iOS 17.0, SDK 26.5, with 0 unresolved symbols. A second
framework build with unchanged inputs reused every package after matching its
source/build/toolchain fingerprints.

A separate zlib/libpng Simulator build produced `platform IOSSIMULATOR` archives.
The archive inspector rejected a Simulator archive presented as a device
artifact, confirming platform enforcement.

## Portability fixes exercised

- CMake and pkg-config lookup are isolated from Homebrew, `/usr/local`, and host
  dependency paths.
- Generated HarfBuzz and Boost CMake packages resolve paths from their
  installation prefix.
- The combined probe supplies CoreText/CoreGraphics for HarfBuzz's static
  CoreText backend and the SDK's iconv library for Exiv2.
- Fontconfig 2.18.2 requires Meson 1.11 or newer, while the locked host Meson is
  1.10.2. The target uses its official Autotools build, limited to the library
  and generated data needed by the install.
- Build fingerprints include the source path, build recipe, platform matrix,
  toolchain file, and builder schema. Changed inputs invalidate their stamps.
- Qt is built from the locked Qt 6.11.1 source outputs with the Nix-provided
  host Qt tools. Its target prefix is isolated from the non-Qt dependency
  prefix, and QuaZip fingerprints the exact target Qt build input.
- Qt 6.11.1 emits iOS API deprecation warnings under the Xcode 26.5 SDK, chiefly
  around older permission APIs. Compilation and linking pass; runtime review
  remains an M5 item.
- The iOS KConfig target contains the libraries. A separate macOS
  `kconfig_compiler_kf6`, built from the identical locked source, is exposed
  through `KF6_HOST_TOOLING` for cross-build code generation.
- Host Qt Linguist tools generate `.qm` files while the generated translations
  and libraries are installed only into the target prefix.
- Target lookup roots contain the isolated dependency, Qt, and KF6 prefixes;
  pkg-config consumes target `.pc` files.
- Static GNU libintl requires the iOS SDK's iconv library at final link time.
  The framework probe records that requirement explicitly.

## M2 gate result

Technical gate G1 passes. Qt Widgets/OpenGL, ECM 6.28.0, the selected KF6 set,
host-side code generation, and all required target libraries can be linked as
one iOS arm64 application. M3 can proceed to the first reduced Krita configure,
compile, and link attempt.
