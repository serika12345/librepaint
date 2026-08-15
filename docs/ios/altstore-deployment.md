# AltStore physical-device deployment

Date: 2026-08-02

`packaging/ios/scripts/build-librepaint-incremental.sh deploy` automates the path
from the current source tree to a launched physical-device build. The unsigned
CMake product remains immutable; AltStore performs the signing operation, while
credentials remain in the local signing environment.

## Prerequisites

- The validated Xcode/iOS SDK and Nix environment are installed.
- Developer Mode is enabled on the iPad.
- AltServer is running on the Mac.
- AltStore is installed and configured on the iPad.
- The Mac and iPad can reach each other over the local network while deploying.

This workflow covers local development signing and sideloaded installation
through AltStore.

## One-command flow

```sh
packaging/ios/scripts/build-librepaint-incremental.sh deploy
```

The first available CoreDevice is selected. An explicit identifier can be
passed as the only positional argument:

```sh
packaging/ios/scripts/build-librepaint-incremental.sh deploy \
  216CE849-760C-5BFF-8835-CF7C6A1AD431
```

If the build is already current, the same command plans zero compilation steps
and proceeds directly to packaging and installation. Bare `deploy-altstore.sh`
is an equivalent compatibility entry point.
`deploy-altstore.sh --skip-build` is reserved for the helper's internal
handoff because it requires the exact `KRITA_IOS_BUILD_DIR`.

The first baseline for a new build configuration must be created once:

```sh
packaging/ios/scripts/build-librepaint-incremental.sh bootstrap
```

To generate only the reproducible unsigned IPA for selection in AltStore or a
compatible sideloading store, use:

```sh
nix build .#librepaint-ios-ipa \
  --out-link build-ios/nix-results/librepaint-ios-ipa
```

Select
`build-ios/nix-results/librepaint-ios-ipa/LibrePaint-iOS-unsigned.ipa`. The
archive is unsigned; AltStore supplies the development signature required by
iOS or iPadOS at installation time.

The script performs these operations in order:

1. Reuse the recorded source-independent Nix environment and build only the
   affected Ninja targets.
2. Reject a binary with the wrong architecture, Apple platform, or deployment
   target.
3. Compare every static archive's Qt resource initializers with the final
   executable.
4. Generate LibrePaint's declared install-time data tree and copy it into the
   staged app.
5. Compare every staged runtime file with the app and require bundles, brush
   presets, ICC profiles, and actions.
6. Normalize the private app stage, generate a timestamp-versioned IPA under
   `build-ios/deploy/`, and test its structure and ZIP permission metadata.
7. Open an AltStore install URL, wait for its download, and wait for the new
   bundle version on-device.
8. Launch LibrePaint and copy its startup log back to `build-ios/deploy/`.
9. Root the Nix store inputs directly from the active build graph, then run
   low-space maintenance if needed.

Optional environment variables are `KRITA_IOS_DEVICE`,
`KRITA_IOS_BUNDLE_VERSION`, `KRITA_IOS_DEPLOY_PORT`, and
`KRITA_IOS_LAUNCH_SETTLE_SECONDS`. The incremental rebuild guard defaults to
200 planned steps and can be adjusted with `KRITA_IOS_INCREMENTAL_MAX_STEPS`.

## Runtime data handling

On iOS, LibrePaint resolves its installation prefix from the application bundle.
The deployment stage includes the exact CMake-installed `share` tree inside the
signed app. Current validation covers 228 files, including one audited resource
bundle, 31 ICC profiles, and 37 action definitions. The action set includes
the core `krita.action` and `kritamenu.action` registries; packaging fails if
either file or a representative core menu action is missing.

The private packaging stage is the writable copy; the CMake product and Nix
Store inputs remain immutable. Every staged directory is fixed to `0755`,
every data file to `0644`, and the main executable to `0755`. Copying omits
ACLs, BSD flags, extended attributes, and quarantine/resource metadata.
The completed IPA is then rejected if it contains a symlink or special file,
an unsafe or duplicate path, ZIP extra metadata, a non-canonical Unix mode, or
the DOS read-only bit, or if its inventory differs from the complete staged
application bundle. ZIP option environment variables are cleared and every
literal input path must match, preventing silent entry omission. This is
required for importers such as LiveContainer, which restore archive permissions
before patching, signing, and cleaning up an application bundle.

