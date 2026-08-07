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

The current iPad-compatible paths are intentionally retained until the later
filename migration:

- `krita/pics/branding/Next/1024-apps-krita.png` is the square icon composited
  onto opaque white for iOS app-icon compatibility.
- `krita/pics/branding/Next/sc-apps-krita.svgz` is a deterministic gzip copy of
  the square vector icon for Qt runtime branding.
- `krita/data/splash/electrichearts_20250824A_kiki_4K.png` is the full
  LibrePaint logo fitted to the existing 3840 x 1920 splash canvas.
- `krita/data/splash/electrichearts_20250824A_kiki_HD.jpg` is the matching
  1920 x 960 Android fallback.

These files are complete replacements. Copyright, attribution, title, and
CC-BY-SA metadata belonging to the former Krita artwork are not carried onto
the new LibrePaint artwork. The old split icon/banner widgets are hidden on
the splash so that the complete logo is not drawn more than once.

CC0-1.0 permits unrestricted reuse to the extent the author's copyright and
related rights can be waived, with a permissive public-license fallback where
waiver is not effective:

https://creativecommons.org/publicdomain/zero/1.0/
