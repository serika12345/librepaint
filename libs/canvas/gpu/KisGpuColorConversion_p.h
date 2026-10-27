/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once
#include <KoColorConversionStages.h>
#include <gpu/KisGpuOperationDocument.h>
namespace LibrePaint::Canvas::Gpu::Detail {
bool sameDisplayConversion(const KoColorConversionStages &, const KoColorConversionStages &);
bool displayEncodingMatches(const KoColorConversionStages::Encoding &, const Image::Gpu::Snapshot &);
bool packDisplayConversion(const KoColorConversionStages &, quint64 maximumBytes,
                           QByteArray &, QString &diagnostic);
}
