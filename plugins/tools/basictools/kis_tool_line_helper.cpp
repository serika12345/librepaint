/*
 *  SPDX-FileCopyrightText: 2014 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tool_line_helper.h"

#include <QtMath>

#include "KoCanvasResourcesIds.h"
#include "kis_algebra_2d.h"
#include "kis_assert.h"
#include "kis_global.h"
#include "kis_image_interfaces.h"
#include "kis_paint_information.h"
#include "kis_painting_information_builder.h"
#include "kis_image.h"
#include <gpu/KisGpuPaintImage.h>
#include <KLocalizedString>
#include "kis_smoothing_options.h"
#include "kis_tool_freehand_helper.h"
#include "kis_types.h"

#include <KoCanvasResourceProvider.h>
#include <KoToolBase.h>
#include <QPointer>
#include <brushengine/kis_paintop_preset.h>
#include <cmath>
#include <cstdlib>
#include <qcontainerfwd.h>
#include <qnumeric.h>
#include <qpoint.h>
#include <QtGlobal>

struct KisToolLineHelper::Private
{
    Private(KisPaintingInformationBuilder *_infoBuilder)
        : infoBuilder(_infoBuilder),
          useSensors(true),
          enabled(true)
    {
    }

    QVector<KisPaintInformation> linePoints;
    KisPaintingInformationBuilder *infoBuilder;
    bool useSensors;
    bool enabled;
};

KisToolLineHelper::KisToolLineHelper(KisPaintingInformationBuilder *infoBuilder,
                                     KoCanvasResourceProvider *resourceManager,
                                     const KUndo2MagicString &transactionText,
                                     KoToolBase *errorReceiver)
    : KisToolFreehandHelper(infoBuilder,
                            resourceManager,
                            transactionText,
                            new KisSmoothingOptions(false)),
      m_d(new Private(infoBuilder))
{
    connect(this, &KisToolFreehandHelper::requestErrorMessage,
            errorReceiver, &KoToolBase::statusTextChanged);
}

KisToolLineHelper::~KisToolLineHelper()
{
    delete m_d;
}

void KisToolLineHelper::setEnabled(bool value)
{
    m_d->enabled = value;
}

void KisToolLineHelper::setUseSensors(bool value)
{
    m_d->useSensors = value;
}

void KisToolLineHelper::repaintLine(KisImageWSP image, KisNodeSP node,
                                    KisStrokesFacade *strokesFacade)
{
    if (!m_d->enabled) return;

    const QPointer<KisToolFreehandHelper> guard(this);
    cancelPaint();
    if (!guard) return;
    if (m_d->linePoints.isEmpty()) return;

    qreal startAngle = 0.0;
    if (m_d->linePoints.length() > 1) {
        startAngle = KisAlgebra2D::directionBetweenPoints(m_d->linePoints[0].pos(),
                                                          m_d->linePoints[1].pos(),
                                                          0.0);
    }

    KisPaintOpPresetSP preset = resourceManager()->resource(KoCanvasResource::CurrentPaintOpPreset)
            .value<KisPaintOpPresetSP>();

    if (preset->settings()->paintOpSize() <= 1) {
        KisPaintInformation begin = m_d->linePoints.first();
        KisPaintInformation end = m_d->linePoints.last();
        m_d->linePoints.clear();
        m_d->linePoints.append(begin);
        m_d->linePoints.append(end);
        adjustPointsToDDA(m_d->linePoints);
    }

    QVector<KisPaintInformation>::const_iterator it = m_d->linePoints.constBegin();
    QVector<KisPaintInformation>::const_iterator end = m_d->linePoints.constEnd();

    initPaintImpl(startAngle, *it, resourceManager(), image, node, strokesFacade);
    if (!guard || !isRunning()) return;
    ++it;

    while (it != end) {
        paintLine(*(it - 1), *it);
        if (!guard || !isRunning()) return;
        ++it;
    }
    if (const auto owner = image->gpuImage()) {
        namespace Gpu = LibrePaint::Image::Gpu;
        const auto result = owner->updateShape(owner->operations().activeOperation(),
            {Gpu::ShapeKind::Line, m_d->linePoints.first().pos(), m_d->linePoints.last().pos()});
        if (!guard) return;
        if (result != Gpu::Result::Success) {
            cancelPaint();
            if (guard) Q_EMIT requestErrorMessage(i18n("The line could not be drawn and was cancelled."));
        }
    }
}

void KisToolLineHelper::start(KoPointerEvent *event, KoCanvasResourceProvider *resourceManager)
{
    if (!m_d->enabled) return;

    // Ignore the elapsed stroke time, so that the line tool will behave as if the whole stroke is
    // drawn at once. This should prevent any possible spurious dabs caused by airbrushing features.
    KisPaintInformation pi =
            m_d->infoBuilder->startStroke(event, 0, resourceManager);

    if (!m_d->useSensors) {
        pi = KisPaintInformation(pi.pos());
    }

    m_d->linePoints.append(pi);
}

void KisToolLineHelper::addPoint(KoPointerEvent *event, const QPointF &overridePos)
{
    if (!m_d->enabled) return;

    // Ignore the elapsed stroke time, so that the line tool will behave as if the whole stroke is
    // drawn at once. This should prevent any possible spurious dabs caused by airbrushing features.
    KisPaintInformation pi =
            m_d->infoBuilder->continueStroke(event, 0);

    addPoint(pi, overridePos);
}

void KisToolLineHelper::addPoint(KisPaintInformation pi, const QPointF &overridePos)
{
    if (!m_d->enabled) return;
    if (!m_d->useSensors) {
        pi = KisPaintInformation(pi.pos());
    }

    if (!overridePos.isNull()) {
        pi.setPos(overridePos);
    }

    if (m_d->linePoints.size() > 1) {
        const QPointF startPos = m_d->linePoints.first().pos();
        const QPointF endPos = pi.pos();

        if (!KisAlgebra2D::fuzzyPointCompare(startPos, endPos)) {
            const qreal maxDistance = kisDistance(startPos, endPos);
            const QPointF unit = (endPos - startPos) / maxDistance;

            QVector<KisPaintInformation>::iterator it = m_d->linePoints.begin();
            ++it;
            while (it != m_d->linePoints.end()) {
                qreal dist = kisDistance(startPos, it->pos());
                if (dist < maxDistance) {
                    QPointF pos = startPos + unit * dist;
                    it->setPos(pos);
                    ++it;
                } else {
                    it = m_d->linePoints.erase(it);
                }
            }
        } else {
            m_d->linePoints.clear();
        }
    }

    m_d->linePoints.append(pi);

}

void KisToolLineHelper::translatePoints(const QPointF &offset)
{
    if (!m_d->enabled) return;

    QVector<KisPaintInformation>::iterator it = m_d->linePoints.begin();
    while (it != m_d->linePoints.end()) {
        it->setPos(it->pos() + offset);
        ++it;
    }
}

void KisToolLineHelper::movePointsTo(const QPointF &startPoint, const QPointF &endPoint)
{
    if (m_d->linePoints.size() <= 1 ) {
        return;
    }

    if (KisAlgebra2D::fuzzyPointCompare(startPoint, endPoint)) {
        return;
    }

    if (m_d->linePoints.size() > 1) {
        const qreal maxDistance = kisDistance(startPoint, endPoint);
        const QPointF unit = (endPoint - startPoint) / maxDistance;

        QVector<KisPaintInformation>::iterator it = m_d->linePoints.begin();
        ++it;
        while (it != m_d->linePoints.end()) {
            qreal dist = kisDistance(startPoint, it->pos());
            QPointF pos = startPoint + unit * dist;
            it->setPos(pos);
            ++it;
        }
    }
}

void KisToolLineHelper::end()
{
    if (!m_d->enabled) return;
    if (!isRunning()) {
        clearPoints();
        return;
    }

    const QPointer<KisToolFreehandHelper> guard(this);
    endPaint();
    if (!guard) return;
    clearPoints();
}


void KisToolLineHelper::cancel()
{
    if (!m_d->enabled) return;
    KIS_ASSERT_RECOVER_RETURN(isRunning());

    const QPointer<KisToolFreehandHelper> guard(this);
    cancelPaint();
    if (!guard) return;
    clearPoints();
}


void KisToolLineHelper::clearPoints()
{
    m_d->linePoints.clear();
}


void KisToolLineHelper::clearPaint()
{
    if (!m_d->enabled) return;

    cancelPaint();
}

void KisToolLineHelper::adjustPointsToDDA(QVector<KisPaintInformation> &points)
{
    int x = qFloor(points.first().pos().x());
    int y = qFloor(points.first().pos().y());

    int x2 = qFloor(points.last().pos().x());
    int y2 = qFloor(points.last().pos().y());

    // Width and height of the line
    int xd = x2 - x;
    int yd = y2 - y;

    float m = 0;
    bool lockAxis = true;

    if (xd == 0) {
        m = 2.0;
    } else if ( yd != 0) {
        lockAxis = false;
        m = (float)yd / (float)xd;
    }

    float fx = x;
    float fy = y;

    int inc;
    int dist;

    if (fabs(m) > 1.0f) {
        inc = (yd > 0) ? 1 : -1;
        m = (lockAxis)? 0 : 1.0f / m;
        m *= inc;

        for (int i = 0; i < points.size(); i++){
            dist = abs(qFloor(points.at(i).pos().y()) - y);
            fy = y + (dist * inc);
            fx = qRound(x + (dist * m));
            points[i].setPos(QPointF(fx,fy));
        }

    } else {
        inc = (xd > 0) ? 1 : -1;
        m *= inc;

        for (int i = 0; i < points.size(); i++){
            dist = abs(qFloor(points.at(i).pos().x()) - x);
            fx = x + (dist * inc);
            fy = qRound(y + (dist * m));
            points[i].setPos(QPointF(fx,fy));
        }
    }
}
