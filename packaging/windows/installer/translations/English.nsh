;
;  SPDX-License-Identifier: GPL-3.0-or-later
;

!define CURRENT_LANG ${LANG_ENGLISH}

# Strings to show in the installation log:
LangString RemovingShellEx ${CURRENT_LANG} "Removing LibrePaint Shell Integration..."
LangString RemoveShellExFailed ${CURRENT_LANG} "Failed to remove LibrePaint Shell Integration."
LangString RemoveShellExDone ${CURRENT_LANG} "LibrePaint Shell Integration removed."
LangString RemovingOldVer ${CURRENT_LANG} "Removing previous version..."
LangString RemoveOldVerFailed ${CURRENT_LANG} "Failed to remove previous version of Krita."
LangString RemoveOldVerDone ${CURRENT_LANG} "Previous version removed."

# Strings for the component selection dialog:
LangString SectionRemoveOldVer ${CURRENT_LANG} "Remove Old Version"
LangString SectionRemoveOldVerDesc ${CURRENT_LANG} "Remove previously installed Krita $KritaNsisVersion ($KritaNsisBitness-bit)."
LangString SectionShellEx ${CURRENT_LANG} "Shell Integration"
LangString SectionShellExDesc ${CURRENT_LANG} "Shell Extension component to provide thumbnails and file properties display for LibrePaint files.$\r$\n$\r$\nVersion: ${KRITASHELLEX_VERSION}"
LangString SectionMainDesc ${CURRENT_LANG} "${KRITA_PRODUCTNAME} ${KRITA_VERSION_DISPLAY}$\r$\n$\r$\nVersion: ${KRITA_VERSION}"

# Main dialog strings:
LangString SetupLangPrompt ${CURRENT_LANG} "Choose the language to be used for the setup process:"
LangString ShellExLicensePageHeader ${CURRENT_LANG} "License Agreement (LibrePaint Shell Extension)"
LangString ConfirmInstallPageHeader ${CURRENT_LANG} "Confirm Installation"
LangString ConfirmInstallPageDesc ${CURRENT_LANG} "Confirm installation of ${KRITA_PRODUCTNAME} ${KRITA_VERSION_DISPLAY}."
LangString DesktopIconPageDesc2 ${CURRENT_LANG} "You can choose whether to create a shortcut icon on the desktop for launching LibrePaint:"
LangString DesktopIconPageCheckbox ${CURRENT_LANG} "Create a desktop icon"
LangString ConfirmInstallPageDesc2 ${CURRENT_LANG} "Setup is ready to install ${KRITA_PRODUCTNAME} ${KRITA_VERSION_DISPLAY}. You may go back to review the install options before you continue.$\r$\n$\r$\n$_CLICK"

# Misc. message prompts:
LangString MsgRequireWin7 ${CURRENT_LANG} "${KRITA_PRODUCTNAME} ${KRITA_VERSION_DISPLAY} requires Windows 7 or above."
LangString Msg64bitOn32bit ${CURRENT_LANG} "You are running 32-bit Windows, but this installer installs LibrePaint 64-bit, which can only be installed on 64-bit Windows. Please obtain the 32-bit LibrePaint build."
LangString Msg32bitOn64bit ${CURRENT_LANG} "You are trying to install 32-bit LibrePaint on 64-bit Windows. You are strongly recommended to install the 64-bit LibrePaint build instead since it offers better performance.$\n$\nDo you still wish to install 32-bit LibrePaint?"
# These prompts are used for when Krita 2.9 or earlier, or the 3.0 alpha 1 MSI version is installed.
LangString MsgAncientVerMustBeRemoved ${CURRENT_LANG} "An ancient version of Krita is detected. This program will now attempt to remove any old versions of Krita.$\nDo you wish to continue?"
LangString MsgKrita3alpha1RemoveFailed ${CURRENT_LANG} "Failed to remove Krita 3.0 Alpha 1."
LangString MsgKrita2msi32bitRemoveFailed ${CURRENT_LANG} "Failed to remove old Krita (32-bit)."
LangString MsgKrita2msi64bitRemoveFailed ${CURRENT_LANG} "Failed to remove old Krita (64-bit)."
#
LangString MsgKritaSameVerReinstall ${CURRENT_LANG} "It appears that ${KRITA_PRODUCTNAME} ${KRITA_VERSION_DISPLAY} is already installed.$\nThis setup will reinstall it."
LangString MsgKrita3264bitSwap ${CURRENT_LANG} "It appears that Krita $KritaNsisBitness-bit ($KritaNsisVersion) is currently installed. This setup will replace it with LibrePaint ${KRITA_INSTALLER_BITNESS}-bit ${KRITA_VERSION_DISPLAY}."
LangString MsgKritaNewerAlreadyInstalled ${CURRENT_LANG} "It appears that a newer version of Krita $KritaNsisBitness-bit ($KritaNsisVersion) is currently installed. If you want to install LibrePaint ${KRITA_VERSION_DISPLAY}, please uninstall the newer version manually before running this setup."
LangString MsgKritaRunning ${CURRENT_LANG} "LibrePaint appears to be running. Please close LibrePaint before running this installer."
LangString MsgUninstallKritaRunning ${CURRENT_LANG} "LibrePaint appears to be running. Please close LibrePaint before uninstalling."

!undef CURRENT_LANG
