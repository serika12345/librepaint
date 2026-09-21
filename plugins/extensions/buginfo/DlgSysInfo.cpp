/*
 * SPDX-FileCopyrightText: 2017 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DlgSysInfo.h"
#include "dlg_buginfo.h"
#include "ui_wdg_buginfo.h"
#include <QStandardPaths>
#include <qhashfunctions.h>
#include <qwidget.h>

DlgSysInfo::DlgSysInfo(QWidget *parent)
    : DlgBugInfo(parent)
{
    initialize();
}

QString DlgSysInfo::originalFileName()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/librepaint-sysinfo.log";
}

QString DlgSysInfo::captionText()
{
    return i18nc("Caption of the dialog with system information for bug reports", "LibrePaint System Information: please paste this information to the bug report");
}

QString DlgSysInfo::replacementWarningText()
{
    return "WARNING: The system information file doesn't exist.";
}

DlgSysInfo::~DlgSysInfo()
{
}
