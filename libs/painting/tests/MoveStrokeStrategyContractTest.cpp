/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorSpaceRegistry.h>

#include <kis_image.h>
#include <kis_paint_layer.h>
#include <strokes/move_stroke_strategy.h>

#include <QTest>

namespace
{
struct MoveFixture {
    KisImageSP image;
    KisPaintLayerSP layer;
};

MoveFixture createMoveFixture(const QPoint &position)
{
    MoveFixture fixture {
        new KisImage(nullptr,
                     64,
                     48,
                     KoColorSpaceRegistry::instance()->rgb8(),
                     QStringLiteral("move-stroke")),
        nullptr,
    };

    fixture.layer = new KisPaintLayer(fixture.image, QStringLiteral("movable"), OPACITY_OPAQUE_U8);
    fixture.image->addNode(fixture.layer);
    fixture.layer->setX(position.x());
    fixture.layer->setY(position.y());
    fixture.image->initialRefreshGraph();
    fixture.image->waitForDone();

    return fixture;
}

KisStrokeId startMove(MoveFixture &fixture)
{
    return fixture.image->startStroke(
        new MoveStrokeStrategy({fixture.layer}, fixture.image.data(), fixture.image.data()));
}

void addFinalOffset(MoveFixture &fixture, KisStrokeId stroke, const QPoint &offset)
{
    fixture.image->addJob(stroke, new MoveStrokeStrategy::Data(QPoint()));
    QTest::qSleep(40);
    fixture.image->addJob(stroke, new MoveStrokeStrategy::Data(offset));
}
} // namespace

class MoveStrokeStrategyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void completedDragMovesTheLayerByItsFinalOffset();
    void cancelledDragRestoresTheLayerPosition();
};

void MoveStrokeStrategyContractTest::completedDragMovesTheLayerByItsFinalOffset()
{
    // Consumer: Move Tool users.
    // Operation: Complete a layer drag.
    // Observable result: The layer moves by the final drag offset.
    // Failure impact: The completed drag leaves the layer at the wrong canvas position.
    const QPoint initialPosition(11, 7);
    const QPoint finalOffset(9, -4);
    MoveFixture fixture = createMoveFixture(initialPosition);

    const KisStrokeId stroke = startMove(fixture);
    addFinalOffset(fixture, stroke, finalOffset);
    fixture.image->endStroke(stroke);
    fixture.image->waitForDone();

    QCOMPARE(fixture.layer->x(), initialPosition.x() + finalOffset.x());
    QCOMPARE(fixture.layer->y(), initialPosition.y() + finalOffset.y());
}

void MoveStrokeStrategyContractTest::cancelledDragRestoresTheLayerPosition()
{
    // Consumer: Move Tool users.
    // Operation: Cancel a layer drag after its live position changes.
    // Observable result: The layer returns to its position before the drag.
    // Failure impact: Cancelling a drag leaves unintended layer movement in the document.
    const QPoint initialPosition(-5, 13);
    const QPoint finalOffset(8, 6);
    MoveFixture fixture = createMoveFixture(initialPosition);

    const KisStrokeId stroke = startMove(fixture);
    addFinalOffset(fixture, stroke, finalOffset);

    QTRY_COMPARE(fixture.layer->x(), initialPosition.x() + finalOffset.x());
    QTRY_COMPARE(fixture.layer->y(), initialPosition.y() + finalOffset.y());

    QVERIFY(fixture.image->cancelStroke(stroke));
    fixture.image->waitForDone();

    QCOMPARE(fixture.layer->x(), initialPosition.x());
    QCOMPARE(fixture.layer->y(), initialPosition.y());
}

QTEST_GUILESS_MAIN(MoveStrokeStrategyContractTest)

#include "MoveStrokeStrategyContractTest.moc"
