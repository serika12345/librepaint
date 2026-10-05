/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef CANVAS_FRAME_RETURN_OBSERVATION_H
#define CANVAS_FRAME_RETURN_OBSERVATION_H
#include "../canvas/kis_canvas_performance_measurement_p.h"
#include <QDynamicPropertyChangeEvent>
#include <QList>

/** Comparison-only observation of CPU frame returns for one input sequence. */
class CanvasFrameReturnObservation : public QObject
{
public:
    CanvasFrameReturnObservation(QObject &owner, const char *stage)
        : m_owner(owner), m_stage(QString::fromLatin1(stage)) { owner.installEventFilter(this); }
    ~CanvasFrameReturnObservation() override { m_owner.removeEventFilter(this); }
    void reset(qint64 input) { m_input = input; m_returns.clear(); m_generations.clear(); }
    qint64 firstReturnContaining(quint64 generation) const {
        for(int i=0;i<m_returns.size();++i) if(m_generations[i]>=generation) return m_returns[i];
        return 0;
    }
    qint64 firstReturnAtOrAfter(qint64 projectionFinish) const
    {
        for (qint64 finish : m_returns) {
            if (finish >= projectionFinish) return finish;
        }
        return 0;
    }
private:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        namespace Perf = Krita::Canvas::Performance;
        if (object == &m_owner && event->type() == QEvent::DynamicPropertyChange
            && static_cast<QDynamicPropertyChangeEvent *>(event)->propertyName() == Perf::samplesProperty) {
            const auto frame = m_owner.property(Perf::samplesProperty).toMap().value(m_stage).toMap();
            const qint64 finish = frame.value(QStringLiteral("finish_ns")).toLongLong();
            if (finish && frame.value(QStringLiteral("input")).toLongLong() == m_input
                && (m_returns.isEmpty() || m_returns.last() != finish)) {
                m_returns.append(finish);
                m_generations.append(m_owner.property("librepaintCanvasPresentedGeneration").toULongLong());
            }
        }
        return false;
    }
    QObject &m_owner;
    QString m_stage;
    qint64 m_input = 0;
    QList<qint64> m_returns;
    QList<quint64> m_generations;
};
#endif
