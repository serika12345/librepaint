# M4 static-plugin foundation validation

Date: 2026-08-02

The M4 profile links the P0 minimum plugin set statically into the unsigned
iPadOS application: KRA and PNG import/export, the default Pixel Brush paint-op,
basic canvas tools, the Layer Docker, and the startup-critical LittleCMS color
engine. This validates conversion, registration, metadata discovery, and
dead-stripping on a physical iPad before expanding the profile.

## Historical build

At the M4 foundation gate, the eight-plugin artifact below was produced with:

```sh
nix develop .#librepaint-ios --command packaging/ios/scripts/configure-krita.sh device
nix develop --command cmake --build --preset ios-device --parallel 8
```

The resulting application is:

`build-ios/krita/device-ninja/bin/krita.app`

## Implemented path

- `kis_add_library(... MODULE ...)` becomes `STATIC` only for iOS.
- CMake rewrites generic KPlugin factory names such as `ImportFactory` to
  target-derived names, making every static factory symbol unique in the final
  executable.
- The enabled targets are collected as they are declared. CMake generates one
  application source file that references and registers every factory and links
  the corresponding archive.
- Factory references originate in a directly linked application object. This
  retains the factory object and its MOC-embedded JSON metadata when dead
  stripping is enabled.
- `KoJsonTrader` adds the KCoreAddons static-plugin registry to its existing
  filesystem plugin search and preserves its established metadata interface.
- The current iOS profile enables `kritakraimport`, `kritakraexport`,
  `kritapngimport`, `kritapngexport`, and `kritadefaultpaintops`, plus their
  support libraries. `kritadefaulttools` supplies Freehand Brush, Fill,
  Gradient, Color Sampler, Line, Rectangle, Ellipse, Move, Pan, and the other
  basic canvas tools. `kritalayerdocker` supplies the minimum layer UI, and
  `kritalcmsengine` supplies the required color-space engine.

KRA's two plugins now use `K_PLUGIN_CLASS_WITH_JSON`. This is behaviorally
equivalent to their former factory macros, while allowing KCoreAddons' internal
factory-name override to make the static symbols unique.

## Inspection results

The executable contains all eight complete static plugin paths. The relevant
factory entry points are:

```text
qt_static_plugin_kritakraimport_factory()
qt_plugin_instance_kritakraimport_factory()
qt_plugin_query_metadata_kritakraimport_factory()
qt_static_plugin_kritakraexport_factory()
qt_plugin_instance_kritakraexport_factory()
qt_plugin_query_metadata_kritakraexport_factory()
qt_static_plugin_kritapngimport_factory()
qt_static_plugin_kritapngexport_factory()
qt_static_plugin_kritadefaultpaintops_factory()
qt_static_plugin_kritadefaulttools_factory()
qt_static_plugin_kritalayerdocker_factory()
qt_static_plugin_kritalcmsengine_factory()
```

The KRA, PNG, paint-op, tool, Docker, and LCMS service metadata is present in
the linked image. The rebuilt app is Mach-O arm64 for platform IOS, minimum iOS
17.0, built against SDK 26.5. Its Info.plist passes `plutil -lint`, and
`otool -L` shows only iOS system libraries and frameworks.

Static archive auditing also found three Qt resource collections that would
otherwise be removed by dead stripping: `sql`, `kxmlgui`, and `defaulttools`.
All three are registered from generated application code. The
`inspect-static-resources.sh` check derives the complete `qInitResources_*` set
from every built static archive and rejects the executable if any is missing.

Static linking exposed two non-inline function definitions in
`kis_paintop_plugin_utils.h`. Marking those header implementations `inline`
resolves the ODR violation for every static paint-op in shared code.

## Feature profile

At this gate, the device and Simulator presets explicitly enabled the P0
minimum profile.
Configure-time options control each functional group independently:

| CMake option | Static targets |
|---|---|
| `KRITA_IOS_PLUGIN_KRA` | KRA import and export |
| `KRITA_IOS_PLUGIN_PNG` | PNG import and export |
| `KRITA_IOS_PLUGIN_DEFAULT_PAINTOPS` | Pixel Brush, eraser, and clone paint-ops |
| `KRITA_IOS_PLUGIN_BASIC_TOOLS` | Basic canvas tools, including Freehand Brush |
| `KRITA_IOS_PLUGIN_LAYER_DOCKER` | Layer Docker |
| `KRITA_IOS_PLUGIN_LCMS_ENGINE` | LittleCMS color management engine |

For example, the following diagnostic configuration omits the Layer Docker:

```sh
cmake -S . -B build-ios/krita/device-ninja \
  -DKRITA_IOS_PLUGIN_LAYER_DOCKER=OFF
```

This was verified to remove `kritalayerdocker` from the generated registration
source while retaining the other factory registrations. Reapplying the
`ios-device` preset at this gate restored the complete eight-plugin profile.

## Physical-device validation

AltStore-signed builds now launch on a physical iPad. The pre-LCMS build stopped
during startup; the registered-LCMS build presented the main window. This
comparison confirms runtime enumeration and factory instantiation for the
required engine.
The SQL cache schema, XMLGUI data, default tool data, ICC profiles, and four
resource bundles also load on-device. The resource database contains 169 brush
presets after first-run synchronization.

Subsequent profile expansion covers JPEG/ORA and the remaining
Android-equivalent plugins. The current 162-target profile has passed final
linking, IPA inspection, physical-device installation, and startup. It includes
plugins declared with both `K_PLUGIN_CLASS_WITH_JSON` and
`K_PLUGIN_FACTORY_WITH_JSON`; CMake gives both forms target-derived static
symbols. Per-plugin UI and interaction validation remains tracked in `TODO.md`.
