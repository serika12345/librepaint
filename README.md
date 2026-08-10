# LibrePaint

[日本語版](README.ja.md)

LibrePaint is an independently maintained, cross-platform digital painting application derived from Krita. Its goal is to evolve the codebase as a distinct product—including shared application behavior, UI, workflows, branding, packaging, and platform integrations—and make LibrePaint available across desktop and mobile platforms.

iPadOS is currently the most thoroughly validated target. Reproducible arm64 device builds and deployment, Apple Pencil and touch input, Files integration, and core drawing workflows have been exercised on physical hardware. Other targets have source and packaging paths at the stages described in the platform table below.

> [!WARNING]
> LibrePaint remains under development, and maturity and validation coverage vary by platform. The tables below record the exact paths exercised so far. Keep separate backups of important artwork.

## Project Direction

| Item | Policy |
|---|---|
| Product scope | Develop and maintain LibrePaint as a complete application with shared and platform-specific components |
| Platform goal | Windows, macOS, Linux, Android (including Android on ChromeOS), iPadOS, and additional targets that the Krita/Qt codebase can support |
| Shared development | Implement features and UX in shared code when appropriate, with focused platform integrations where required |
| Compatibility | Preserve artwork, resources, and stable technical identifiers deliberately; make incompatible migrations explicit |
| Distribution goal | Establish a build, package, validation, and delivery path appropriate to each platform |

The platform status below separates roadmap coverage from verified availability. Maturity, feature coverage, and delivery paths currently vary by platform.

## Platform Status

| Platform | Current repository state | Current validation |
|---|---|---|
| iPadOS | Pinned Nix/Xcode environment, unsigned IPA generation, and AltStore/LiveContainer deployment paths | Most actively developed target; detailed physical-device verification on arm64 iPads running iPadOS 17 or later |
| Android / ChromeOS | LibrePaint APK configuration and a [local build guide](README.android.md) | Next gate: prepare the dependency prefix and complete end-to-end device validation |
| Linux | Nix dependency and completed-build recipes, plus [local AppImage scripts and guide](packaging/linux/appimage/README.md) | Next gates: validate runtime behavior, publishing, and signing |
| macOS | Nix recipe for the LibrePaint app bundle, plus the existing DMG packaging path | Clean arm64 build and application startup verified with the nixpkgs LLVM toolchain and SDK; interactive UI and distribution validation follow |
| Windows | LibrePaint executable and NSIS installer packaging paths | Next gates: clean build, installer validation, signing, and end-to-end validation |

Current iOS support covers iPad. Support status for additional Apple form factors and platforms will follow their UI, build, packaging, and device validation work.

Compatibility contracts retain the established CMake targets, configuration directories, KRA MIME/UTI identifiers, plugin IDs, and action IDs.

## Current iPadOS Status

The following reflects the iPadOS workstream as of August 9, 2026. [`TODO.md`](TODO.md) is the authoritative source for iPadOS progress and physical-device verification results, while [`packaging/ios/manifests/initial-plugin-profile.json`](packaging/ios/manifests/initial-plugin-profile.json) is the authoritative source for statically included iPadOS functionality.

### Verified on a Physical Device

| Area | Verified coverage |
|---|---|
| Launch and basic UI | Installation and launch through AltStore, fresh IPA import and launch through LiveContainer, the main window, portrait and landscape splash layouts, and the Configure LibrePaint dialog in the initial portrait orientation and after rotation |
| Touch UI | Swipes handled as scrolling and taps handled as selection outside the canvas, tap confirmation in combo boxes, and text-entry focus that begins with explicit editing |
| Apple Pencil | Press, move, and release events; pressure and tilt; and immediate drawing with the initially selected brush |
| Pencil double tap | Switching between independent pen and eraser brush presets through `eraser_preset_action` |
| Files | Native open and save through iPadOS Files, followed by saving, reloading, retrieving, and externally inspecting KRA, PNG, JPEG, and ORA files |
| Brush engines | Pixel Brush, MyPaint preset registration, and drawing with Color Smudge, Spray, Hatching, and Filter Brush (Invert) |
| Tools and dockers | Major tools displayed in the Toolbox, major dockers listed in the menu, and basic use of features including the Layer Docker |
| Filters and generators | Registration of 33 filters and the six legacy generators, layer creation, and KRA open/save/reopen |
| Canvas display | High-DPI rendering at DPR 2 and canvas rendering through OpenGL ES 3.0 |
| App lifecycle | Returning from the background with the same process, restoring the full canvas, resuming Pencil drawing, and creating a recovery checkpoint for one modified KRA document |

“Verified” means that the described interaction path was successfully exercised on a specific physical device. Coverage is limited to the recorded settings, documents, devices, and test duration.

### File Formats

