#!/bin/bash
# Launcher script for running the LibrePaint AppImage inside of the Steam Linux Runtime.
# Include alongside LibrePaint.AppImage (probably in `content/linux/`) and
# configure Steamworks to use this as the default/primary launch option.

# Prevent runtime environment or host system libraries from being used over AppImage packaged ones. 
unset LD_LIBRARY_PATH

# UTF-8 Locale support.
LC_ALL=en_US.UTF-8

# Prevents Auto-Updates.
STARTED_BY_STEAM=1

# Launch LibrePaint.
appdir=$(dirname "$0")
$appdir/LibrePaint.AppImage
