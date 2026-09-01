/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KoSnapGuide.h>

#include <QFlags>
#include <QTest>

#include <type_traits>

class KoSnapGuideSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void snapGuideTypeAndStrategySchemaRemainsStable();
    void snapGuideLifetimeAndInteractionSignaturesRemainStable();
    void snapGuideStrategyPolicySignaturesRemainStable();
    void snapGuideAssociationSignaturesRemainStable();
    void snapGuideDistanceSignaturesRemainStable();
};

void KoSnapGuideSchemaContractTest::snapGuideTypeAndStrategySchemaRemainsStable()
{
    static_assert(std::is_class_v<KoSnapGuide>);
    static_assert(std::is_enum_v<KoSnapGuide::Strategy>);
    static_assert(std::is_same_v<KoSnapGuide::Strategies, QFlags<KoSnapGuide::Strategy>>);

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

void KoSnapGuideSchemaContractTest::snapGuideLifetimeAndInteractionSignaturesRemainStable()
{
    using FirstSnap = QPointF (KoSnapGuide::*)(const QPointF &, Qt::KeyboardModifiers);
    using OffsetSnap = QPointF (KoSnapGuide::*)(const QPointF &, const QPointF &, Qt::KeyboardModifiers);
    using Paint = void (KoSnapGuide::*)(QPainter &, const KoViewConverter &, const KoColorDisplayRendererInterface *);
    using BoundingRect = QRectF (KoSnapGuide::*)();

    static_assert(std::is_constructible_v<KoSnapGuide, KoCanvasBase *>);
    static_assert(std::is_destructible_v<KoSnapGuide>);
    static_assert(std::has_virtual_destructor_v<KoSnapGuide>);
    static_assert(std::is_same_v<decltype(static_cast<FirstSnap>(&KoSnapGuide::snap)), FirstSnap>);
    static_assert(std::is_same_v<decltype(static_cast<OffsetSnap>(&KoSnapGuide::snap)), OffsetSnap>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::paint), Paint>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::boundingRect), BoundingRect>);
}

void KoSnapGuideSchemaContractTest::snapGuideStrategyPolicySignaturesRemainStable()
{
    using EnableStrategy = void (KoSnapGuide::*)(KoSnapGuide::Strategy, bool);
    using IsStrategyEnabled = bool (KoSnapGuide::*)(KoSnapGuide::Strategy) const;
    using EnableStrategies = void (KoSnapGuide::*)(KoSnapGuide::Strategies);
    using EnabledStrategies = KoSnapGuide::Strategies (KoSnapGuide::*)() const;
    using AddCustomStrategy = bool (KoSnapGuide::*)(KoSnapStrategy *);
    using OverrideStrategy = void (KoSnapGuide::*)(KoSnapGuide::Strategy, KoSnapStrategy *);
    using EnableSnapping = void (KoSnapGuide::*)(bool);
    using IsSnapping = bool (KoSnapGuide::*)() const;

    static_assert(std::is_same_v<decltype(&KoSnapGuide::enableSnapStrategy), EnableStrategy>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::isStrategyEnabled), IsStrategyEnabled>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::enableSnapStrategies), EnableStrategies>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::enabledSnapStrategies), EnabledStrategies>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::addCustomSnapStrategy), AddCustomStrategy>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::overrideSnapStrategy), OverrideStrategy>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::enableSnapping), EnableSnapping>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::isSnapping), IsSnapping>);
}

void KoSnapGuideSchemaContractTest::snapGuideAssociationSignaturesRemainStable()
{
    using SetAdditionalShape = void (KoSnapGuide::*)(KoShape *);
    using AdditionalShape = KoShape *(KoSnapGuide::*)() const;
    using Canvas = KoCanvasBase *(KoSnapGuide::*)() const;
    using SetIgnoredPathPoints = void (KoSnapGuide::*)(const QList<KoPathPoint *> &);
    using IgnoredPathPoints = QList<KoPathPoint *> (KoSnapGuide::*)() const;
    using SetIgnoredShapes = void (KoSnapGuide::*)(const QList<KoShape *> &);
    using IgnoredShapes = QList<KoShape *> (KoSnapGuide::*)() const;
    using Reset = void (KoSnapGuide::*)();

    static_assert(std::is_same_v<decltype(&KoSnapGuide::setAdditionalEditedShape), SetAdditionalShape>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::additionalEditedShape), AdditionalShape>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::canvas), Canvas>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::setIgnoredPathPoints), SetIgnoredPathPoints>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::ignoredPathPoints), IgnoredPathPoints>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::setIgnoredShapes), SetIgnoredShapes>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::ignoredShapes), IgnoredShapes>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::reset), Reset>);
}

void KoSnapGuideSchemaContractTest::snapGuideDistanceSignaturesRemainStable()
{
    using SetSnapDistance = void (KoSnapGuide::*)(int);
    using SnapDistance = int (KoSnapGuide::*)() const;

    static_assert(std::is_same_v<decltype(&KoSnapGuide::setSnapDistance), SetSnapDistance>);
    static_assert(std::is_same_v<decltype(&KoSnapGuide::snapDistance), SnapDistance>);
}

QTEST_GUILESS_MAIN(KoSnapGuideSchemaContractTest)

#include "KoSnapGuideSchemaContractTest.moc"
