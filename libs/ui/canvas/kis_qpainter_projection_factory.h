/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_QPAINTER_PROJECTION_FACTORY_H
#define KIS_QPAINTER_PROJECTION_FACTORY_H

#include <memory>

#include <QSize>

#include <kritaui_export.h>
#include <kis_projection_backend.h>

KRITAUI_EXPORT std::unique_ptr<KisProjectionBackend> createQPainterProjectionBackend();
KRITAUI_EXPORT QSize qPainterProjectionUpdatePatchSize();

#endif
