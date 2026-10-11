/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_DISPLAY_COLOR_H
#define KIS_GPU_DISPLAY_COLOR_H
#include <KoColorConversionTransformation.h>
#include <QObject>
#include <memory>
class KisGpuDocumentWindow;
class KoColorSpace;

/** GUI-thread display mapping for one editing window. Registry-owned color spaces
 * outlive this owner. One cancellable worker and one latest request bound CPU work.
 * Destruction cancels and joins the worker before releasing its values.
 */
class KisGpuDisplayColor : public QObject {
    Q_OBJECT
public:
    KisGpuDisplayColor(KisGpuDocumentWindow &window, const KoColorSpace *source);
    ~KisGpuDisplayColor() override;
    bool request(const KoColorSpace *destination, KoColorConversionTransformation::Intent intent,
                 KoColorConversionTransformation::ConversionFlags flags);
    bool busy() const;
Q_SIGNALS:
    void ready();
    void failed();
private:
    void startNext();
    struct Private;
    std::unique_ptr<Private> d;
};
#endif
