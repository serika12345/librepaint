# LibrePaint iPadOS Port

[日本語版](README.ja.md)

LibrePaint is an unofficial, experimental iPadOS paint app for physical arm64 iPads, currently built from Krita-derived code. The goal is to provide a practical, locally installed drawing environment with functionality comparable to the Android build, using Apple Pencil, touch input, and the iPadOS Files app.

> [!WARNING]
> LibrePaint is not an official iPad version from the Krita Foundation or KDE. It remains under development and does not guarantee data integrity, long-term stability, or compatibility with every file format and upstream Krita feature. Keep separate backups of important artwork.

## Scope

| Item | Policy |
|---|---|
| Target devices | Physical arm64 iPads |
| Minimum OS | iPadOS 17.0 |
| Input | Apple Pencil and touch |
| Acceptance target | Physical hardware; Simulator is used only to diagnose toolchain and bundle issues |
| Installation | Local sideloading through AltStore or LiveContainer |
| Primary goal | Core drawing functionality comparable to the Android build, internal plugins, and Files integration |
| Excluded devices | **iPhone is not supported** |
| Excluded distribution | App Store, official alternative app marketplaces, notarization, and public distribution |

Python/PyQt, G'MIC, printing, video and audio features, automatic updates, features that launch external processes, and external third-party plugins are also currently out of scope. Selected internal Krita plugins that provide brushes, tools, dockers, color management, and image I/O are statically linked and registered.

## Project Direction and Long-Term Vision

LibrePaint is intended primarily to give its author access to Krita-equivalent functionality on an iPad as part of a personal creative environment. Although the differences from upstream Krita are currently kept as small as practical, the project does not guarantee continued alignment with upstream or easy integration of future upstream changes. To prioritize the author's creative workflow and usability on iPad, the UI, features, and internal architecture may eventually diverge substantially from upstream Krita.

