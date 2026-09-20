/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_lod_transform.h"

#include <QTest>

#include <map>

namespace
{

struct PaintInformationState {
    QPointF position;
    qreal pressure;
    int levelOfDetail = 0;
};

std::map<const KisPaintInformation *, PaintInformationState> paintInformationStates;

const PaintInformationState &paintInformationState(const KisPaintInformation *information)
{
    return paintInformationStates.at(information);
}

PaintInformationState &mutablePaintInformationState(KisPaintInformation *information)
{
    return paintInformationStates.at(information);
}

int observedLevelOfDetail(const KisPaintInformation &information)
{
    return paintInformationState(&information).levelOfDetail;
}

class MockBounds
{
public:
    int currentLevelOfDetail() const
    {
        return 3;
    }
};

class MockPaintDevice
{
public:
    MockBounds *defaultBounds()
    {
        return &m_bounds;
    }

private:
    MockBounds m_bounds;
};

} // namespace

KisPaintInformation::KisPaintInformation(const QPointF &position, qreal pressure)
    : d(nullptr)
{
    paintInformationStates.emplace(this, PaintInformationState{position, pressure, 0});
}

KisPaintInformation::KisPaintInformation(const KisPaintInformation &rhs)
    : d(nullptr)
{
    paintInformationStates.emplace(this, paintInformationState(&rhs));
}

KisPaintInformation::~KisPaintInformation()
{
    paintInformationStates.erase(this);
}

const QPointF &KisPaintInformation::pos() const
{
    return paintInformationState(this).position;
}

void KisPaintInformation::setPos(const QPointF &position)
{
    mutablePaintInformationState(this).position = position;
}

qreal KisPaintInformation::pressure() const
{
    return paintInformationState(this).pressure;
}

void KisPaintInformation::setLevelOfDetail(int levelOfDetail)
{
    mutablePaintInformationState(this).levelOfDetail = levelOfDetail;
}

class KisLodTransformContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void deviceConstructorUsesCurrentLevelOfDetail();
    void paintInformationMapScalesPositionSetsLevelAndPreservesSource();
};

void KisLodTransformContractTest::deviceConstructorUsesCurrentLevelOfDetail()
{
    MockPaintDevice device;
    const KisLodTransform transform(&device);

    QCOMPARE(transform.transform(), QTransform::fromScale(0.125, 0.125));
    QCOMPARE(transform.map(QPointF(8.0, -16.0)), QPointF(1.0, -2.0));
}

void KisLodTransformContractTest::paintInformationMapScalesPositionSetsLevelAndPreservesSource()
{
    const QPointF sourcePosition(12.0, -20.0);
    const qreal sourcePressure = 0.625;
    const KisPaintInformation source(sourcePosition, sourcePressure);
    const KisLodTransform transform(2);

    const KisPaintInformation mapped = transform.map(source);

    QCOMPARE(mapped.pos(), QPointF(3.0, -5.0));
    QCOMPARE(mapped.pressure(), sourcePressure);
    QCOMPARE(observedLevelOfDetail(mapped), 2);

    QCOMPARE(source.pos(), sourcePosition);
    QCOMPARE(source.pressure(), sourcePressure);
    QCOMPARE(observedLevelOfDetail(source), 0);
}

QTEST_GUILESS_MAIN(KisLodTransformContractTest)

#include "KisLodTransformContractTest.moc"