| Status | Formats and verified coverage |
|---|---|
| Basic round trip verified | KRA, PNG, JPEG, ORA |
| Limited physical-device verification | WebP saving; PSD, GIF, HEIF, and JPEG XL saving and reloading; TIFF saving and reloading with JPEG compression |
| Import verified | PDF; an NEF produced by a Nikon Z7, loaded as 8288×5520 16-bit RGBA |
| Bundled; next validation set | CSV, SVG, XCF, QML, TGA, Heightmap, brush resources, Spriter, KRZ, RGBE, OpenEXR, JPEG 2000, Exif/IPTC/XMP, and others |

The individual results in this table—including PDF and the tested RAW sample—cover the recorded samples and paths. Additional variants, compression methods, color spaces, metadata, and KRA filter configurations form the next test set.

### Included Features Under Interaction Testing

The current iPad profile statically registers 162 internal plugins. Final arm64 linking, IPA inspection, physical-device installation, and startup have been verified. UI and interaction testing continues in these areas:

- The complete SeExpr generator and Fill Layer workflow
- Displaying the LUT Docker and applying OpenColorIO LUTs
- Bundle import and export through the Resource Manager
- Detailed operation of the Colorize Tool and individual tools and dockers
- OpenEXR round trips, JPEG 2000 import, and implemented import/export paths for other additional formats
- The Pencil double-tap options for “previous preset,” “palette,” and “do nothing,” along with the settings UI for arbitrary actions
- Complete interaction and regression testing for the iPad canvas-only touch UI, Brush Library, and Layer HUD

### Next iPadOS Validation Work

- Complete separation of Pencil drawing from finger gestures, plus systematic regression testing of undo/redo, pan, zoom, and rotation gestures
- Safe Area behavior, Split View, Stage Manager, external displays, and compact-window geometry
- Repeated background/foreground transitions, rotation and document-close boundaries while suspended, and recovery under failure or expiration paths
- Pencil hover and external keyboards
- Cold and warm launches from Files, recent documents, iCloud Drive, and autosave recovery after forced termination
- Severe memory pressure, Jetsam, 2K/4K/8K canvas limits, one-hour continuous drawing sessions, and thermal and battery testing
- Small touch targets and overlap between modal dialogs and the software keyboard

The iOS memory policy sets the default tile-memory budget to 25% of physical RAM, capped at 1 GiB, and limits manual configuration to 37.5% of physical RAM, capped at 1.5 GiB. Purging tile and pixmap caches in response to memory warnings is implemented. The next physical-device gate covers recovery under severe memory pressure and preservation of unsaved data.

### Current iPadOS Profile

The current iPadOS workstream covers an iPad touch UI and local delivery through AltStore or LiveContainer. iPhone UI adaptation, App Store delivery, and production signing require separately scoped platform work.

The self-contained build concentrates on drawing, bundled resources, and local file workflows. The following integrations sit outside the current iPadOS profile:

- Python/PyQt and G'MIC
- Qt PrintSupport and printing
- Video and audio import/export through FFmpeg, MLT, or SDL
- The updater, bug-reporting features, and auxiliary functionality that use external processes
- SVG Text Tool/Text Properties, Storyboard, and Small Color Selector
- Video export from Recorder and animation export from Composition

Animation UI is a possible low-priority iPadOS addition; multimedia export sits outside the current profile. Feature matrices for other LibrePaint platform builds will be maintained separately.

## Build and Development

The standard macOS and Linux Nix builds are defined in [`nix/macos/`](nix/macos/) and [`nix/linux/`](nix/linux/). Android build instructions are in [`README.android.md`](README.android.md), and the local Linux AppImage path is documented in [`packaging/linux/appimage/README.md`](packaging/linux/appimage/README.md). Platform packaging is kept under [`packaging/`](packaging/).

### macOS Nix Build

The macOS package is the default flake output on Apple Silicon. Build the named output from the repository root:

```sh
nix build .#librepaint-macos
```

The application bundle is written to `result/bin/LibrePaint.app`. Launch it with:

```sh
open result/bin/LibrePaint.app
```

Open the matching development shell with:

```sh
nix develop .#librepaint-macos
```

The clean build has been verified with this locked toolchain:

| Component | Verified value |
|---|---|
| Host | Apple Silicon macOS (`aarch64-darwin`) |
| Compiler | LLVM Clang 21.1.8 from nixpkgs |
| Linker and archive tools | cctools/ld64 from nixpkgs |
| SDK | Apple SDK 14.4 from the Nix store |
| Qt | 6.11.1 |
| KDE Frameworks / ECM | 6.28.0 |
| Deployment target | macOS 14.0 |
| Architecture | arm64 |

The locked Nix graph supplies the declared build toolchain and dependency set. Darwin tooling comes from the nixpkgs LLVM Clang, cctools, SDK, and open-source `xcbuild` packages.

