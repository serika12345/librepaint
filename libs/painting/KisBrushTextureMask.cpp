/*
 * SPDX-FileCopyrightText: 2017 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisBrushTextureMask.h"
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <kis_algebra_2d.h>
#include <QColor>
#include <QTransform>

QImage prepareBrushTextureMask(const QImage &pattern, const KisBrushTextureMaskSettings &settings)
{
    if (pattern.isNull()) return {};
    QImage mask = pattern.convertToFormat(QImage::Format_ARGB32);
    if (!qFuzzyCompare(settings.scale, 0.) && !qFuzzyCompare(settings.scale, 1.)) {
        QTransform transform; transform.scale(settings.scale, settings.scale);
        const QRect bounds = KisAlgebra2D::ensureRectNotSmaller(transform.mapRect(mask.rect()), QSize(2, 2));
        mask = mask.scaled(bounds.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else mask.detach();
    if (mask.isNull()) return {};
    QImage result = settings.preserveAlpha ? mask : QImage(mask.size(), QImage::Format_Alpha8);
    if (result.isNull()) return {};
    const auto *alphaSpace = KoColorSpaceRegistry::instance()->alpha8();
    for (int row = 0; row < mask.height(); ++row) {
        const auto *source = reinterpret_cast<const QRgb *>(mask.constScanLine(row));
        auto *destination = result.scanLine(row);
        for (int col = 0; col < mask.width(); ++col) {
            const auto pixel = source[col];
            float alpha = qAlpha(pixel) / 255.0;
            const int grayValue = (qRed(pixel) * 11 + qGreen(pixel) * 16 + qBlue(pixel) * 5) / 32;
            float maskValue = (grayValue / 255.0) * alpha + (1 - alpha);
            maskValue = maskValue - settings.brightness;
            maskValue = ((maskValue - .5) * settings.contrast) + .5;
            if (maskValue > 1.) maskValue = 1;
            else if (maskValue < 0) maskValue = 0;
            if (settings.invert) maskValue = 1 - maskValue;
            maskValue = qBound(0.f, maskValue, 1.f);
            float adjusted;
            if (settings.neutralPoint == 1 || (settings.neutralPoint != 0 && maskValue <= settings.neutralPoint))
                adjusted = maskValue / (2 * settings.neutralPoint);
            else adjusted = .5 + (maskValue - settings.neutralPoint) / (2 - 2 * settings.neutralPoint);
            const bool outside = adjusted < settings.cutoffLeft / 255.0 || adjusted > settings.cutoffRight / 255.0;
            if (settings.cutoffPolicy == 1 && outside) {
                alpha = 0;
                if (!settings.preserveAlpha) adjusted = alpha;
            } else if (settings.cutoffPolicy == 2 && outside) {
                alpha = 1;
                if (!settings.preserveAlpha) adjusted = alpha;
            }
            if (settings.preserveAlpha) {
                const int value = qRound(adjusted * 255.0);
                reinterpret_cast<QRgb *>(destination)[col] = QColor(value, value, value, qRound(alpha * 255.0)).rgba();
            } else alphaSpace->setOpacity(destination + col, qreal(adjusted), 1);
        }
    }
    return result;
}
