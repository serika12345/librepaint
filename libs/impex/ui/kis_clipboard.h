/*
 *  kis_clipboard.h - part of Krayon
 *
 *  SPDX-FileCopyrightText: 2004 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2019 Dmitrii Utkin <loentar@gmail.com>
 *  SPDX-FileCopyrightText: 2022 L. E. Segovia <amy@amyspark.me>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef __KIS_CLIPBOARD_H_
#define __KIS_CLIPBOARD_H_

#include <QObject>
#include <QRect>
#include <QUuid>
#include <QSize>
#include <optional>

#include <kis_types.h>

#include <kritaui_export.h>
#include <QImage>

class QRect;
class QMimeData;
class KisTimeSpan;
class KoColorSpace;

struct GpuClipboardMask
{
    QRect rect;
    bool ellipse = false;
    bool keepInside = false;
    QImage coverage;
};

class KisClipboardPrivate;

/**
 * The Krita clipboard is a clipboard that can store paint devices
 * instead of just qimage's.
 */
class KRITAUI_EXPORT KisClipboard : public QObject
{

    Q_OBJECT
    Q_PROPERTY(bool clip READ hasClip NOTIFY clipChanged)

public:
    enum PasteBehaviour { PASTE_ASSUME_WEB = 0, PASTE_ASSUME_MONITOR, PASTE_ASK };

    enum PasteFormatBehaviour { PASTE_FORMAT_ASK = 0, PASTE_FORMAT_DOWNLOAD, PASTE_FORMAT_LOCAL, PASTE_FORMAT_CLIP };

    KisClipboard();
    ~KisClipboard() override;

    static KisClipboard* instance();

    /**
     * Sets the clipboard to the contents of the specified paint device; also
     * set the system clipboard to a QImage representation of the specified
     * paint device.
     *
     * @param dev The paint device that will be stored on the clipboard
     * @param topLeft a hint about the place where the clip should be pasted by default
     */
    void setClip(KisPaintDeviceSP dev, const QPoint& topLeft);

    void setClip(KisPaintDeviceSP dev, const QPoint& topLeft, const KisTimeSpan &range);

    /**
     * Get the contents of the clipboard in the form of a paint device.
     */
    KisPaintDeviceSP clip(const QRect &imageBounds,
                          bool showPopup,
                          int overridePasteBehaviour = -1,
                          KisTimeSpan *clipRange = nullptr) const;

    /**
     * Given the mimedata for a paste event, ask user which source
     * they'll want to use for the image.
     *
     * @return A pair of bool and PasteFormatBehaviour:
     *   The boolean indicates acceptance of the paste event.
     *   If set, the PasteFormatBehaviour member indicates the chosen source.
     */
    QPair<bool, PasteFormatBehaviour> askUserForSource(const QMimeData *data,
                                                       bool useClipboardFallback = false) const;

    //A function to allow for caching the clipboard data
    QPair<bool, PasteFormatBehaviour> askUserForSourceWithData(QImage qimage,
                                                               const QList<QUrl> urls,
                                                               bool useClipboardFallback = false) const;
    /**
     * Get the contents of the specified mimedata buffer in the form of a paint device.
     */
    KisPaintDeviceSP clipFromMimeData(const QMimeData *data,
                                      const QRect &imageBounds,
                                      bool showPopup,
                                      int overridePasteBehaviour = -1,
                                      KisTimeSpan *clipRange = nullptr,
                                      bool useClipboardFallback = false) const;

    KisPaintDeviceSP clipFromKritaLayers(const KoColorSpace *cs) const;

    KisPaintDeviceSP clipFromBoardContents(const QMimeData *data,
                          const QRect &imageBounds,
                          bool showPopup,
                          int overridePasteBehaviour = -1,
                          bool useClipboardFallback = false,
                          QPair<bool, PasteFormatBehaviour> source = {
                              false,
                              PasteFormatBehaviour::PASTE_FORMAT_ASK}) const;

    //A function to allow for caching the clipboard data
    KisPaintDeviceSP clipFromBoardContentsWithData(
        QImage image,
        const QList<QUrl> urls,
        const QRect &imageBounds,
        bool showPopup,
        int overridePasteBehaviour = -1,
        bool useClipboardFallback = false,
        QPair<bool, PasteFormatBehaviour> source = {false, PasteFormatBehaviour::PASTE_FORMAT_ASK}) const;

    QImage getImageWithFallback(const QMimeData *cbData, bool useClipboardFallback = false) const;

    bool hasClip() const;

    QSize clipSize() const;

    /** Captures layers before replacing clipboard data. Returns false on
     * capture refusal, preserving the previous clipboard contents.
     */
    bool setLayers(KisNodeList nodes, KisImageSP image, bool forceCopy = false);
    /** Captures the committed GPU canvas as one independent paint layer.
     * Requires the GUI/image thread. The workspace budget covers packed source
     * pixels, padded raster tiles and their checkpoint encoding. Refusal keeps
     * the previous clipboard and source state; the copy retains execution.
     */
    bool setGpuProjection(KisImageSP image, qsizetype memoryBudgetBytes = 256 * 1024 * 1024, QRect region = {},
                          std::optional<GpuClipboardMask> mask = {});
    /** Captures one GPU layer's pixels in region; false keeps the previous clipboard. */
    bool setGpuLayer(KisImageSP image, const QUuid &layer, qsizetype memoryBudgetBytes = 256 * 1024 * 1024,
                     QRect region = {}, std::optional<GpuClipboardMask> mask = {});
    bool hasLayers() const;
    bool hasLayerStyles() const;

    const QMimeData* layersMimeData() const;

    bool hasUrls() const;
    bool hasImage() const;

Q_SIGNALS:
    void clipChanged();

private Q_SLOTS:
    void clipboardDataChanged();

private:
    Q_DISABLE_COPY(KisClipboard);

    KisPaintDeviceSP
    clipFromKritaSelection(const QMimeData *data, const QRect &imageBounds, KisTimeSpan *clipRange) const;

    KisPaintDeviceSP fetchImageByURL(const QUrl &originalUrl) const;

    QImage getImageFromMimeData(const QMimeData *cbData) const;

    KisClipboardPrivate *const d;
};

#endif // __KIS_CLIPBOARD_H_