The native C++ desktop profile includes the drawing application, its dynamically loaded plugins, PDF import through Poppler, RAW import through LibRaw/KDcraw, KSeExpr generators, OpenColorIO, MLT/SDL audio-video support, FFmpeg/FFprobe, and the image-format libraries declared in [`nix/macos/krita.nix`](nix/macos/krita.nix).

The next macOS dependency work adds the Python/PyQt scripting closure together with its embedded-runtime path integration.

The Nix result is a reproducible development and checkpoint bundle whose runtime libraries remain in its Nix closure. A distribution recipe can layer standalone bundling, DMG generation, signing, and notarization onto this build.

### Linux Nix Build

The x86_64 Linux flake provides a source-independent dependency closure and the completed, wrapped LibrePaint build. Build the dependency closure first to populate the local or configured binary cache without making it sensitive to LibrePaint source changes:

```sh
nix build .#linux-dependencies --no-link
```

Build the application with the same dependency recipe:

```sh
nix build .#librepaint-linux
```

The result exposes `result/bin/krita`, retaining Krita's compatible desktop entry point and identifiers while presenting LibrePaint branding. The completed build follows nixpkgs' Krita unwrapped/wrapper structure, including the G'MIC plugin and Qt/GLib runtime wrapper. Open the matching development shell with:

```sh
nix develop .#librepaint-linux
```

### iPadOS Build and Local Deployment

Run all commands from the repository root. For normal source development, use the incremental workflow, which reuses a pinned Nix environment and a persistent Ninja tree selected by fingerprint.

#### Pinned Toolchain

The exact pinned values are defined in [`packaging/ios/versions.env`](packaging/ios/versions.env).

| Component | Pinned value |
|---|---|
| Krita base revision | `7173825999953623d28777a163a65b42a3f26f0a` |
| Host | Apple Silicon macOS (`aarch64-darwin`) |
| Nix | 2.31 or later |
| Xcode | 26.6 (`17F113`) |
| iPhoneOS SDK | 26.5 (`23F81a`) |
| Apple Clang | 21.0.0 (`2100.1.1.101`) |
| Qt | 6.11.1 |
| KDE Frameworks / ECM | 6.28.0 |
| Deployment target | iPadOS 17.0 |
| Architecture | arm64 |

`iPhoneOS SDK` is Apple's SDK name; the current bundle targets iPad. Keep the pinned values during normal development and treat version upgrades as separate validation work.

#### Prerequisites

- An Apple Silicon Mac with the Xcode version listed above installed at `/Applications/Xcode.app`
- Nix 2.31 or later, with a Nix daemon configured to support Flakes
- For automated AltStore deployment: an iPad running iPadOS 17 or later, connected over USB, unlocked, trusted by the Mac, and in Developer Mode
- For automated AltStore deployment: AltServer running on the Mac, AltStore configured on the iPad, the required local development-signing environment, and local-network connectivity between the Mac and iPad
- For LiveContainer installation: LiveContainer installed and configured on the iPad; the verified iOS 26 configuration uses its JIT-Less mode

Enable sandboxing in the Nix daemon, disable sandbox fallback, and allow only Xcode as an explicit impure host dependency. The following nix-darwin configuration has been verified:

```nix
nix.settings.sandbox = true;
nix.settings.sandbox-fallback = false;
nix.settings.extra-allowed-impure-host-deps = [
  "/Applications/Xcode.app"
];
```

Keep Xcode out of `sandbox-paths`. Check the environment with:

```sh
nix develop --command packaging/ios/scripts/check-host.sh
```

This check validates the versions of Xcode, the SDK, Clang, Nix, CMake, and related tools, as well as the Nix daemon's sandbox policy.

#### First Incremental Build

For a new build configuration, create the baseline once:

```sh
packaging/ios/scripts/build-librepaint-incremental.sh path
packaging/ios/scripts/build-librepaint-incremental.sh bootstrap
```

The wrapper creates and reuses a pinned, source-independent Nix profile. The initial baseline is a full build and may take some time.

To validate only the dependency closure and the KF6 consumer link first, run:

```sh
nix build .#ios-dependencies --no-link
nix build .#kf6-consumer-check --no-link
```

#### Normal Development Builds

After making changes, inspect the work planned by Ninja, then run the incremental build:

```sh
packaging/ios/scripts/build-librepaint-incremental.sh plan
packaging/ios/scripts/build-librepaint-incremental.sh build
```

`path` prints the currently selected build tree. By default, normal `build` and `deploy` operations reject plans larger than 200 Ninja steps so that an unintended full rebuild is caught before compilation starts. After an intentional broad configuration change, review the plan and use `bootstrap` to create a new baseline.

The normal edit-build-test loop uses this wrapper. Direct `cmake --preset` and `nix build .#librepaint-ios-ipa` invocations are reserved for configuration work and clean checkpoints, respectively. The former `build-krita-incremental.sh` and `krita-ios-*` entry points remain compatibility aliases.

