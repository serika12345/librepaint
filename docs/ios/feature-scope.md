# Initial feature scope

## P0: required for the first useful build

- New/open/save/save-as for KRA, ORA, PNG, and JPEG.
- Pixel brush and eraser with presets.
- Freehand/basic shape, selection, move, transform, crop, fill, and gradient tools.
- Layer operations, undo/redo, canvas pan/zoom/rotate.
- Layer, Brush Presets, Tool Options, Advanced Color Selector, and touch UI.
- Canvas-only touch UI with a vertically scrolling named brush list and an
  in-window color panel anchored below a drawing toolbar that is flush with
  the safe area's top and side edges. The document presentation switches to a
  maximized frameless and tabless view in canvas-only mode and restores its
  window flags and tabbed view in the normal interface.
- Apple Pencil pressure and separation of Pencil drawing from finger gestures.
- A quick inward two-finger pinch snaps canvas rotation to the nearest quarter
  turn and eases rotation, zoom, and centering into the visible viewport below
  the drawing toolbar on release. Hiding the controls restores the full canvas
  viewport. Ordinary pinch and pinch-twist continue to zoom and rotate
  continuously, and a new touch interrupts the fit animation.
- Apple Pencil double tap follows the iPadOS preference for switching to the
  eraser or the previously used preset. The interaction remains owned by the
  main UIKit window across foreground transitions and auxiliary keyboard
  windows used by touch UI controls.
- Files document picker, autosave recovery, background/foreground restoration.

## P1: after the first useful build

- Additional Krita brush engines and common filters.
- Resource bundle import/export.
- Split View, Stage Manager, external keyboard, and external display polish.
- WebP/TIFF when their dependency cost is acceptable.

## Deferred from the initial profile

- Animation editing UI; audio/video export remains a later backend project.
- OpenEXR, HEIF, JPEG XL, RAW, PDF, and uncommon import/export formats.
- Optional dockers, generators, assistants, and specialized filters.

## Outside the initial port baseline

- Python/PyQt scripting and downloadable executable plugins.
- G'MIC.
- Printing and Qt PrintSupport.
- FFmpeg/MLT/SDL audio/video processing and external process execution.
- Built-in updater and desktop integration.
- App Store/marketplace distribution, notarization, and iPhone layouts.

## Android adaptations to reuse or generalize

Generalization in this table is upstream-facing follow-up work. Current local
reuse follows the one-line guard policy in `android-reuse-audit.md`.

| Area | Existing code | iPadOS action |
|---|---|---|
| Touch scrolling/long press | `KisKineticScroller`, `KisLongPressEventFilter` | Generalize behind a mobile/touch capability |
| Compact main window | `KisMainWindow`, `KisWelcomePageWidget` | Extract shared tablet policy |
| File dialog behavior | `KoFileDialog`, recent files | Reuse UX policy; replace Android URI handling with UIDocumentPicker |
| Resource locations | `KoResourcePaths`, Android asset setup | Reuse container-safe assumptions; implement Apple bundle paths |
| High-DPI/canvas setup | Android branches in UI/OpenGL | Validate and move shared parts behind a mobile feature flag |
| Text input | Android SVG text tool branches | Reuse intent; implement iOS keyboard bridge separately |
| Logging/crash handling | Android log handlers | Implement a native OSLog bridge |
| Recorder/video | extensive Android specialization | Defer until an iOS backend is selected |
