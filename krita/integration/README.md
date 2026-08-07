# LibrePaint's macOS integration

This Xcode project generates Quick Look Thumbnailing and Preview app extensions,
and a Spotlight importer, for `.kra` and `.ora` files.

The Quick Look extensions take the `preview.png` image in the root of the ZIP
container and use it as the thumbnail image, and the `mergedimage.png` file as
the preview image. On files created with older versions of Krita that do not
have `mergedimage.png`, QuickLook will simply fall back to using the thumbnail
image instead.

The Spotlight plugin extracts the following metadata from the kra file, if
available:
- image dimensions, DPI, bit depth, and color space
- color space profile name (not the actual name, as it's not embedded in the container; only the file path)
- layer names
- authors, image title and description

# Building and bundling

The normal macOS CMake build invokes this Xcode project and builds these targets:

- `kritaspotlight`
- `krita-thumbnailer`
- `krita-preview`

The install step stages their products under the plugin directory.
`packaging/macos/macos-deploy.py` then places them in the runnable bundle:

- `kritaspotlight.mdimporter` in
  `LibrePaint.app/Contents/Library/Spotlight`
- `krita-thumbnailer.appex` and `krita-preview.appex` in
  `LibrePaint.app/Contents/PlugIns`

The `krita-*` product names and the `.kra`/`.ora` identifiers are retained for
file-format and system integration compatibility; they are not the application
display name.

# Testing

After building and bundling `LibrePaint.app`, reset the Quick Look cache and
exercise both thumbnail and preview generation:

```sh
qlmanage -r
qlmanage -r cache
qlmanage -d2 -t <path-to-file.kra>
qlmanage -d2 -p <path-to-file.kra>
mdimport -d2 -t <path-to-file.kra>
```

If Finder does not pick up a rebuilt extension immediately, restart Finder.
