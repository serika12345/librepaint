<!--
SPDX-FileCopyrightText: 2026 LibrePaint contributors
SPDX-License-Identifier: CC0-1.0
-->

# Retained functional image assets

This notice documents original functional images retained by the LibrePaint
iOS build. It supplements more-specific notices in individual assets, which
continue to apply according to their terms. Exact paths, source-content hashes,
license expressions, and this notice's hash are recorded in
`white-brand-assets.json` distributed beside this file.

## Creative Commons functional artwork

The application retains 703 SVG/SVGZ images whose `cc:Work` metadata directly
links to CC-BY-SA-4.0. Their exact path and whole-file content hashes are
sealed by the manifest, which detects changes to creator, rights, and license
metadata.

Another 93 functional SVG files are retained under CC-BY-SA-4.0. Eighty-nine
contain the same canonical license URL and original creator/provenance
metadata, but their historical RDF either lacks the `cc:Work` link or contains
malformed XML. Their supplied credits include Wolthera van Hövell tot
Westerflier (vectorization), Tomtomtom, David Revoy, Deif Lou, Timothée Giet,
Andrei Rudenko, and superfurryfrog. Four tool icons credit Andrei Rudenko and
were explicitly placed under CC-BY-SA-4.0 in Raghavendra Kamath's upstream
license-cleanup commit. The original SVG source forms are retained unchanged.

https://invent.kde.org/graphics/krita/-/commit/6c3b1edf289a48d170b3ca974136b3981058ea73

https://invent.kde.org/graphics/krita/-/commit/3e21da98bd5d3f5d59e094425f0655a18e2a9ce0

https://creativecommons.org/licenses/by-sa/4.0/

## Oxygen icons

Twenty-two functional images are retained from the Oxygen Icon Theme under
LGPL-3.0-or-later. The Oxygen Icon Theme is copyright 2007 Nuno Pinheiro,
David Vignoni, David Miller, Johann Ollivier Lapeyre, Kenneth Wimer, Riccardo
Iaconelli, and others. The local source forms and exact hashes are in the
manifest. The fixed upstream license evidence and the Krita import commits are:

https://invent.kde.org/frameworks/oxygen-icons/-/raw/246760ed7e382d1ecc92cbfc194acee888d2d308/COPYING

https://invent.kde.org/graphics/krita/-/commit/ab9bc009bf476fbec4f5fb4b5329f119c0cce69a

https://invent.kde.org/graphics/krita/-/commit/9a01a0ee3de72a6005fc061baac64585b7f8517d

## Breeze icons

The 297 retained Breeze images comprise 87 SVG files under
`krita/pics/Breeze-dark` and `krita/pics/Breeze-light`, plus 210 Breeze-derived
functional icons under `pics`. The Breeze Icon Theme is copyright 2014 Uri
Herrera and others and is licensed under LGPL-3.0-or-later. SVG is retained as
the preferred source form. The fixed upstream notice is:

https://invent.kde.org/frameworks/breeze-icons/-/raw/a0ce9f0faea836e45d765bfb48af2ada6f9bfc70/COPYING-ICONS

https://develop.kde.org/frameworks/breeze-icons/

The 210 root copies are sealed against the four upstream Krita import commits:

- https://invent.kde.org/graphics/krita/-/commit/4b7dc61611894c8598cf111796dfb43a9b54d619
- https://invent.kde.org/graphics/krita/-/commit/ad7e73142669c61080289dc2f6d59b353c7b4ce2
- https://invent.kde.org/graphics/krita/-/commit/3295c4bdaf79b380c539fabbc9e26ca27a32ebf8
- https://invent.kde.org/graphics/krita/-/commit/f6b8fc14c362256b79b571c3002bd0918e8b7839

The two small KDE logo icons serve exclusively as factual KDE attribution;
LibrePaint application branding uses the project-owned CC0 artwork.

## Android robot

`krita/pics/svg/dark_show_android_log.svg` and
`krita/pics/svg/light_show_android_log.svg` remain functional log-action icons.
The composite icons remain under CC-BY-SA-4.0, while the Android robot portion
is used under CC-BY-3.0 in accordance with Google's official terms:

> The Android robot is reproduced or modified from work created and shared by Google and used according to terms described in the Creative Commons 3.0 Attribution License.

Android is a trademark of Google LLC. Its factual use here does not imply
Google sponsorship, affiliation, or endorsement of LibrePaint.

https://developer.android.com/distribute/marketing-tools/brand-guidelines

## KXmlGui artwork

`libs/widgetutils/xmlgui/thumb_frame.png` is retained from KDE KXmlGui under
LGPL-2.0-or-later as a functional shortcut-editor frame. The local file
matches the KXmlGui 6.28.0 source:

https://invent.kde.org/frameworks/kxmlgui/-/blob/v6.28.0/src/thumb_frame.png

## Krita project-wide GPL fallback

For assets lacking a more-specific asset or family license, 148 retained QRC
images and 32 images discovered directly from the selected CMake install
definitions remain part of this modified application under Krita's
project-wide GPL version 3 declaration. This distribution classification
preserves each file's existing license status. Application branding and images
with separate notices follow those specific terms.

https://krita.org/en/about/license/

The seven restored MyPaint preview PNGs in this group were created for Krita's
MyPaint preset set by David Revoy. Their `.myb` definitions retain their own
separate comments: five CC0-1.0 and two GPL-2.0-or-later.

https://invent.kde.org/graphics/krita/-/commit/1c976c01fb27cce7cf60ee38b352d0c2760fae92

## LibrePaint branding and asset cleanup

The iOS QRC scope contains 23 LibrePaint-owned branding images under CC0-1.0.
They are generated from the project owner's source artwork and documented in
`librepaint-brand-assets.md`. This includes the splash, application and
document icons, log icons, and neutral PDF fallback icons.

Asset cleanup removed upstream branding, donation, sponsor, news, and
disabled-tool images together with their UI or resource references. It also
removed every white placeholder and zero-byte image from the iOS QRC scope and
the source-wide white selection. The macOS DMG background now reuses the CC0
LibrePaint splash. Original license wording and factual attribution remain
intact.
