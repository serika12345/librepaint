# iOS non-code asset boundary

## Purpose

The iOS build uses a deliberately restricted set of installed non-code data
while LibrePaint's asset provenance is reviewed. This document defines that
build and packaging boundary. The source tree remains a GPL-licensed Krita
derivative; component-specific terms and notices remain applicable.

This boundary was installed and launched on a physical iPad on 2026-08-07.
A QuickTime device capture confirmed that the restored toolbar, toolbox, color
selector, and brush-preview assets render in the app. New changes to the
available brushes, color resources, templates, or workspace defaults still
require a focused device build before they are described as verified.

## Installed on iOS

`krita/data/CMakeLists.txt` installs only these data groups on iOS:

- action descriptions;
- the restricted bundle set described below;
- ICC color profiles;
- workspaces, themes, predefined image sizes, input profiles, and shortcuts;
- metadata schemas.

These groups are kept because they provide functional configuration or required
color-management data. Each group retains its applicable license. Existing
license and provenance notices for the ICC profiles are installed verbatim
under `share/doc/librepaint/non-code-licenses/profiles`.

The iOS bundle set is limited to `README` and
`Krita_4_Default_Resources.bundle`. Its `meta.xml` explicitly declares `CC-0`
as the bundle license; Krita's resource-bundle editor reads and writes this
field as the license of the bundle. The more-specific unmodified notice in
`krita/data/README` takes precedence for the 31 GIH and six GBR brushes and
places those files under CC-BY-3.0. The remaining 244 files are CC0, including
the LibrePaint icon preview. The complete CC-BY-3.0 and CC0-1.0
texts are installed under `share/doc/librepaint/non-code-licenses`.

`packaging/ios/manifests/default-resource-bundle-licenses.json` records the
three license groups, their expected 244/37 totals, the bundle attribution,
and the four embedded PNG author fields. The corresponding audit classifies
every bundle file, parses KPP, SVG, and PNG metadata for individual legal
overrides, and checks the bundle's internal resource manifest. Run
`packaging/ios/scripts/audit-default-resource-bundle.py --list` to print the
file-by-file license and classification basis.

The bundle metadata records its historical generator, authorship, source
website, and other provenance. Its root preview is a separate branding surface
derived from the CC0 LibrePaint icon.

## Source-only data groups

The iOS data CMake branch leaves the following source-tree directories outside
the installed application:

- `brushes`, `patterns`, `gradients`, and `paintoppresets`;
- `templates`;
- `palettes`, `symbols`, `preset_icons`, and `gamutmasks`;
- `seexpr_scripts` and `windowlayouts`.

These collections are source-only in the current profile. Parts of them still
await a complete file-to-license mapping or artwork and branding review, while
the initial iPad runtime uses the selected data groups above. Separately
compiled Qt resources carry explicit SPDX notices and are audited independently
of this install-directory boundary.

The iOS bundle profile also leaves out `Krita_3_Default_Resources.bundle`,
`Krita_Artists_SeExpr_examples.bundle`, and `RGBA_brushes.bundle`. In
particular, the RGBA bundle's license field is scoped to the default brushes,
and the SeExpr bundle records CC-BY-SA without a version.

Splash, application icons, document icons, and bundle previews use the CC0
LibrePaint artwork described in
`packaging/ios/notices/librepaint-brand-assets.md`.

`packaging/ios/manifests/ios-image-licenses.json` is the closed license
inventory for repository-owned image inputs linked into the iOS app. Run
`packaging/ios/scripts/audit-ios-image-licenses.py --check` to verify it. The
QRC set contains 1,289 unique images:

- 703 SVG/SVGZ files with a direct, valid CC-BY-SA-4.0 metadata link;
- 93 additional functional SVG files with CC-BY-SA-4.0 provenance documented
  by their embedded metadata and upstream licensing commits;
- 297 Breeze icons and 22 Oxygen icons under LGPL-3.0-or-later;
- two Android log icons under CC-BY-SA-4.0 with their Android robot portions
  under CC-BY-3.0;
- one KXmlGui image under LGPL-2.0-or-later;
- 148 functional images retained under Krita's project-wide GPL-3.0-only
  distribution fallback where a more-specific asset notice is absent;
- 23 LibrePaint branding images under CC0-1.0.

