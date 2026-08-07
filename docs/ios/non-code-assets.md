# iOS non-code asset boundary

## Purpose

The iOS build uses a deliberately restricted set of installed non-code data
while LibrePaint's asset provenance is reviewed. This is a build and packaging
boundary, not a relicensing statement and not legal advice. The current source
tree remains a GPL-licensed derivative, and every retained third-party notice
continues to apply.

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
color-management data. Keeping them does not claim that they have all been
placed under one license. Existing license and provenance notices for the ICC
profiles are installed under
`share/doc/librepaint/non-code-licenses/profiles`, without editing their text.

The iOS bundle set is limited to `README` and
`Krita_4_Default_Resources.bundle`. Its `meta.xml` explicitly declares `CC-0`
as the bundle license; Krita's resource-bundle editor reads and writes this
field as the license of the bundle. The more-specific unmodified notice in
`krita/data/README` takes precedence for the 31 GIH and six GBR brushes and
places those files under CC-BY-3.0. The remaining 244 files are CC0, including
the separately replaced white preview. The complete CC-BY-3.0 and CC0-1.0
texts are installed under `share/doc/librepaint/non-code-licenses`.

`packaging/ios/manifests/default-resource-bundle-licenses.json` seals the exact
281-file classification, the four directory entries, the source-notice and
license-text hashes, the unmodified bundle metadata, and the four embedded PNG
author fields. The corresponding audit also parses all 117 KPP metadata
records, rejects an unrecorded individual legal override, and checks the
self-contained startup brush, eraser, and color-smudge fallback. There are no
unclassified bundle files. Run
`packaging/ios/scripts/audit-default-resource-bundle.py --list` to print the
file-by-file license and classification basis.

The bundle's historical generator, authorship, source website, and other
provenance fields remain unchanged. Its root preview is a separate branding
surface and must use the neutral replacement supplied by the branding cleanup;
the CC0 copyright status does not remove trademark concerns.

## Excluded from the iOS data install

The following directories remain in the source tree but are not installed by
the iOS data CMake branch:

- `brushes`, `patterns`, `gradients`, and `paintoppresets`;
- `templates` and `storyboard_templates`;
- `palettes`, `symbols`, `preset_icons`, and `gamutmasks`;
- `seexpr_scripts` and `windowlayouts`;
- Android donation artwork.

They are excluded because the current collection either lacks a complete
file-to-license mapping, contains artwork or branding that needs replacement,
or is not required for the initial iPad runtime. This does not delete them and
does not assert that they are unlicensed. Some separately compiled Qt resources
have their own explicit SPDX notices and are audited independently of this
install-directory boundary.

`Krita_3_Default_Resources.bundle`,
`Krita_Artists_SeExpr_examples.bundle`, and `RGBA_brushes.bundle` are also
excluded from iOS. In particular, the RGBA bundle's license field refers only
to the default brushes instead of stating reusable terms, and the SeExpr bundle
does not specify the version of its CC-BY-SA license.

Sponsor logos are no longer compiled through `sponsors.qrc`. Splash, application
icons, bundle previews, and other visible branding are handled by the neutral
asset replacement work rather than by this install filter.

The exact replacement set and its original dimensions are recorded in
`packaging/ios/manifests/white-brand-assets.json`. Run
`packaging/ios/scripts/replace-brand-art-with-white.py --check` to verify all
selected raster, vector, icon-container, and bundle-preview assets. The new
opaque-white drawing content is dedicated under CC0-1.0. Preserved SVG legal
metadata and non-preview bundle content retain their existing licenses and
provenance; the replacement does not relicense them.

The same manifest audits the repository-owned image inputs linked into the iOS
app. The closed QRC set contains 1,301 unique images:

- 709 SVG/SVGZ files with a direct, valid CC-BY-SA-4.0 metadata link;
- 93 additional functional SVG files with CC-BY-SA-4.0 provenance sealed from
  their embedded metadata and upstream licensing commits;
- 297 Breeze icons and 22 Oxygen icons under LGPL-3.0-or-later;
- two Android log icons under CC-BY-SA-4.0 with their Android robot portions
  under CC-BY-3.0;
- two KXmlGui images under LGPL-2.0-or-later;
- 148 functional images retained under Krita's project-wide GPL-3.0-only
  distribution fallback when no more-specific asset notice exists;