#### Reproducible App and Unsigned IPA

The app bundle and IPA used for clean checkpoints can be built with Nix:

```sh
nix build .#librepaint-ios-app \
  --out-link build-ios/nix-results/librepaint-ios-app
nix build .#librepaint-ios-ipa \
  --out-link build-ios/nix-results/librepaint-ios-ipa
```

The resulting artifacts are placed at:

- `build-ios/nix-results/librepaint-ios-app/LibrePaint.app`
- `build-ios/nix-results/librepaint-ios-ipa/LibrePaint-iPad-unsigned.ipa`

Building `librepaint-ios-ipa` also builds the required app and dependencies automatically. The generated IPA is unsigned. Keep signing information, provisioning profiles, Apple IDs, and device credentials outside the repository.

#### Deploying to a Physical Device with AltStore

After satisfying the prerequisites and starting AltServer, run the following command to perform the incremental build, validate the binary, plugins, and runtime data, generate the IPA, sign and install it through AltStore, launch LibrePaint, and collect the startup log:

```sh
packaging/ios/scripts/build-librepaint-incremental.sh deploy [device-id]
```

If `device-id` is omitted, the first available CoreDevice is selected. List connected devices with:

```sh
xcrun devicectl list devices
```

Timestamped IPAs and collected `librepaint.log` files are stored under `build-ios/deploy/`. The workflow uses `packaging/ios/scripts/deploy-altstore.sh --skip-build` internally and supplies the exact current build tree.

This workflow provides development signing for the author's local use.

#### Installing with LiveContainer

The reproducible unsigned IPA at `build-ios/nix-results/librepaint-ios-ipa/LibrePaint-iPad-unsigned.ipa` can also be imported into LiveContainer. The packaging workflow normalizes the archive permissions required for LiveContainer to patch, launch, and clean up the app bundle. A fresh import and launch have been verified on a physical iPad using LiveContainer's iOS 26 JIT-Less mode.

An earlier failed import can leave a read-only temporary `Payload` inside LiveContainer. If that stale-state error occurs, preserve any required app data, clean up or reset the affected LiveContainer state, and import the corrected IPA. Physical-device verification of the exact cleanup UI is the next recovery step; see [`docs/ios/altstore-deployment.md`](docs/ios/altstore-deployment.md) for the current archive-permission and recovery notes.

#### Simulator Smoke Test

```sh
nix develop --command packaging/ios/scripts/build-smoke.sh simulator
```

This smoke test diagnoses the Objective-C++, UIKit, SDK, deployment-target, and bundle-metadata integration. Physical-device testing remains the runtime acceptance path.

#### Maintainer Note for Dependency Recipe Changes

`packaging/ios/scripts/bootstrap-ios-dependencies.sh --confirm-pinning-complete` is the dependency-recipe finalization procedure. Its execution boundary is after every dependency recipe has been pinned and committed, when existing unrooted cached outputs can be discarded. It releases known legacy GC roots, performs a **full Nix garbage collection**, and then rebuilds the final aggregate. Normal source development uses the incremental workflow above.

## Documentation and Outputs

| Path | Description |
|---|---|
| [`TODO.md`](TODO.md) | Source of truth for the iPadOS milestones, remaining work, and physical-device validation results |
| [`README.android.md`](README.android.md) | Current local Android build path and its dependency-prefix limitations |
| [`packaging/linux/appimage/README.md`](packaging/linux/appimage/README.md) | Current local Linux AppImage build path and prerequisites |
| [`docs/ios/README.md`](docs/ios/README.md) | Detailed toolchain, dependency-build, and cache design documentation |
| [`docs/ios/altstore-deployment.md`](docs/ios/altstore-deployment.md) | Details of AltStore deployment, IPA permissions, and LiveContainer import caveats |
| [`packaging/ios/versions.env`](packaging/ios/versions.env) | Pinned versions and deployment target |
| [`packaging/ios/manifests/initial-plugin-profile.json`](packaging/ios/manifests/initial-plugin-profile.json) | Static plugin profile for iPad |
| `build-ios/` | App and IPA artifacts, incremental build trees, and Nix profiles |
| `logs/ios/` | Timestamped build logs |

Keep local artifacts such as `build-ios/`, signed build products, credentials, and private cache keys out of Git.

## License and Upstream

LibrePaint is a derivative work based on [Krita](https://krita.org/) and is distributed under the GNU General Public License Version 3. Individual files and bundled components carry their respective compatible licenses. See [`COPYING`](COPYING) and the license notices in individual files for the exact terms.

Krita is developed by the Krita Foundation, KDE, and Krita contributors. LibrePaint is maintained independently by LibrePaint contributors. See the upstream [graphics/krita](https://invent.kde.org/graphics/krita) repository for the original project and its history.