A previous failed LiveContainer import can leave a read-only temporary
`Payload`. LiveContainer may try to remove that stale path before reading the
replacement archive, before the fixed IPA can take effect. If the same
permission error appears once with a fixed IPA, first use a verified
LiveContainer cleanup or container-reset procedure, preserving any needed app
data. The exact UI procedure is still pending real-device verification. Rule
out that stale state before retrying the import.

AltStore updates preserve the application data container. Packaged bundle
updates use their bundle-name inventory independently of Krita's semantic
version. The iOS resource locator imports newly packaged bundle names and
preserves existing resources and user data.

## Validated result

The physical-device run `20260802121956` completed the automated flow and
reached LibrePaint's main window. The migration retained the existing data
container. Its resource database contained:

- four bundle storages, three active by their defaults;
- 169 paint-op presets;
- 256 brushes;
- 274 patterns;
- 36 gradients and 25 palettes.

A subsequent launch reached an existing 2480 x 3508 canvas within five seconds
and displayed rendered brush strokes, the Freehand Brush tools, Tool Options,
and the Layer Docker. The capture recorded visual output only; pressure and
native input-event validation remain M5 work.

The first synchronization of approximately 36 MiB of bundled resources showed
the splash screen for about 20 seconds on the tested device. Later launches
repeat synchronization when the packaged bundle-name set changes.

The follow-up physical-device run `20260802123518` validated 496 runtime files
and restored every Edit-menu label that had been blank when the two core action
registries were absent. Disabled entries remain visible in gray as expected.

The physical-device run `20260802124350` disabled Qt's UIKit-native combo-box
picker at the LibrePaint application-style boundary. Qt 6.11 otherwise presents a
`UIPickerView` whose Done/Cancel input toolbar falls outside this window's
visible layout, making its commit and dismiss controls inaccessible. Combo
boxes now use Qt's inline popup list on iPadOS.
`Settings > Configure LibrePaint > General > Tools` was used to verify that
`Touch Painting` opens all three choices, commits
`Enabled` with one tap, and closes the list while keeping the dialog's OK and
Cancel buttons reachable. The override is application-wide and is preserved
when LibrePaint's widget style changes.

The physical-device run `20260802125200` fixed the startup splash position in
landscape. UIKit can publish the initial portrait screen geometry before the
application scene settles into its requested orientation. LibrePaint now recenters
the splash after its native view is attached and whenever Qt reports updated
screen geometry. The centered result was confirmed on the connected iPad.

The physical-device run `20260802125811` changed the initial focus of
`Settings > Configure LibrePaint` on iPadOS. KDE's `KPageView` normally proxies
dialog focus to its search line, which immediately summons the iOS software
keyboard. LibrePaint now focuses the settings category list when the dialog opens;
the search field remains available for explicit tap and keyboard navigation.
QuickTime verification confirmed that the complete dialog opens with Search
unfocused and every control unobscured. Explicitly tapping Search then focused
the field and presented the software keyboard normally.

The physical-device run `20260802131216` enabled touch-first kinetic scrolling
for every non-canvas `QAbstractScrollArea`, including controls created by
plugins. iPadOS uses the left-mouse flick-recognizer path on each viewport so
Qt delays the initial press and discards it when a swipe becomes a scroll; this
prevents item selections from moving under the finger. An ordinary tap still
selects its item. The Configure LibrePaint category list and brush selection were
verified with swipe scrolling, inertia, stable selection during a drag, and
normal tap selection.

The physical-device run `20260802132451` stopped editable item selections from
opening the iOS software keyboard. Qt marks a `QAbstractItemView` as an input
method target whenever its current model item has `Qt::ItemIsEditable`; on
iPadOS that presents the keyboard even when a brush or layer was only selected.
LibrePaint now disables that implicit item-view input method while leaving explicit
delegate editors and text fields unchanged. Brush selection was verified on
the connected iPad. The post-event iOS widget hooks also retain receivers with
`QPointer`, because event delivery may destroy transient widgets. This fixes
the `KisApplication::notify` invalid-pointer crash found in the superseded
`20260802132041` run; the final build reached the main window and remained
running before the input behavior was retested.
