# Translation extraction scripts

`Messages.sh` generates the application message catalog. `ExtraDesktop.sh`
lists the desktop-entry-style `.tag` files that provide translated paint-preset
tag names. Both scripts resolve the repository root before processing files, so
they can be run from any working directory.

The other files in this directory are helpers invoked by `Messages.sh`.
