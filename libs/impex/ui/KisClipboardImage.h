/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_CLIPBOARD_IMAGE_H
#define KIS_CLIPBOARD_IMAGE_H

#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QMimeData>
#include <QString>

/**
 * Formatting of image data for the system clipboard.
 *
 * Qt's Windows backend offers CF_DIB together with CF_PNG and CF_DIBV5 for
 * every QImage that has an alpha channel. Chrome and Edge show a CF_DIB with
 * alpha as a corrupted image (KDE bug 449624 and 464839), so an image with
 * alpha is exported as the native PNG clipboard format instead. Opaque images
 * keep using QMimeData::setImageData(), because CF_DIB is correct for them
 * and is read by more applications than PNG.
 */
namespace KisClipboardImage
{

inline QString pngClipboardFormat()
{
    return QStringLiteral("application/x-qt-windows-mime;value=\"PNG\"");
}

inline QString pngMimeType()
{
    return QStringLiteral("image/png");
}

inline bool needsAlphaPreservingPng(const QImage &image)
{
    return !image.isNull() && image.hasAlphaChannel();
}

/**
 * Put @p image on @p mimeData. @p windowsHost selects the Windows format
 * rules; production code derives it from the target platform, and the
 * contract test exercises both branches on one host.
 */
inline void setImage(QMimeData *mimeData, const QImage &image, bool windowsHost)
{
    if (!mimeData || image.isNull()) {
        return;
    }

#if defined(Q_OS_WIN)
    Q_UNUSED(windowsHost);
    const bool usePng = needsAlphaPreservingPng(image);
#else
    const bool usePng = windowsHost && needsAlphaPreservingPng(image);
#endif

    if (!usePng) {
        mimeData->setImageData(image);
        return;
    }

    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG") || png.isEmpty()) {
        // Falling back to QImage keeps the copy working even if the PNG
        // encoder is unavailable; the Windows corruption is then still
        // possible, but losing the clipboard is worse.
        mimeData->setImageData(image);
        return;
    }

    mimeData->setData(pngClipboardFormat(), png);
    mimeData->setData(pngMimeType(), png);
}

} // namespace KisClipboardImage

#endif // KIS_CLIPBOARD_IMAGE_H
