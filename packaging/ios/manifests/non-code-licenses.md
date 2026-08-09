<!--
SPDX-FileCopyrightText: 2026 LibrePaint contributors
SPDX-License-Identifier: CC0-1.0
-->

# LibrePaint iOS non-code asset notices

This index describes the non-code material intentionally retained in the
LibrePaint iOS application. It supplements the license and attribution notices
embedded in individual files, which continue to apply according to their
terms.

## Static and user-interface images

The iOS resource audit covers 1,289 unique image inputs compiled through Qt
resources. Their exclusive final classification is:

- 703 direct metadata-linked CC-BY-SA-4.0 images;
- 93 additional CC-BY-SA-4.0 functional images with detached, historically
  malformed, or commit-level license evidence;
- 297 Breeze and 22 Oxygen icons under LGPL-3.0-or-later;
- two Android log icons under CC-BY-SA-4.0 and CC-BY-3.0;
- one KXmlGui image under LGPL-2.0-or-later;
- 148 functional images covered by Krita's project-wide GPL-3.0-only
  distribution fallback;
- 23 LibrePaint-owned brand images under CC0-1.0.

The iOS image audit assigns all 1,289 QRC inputs to a license classification;
the unclassified-image count is 0.

A further 32 functional images installed by the selected static plugins and
data targets are retained under the project-wide GPL fallback. Their exact set
is rediscovered from 19 CMake install definitions during every audit.

For the eight explicit license groups, `ios-image-licenses.json` records the
exact paths, license expressions and sources, Qt resource inputs, and CMake
install definitions. It is distributed beside this file.
`retained-functional-assets.md` supplies the Breeze, Oxygen, Android, KXmlGui,
KDE trademark, project-wide GPL, and modification notices.
`librepaint-brand-assets.md` records the author statement, source files,
CC0 dedication, and current compatibility paths for the new logo. Copyright,
creator, title, license, and other provenance metadata retained inside image
files continue to apply. The complete applicable CC, GPL, LGPL, and CC0 texts
are distributed beside this index.

## LibrePaint branding

The project owner stated that they created the supplied LibrePaint icon and
wordmark and released them under CC0-1.0. The four unmodified SVG/PNG source
files are retained in `packaging/ios/brand-assets`; `.license` sidecars apply
the CC0 declaration while preserving their original bytes. The cross-platform
application and document icons, Qt runtime icon, resource bundle previews, and
both splash variants are derived from those sources.

The generated assets use the existing compatibility filenames consumed by the
build. Their provenance and licensing come from the LibrePaint source set and
its CC0 declaration; `librepaint-brand-assets.md` lists the source files and
their current derivatives.

## Default resource bundle

`Krita_4_Default_Resources.bundle` is retained because it supplies the initial
brush resources needed for drawing. Its 281 files are classified during the
audit: 244 are covered by the bundle-wide `CC-0` declaration in its `meta.xml`,
and the more-specific existing notice applies CC-BY-3.0 to its 31 GIH and six
GBR brush files. That unmodified notice is distributed as `bundles/README` and
credits David Revoy and the Blender Foundation and identifies Ramon Miranda's
Gimp Paint Studio source. Four PNG brush tips additionally retain their
embedded author fields for David Revoy or Scott Petrovic. The root bundle
preview is a same-size rendering of the CC0 LibrePaint icon; the other archive
entries retain their applicable licenses.

`default-resource-bundle-licenses.json` records the three license groups, their
expected 244/37 totals, the bundle attribution, the applicable notice paths,
and the four embedded PNG author fields. Its audit classifies every file,
checks the internal resource manifest, and inspects KPP, SVG, and PNG metadata
for individual legal overrides.

The complete CC-BY-3.0 and CC0-1.0 texts are distributed beside this index.

## Static dependency resources

Seven resource groups statically supplied by pinned QtBase and KColorScheme builds
are retained as a closed 253-file set: 247 Qt style PNGs, the unmodified Qt
about-dialog logo, two Qt backing-store QSB shaders, the ICC `sRGB2014.icc`
profile used by PDF output, and two Breeze color scheme files.
`static-dependency-resources.json` records every source path and runtime alias,
the pinned source archive hashes, initializer and payload paths, license
evidence, and the expected resource symbols in the final Mach-O binary.

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

The iOS link omits the unused `kcharselect_data` Unicode database. The binary
audit rejects its resource initializer and every KCharSelect implementation
symbol, making any future feature that requires KCharSelect an explicit
dependency change.

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
included here for continuity; more-specific notices embedded in files or
preserved in source history retain precedence.

## Scope

This packaging inventory records included resources, provenance, and license
evidence. It leaves the cited license texts and source notices intact,
including the rights and obligations stated in them. Technical compatibility
identifiers such as the iOS bundle ID, KRA MIME/UTI values,
application/configuration IDs, resource paths, and plugin/action IDs remain
stable. The logical CMake target remains `krita`; the physical iOS app bundle
and executable are `LibrePaint.app` and `LibrePaint`.
