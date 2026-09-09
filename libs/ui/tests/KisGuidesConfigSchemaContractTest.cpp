/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QColor>
#include <QPen>
#include <QTest>
#include <QTransform>

#include <canvas/kis_guides_config.h>
#include <canvas/kis_guides_manager.h>

#include <type_traits>

namespace
{
#define ASSERT_GUIDES_CONFIG_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisGuidesConfig::method)), signature>)
#define ASSERT_GUIDES_MANAGER_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisGuidesManager::method)), signature>)
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
    void guidesManagerTypeLifetimeAndEventSchemaRemainStable();
    void guidesManagerViewAndStateQuerySignaturesRemainStable();
    void guidesManagerConfigurationAndPrimaryStateSignaturesRemainStable();
    void guidesManagerCreationAndSnappingSignaturesRemainStable();
    void guidesManagerPersistenceAndNotificationSignaturesRemainStable();
};

void KisGuidesConfigSchemaContractTest::guideValueIdentityAndEqualitySchemaRemainStable()
{
    static_assert(std::is_class_v<KisGuidesConfig>);
    static_assert(std::is_default_constructible_v<KisGuidesConfig>);
    static_assert(std::is_copy_constructible_v<KisGuidesConfig>);
    static_assert(std::is_copy_assignable_v<KisGuidesConfig>);
    static_assert(std::is_destructible_v<KisGuidesConfig>);
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
    ASSERT_GUIDES_CONFIG_SIGNATURE(isDefault, bool (KisGuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(loadStaticData, void (KisGuidesConfig::*)());
    ASSERT_GUIDES_CONFIG_SIGNATURE(saveStaticData, void (KisGuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(loadFromXml, bool (KisGuidesConfig::*)(const QDomElement &));
    ASSERT_GUIDES_CONFIG_SIGNATURE(saveToXml, QDomElement (KisGuidesConfig::*)(QDomDocument &, const QString &) const);
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

void KisGuidesConfigSchemaContractTest::guidesManagerTypeLifetimeAndEventSchemaRemainStable()
{
    static_assert(std::is_class_v<KisGuidesManager>);
    static_assert(std::is_base_of_v<QObject, KisGuidesManager>);
    static_assert(std::is_default_constructible_v<KisGuidesManager>);
    static_assert(std::is_constructible_v<KisGuidesManager, QObject *>);
    static_assert(std::has_virtual_destructor_v<KisGuidesManager>);
    ASSERT_GUIDES_MANAGER_SIGNATURE(eventFilter, bool (KisGuidesManager::*)(QObject *, QEvent *));
}

void KisGuidesConfigSchemaContractTest::guidesManagerViewAndStateQuerySignaturesRemainStable()
{
    ASSERT_GUIDES_MANAGER_SIGNATURE(setup, void (KisGuidesManager::*)(KisActionManager *));
    ASSERT_GUIDES_MANAGER_SIGNATURE(setView, void (KisGuidesManager::*)(QPointer<KisView>));
    ASSERT_GUIDES_MANAGER_SIGNATURE(showGuides, bool (KisGuidesManager::*)() const);
    ASSERT_GUIDES_MANAGER_SIGNATURE(lockGuides, bool (KisGuidesManager::*)() const);
    ASSERT_GUIDES_MANAGER_SIGNATURE(snapToGuides, bool (KisGuidesManager::*)() const);
    ASSERT_GUIDES_MANAGER_SIGNATURE(rulersMultiple2, bool (KisGuidesManager::*)() const);
    ASSERT_GUIDES_MANAGER_SIGNATURE(unitType, KoUnit::Type (KisGuidesManager::*)() const);
}

void KisGuidesConfigSchemaContractTest::guidesManagerConfigurationAndPrimaryStateSignaturesRemainStable()
{
    ASSERT_GUIDES_MANAGER_SIGNATURE(setGuidesConfig, void (KisGuidesManager::*)(const KisGuidesConfig &));
    ASSERT_GUIDES_MANAGER_SIGNATURE(slotDocumentRequestedConfig, void (KisGuidesManager::*)(const KisGuidesConfig &));
    ASSERT_GUIDES_MANAGER_SIGNATURE(setShowGuides, void (KisGuidesManager::*)(bool));
    ASSERT_GUIDES_MANAGER_SIGNATURE(setLockGuides, void (KisGuidesManager::*)(bool));
    ASSERT_GUIDES_MANAGER_SIGNATURE(setSnapToGuides, void (KisGuidesManager::*)(bool));
    ASSERT_GUIDES_MANAGER_SIGNATURE(setRulersMultiple2, void (KisGuidesManager::*)(bool));
    ASSERT_GUIDES_MANAGER_SIGNATURE(setUnitType, void (KisGuidesManager::*)(KoUnit::Type));
}

void KisGuidesConfigSchemaContractTest::guidesManagerCreationAndSnappingSignaturesRemainStable()
{
    ASSERT_GUIDES_MANAGER_SIGNATURE(slotGuideCreationInProgress,
                                    void (KisGuidesManager::*)(Qt::Orientation, const QPoint &));
    ASSERT_GUIDES_MANAGER_SIGNATURE(slotGuideCreationFinished,
                                    void (KisGuidesManager::*)(Qt::Orientation, const QPoint &));
    ASSERT_GUIDES_MANAGER_SIGNATURE(slotShowSnapOptions, void (KisGuidesManager::*)());
    ASSERT_GUIDES_MANAGER_SIGNATURE(setSnapOrthogonal, void (KisGuidesManager::*)(bool));
    ASSERT_GUIDES_MANAGER_SIGNATURE(setSnapNode, void (KisGuidesManager::*)(bool));
    ASSERT_GUIDES_MANAGER_SIGNATURE(setSnapExtension, void (KisGuidesManager::*)(bool));
    ASSERT_GUIDES_MANAGER_SIGNATURE(setSnapIntersection, void (KisGuidesManager::*)(bool));
    ASSERT_GUIDES_MANAGER_SIGNATURE(setSnapBoundingBox, void (KisGuidesManager::*)(bool));
    ASSERT_GUIDES_MANAGER_SIGNATURE(setSnapImageBounds, void (KisGuidesManager::*)(bool));
    ASSERT_GUIDES_MANAGER_SIGNATURE(setSnapImageCenter, void (KisGuidesManager::*)(bool));
    ASSERT_GUIDES_MANAGER_SIGNATURE(setSnapToPixel, void (KisGuidesManager::*)(bool));
}

void KisGuidesConfigSchemaContractTest::guidesManagerPersistenceAndNotificationSignaturesRemainStable()
{
    ASSERT_GUIDES_MANAGER_SIGNATURE(slotUploadConfigToDocument, void (KisGuidesManager::*)());
    ASSERT_GUIDES_MANAGER_SIGNATURE(sigRequestUpdateGuidesConfig, void (KisGuidesManager::*)(const KisGuidesConfig &));
}

#undef ASSERT_GUIDES_MANAGER_SIGNATURE
#undef ASSERT_GUIDES_CONFIG_SIGNATURE

QTEST_APPLESS_MAIN(KisGuidesConfigSchemaContractTest)

#include "KisGuidesConfigSchemaContractTest.moc"
