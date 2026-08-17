/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_qpainter_projection_factory.h"

#include <kis_image_config.h>

#include "kis_image_pyramid.h"

std::unique_ptr<KisProjectionBackend> createQPainterProjectionBackend()
{
    // A single plane is intentional: higher planes are not kept current when
    // layer visibility changes.
    return std::make_unique<KisImagePyramid>(1);
}

QSize qPainterProjectionUpdatePatchSize()
{
    KisImageConfig config(true);
    return QSize(config.updatePatchWidth(), config.updatePatchHeight());
}
