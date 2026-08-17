/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisImportExportDialogs.h"

#include <QDialog>
#include <QApplication>
#include <QCursor>
#include <QStack>

#include <KoDialog.h>

#include "kis_dlg_missing_color_profile.h"
#include "kis_dlg_paste_format.h"
#include "kis_dlg_png_import.h"

namespace
{
class CursorOverrideRestorer
{
public:
    CursorOverrideRestorer()
    {
        while (qApp->overrideCursor()) {
            m_cursors.push(*qApp->overrideCursor());
            qApp->restoreOverrideCursor();
        }
    }

    ~CursorOverrideRestorer()
    {
        while (!m_cursors.isEmpty()) {
            qApp->setOverrideCursor(m_cursors.pop());
        }
    }

private:
    QStack<QCursor> m_cursors;
};
}

std::optional<int> KisImportExportDialogs::choosePasteSource(QWidget *parent,
                                                             bool remoteAvailable,
                                                             bool localAvailable,
                                                             bool bitmapAvailable,
                                                             bool *remember)
{
    KisDlgPasteFormat dialog(parent);
    dialog.setSourceAvailable(KisClipboard::PASTE_FORMAT_DOWNLOAD, remoteAvailable);
    dialog.setSourceAvailable(KisClipboard::PASTE_FORMAT_LOCAL, localAvailable);
    dialog.setSourceAvailable(KisClipboard::PASTE_FORMAT_CLIP, bitmapAvailable);

    if (dialog.exec() != KoDialog::Accepted) {
        return std::nullopt;
    }

    *remember = dialog.remember();
    return static_cast<int>(dialog.source());
}

std::optional<int> KisImportExportDialogs::chooseClipboardColorSource(QWidget *parent,
                                                                      bool *remember)
{
    KisDlgMissingColorProfile dialog(parent);
    if (dialog.exec() != QDialog::Accepted) {
        return std::nullopt;
    }

    *remember = dialog.remember();
    return static_cast<int>(dialog.source());
}

QString KisImportExportDialogs::choosePngProfile(const QString &path,
                                                  const QString &colorModel,
                                                  const QString &colorDepth)
{
    KisDlgPngImport dialog(path, colorModel, colorDepth);
    CursorOverrideRestorer cursorOverride;
    Q_UNUSED(cursorOverride);
    dialog.exec();
    return dialog.profile();
}