The selected static plugins and data targets install another 32 functional
images under the same project-wide GPL fallback. The audit rediscovers that set
from 19 exact CMake install definitions, which remain its source of truth. The
QRC and installed sets have zero unclassified images. The manifest records the
explicit license groups, the QRC and CMake scope, the accepted metadata license
URL, and the applicable notice paths. It is installed with the app alongside
the complete CC, GPL, and LGPL texts and
`packaging/ios/notices/retained-functional-assets.md`.

All QRC and installed-only image inputs have a license classification. Required
branding surfaces use LibrePaint's CC0 artwork, and the macOS DMG background
uses the same LibrePaint splash.

Static Qt and KDE Frameworks dependencies add a separate, closed resource
boundary. Seven retained groups contain 253 files: 247 functional Qt style
PNGs, the unmodified Qt about-dialog logo, two Qt backing-store shaders, one
ICC sRGB profile used by PDF output, and two KColorScheme color definitions.
Their exact source inventory, pinned archive hashes, generated initializer
objects, archive payload members, license evidence, and final
`qInitResources_*` symbols are recorded in
`packaging/ios/manifests/static-dependency-resources.json`. The corresponding
audit checks the pinned source inventory, linked payload members, and final
resource symbols.

The Qt-owned resources are distributed under the LGPL-3.0-only option from
their upstream multi-license expression. The unmodified Qt logo remains only
as part of Qt's standard `aboutQt` resource: Qt documents that dialog as a
factual display of the Qt version used by an application, and the official
[Qt trademark guidelines](https://www.qt.io/trademark/) permit factual,
non-endorsement use of the unmodified logo with software built using Qt. Its
role in the application is factual dependency attribution. The color schemes
retain their in-file LGPL-2.0-or-later copyright and license headers. The ICC profile
keeps the International Color Consortium's original notice and Qt attribution
JSON; both are installed verbatim. The relevant LGPL,
GPL, and ICC texts are installed beside this document.

One dependency resource group is deliberately removed from the iOS link. The
unused `kcharselect_data` group is a 3.17 MB generated Unicode character
database whose pinned KF6 archive lacks a self-contained file-to-license
mapping. The linked-class audit requires zero `KCharSelect` symbols, keeping
future use of this data behind an explicit boundary update.

The profile set is restricted to 23 Elle Stone profiles under CC-BY-SA-3.0 and
eight YCbCr profiles under CC-BY-SA-4.0. Three legacy profiles with ambiguous or
conflicting local license records (`scRGB.icm`, `cmyk.icm`, and
`krita25_lcms-builtin-sRGB_g100-truegamma.icc`) are excluded on iOS. The
original profile documentation and both Creative Commons license texts are
installed unchanged.

Seven MyPaint brush definitions installed by the static plugin retain their own
embedded comments: five state CC0-1.0 and two state GPL-2.0-or-later. Their
seven separate preview PNGs, created for this preset set by David Revoy, have
been restored as functional artwork under the project-wide GPL-3.0-only
distribution fallback. The definition and preview classifications remain
separate, and the relevant license texts and provenance notice are installed
with the app.

The CC-BY-NC-ND-4.0 Netflix JPEG XL source fixtures, their derived expected
results, and the tests dedicated to those files have been removed. The remaining
JPEG XL tests cover import, export, animation, layers, multipage
documents, and invalid or inaccessible input.

## Attribution and stable identifiers

License text, copyright statements, credits, authorship, historical generator
values, and source URLs remain verbatim. In particular, factual attribution to
Krita and the Krita Foundation remains intact. The generic phrase
"the free paint app" is reserved for taglines and short generic product
descriptions. Ordinary product prose uses `LibrePaint` or a context-appropriate
neutral noun. Provenance keeps the original attribution.

Technical and compatibility identifiers remain stable until a dedicated
compatibility migration changes them. This includes existing KRA format names,
`application/x-krita` MIME identifiers, API namespaces, action IDs,
configuration keys, and resource lookup paths.

## Work still required

This boundary covers the installed iOS data selected by CMake. Build
`20260807134423` was installed and launched on a physical iPad, and a QuickTime
capture confirmed the restored functional image groups. The current asset set
and cross-platform LibrePaint icon unification still require a fresh build and
focused device check.

The complete source checkout still needs a source-wide asset inventory:

- a complete REUSE/DEP5-style file-to-license inventory;
- local provenance mappings for many source-only test and benchmark fixtures.

Future cleanup should replace or remove an asset, or document its exact license
and attribution. Existing legal notices retain their original wording.
