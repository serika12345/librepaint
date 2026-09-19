/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KoSnapGuide.h>
#include <KoSnapProxy.h>
#include <KoSnapStrategy.h>

#include <canvas/KisSnapPointStrategy.h>

#include <QFlags>
#include <QTest>

#include <utility>

namespace
{
class SnapStrategyConstructorProbe : public KoSnapStrategy
{
public:
    using KoSnapStrategy::KoSnapStrategy;

    bool snap(const QPointF &mousePosition, KoSnapProxy *proxy, qreal maxSnapDistance) override;
    QPainterPath decoration(const KoViewConverter &converter) const override;
};
} // namespace

class KoSnapGuideSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void snapGuideTypeAndStrategySchemaRemainsStable();
    void snapStrategyBaseTypeAndStateSchemaRemainsStable();
};

void KoSnapGuideSchemaContractTest::snapGuideTypeAndStrategySchemaRemainsStable()
{

    QCOMPARE(int(KoSnapGuide::OrthogonalSnapping), 1);
    QCOMPARE(int(KoSnapGuide::NodeSnapping), 2);
    QCOMPARE(int(KoSnapGuide::ExtensionSnapping), 4);
    QCOMPARE(int(KoSnapGuide::IntersectionSnapping), 8);
    QCOMPARE(int(KoSnapGuide::GridSnapping), 0x10);
    QCOMPARE(int(KoSnapGuide::BoundingBoxSnapping), 0x20);
    QCOMPARE(int(KoSnapGuide::GuideLineSnapping), 0x40);
    QCOMPARE(int(KoSnapGuide::DocumentBoundsSnapping), 0x80);
    QCOMPARE(int(KoSnapGuide::DocumentCenterSnapping), 0x100);
    QCOMPARE(int(KoSnapGuide::CustomSnapping), 0x200);
    QCOMPARE(int(KoSnapGuide::PixelSnapping), 0x400);
}

void KoSnapGuideSchemaContractTest::snapStrategyBaseTypeAndStateSchemaRemainsStable()
{


    QCOMPARE(int(KoSnapStrategy::ToPoint), 0);
    QCOMPARE(int(KoSnapStrategy::ToLine), 1);
    QVERIFY(KoSnapStrategy::ToPoint < KoSnapStrategy::ToLine);
    QVERIFY(!(KoSnapStrategy::ToLine < KoSnapStrategy::ToPoint));
}

QTEST_GUILESS_MAIN(KoSnapGuideSchemaContractTest)

#include "KoSnapGuideSchemaContractTest.moc"
