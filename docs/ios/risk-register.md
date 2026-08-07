# iPadOS port risk register

| ID | Area | Risk | Probability | Impact | Gate/Mitigation |
|---|---|---|---|---|---|
| R1 | Dependencies | Required KDE Frameworks fail to cross-compile or assume desktop APIs | High | Critical | G1; package each framework separately and patch at its boundary |
| R2 | Qt | Qt Widgets/OpenGL behavior differs on iPadOS | Medium | Critical | G1/G3; prove smoke, then minimal canvas before broad porting |
| R3 | Plugins | KPlugin factories/resources are removed or collide under static linking | High | Critical | G2; generated registry, whole-archive only where required |
| R4 | Platform split | Existing `APPLE` branches compile macOS-only code for iOS | High | High | Replace with `APPLE AND NOT IOS`; audit all 33 build branches |
| R5 | Unsupported APIs | PrintSupport, QProcess, filesystem, updater, or desktop integration leaks into link | High | High | Explicit exclusions and link-time checks |
| R6 | Input | Qt tablet events omit Pencil pressure/tilt or conflict with touch | Medium | Critical | G3; native UIKit event bridge fallback |
| R7 | GPU | Context loss or deprecated rendering path causes black canvas | Medium | Critical | Minimal canvas test; lifecycle recreation tests |
| R8 | Memory | iPadOS terminates large documents under pressure | High | High | Device-specific cache limits and memory-warning handling |
| R9 | Files | Security-scoped URLs or cloud coordination cause data loss | Medium | Critical | UIDocumentPicker bridge, bookmarks, autosave/recovery tests |
| R10 | Reproducibility | Nix cache hides an Xcode/SDK mismatch | Medium | High | Version check before build; include validated matrix in artifacts/logs |
| R11 | Regression | Shared CMake/mobile changes break macOS or Android | Medium | High | Keep platform feature flags narrow and run local platform build/test checks |

## Gate exit criteria

- **G1:** Qt/KF and required C/C++ libraries link into an iOS arm64 binary.
- **G2:** Selected Krita factories and resources load from static libraries.
- **G3:** A real iPad produces stable Pencil strokes and survives canvas lifecycle changes.
