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
class QString;

/**
 * Owns CPU/GPU sampling requests, retained pixel versions and completion for
 * one color-sampling stroke. Input coordinates, source-node selection and
 * presentation belong to the caller. All entry points use the image thread.
 */
class KRITAPAINTING_EXPORT KisColorSamplerStroke : public QObject
{
    Q_OBJECT

public:
    explicit KisColorSamplerStroke(QObject *parent = nullptr);
    ~KisColorSamplerStroke() override;

    bool isActive() const;

    bool start(const KisImageSP &image, int radius, int blend);
    /** A null node selects the image projection. */
    void addSample(const KisNodeSP &node,
                   const QPoint &imagePoint,
                   const KoColor &currentColor);
    /** Stop accepting samples; queued work may still deliver updates and a final color. */
    void finish();
    /** Suppress old notifications; submitted GPU reads retain resources until completion. */
    void cancel();
    /** Already resolved reference-image priority sample. */
    void addReferenceColor(const KoColor &color);

Q_SIGNALS:
    void sigColorUpdated(const KoColor &color);
    void sigFinalColorSelected(const KoColor &color);
    void sigSamplingFailed(const QString &diagnostic);

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif // KISCOLORSAMPLERSTROKE_H
