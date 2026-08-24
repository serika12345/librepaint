/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISCOLORSAMPLERSTROKE_H
#define KISCOLORSAMPLERSTROKE_H

#include <QObject>
#include <QScopedPointer>

#include <kis_types.h>
#include <kritapainting_export.h>

class KoColor;
class QPoint;
class KisStrokesFacade;

/**
 * Owns the queued painting work for one active color-sampling stroke.
 *
 * The strokes facade is borrowed from start() until finish(). Sampling source
 * selection and presentation remain responsibilities of the caller.
 */
class KRITAPAINTING_EXPORT KisColorSamplerStroke : public QObject
{
    Q_OBJECT

public:
    explicit KisColorSamplerStroke(QObject *parent = nullptr);
    ~KisColorSamplerStroke() override;

    bool isActive() const;

    void start(KisStrokesFacade *strokesFacade, int radius, int blend);
    void addSample(const KisPaintDeviceSP &device,
                   const QPoint &imagePoint,
                   const KoColor &currentColor);
    void finish();

Q_SIGNALS:
    void sigColorUpdated(const KoColor &color);
    void sigFinalColorSelected(const KoColor &color);

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif // KISCOLORSAMPLERSTROKE_H
