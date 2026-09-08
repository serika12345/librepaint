/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/kis_paintop.h"

#include <QTest>

#include <type_traits>

namespace
{

class PaintOpProbe : public KisPaintOp
{
public:
    explicit PaintOpProbe(KisPainter *painter);

protected:
    KisSpacingInformation paintAt(const KisPaintInformation &info) override;
    KisSpacingInformation updateSpacingImpl(const KisPaintInformation &info) const override;
};

#define ASSERT_PAINTOP_SIGNATURE(method, ...)                                                                          \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisPaintOp::method)), __VA_ARGS__>)

} // namespace

class KisPaintOpSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void abstractTypeConstructionAndLifetimeSchemaRemainStable();
    void paintingOperationSignaturesRemainStable();
    void spacingAndTimingUpdateSignaturesRemainStable();
    void coordinateAndAsynchronousUpdateSignaturesRemainStable();
};

void KisPaintOpSchemaContractTest::abstractTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisPaintOp>);
    static_assert(std::is_abstract_v<KisPaintOp>);
    static_assert(std::is_base_of_v<KisShared, KisPaintOp>);
    static_assert(std::is_base_of_v<KisPaintOp, PaintOpProbe>);
    static_assert(std::is_constructible_v<PaintOpProbe, KisPainter *>);
    static_assert(std::has_virtual_destructor_v<KisPaintOp>);

    QVERIFY(true);
}

void KisPaintOpSchemaContractTest::paintingOperationSignaturesRemainStable()
{
    ASSERT_PAINTOP_SIGNATURE(paintAt, void (KisPaintOp::*)(const KisPaintInformation &, KisDistanceInformation *));
    ASSERT_PAINTOP_SIGNATURE(
        paintLine,
        void (KisPaintOp::*)(const KisPaintInformation &, const KisPaintInformation &, KisDistanceInformation *));
    ASSERT_PAINTOP_SIGNATURE(paintBezierCurve,
                             void (KisPaintOp::*)(const KisPaintInformation &,
                                                  const QPointF &,
                                                  const QPointF &,
                                                  const KisPaintInformation &,
                                                  KisDistanceInformation *));
    ASSERT_PAINTOP_SIGNATURE(canPaint, bool (KisPaintOp::*)() const);
}

void KisPaintOpSchemaContractTest::spacingAndTimingUpdateSignaturesRemainStable()
{
    ASSERT_PAINTOP_SIGNATURE(updateSpacing,
                             void (KisPaintOp::*)(const KisPaintInformation &, KisDistanceInformation &) const);
    ASSERT_PAINTOP_SIGNATURE(updateTiming,
                             void (KisPaintOp::*)(const KisPaintInformation &, KisDistanceInformation &) const);
}

void KisPaintOpSchemaContractTest::coordinateAndAsynchronousUpdateSignaturesRemainStable()
{
    ASSERT_PAINTOP_SIGNATURE(splitCoordinate, void (*)(qreal, qint32 *, qreal *));
    ASSERT_PAINTOP_SIGNATURE(doAsynchronousUpdate,
                             std::pair<int, bool> (KisPaintOp::*)(QVector<KisRunnableStrokeJobData *> &));
}

#undef ASSERT_PAINTOP_SIGNATURE

QTEST_GUILESS_MAIN(KisPaintOpSchemaContractTest)

#include "KisPaintOpSchemaContractTest.moc"