- 27 same-size white replacements and one intentional zero-byte splash alias.

The selected static plugins and data targets install another 32 functional
images under the same project-wide GPL fallback. Unlike the earlier provisional
audit, that set is rediscovered from 19 exact CMake install definitions rather
than treating the manifest as its own source of truth. The QRC and installed
sets have zero unclassified images. Exact path, whole-content, license-URL,
CMake-definition, and notice hashes are installed with the app alongside the
complete CC, GPL, and LGPL texts and
`packaging/ios/notices/retained-functional-assets.md`.

The cleanup now keeps 596 functional images that had been over-broadly made
white in the provisional pass. The remaining manifest-wide white selection is
249 assets, covering application branding, splash/donation/sponsor material,
ambiguous bundle previews, two unprovenanced temporary icons, and narrowly
identified trademark exceptions. White drawing content is CC0; preserved
metadata or container material retains its original license. The shipped
notice records the 2026-08-07 modification and specifically identifies the
CC-BY-SA and LGPL sources whose visible drawing was changed.

Static Qt and KDE Frameworks dependencies add a separate, closed resource
boundary. Seven retained groups contain 253 files: 247 functional Qt style
PNGs, the unmodified Qt about-dialog logo, two Qt backing-store shaders, one
ICC sRGB profile used by PDF output, and two KColorScheme color definitions.
Their exact source inventory, pinned archive hashes, generated initializer
objects, archive payload members, license evidence, and final
`qInitResources_*` symbols are sealed in
`packaging/ios/manifests/static-dependency-resources.json`. The corresponding
audit rejects an added, removed, or byte-changed dependency payload.

The Qt-owned resources are distributed under the LGPL-3.0-only option from
their upstream multi-license expression. The unmodified Qt logo remains only
as part of Qt's standard `aboutQt` resource: Qt documents that dialog as a
factual display of the Qt version used by an application, and the official
[Qt trademark guidelines](https://www.qt.io/trademark/) permit factual,
non-endorsement use of the unmodified logo with software built using Qt. It is
dependency attribution, not LibrePaint branding. The color schemes retain
their in-file LGPL-2.0-or-later copyright and license headers. The ICC profile
keeps the International Color Consortium's original notice and Qt attribution
JSON; both are installed without editing their wording. The relevant LGPL,
GPL, and ICC texts are installed beside this document.

One dependency resource group is deliberately removed from the iOS link. The
unused `kcharselect_data` group is a 3.17 MB generated Unicode character
database whose pinned KF6 archive does not carry a self-contained
file-to-license mapping. The audit also rejects any linked `KCharSelect` class
symbol, so a future feature cannot silently begin relying on the excluded data.

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
JPEG XL tests continue to cover import, export, animation, layers, multipage
documents, and invalid or inaccessible input without that restricted dataset.

## Attribution and stable identifiers

Do not rewrite license text, copyright statements, credits, authorship,
historical generator values, or source URLs. In particular, factual attribution
to Krita and the Krita Foundation must remain intact. The generic phrase
"the free paint app" is only suitable for non-legal product prose; substituting
it into provenance would make the attribution inaccurate.

Technical and compatibility identifiers are not product branding. Existing KRA
format names, `application/x-krita` MIME identifiers, API namespaces, action
IDs, configuration keys, and resource lookup paths must remain stable until a
separate compatibility migration changes them deliberately.

## Work still required

This boundary covers the installed iOS data selected by CMake. The locally
built and staged application contains 228 runtime-data files, exactly matching
the generated install tree; the packaging checks also reject excluded data,
the three ambiguous ICC profiles, and retired sponsor resources. Build
`20260807134423` was installed and launched on a physical iPad, and a QuickTime
capture confirmed the restored functional image groups listed above. This
focused visual check is not a claim that every application view has been
exercised.

This does not yet make the complete source checkout a clean-room asset
collection:

- the repository has no complete REUSE/DEP5-style file-to-license inventory;
- many source-only test and benchmark fixtures still lack a local provenance
  mapping.

Future cleanup should replace or remove an asset, or document its exact license
and attribution. It must never obtain a cleaner result by editing the wording of
an existing legal notice.
