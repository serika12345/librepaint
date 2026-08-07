<!--
SPDX-FileCopyrightText: 2026 LibrePaint contributors
SPDX-License-Identifier: CC0-1.0
-->

# LibrePaint iOS non-code asset notices

This index describes the non-code material intentionally retained in the
LibrePaint iOS application. It supplements, and does not replace or modify,
the license and attribution text embedded in individual files.

## Static and user-interface images

The iOS resource audit covers 1,301 unique image inputs compiled through Qt
resources. Their exclusive final classification is:

- 709 direct metadata-linked CC-BY-SA-4.0 images;
- 93 additional CC-BY-SA-4.0 functional images with detached, historically
  malformed, or commit-level license evidence;
- 297 Breeze and 22 Oxygen icons under LGPL-3.0-or-later;
- two Android log icons under CC-BY-SA-4.0 and CC-BY-3.0;
- two KXmlGui images under LGPL-2.0-or-later;
- 148 functional images covered by Krita's project-wide GPL-3.0-only
  distribution fallback;
- two LibrePaint-owned brand images under CC0-1.0;
- 25 same-size white replacements and one intentional zero-byte alias.

A further 32 functional images installed by the selected static plugins and
data targets are retained under the project-wide GPL fallback. Their exact set
is rediscovered from 19 CMake install definitions during every audit.

The exact paths, original dimensions, retained metadata hashes, Qt resource
inputs, whole-content hashes, CMake definitions, and audit invariants are
recorded in `white-brand-assets.json`, which is distributed beside this file.
`retained-functional-assets.md` supplies the Breeze, Oxygen, Android, KXmlGui,
KDE trademark, project-wide GPL, and modification notices.
`librepaint-brand-assets.md` records the author statement, source hashes,
CC0 dedication, and current compatibility paths for the new logo. Copyright,
creator, title, license, and other provenance metadata retained inside image
files continues to apply. The complete applicable CC, GPL, LGPL, and CC0 texts
are distributed beside this index.

## LibrePaint branding

The project owner stated that they created the supplied LibrePaint icon and
wordmark and released them under CC0-1.0. The four unmodified SVG/PNG source
files are retained in `packaging/ios/brand-assets`; `.license` sidecars apply
the CC0 declaration without changing their original bytes. The active iOS app
icon, Qt runtime icon, and 4K splash are derived from those sources. The HD
splash fallback is derived under the same terms.

The generated assets temporarily retain their existing Krita-era filenames so
the current build can load them. They are complete replacements and therefore
do not carry forward creator or CC-BY-SA metadata from the former Krita art.
Exact source hashes and transformations are listed in
`librepaint-brand-assets.md`.

## Default resource bundle

`Krita_4_Default_Resources.bundle` is retained because it supplies the initial
brush resources needed for drawing. All 281 files have an exact-set audit: 244
are covered by the bundle-wide `CC-0` declaration in its unmodified `meta.xml`,
and the more-specific existing notice applies CC-BY-3.0 to its 31 GIH and six
GBR brush files. That unmodified notice is distributed as `bundles/README` and
credits David Revoy and the Blender Foundation and identifies Ramon Miranda's
Gimp Paint Studio source. Four PNG brush tips additionally retain their
embedded author fields for David Revoy or Scott Petrovic. The root bundle
preview has been replaced by same-size white CC0 content without changing the
remaining archive entries.

`default-resource-bundle-licenses.json` records the license rules, exact file
and format counts, preserved notice hashes, bundle metadata, embedded PNG
attribution, and a content-sealed classification digest. Its audit parses all
117 KPP metadata records and verifies that the startup brush, eraser, and a
color-smudge preset remain self-contained. The audit reports zero unclassified
files.

The complete CC-BY-3.0 and CC0-1.0 texts are distributed beside this index.

## Static dependency resources

Seven resources statically supplied by pinned QtBase and KColorScheme builds
are retained as a closed 253-file set: 247 Qt style PNGs, the unmodified Qt
about-dialog logo, two Qt backing-store QSB shaders, the ICC `sRGB2014.icc`
profile used by PDF output, and two Breeze color scheme files.
`static-dependency-resources.json` records every source path and runtime alias,
the pinned source archives, source and payload hashes, generated initializer
objects, license evidence, and the exact expected resource symbols in the
final Mach-O binary.

The 250 Qt-owned files retain the Qt Company's copyright and are distributed
under the LGPL-3.0-only option stated by Qt's unmodified REUSE records. The Qt
logo is retained only in Qt's unmodified standard `aboutQt` resource as factual
dependency attribution, consistent with the official
[Qt trademark guidelines](https://www.qt.io/trademark/).
The two Breeze files retain their embedded author and LGPL-2.0-or-later lines.
The ICC profile remains under `LicenseRef-ICC-License`; its original ICC notice
and Qt's unmodified attribution JSON are distributed beside this index. The
LGPL-2.0-or-later, LGPL-3.0-only, GPL-3.0-only, and ICC license texts are also
included.

The unused `kcharselect_data` Unicode database is excluded from the iOS link.
The binary audit rejects its resource initializer and any KCharSelect
implementation symbol so that this exclusion cannot silently break a future
feature.

## ICC profiles

The iOS application includes exactly 31 profiles with explicit local license
records:

- 23 profiles from Elle Stone's Well Behaved Profiles, CC-BY-SA-3.0;
- eight YCbCr profiles by Amyspark, CC-BY-SA-4.0.

The original README and license documents for both groups are distributed in
the `profiles` subdirectory. Three legacy profiles with ambiguous or
conflicting local license records (`scRGB.icm`, `cmyk.icm`, and
`krita25_lcms-builtin-sRGB_g100-truegamma.icc`) are deliberately excluded from
the iOS installation.

## MyPaint brush definitions and functional configuration

The selected MyPaint plugin installs seven JSON brush definitions. Their own
comments identify five as CC0-1.0 and two as GPL-2.0-or-later. Their seven
separate preview PNGs, created for this preset set by David Revoy, are retained
under Krita's project-wide GPL-3.0-only distribution fallback. The definition
and preview classifications are intentionally separate. The relevant CC0 and
GPL texts and the provenance notice are distributed beside this index.

Action descriptions, workspaces, themes, input and shortcut profiles,
predefined image sizes, metadata schemas, and similar functional configuration
remain part of the GPL application distribution. The GPL-3.0-or-later text is
included here for continuity; this statement does not override a more specific
notice embedded in a file or preserved in its source history.

## Scope

This is a packaging inventory, not a relicensing statement or legal opinion.
Technical compatibility identifiers such as KRA MIME types, resource paths,
action IDs, and the `krita` executable name are intentionally unchanged.
