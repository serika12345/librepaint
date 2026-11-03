/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_KRA_PREVIEW_P_H
#define KIS_KRA_PREVIEW_P_H

#include <QImage>

namespace LibrePaint::ImportExport::Kra::Detail {
QImage makePreview(const QImage &projection);
}

#endif
