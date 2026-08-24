/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_PAINTING_INFORMATION_BUILDER_H
#define KIS_PAINTING_INFORMATION_BUILDER_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QVector>

#include <brushengine/kis_paint_information.h>
#include <kritatools_export.h>

class KoCanvasResourceProvider;
class KoPointerEvent;
class KisSpeedSmoother;

struct KRITATOOLS_EXPORT KisPaintingInformationSettings
{
    QString pressureCurve {QStringLiteral("0,0;1,1;")};
    int maximumSpeed {30};
    qreal tiltDirectionOffset {0.0};
    bool useEventTimestamps {false};
    int speedSmoothingSamples {3};
};

class KRITATOOLS_EXPORT KisPaintingInformationBuilder : public QObject
{
    Q_OBJECT

public:
    KisPaintingInformationBuilder();
    ~KisPaintingInformationBuilder() override;

    void setSettings(const KisPaintingInformationSettings &settings);

    KisPaintInformation startStroke(KoPointerEvent *event,
                                    int timeElapsed,
                                    const KoCanvasResourceProvider *manager);

    KisPaintInformation continueStroke(KoPointerEvent *event,
                                       int timeElapsed);

    KisPaintInformation hover(const QPointF &imagePoint,
                              const KoPointerEvent *event,
                              bool isStrokeStarted);

    qreal pressureToCurve(qreal pressure);
    void reset();

protected:
    virtual QPointF adjustDocumentPoint(const QPointF &point,
                                        const QPointF &startPoint);
    virtual QPointF documentToImage(const QPointF &point);
    virtual QPointF imageToDocument(const QPointF &point);
    virtual QPointF imageToView(const QPointF &point);
    virtual qreal calculatePerspective(const QPointF &documentPoint);

    virtual qreal canvasRotation() const;
    virtual bool canvasMirroredX() const;
    virtual bool canvasMirroredY() const;

private:
    KisPaintInformation createPaintingInformation(KoPointerEvent *event,
                                                  int timeElapsed);

    static const int PressureResolution;

private:
    QVector<qreal> m_pressureSamples;
    QPointF m_startPoint;
    QScopedPointer<KisSpeedSmoother> m_speedSmoother;
    bool m_pressureDisabled {false};
    int m_maximumSpeed {30};
    qreal m_tiltDirectionOffset {0.0};
};

#endif
