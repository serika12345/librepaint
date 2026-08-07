<!--
SPDX-FileCopyrightText: 2026 LibrePaint contributors
SPDX-License-Identifier: CC0-1.0
-->

# Retained and modified functional image assets

This notice documents original functional images retained by the LibrePaint
iOS build and the original notices that remain attached to modified white
replacements. It supplements, and never replaces, a more-specific notice in
an asset. Exact paths, source-content hashes, license expressions, and this
notice's hash are recorded in `white-brand-assets.json` distributed beside
this file.

## Creative Commons functional artwork

The application retains 709 SVG/SVGZ images whose `cc:Work` metadata directly
links to CC-BY-SA-4.0. Their exact path and whole-file content hashes are
sealed by the manifest, so creator, rights, and license metadata cannot change
silently.

Another 93 functional SVG files are retained under CC-BY-SA-4.0. Eighty-nine
contain the same canonical license URL and original creator/provenance
metadata, but their historical RDF either omits the `cc:Work` link or is not
well-formed XML. Their supplied credits include Wolthera van Hövell tot
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

The two small KDE logo icons are used only for factual KDE attribution and are
not LibrePaint application icons.

## Android robot

`krita/pics/svg/dark_show_android_log.svg` and
`krita/pics/svg/light_show_android_log.svg` remain functional log-action icons.
The composite icons remain under CC-BY-SA-4.0, while the Android robot portion
is used under CC-BY-3.0 in accordance with Google's official terms:

> The Android robot is reproduced or modified from work created and shared by Google and used according to terms described in the Creative Commons 3.0 Attribution License.

Android is a trademark of Google LLC. Its factual use here does not imply
Google sponsorship, affiliation, or endorsement of LibrePaint.

https://developer.android.com/distribute/marketing-tools/brand-guidelines

## KXmlGui artwork and KDE attribution

`libs/widgetutils/xmlgui/aboutkde.png` and
`libs/widgetutils/xmlgui/thumb_frame.png` are retained from KDE KXmlGui under
LGPL-2.0-or-later. The former appears only inside the standard factual About
KDE dialog; the latter is a functional shortcut-editor frame. The local files
match their upstream sources, including the historical About KDE artwork
commit and the KXmlGui 6.28.0 `thumb_frame.png` source:

https://invent.kde.org/frameworks/kxmlgui/-/commit/d5ed5228d59072d3c1d8765087e8a55f84b18cc8

https://invent.kde.org/frameworks/kxmlgui/-/blob/v6.28.0/src/thumb_frame.png

KDE® and the K Desktop Environment® logo are registered trademarks of KDE
e.V. LibrePaint is an independent project and is not endorsed or sponsored by
KDE e.V.

## Krita project-wide GPL fallback

When no more-specific asset or family license is available, 148 retained QRC
images and 32 images discovered directly from the selected CMake install
definitions remain part of this modified application under Krita's
project-wide GPL version 3 declaration. This is a distribution classification,
not a claim that the individual files have been relicensed. Application
branding, sponsor/donation material, and any image with a separate notice are
excluded from this fallback.

https://krita.org/en/about/license/

The seven restored MyPaint preview PNGs in this group were created for Krita's
MyPaint preset set by David Revoy. Their `.myb` definitions retain their own
separate comments: five CC0-1.0 and two GPL-2.0-or-later.

https://invent.kde.org/graphics/krita/-/commit/1c976c01fb27cce7cf60ee38b352d0c2760fae92

## White replacements and modification notice

On 2026-08-07, LibrePaint replaced the visible drawing in every path selected
by `white-brand-assets.json` with deterministic same-size opaque-white content.
The newly drawn white pixels or SVG elements are dedicated under CC0-1.0. This
does not place a whole container under CC0 when original copyright, creator,
license, title, or other provenance metadata remains; those original notices
and licenses continue to apply and are byte-hash checked.

The following five CC-BY-SA-4.0 SVG/SVGZ sources specifically retain their
original metadata while indicating here that their visible artwork was changed
to white:

- `krita/pics/branding/Next/sc-apps-krita.svgz`
- `krita/pics/svg/dark_krita_log.svg`
- `krita/pics/svg/light_krita_log.svg`
- `pics/32_dark_application-x-krz.svg`
- `pics/32_light_application-x-krz.svg`

The visible artwork in
`krita/pics/Breeze-dark/dark_application-pdf.svg` and
`krita/pics/Breeze-light/light_application-pdf.svg` was also changed to white.
Their original Breeze LGPL notice remains applicable. The replacement avoids
shipping an Adobe PDF logo-shaped icon in software, which Adobe's current icon
guidelines do not permit without separate authorization.

https://www.adobe.com/legal/permissions/icons-web-logos.html

Krita application names, icons, splash/donation/sponsor imagery, and other
branding remain white for trademark separation, consistent with Krita's own
redistribution guidance. Two temporary comic-panel icons without verified
individual provenance also remain white. License wording and factual
attribution are never rewritten.
