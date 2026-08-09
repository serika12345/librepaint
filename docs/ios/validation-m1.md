# M1 validation record

Date: 2026-08-02

## Environment

- Xcode 26.6 (`17F113`)
- iPhoneOS/iPhoneSimulator SDK 26.5
- Apple Clang 21.0.0
- Nix 2.31.3
- Nix-pinned CMake 4.3.4
- Host `aarch64-darwin`

## Commands

```sh
XDG_CACHE_HOME="$PWD/.cache/nix" nix flake check "path:$PWD" --no-build
XDG_CACHE_HOME="$PWD/.cache/nix" nix develop "path:$PWD" --command \
    packaging/ios/scripts/build-smoke.sh device
XDG_CACHE_HOME="$PWD/.cache/nix" nix develop "path:$PWD" --command \
    packaging/ios/scripts/build-smoke.sh simulator
```

At validation time the new flake existed in the working tree, so the explicit
`path:` form included it. The committed workflow uses plain `nix develop`.

## Results

| Target | Result | Mach-O metadata |
|---|---|---|
| Device | pass | arm64, `platform IOS`, minOS 17.0, SDK 26.5 |
| Simulator | pass | arm64, `platform IOSSIMULATOR`, minOS 17.0, SDK 26.5 |

Both UIKit `.app` bundles compiled, linked, and passed Xcode bundle validation.
The artifacts are unsigned; the signing and installation gate begins at M5.
