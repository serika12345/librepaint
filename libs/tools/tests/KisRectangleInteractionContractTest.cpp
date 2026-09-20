/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_rectangle_interaction.h"

#include <QTest>

#include <cmath>

class KisRectangleInteractionContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constraintsProduceRequestedGeometry();
    void modifiersSwitchInteractionModes();
    void rotationModeReportsGeometricAngle();
    void explicitEndpointsAreRoundedAndNormalized();
};

void KisRectangleInteractionContractTest::constraintsProduceRequestedGeometry()
{
    KisRectangleInteraction interaction;
    interaction.setConstraints(true, false, false, 2.0, 0.0, 0.0);
    interaction.begin(QPointF(10.0, 20.0));
    interaction.update(QPointF(50.0, 50.0));

    QCOMPARE(interaction.rectangle(), QRectF(10.0, 20.0, 60.0, 30.0));

    interaction.setConstraints(false, true, true, 1.0, 40.0, 20.0);
    interaction.begin(QPointF(0.0, 0.0));
    QCOMPARE(interaction.rectangle(), QRectF(0.0, 0.0, 40.0, 20.0));

    interaction.update(QPointF(100.0, 200.0));
    QCOMPARE(interaction.rectangle(), QRectF(100.0, 200.0, 40.0, 20.0));
}

void KisRectangleInteractionContractTest::modifiersSwitchInteractionModes()
{
    KisRectangleInteraction interaction;
    interaction.begin(QPointF(10.0, 20.0));
    interaction.setModifier(Qt::ShiftModifier, true);
    interaction.update(QPointF(40.0, 60.0));
    QCOMPARE(interaction.rectangle(), QRectF(10.0, 20.0, 40.0, 40.0));

    interaction.setModifier(Qt::ShiftModifier, false);
    interaction.setModifier(Qt::AltModifier, true);
    interaction.update(QPointF(70.0, 80.0));
    QCOMPARE(interaction.rectangle(), QRectF(30.0, 40.0, 40.0, 40.0));
    QVERIFY(interaction.isTranslating());

    interaction.begin(QPointF(10.0, 20.0));
    interaction.setModifier(Qt::ControlModifier, true);
    interaction.update(QPointF(30.0, 50.0));
    QCOMPARE(interaction.start(), QPointF(0.0, 5.0));
    QCOMPARE(interaction.rectangle(), QRectF(0.0, 5.0, 20.0, 30.0));
    QCOMPARE(interaction.center(), QPointF(10.0, 20.0));
}

void KisRectangleInteractionContractTest::rotationModeReportsGeometricAngle()
{
    KisRectangleInteraction interaction;
    interaction.setConstraints(false, true, true, 1.0, 10.0, 20.0);
    interaction.begin(QPointF(0.0, 0.0));
    interaction.setModifier(Qt::ControlModifier, true);
    interaction.setModifier(Qt::AltModifier, true);
    interaction.update(QPointF(0.0, 10.0));

    const qreal expectedAngle = std::atan2(10.0, 0.0) - std::atan2(20.0, 10.0);
    QVERIFY(qAbs(interaction.rotationAngle() - expectedAngle) < 0.000001);
}

void KisRectangleInteractionContractTest::explicitEndpointsAreRoundedAndNormalized()
{
    KisRectangleInteraction interaction;
    interaction.begin(QPointF(10.2, 20.4));
    interaction.update(QPointF(50.6, 60.8));

    QCOMPARE(interaction.end(), QPointF(50.6, 60.8));
    QCOMPARE(interaction.rectangle(QPointF(50.6, 60.8), QPointF(10.2, 20.4)), QRectF(10.0, 20.0, 41.0, 41.0));
}

QTEST_GUILESS_MAIN(KisRectangleInteractionContractTest)

#include "KisRectangleInteractionContractTest.moc"
