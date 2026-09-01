/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QColor>
#include <QPen>
#include <QTest>
#include <QTransform>

#include <canvas/kis_guides_config.h>

#include <type_traits>

namespace
{
#define ASSERT_GUIDES_CONFIG_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisGuidesConfig::method)), signature>)
} // namespace

class KisGuidesConfigSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void guideValueIdentityAndEqualitySchemaRemainStable();
    void guidePositionCollectionAndTransformSignaturesRemainStable();
    void guideVisibilityInteractionSignaturesRemainStable();
    void guideUnitAndLineTypeSignaturesRemainStable();
    void guideColorAndPenSignaturesRemainStable();
};

void KisGuidesConfigSchemaContractTest::guideValueIdentityAndEqualitySchemaRemainStable()
{
    static_assert(std::is_class_v<KisGuidesConfig>);
    static_assert(std::is_enum_v<KisGuidesConfig::LineTypeInternal>);

    QCOMPARE(int(KisGuidesConfig::LINE_SOLID), 0);
    QCOMPARE(int(KisGuidesConfig::LINE_DASHED), 1);
    QCOMPARE(int(KisGuidesConfig::LINE_DOTTED), 2);

    ASSERT_GUIDES_CONFIG_SIGNATURE(operator==, bool (KisGuidesConfig::*)(const KisGuidesConfig &) const);
}

void KisGuidesConfigSchemaContractTest::guidePositionCollectionAndTransformSignaturesRemainStable()
{
    ASSERT_GUIDES_CONFIG_SIGNATURE(hasSamePositionAs, bool (KisGuidesConfig::*)(const KisGuidesConfig &) const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setHorizontalGuideLines, void (KisGuidesConfig::*)(const QList<qreal> &));
    ASSERT_GUIDES_CONFIG_SIGNATURE(setVerticalGuideLines, void (KisGuidesConfig::*)(const QList<qreal> &));
    ASSERT_GUIDES_CONFIG_SIGNATURE(addGuideLine, void (KisGuidesConfig::*)(Qt::Orientation, qreal));
    ASSERT_GUIDES_CONFIG_SIGNATURE(removeAllGuides, void (KisGuidesConfig::*)());
    ASSERT_GUIDES_CONFIG_SIGNATURE(horizontalGuideLines, const QList<qreal> &(KisGuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(verticalGuideLines, const QList<qreal> &(KisGuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(hasGuides, bool (KisGuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(transform, void (KisGuidesConfig::*)(const QTransform &));
}

void KisGuidesConfigSchemaContractTest::guideVisibilityInteractionSignaturesRemainStable()
{
    ASSERT_GUIDES_CONFIG_SIGNATURE(showGuides, bool (KisGuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setShowGuides, void (KisGuidesConfig::*)(bool));
    ASSERT_GUIDES_CONFIG_SIGNATURE(lockGuides, bool (KisGuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setLockGuides, void (KisGuidesConfig::*)(bool));
    ASSERT_GUIDES_CONFIG_SIGNATURE(snapToGuides, bool (KisGuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setSnapToGuides, void (KisGuidesConfig::*)(bool));
    ASSERT_GUIDES_CONFIG_SIGNATURE(rulersMultiple2, bool (KisGuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setRulersMultiple2, void (KisGuidesConfig::*)(bool));
}

void KisGuidesConfigSchemaContractTest::guideUnitAndLineTypeSignaturesRemainStable()
{
    ASSERT_GUIDES_CONFIG_SIGNATURE(unitType, KoUnit::Type (KisGuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setUnitType, void (KisGuidesConfig::*)(KoUnit::Type));
    ASSERT_GUIDES_CONFIG_SIGNATURE(guidesLineType, KisGuidesConfig::LineTypeInternal (KisGuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setGuidesLineType, void (KisGuidesConfig::*)(KisGuidesConfig::LineTypeInternal));
}

void KisGuidesConfigSchemaContractTest::guideColorAndPenSignaturesRemainStable()
{
    ASSERT_GUIDES_CONFIG_SIGNATURE(guidesColor, QColor (KisGuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setGuidesColor, void (KisGuidesConfig::*)(const QColor &));
    ASSERT_GUIDES_CONFIG_SIGNATURE(guidesPen, QPen (KisGuidesConfig::*)() const);
}

#undef ASSERT_GUIDES_CONFIG_SIGNATURE

QTEST_APPLESS_MAIN(KisGuidesConfigSchemaContractTest)

#include "KisGuidesConfigSchemaContractTest.moc"
