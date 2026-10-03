# LibrePaint

[日本語版](README.ja.md)

LibrePaint is an independently maintained, cross-platform digital painting application derived from Krita. Its goal is to evolve the codebase as a distinct product—including shared application behavior, UI, workflows, branding, packaging, and platform integrations—and make LibrePaint available across desktop and mobile platforms.

## Project Direction

| Item | Policy |
| --- | --- |
| Product scope | Develop and maintain LibrePaint as a complete application with shared and platform-specific components |
| Platform goal | Windows, macOS, Linux, Android (including Android on ChromeOS), iOS/iPadOS, and additional targets that the Krita/Qt codebase can support |
| Shared development | Implement features and UX in shared code when appropriate, with focused platform integrations where required |
| Compatibility | Preserve artwork, resources, and stable technical identifiers deliberately; make incompatible migrations explicit |
| Distribution goal | Establish a build, package, validation, and delivery path appropriate to each platform |

Release-specific downloads, installation requirements, and changes are documented in [GitHub Releases](https://github.com/serika12345/librepaint/releases).

## Documentation

The [development manual (Japanese)](docs/architecture/DEVELOPMENT.md) covers environment setup, platform builds, testing, and maintenance.

## License and Upstream

LibrePaint is a derivative work based on [Krita](https://krita.org/) and is distributed under the GNU General Public License Version 3. Individual files and bundled components carry their respective compatible licenses. See [`COPYING`](COPYING) and the license notices in individual files for the exact terms.

Krita is developed by the Krita Foundation, KDE, and Krita contributors. LibrePaint is maintained independently by LibrePaint contributors. See the upstream [graphics/krita](https://invent.kde.org/graphics/krita) repository for the original project and its history.
