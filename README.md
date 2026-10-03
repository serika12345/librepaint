# LibrePaint

[日本語版](README.ja.md)

LibrePaint is an independently maintained, cross-platform digital painting application derived from Krita. Its goal is to evolve the codebase as a distinct product—including shared application behavior, UI, workflows, branding, packaging, and platform integrations—and make LibrePaint available across desktop and mobile platforms.

iPadOS is currently the most thoroughly validated target. Reproducible arm64 device builds and deployment, Apple Pencil and touch input, Files integration, and core drawing workflows have been exercised on physical hardware. Other targets have source and packaging paths at the stages described in the platform table below.

> [!WARNING]
> LibrePaint remains under development, and maturity and validation coverage vary by platform. The tables below record the exact paths exercised so far. Keep separate backups of important artwork.

## Project Direction

| Item | Policy |
| --- | --- |
| Product scope | Develop and maintain LibrePaint as a complete application with shared and platform-specific components |
| Platform goal | Windows, macOS, Linux, Android (including Android on ChromeOS), iOS/iPadOS, and additional targets that the Krita/Qt codebase can support |
| Shared development | Implement features and UX in shared code when appropriate, with focused platform integrations where required |
| Compatibility | Preserve artwork, resources, and stable technical identifiers deliberately; make incompatible migrations explicit |
| Distribution goal | Establish a build, package, validation, and delivery path appropriate to each platform |

The platform status below separates roadmap coverage from verified availability. Maturity, feature coverage, and delivery paths currently vary by platform.

## Platform Status

| Platform | Current repository state | Current validation |
| --- | --- | --- |
| iOS / iPadOS | Pinned Nix/Xcode environment, universal unsigned IPA generation, and AltStore/LiveContainer deployment paths | IPA metadata and packaging cover arm64 iPhones and iPads running iOS/iPadOS 17 or later; detailed physical-device verification currently covers iPad |
| Android / ChromeOS | ARM64/x86_64 APK and AAB packages | Feature validation on target devices is ongoing |
| Linux | Nix packages and AppImage | Next gates: validate runtime behavior, publishing, and signing |
| macOS | Nix recipe for the LibrePaint app bundle, plus the existing DMG packaging path | Clean arm64 build and application startup verified with the nixpkgs LLVM toolchain and SDK; interactive UI and distribution validation follow |
| Windows | Portable 64-bit Windows directory and ZIP archive | Next gates: Windows runtime, installer, signing, and end-to-end validation |

The iOS application and IPA declare both iPhone and iPad support. The existing iPad-oriented UI is used unchanged on iPhone; iPhone UI adaptation is not part of the current installation path.

## Current iPadOS Status

The following records physical-device verification and feature coverage as of August 9, 2026.

### Verified on a Physical Device

| Area | Verified coverage |
| --- | --- |
| Launch and basic UI | Installation and launch through AltStore, fresh IPA import and launch through LiveContainer, the main window, portrait and landscape splash layouts, and the Configure LibrePaint dialog in the initial portrait orientation and after rotation |
| Touch UI | Swipes handled as scrolling and taps handled as selection outside the canvas, tap confirmation in combo boxes, and text-entry focus that begins with explicit editing |
| Apple Pencil | Press, move, and release events; pressure and tilt; and immediate drawing with the initially selected brush |
| Pencil double tap | Switching between independent pen and eraser brush presets |
| Files | Native open and save through iPadOS Files, followed by saving, reloading, retrieving, and externally inspecting KRA, PNG, JPEG, and ORA files |
| Brush engines | Pixel Brush, MyPaint preset registration, and drawing with Color Smudge, Spray, Hatching, and Filter Brush (Invert) |
| Tools and dockers | Major tools displayed in the Toolbox, major dockers listed in the menu, and basic use of features including the Layer Docker |
| Filters and generators | Registration of 33 filters and the six legacy generators, layer creation, and KRA open/save/reopen |
| Canvas display | High-DPI rendering at DPR 2 and canvas rendering through OpenGL ES 3.0 |
| App lifecycle | Returning from the background with the same process, restoring the full canvas, resuming Pencil drawing, and creating a recovery checkpoint for one modified KRA document |

“Verified” means that the described interaction path was successfully exercised on a specific physical device. Coverage is limited to the recorded settings, documents, devices, and test duration.

### File Formats

| Status | Formats and verified coverage |
| --- | --- |
| Basic round trip verified | KRA, PNG, JPEG, ORA |
| Limited physical-device verification | WebP saving; PSD, GIF, HEIF, and JPEG XL saving and reloading; TIFF saving and reloading with JPEG compression |
| Import verified | PDF; an NEF produced by a Nikon Z7, loaded as 8288×5520 16-bit RGBA |
| Bundled; next validation set | CSV, SVG, XCF, QML, TGA, Heightmap, brush resources, Spriter, KRZ, RGBE, OpenEXR, JPEG 2000, Exif/IPTC/XMP, and others |

The individual results in this table—including PDF and the tested RAW sample—cover the recorded samples and paths. Additional variants, compression methods, color spaces, metadata, and KRA filter configurations form the next test set.

### Current iPadOS Profile

The current iOS workstream uses the existing iPad touch UI unchanged and supports local delivery to iPhone and iPad through AltStore. iPhone UI adaptation, App Store delivery, and production signing require separately scoped platform work.

The self-contained build concentrates on drawing, bundled resources, and local file workflows. The following integrations sit outside the current iPadOS profile:

- Python/PyQt and G'MIC
- Qt PrintSupport and printing
- Video and audio import/export through FFmpeg, MLT, or SDL
- The updater, bug-reporting features, and auxiliary functionality that use external processes
- SVG Text Tool/Text Properties, Storyboard, and Small Color Selector
- Video export from Recorder and animation export from Composition

Animation UI is a possible low-priority iPadOS addition; multimedia export sits outside the current profile. Feature matrices for other LibrePaint platform builds will be maintained separately.

## Documentation

The [development manual (Japanese)](docs/architecture/DEVELOPMENT.md) covers environment setup, platform builds, testing, and maintenance.

## License and Upstream

LibrePaint is a derivative work based on [Krita](https://krita.org/) and is distributed under the GNU General Public License Version 3. Individual files and bundled components carry their respective compatible licenses. See [`COPYING`](COPYING) and the license notices in individual files for the exact terms.

Krita is developed by the Krita Foundation, KDE, and Krita contributors. LibrePaint is maintained independently by LibrePaint contributors. See the upstream [graphics/krita](https://invent.kde.org/graphics/krita) repository for the original project and its history.