The long-term vision is to transition to an independent implementation that does not depend on GPL-licensed code derived from Krita, ultimately establishing a **standalone project released under the MIT License**. This is a future goal, not a declaration that the current Krita-derived code is being relicensed under the MIT License. See [License and upstream](#license-and-upstream) for the licenses that currently apply to this repository.

## Current Support Status

The following reflects the project status as of August 6, 2026. [`TODO.md`](TODO.md) is the authoritative source for progress and physical-device verification results, while [`packaging/ios/manifests/initial-plugin-profile.json`](packaging/ios/manifests/initial-plugin-profile.json) is the authoritative source for statically included functionality.

### Verified on a Physical Device

| Area | Verified coverage |
|---|---|
| Launch and basic UI | Installation and launch through AltStore, fresh IPA import and launch through LiveContainer, the main window, portrait and landscape splash layouts, and the Configure LibrePaint dialog in the initial portrait orientation and after rotation |
| Touch UI | Swiping and kinetic scrolling outside the canvas, suppression of accidental selections while scrolling, tap confirmation in combo boxes, and suppression of the unwanted software keyboard in settings and brush-selection views |
| Apple Pencil | Press, move, and release events; pressure and tilt; and drawing with the initially selected brush without first reselecting it |
| Pencil double tap | Switching between independent pen and eraser brush presets through `eraser_preset_action` |
| Files | Native open and save through iPadOS Files, followed by saving, reloading, retrieving, and externally inspecting KRA, PNG, JPEG, and ORA files |
| Brush engines | Pixel Brush, MyPaint preset registration, and drawing with Color Smudge, Spray, Hatching, and Filter Brush (Invert) |
| Tools and dockers | Major tools displayed in the Toolbox, major dockers listed in the menu, and basic use of features including the Layer Docker |
| Filters and generators | Registration of 33 filters and the six legacy generators, layer creation, and KRA open/save/reopen |
| Canvas display | High-DPI rendering at DPR 2 and canvas rendering through OpenGL ES 3.0 |

“Verified” means that the described interaction path was successfully exercised on a specific physical device. It is not a compatibility guarantee covering every setting, document, device, or extended period of use.

### File Formats

| Status | Formats and verified coverage |
|---|---|
| Basic round trip verified | KRA, PNG, JPEG, ORA |
| Limited physical-device verification | WebP saving; PSD, GIF, HEIF, and JPEG XL saving and reloading; TIFF saving and reloading with JPEG compression |
| Import verified | PDF; an NEF produced by a Nikon Z7, loaded as 8288×5520 16-bit RGBA |
| Bundled; further verification pending | CSV, SVG, XCF, QML, TGA, Heightmap, brush resources, Spriter, KRZ, RGBE, OpenEXR, JPEG 2000, Exif/IPTC/XMP, and others |

The individual results in this table—including PDF and the tested RAW sample—do not guarantee full compatibility with each format. Untested variants, compression methods, color spaces, metadata, and KRA filter configurations remain.

### Included, with Interaction Testing Still in Progress

The current iPad profile statically registers 161 internal Krita plugins. Final arm64 linking, IPA inspection, physical-device installation, and startup have been verified, but not every UI path and interaction for every plugin has been tested. The main areas still under verification are:

- The complete SeExpr generator and Fill Layer workflow
- Displaying the LUT Docker and applying OpenColorIO LUTs
- Bundle import and export through the Resource Manager
- Detailed operation of the Colorize Tool and individual tools and dockers
- OpenEXR round trips, JPEG 2000 import, and implemented import/export paths for other additional formats
- The Pencil double-tap options for “previous preset,” “palette,” and “do nothing,” along with the settings UI for arbitrary actions

### Major Incomplete or Unverified Areas

- Complete separation of Pencil drawing from finger gestures, plus systematic regression testing of undo/redo, pan, zoom, and rotation gestures
- Safe Area behavior, Split View, Stage Manager, external displays, and compact-window geometry
- Destruction and restoration of OpenGL surfaces and resources across background and foreground transitions
- Pencil hover and external keyboards
- Cold and warm launches from Files, recent documents, iCloud Drive, and autosave recovery after forced termination
- Severe memory pressure, Jetsam, 2K/4K/8K canvas limits, one-hour continuous drawing sessions, and thermal and battery testing
- Small touch targets and overlap between modal dialogs and the software keyboard

The iOS memory policy sets the default tile-memory budget to 25% of physical RAM, capped at 1 GiB, and limits manual configuration to 37.5% of physical RAM, capped at 1.5 GiB. Purging tile and pixmap caches in response to memory warnings is also implemented, but recovery under severe memory pressure and preservation of unsaved data still require physical-device verification.

### Explicitly Excluded Features

- iPhone support
- App Store distribution, notarization, and production signing
- Python/PyQt, G'MIC, Qt PrintSupport, and printing
- Video and audio import/export requiring FFmpeg, MLT, or SDL
- The updater, bug-reporting features, and auxiliary functionality that depend on external processes
- SVG Text Tool/Text Properties, Storyboard, and Small Color Selector
- Video export from Recorder and animation export from Composition

The animation UI itself remains a possible low-priority future addition, but video and audio export are out of scope.

## Build

Run all commands from the repository root. For normal source development, use the incremental workflow, which reuses a pinned Nix environment and a persistent Ninja tree selected by fingerprint.

### Pinned Toolchain

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

`iPhoneOS SDK` is Apple's name for the SDK. It does not mean that iPhone is a supported target. Do not silently override the pinned versions during normal development. Treat version upgrades as separate validation work.

### Prerequisites

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

Do not add Xcode to `sandbox-paths`. Check the environment with:

```sh
nix develop --command packaging/ios/scripts/check-host.sh
```

This check validates the versions of Xcode, the SDK, Clang, Nix, CMake, and related tools, as well as the Nix daemon's sandbox policy.

### First Incremental Build

For a new build configuration, create the baseline once:

```sh
packaging/ios/scripts/build-krita-incremental.sh path
packaging/ios/scripts/build-krita-incremental.sh bootstrap
```

The wrapper creates and reuses a pinned, source-independent Nix profile, so you do not need to enter `nix develop` first. The initial baseline is a full build and may take some time.

To validate only the dependency closure and the KF6 consumer link first, run:

```sh
nix build .#ios-dependencies --no-link
nix build .#kf6-consumer-check --no-link
```

### Normal Development Builds

After making changes, inspect the work planned by Ninja, then run the incremental build:

```sh
packaging/ios/scripts/build-krita-incremental.sh plan
packaging/ios/scripts/build-krita-incremental.sh build
```

`path` prints the currently selected build tree. By default, normal `build` and `deploy` operations reject plans larger than 200 Ninja steps so that an unintended full rebuild is caught before compilation starts. After an intentional broad configuration change, review the plan and use `bootstrap` to create a new baseline.

For the normal edit-build-test loop, use this wrapper instead of invoking `cmake --preset` directly or running `nix build .#krita-ios-ipa` after every edit.

### Reproducible App and Unsigned IPA

The app bundle and IPA used for clean checkpoints can be built with Nix:

```sh
nix build .#krita-ios-app \
  --out-link build-ios/nix-results/krita-ios-app
nix build .#krita-ios-ipa \
  --out-link build-ios/nix-results/krita-ios-ipa
```

The resulting artifacts are placed at:

- `build-ios/nix-results/krita-ios-app/krita.app`
- `build-ios/nix-results/krita-ios-ipa/LibrePaint-iPad-unsigned.ipa`

If you need only the IPA, building `krita-ios-ipa` also builds the required app and dependencies automatically. The generated IPA is intentionally unsigned. Never store signing information, provisioning profiles, Apple IDs, or device credentials in the repository.

### Deploying to a Physical Device with AltStore

After satisfying the prerequisites and starting AltServer, run the following command to perform the incremental build, validate the binary, plugins, and runtime data, generate the IPA, sign and install it through AltStore, launch LibrePaint, and collect the startup log:

```sh
packaging/ios/scripts/build-krita-incremental.sh deploy [device-id]
```

If `device-id` is omitted, the first available CoreDevice is selected. List connected devices with:

```sh
xcrun devicectl list devices
```

Timestamped IPAs and collected `krita.log` files are stored under `build-ios/deploy/`. `packaging/ios/scripts/deploy-altstore.sh --skip-build` is reserved for the workflow's internal handoff; do not use it to select an old build tree manually.

This process uses development signing for the author's local use. It is not an App Store submission or general-distribution signing pipeline.

### Installing with LiveContainer

The reproducible unsigned IPA at `build-ios/nix-results/krita-ios-ipa/LibrePaint-iPad-unsigned.ipa` can also be imported into LiveContainer. The packaging workflow normalizes the archive permissions required for LiveContainer to patch, launch, and clean up the app bundle. A fresh import and launch have been verified on a physical iPad using LiveContainer's iOS 26 JIT-Less mode.

A corrected IPA cannot remove a read-only temporary `Payload` left inside LiveContainer by an earlier failed import. If that stale-state error occurs, clean up or reset the affected LiveContainer state while preserving any required app data before importing again. The exact cleanup UI remains pending physical-device verification; see [`docs/ios/altstore-deployment.md`](docs/ios/altstore-deployment.md) for the current archive-permission and recovery notes.

### Simulator Smoke Test

```sh
nix develop --command packaging/ios/scripts/build-smoke.sh simulator
```

This smoke test diagnoses the Objective-C++, UIKit, SDK, deployment-target, and bundle-metadata integration. It does not sign or install the app and is not a substitute for testing on a physical device.

### Maintainer Note for Dependency Recipe Changes

`packaging/ios/scripts/bootstrap-ios-dependencies.sh --confirm-pinning-complete` is not a normal first-build command. It is a maintenance procedure to run only after every dependency recipe has been pinned and committed. It releases known legacy GC roots, performs a **full Nix garbage collection**, and then rebuilds the final aggregate. Do not run it during normal source development or when existing cached outputs must be preserved.

## Outputs and Related Documentation

| Path | Description |
|---|---|
| [`TODO.md`](TODO.md) | Source of truth for milestones, remaining work, and physical-device validation results |
| [`docs/ios/README.md`](docs/ios/README.md) | Detailed toolchain, dependency-build, and cache design documentation |
| [`docs/ios/altstore-deployment.md`](docs/ios/altstore-deployment.md) | Details of AltStore deployment, IPA permissions, and LiveContainer import caveats |
| [`packaging/ios/versions.env`](packaging/ios/versions.env) | Pinned versions and deployment target |
| [`packaging/ios/manifests/initial-plugin-profile.json`](packaging/ios/manifests/initial-plugin-profile.json) | Static plugin profile for iPad |
| `build-ios/` | App and IPA artifacts, incremental build trees, and Nix profiles |
| `logs/ios/` | Timestamped build logs |

Do not commit local artifacts such as `build-ios/`, signed build products, credentials, or private cache keys to Git.

## Development Principles

- Guard iOS-specific behavior with `Q_OS_IOS` unless there is a deliberate reason to change behavior on Android, desktop platforms, or macOS.
- Keep UIKit integration inside thin Objective-C++ bridges and reuse existing Krita actions and subsystems whenever possible.
- Distinguish between a feature being included in the plugin profile and being verified on a physical device.
- Do not consider changes involving touch, Apple Pencil, Files, rotation, lifecycle, or memory complete solely because they compile successfully.
- Update `TODO.md` whenever a milestone or physical-device validation status changes.
- Never commit certificates, provisioning profiles, Apple IDs, signing secrets, device credentials, or signed IPAs.

## License and Upstream

This repository is currently a derivative work based on [Krita](https://krita.org/) and is not licensed under the MIT License. Krita as a whole is licensed under the GNU General Public License Version 3, while individual files and bundled components are governed by their respective compatible licenses. See [`COPYING`](COPYING) and the license notices in individual files for the exact terms.

Krita is developed by the Krita Foundation, KDE, and Krita contributors. See the upstream [graphics/krita](https://invent.kde.org/graphics/krita) repository and the [Krita User Manual](https://docs.krita.org/) for usage documentation. Do not treat an issue specific to this port as an upstream Krita issue unless it has also been confirmed to reproduce upstream.

The future MIT-licensed project described here is intended to be an independent successor implementation that contains no GPL-licensed code derived from Krita. This intention does not alter the licensing terms of the current fork, its current build artifacts, or any Krita-derived code.
