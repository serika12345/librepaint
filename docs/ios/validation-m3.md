# M3 validation record

Date: 2026-08-02

M3 has reached the unsigned-link gate. A reduced Krita application, including
the main window, core painting libraries, Qt resources, and the static Qt/KF6
runtime, configures and links for an arm64 iPadOS device.

## Historical build

At the M3 gate, after completing the M2 dependency, Qt, and framework builds,
the artifact below was produced with:

```sh
nix develop .#librepaint-ios --command packaging/ios/scripts/configure-krita.sh device --build
```

The wrapper validated the host matrix, resolved the pinned host Qt Linguist
tools, supplied only the isolated target prefixes, and used the Ninja generator.
Ninja handles the static Qt target object expressions that the Xcode
generator's cross-compiling compiler checks reject in this configuration.

## Artifact

`build-ios/krita/device-ninja/bin/krita.app` has these inspected properties:

- executable: Mach-O 64-bit arm64
- platform: IOS
- minimum OS: 17.0
- SDK: 26.5
- bundle identifier: `org.krita.ipad.port`
- device family: iPad only (`2`)
- supported orientations: all four iPad orientations
- signing: unsigned
- bundle size: approximately 76 MiB before plugins and production resources
- dynamic libraries: iOS SDK frameworks and system libraries only

The generated Info.plist passes `plutil -lint`. `otool -L` lists only iOS SDK
frameworks and system libraries; Krita, Qt, KF6, and open-source dependencies
are statically linked.

## Port boundaries applied

- `APPLE AND NOT IOS` conditions gate macOS packaging, RPATH, AppKit helpers,
  Finder integration, and Objective-C++ utility code.
- shared Krita libraries become static libraries for iOS.
- The target profile contains host Python build-time generators and leaves
  Python development libraries and Python/PyQt bindings outside the app.
- PrintSupport, updater code, external plugin trees, and QML modules remain
  outside the target profile.
- Animation editing and frame playback remain compiled. FFmpeg video import and
  animation rendering await an iOS execution backend because Qt deletes
  QProcess on iOS.
- One iOS compatibility header maps desktop OpenGL extension names to their
  OpenGL ES numeric equivalents.
- static Fontconfig/Expat and libintl/iconv dependencies are made explicit at
  final link time.

## Physical-device follow-up

The later M4/M5 profile has now been signed by AltStore and launched on a
physical iPad. Static initialization, Qt platform startup, SQL resource lookup,
LittleCMS initialization, packaged runtime data, and main-window presentation
all pass. The bare CMake output remains unsigned; signing and installation are
performed exclusively through the deployment staging path. See
`docs/ios/altstore-deployment.md`.
