<!--
SPDX-FileCopyrightText: 2026 LibrePaint logo author
SPDX-License-Identifier: CC0-1.0
-->

# LibrePaint brand assets

On 2026-08-08, the LibrePaint project owner stated that they personally
created the supplied LibrePaint logo and icon and elected to release both
works under CC0-1.0. The supplied README also states that the artwork was
assembled without generative AI.

The unmodified source files are retained under
`packaging/ios/brand-assets`. Their input SHA-256 values are:

- `librepaint-icon.svg`:
  `2db493d76e8941413762647bbf7d2599ae7e25d4d005eafcf9efced6d0df9c3c`
- `librepaint-icon.png`:
  `5a7e1f43503083c8d6005bce405937b4d2aa0db6a87845a8215804afc596fba0`
- `librepaint-logo.svg`:
  `3bdd573cf384ba67e5e0940c2cb39fa9ca26db87fdcc2c26c3e0533c0a0413fd`
- `librepaint-logo.png`:
  `9650fe80133e0c34d9be036cdc247c6dcc583554d4e1ca363e75a4c19d239dad`

Compatibility filenames are intentionally retained until the later filename
migration. `packaging/ios/scripts/install-librepaint-brand-assets.sh`
reproducibly generates or copies the current derivatives:

- `krita/pics/branding/Next/*-apps-krita.png` and
  `krita/pics/branding/Next/sc-apps-krita.svgz` provide the shared desktop and
  runtime icon. The 1024-pixel PNG is composited onto opaque white for iOS.
- `krita/pics/branding/Next/krita.icon/Assets/librepaint-icon.svg` is the sole
  artwork layer used by the Xcode 26 macOS icon package.
- `krita/pics/mimetypes/*-mimetypes-application-x-krita.png`,
  `pics/krita.png`, the KRA/KRZ compatibility SVGs under `pics`, the two
  application-log SVGs, and the two PDF fallback SVGs are temporary uses of
  the same square icon.
- `packaging/macos/KritaIcon.icns`,
  `krita/pics/mimetypes/krita-kra.icns`, and
  `krita/pics/mimetypes/krz/krita-krz.icns` are generated from that single
  Xcode icon layer. The first file is also the DMG volume icon.
- `packaging/macos/krita_dmgBG.png` reuses the LibrePaint splash artwork in
  the upper half of the 1400 x 870, 144-dpi DMG background, leaving the Finder
  install-icon area clear.
- `packaging/android/apk/res/mipmap-*/ic_launcher.webp` contains the five
  standard Android density renderings. All Android build variants and the
  round-icon reference use this one set during the temporary branding phase.
- The root `preview.png` payload in each tracked resource bundle under
  `krita/data/bundles` is a 256-pixel opaque rendering of the icon. Only that
  replacement payload is CC0-1.0; the other bundle entries retain their own
  applicable licenses.
- `krita/data/splash/electrichearts_20250824A_kiki_4K.png` is the full
  LibrePaint logo fitted to the existing 3840 x 1920 splash canvas, and
  `krita/data/splash/electrichearts_20250824A_kiki_HD.jpg` is the matching
  1920 x 960 Android fallback.

These outputs are complete artwork replacements derived only from the
LibrePaint sources listed above. Copyright, attribution, title, and
CC-BY-SA metadata belonging to former artwork are not carried onto them.

CC0-1.0 permits unrestricted reuse to the extent the author's copyright and
related rights can be waived, with a permissive public-license fallback where
waiver is not effective:

https://creativecommons.org/publicdomain/zero/1.0/
