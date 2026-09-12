/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "utils/KisRecentFileIconCache.h"
#include "widgets/KisColorSamplerPreviewPreview.h"
#include "widgets/KisDockerHud.h"
#include "widgets/KisLodAvailabilityData.h"
#include "widgets/KisMemoryReportButton.h"
#include "widgets/kis_collapsible_button_group.h"
#include "widgets/kis_tool_button.h"
#include "widgets/kis_utility_title_bar.h"

#include <QTest>

#include <type_traits>

class KisCollapsibleButtonGroupSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void autoRaiseAndIconSizeSignaturesRemainStable();
    void collapseStateSignaturesRemainStable();
    void sizingAndActionSignaturesRemainStable();
    void recentFileIconCacheTypeLifetimeAndSingletonSchemaRemainStable();
    void recentFileIconCacheAccessAndNotificationSignaturesRemainStable();
    void utilityTitleBarTypeConstructionAndSizingSchemaRemainStable();
    void utilityTitleBarWidgetAreaAndLockSignaturesRemainStable();
    void lodAvailabilityDefaultsMembersAndEqualityRemainStable();
    void lodAvailabilityPersistenceSignaturesRemainStable();
    void toolButtonTypeAndConstructionSchemaRemainStable();
    void memoryReportButtonTypeAndConstructionSchemaRemainStable();
    void memoryReportButtonStateAndPaintingSignaturesRemainStable();
    void colorSamplerPreviewTypeAndConstructionSchemaRemainStable();
    void colorSamplerPreviewStateAndPaintingSignaturesRemainStable();
    void dockerHudTypeConstructionAndLifetimeSchemaRemainStable();
    void dockerHudStateAndBorrowingSignaturesRemainStable();
};

void KisCollapsibleButtonGroupSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Group = KisCollapsibleButtonGroup;

    static_assert(std::is_class_v<Group>);
    static_assert(std::is_base_of_v<QWidget, Group>);
    static_assert(std::is_constructible_v<Group, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Group>);
}

void KisCollapsibleButtonGroupSchemaContractTest::autoRaiseAndIconSizeSignaturesRemainStable()
{
    using Group = KisCollapsibleButtonGroup;

    static_assert(std::is_same_v<decltype(&Group::setAutoRaise), void (Group::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Group::autoRaise), bool (Group::*)() const>);
    static_assert(std::is_same_v<decltype(&Group::setIconSize), void (Group::*)(const QSize &)>);
    static_assert(std::is_same_v<decltype(&Group::iconSize), QSize (Group::*)() const>);
}

void KisCollapsibleButtonGroupSchemaContractTest::collapseStateSignaturesRemainStable()
{
    using Group = KisCollapsibleButtonGroup;

    static_assert(std::is_same_v<decltype(&Group::setAutoCollapse), void (Group::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Group::setCollapsed), void (Group::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Group::collapsed), bool (Group::*)() const>);
}

void KisCollapsibleButtonGroupSchemaContractTest::sizingAndActionSignaturesRemainStable()
{
    using Group = KisCollapsibleButtonGroup;

    static_assert(std::is_same_v<decltype(&Group::sizeHint), QSize (Group::*)() const>);
    static_assert(std::is_same_v<decltype(&Group::minimumSizeHint), QSize (Group::*)() const>);
    static_assert(std::is_same_v<decltype(&Group::addAction), QToolButton *(Group::*)(QAction *)>);
}

void KisCollapsibleButtonGroupSchemaContractTest::recentFileIconCacheTypeLifetimeAndSingletonSchemaRemainStable()
{
    using Cache = KisRecentFileIconCache;

    static_assert(std::is_class_v<Cache>);
    static_assert(std::is_base_of_v<QObject, Cache>);
    static_assert(std::is_default_constructible_v<Cache>);
    static_assert(std::has_virtual_destructor_v<Cache>);
    static_assert(std::is_same_v<decltype(&Cache::instance), Cache *(*)()>);
}

void KisCollapsibleButtonGroupSchemaContractTest::recentFileIconCacheAccessAndNotificationSignaturesRemainStable()
{
    using Cache = KisRecentFileIconCache;

    static_assert(std::is_same_v<decltype(&Cache::getOrQueueFileIcon), QIcon (Cache::*)(const QUrl &)>);
    static_assert(std::is_same_v<decltype(&Cache::invalidateFileIcon), void (Cache::*)(const QUrl &)>);
    static_assert(std::is_same_v<decltype(&Cache::reloadFileIcon), void (Cache::*)(const QUrl &)>);
    static_assert(std::is_same_v<decltype(&Cache::fileIconChanged), void (Cache::*)(const QUrl &, const QIcon &)>);
}

void KisCollapsibleButtonGroupSchemaContractTest::utilityTitleBarTypeConstructionAndSizingSchemaRemainStable()
{
    using TitleBar = KisUtilityTitleBar;

    static_assert(std::is_class_v<TitleBar>);
    static_assert(std::is_base_of_v<QWidget, TitleBar>);
    static_assert(std::is_default_constructible_v<TitleBar>);
    static_assert(std::is_constructible_v<TitleBar, QWidget *>);
    static_assert(std::is_constructible_v<TitleBar, QLabel *>);
    static_assert(std::is_constructible_v<TitleBar, QLabel *, QWidget *>);
    static_assert(std::has_virtual_destructor_v<TitleBar>);
    static_assert(std::is_same_v<decltype(&TitleBar::sizeHint), QSize (TitleBar::*)() const>);
}

void KisCollapsibleButtonGroupSchemaContractTest::utilityTitleBarWidgetAreaAndLockSignaturesRemainStable()
{
    using TitleBar = KisUtilityTitleBar;

    static_assert(std::is_same_v<decltype(&TitleBar::widgetArea), QWidget *(TitleBar::*)()>);
    static_assert(std::is_same_v<decltype(&TitleBar::setWidgetArea), void (TitleBar::*)(QWidget *)>);
    static_assert(std::is_same_v<decltype(&TitleBar::setLocked), void (TitleBar::*)(bool)>);
}

void KisCollapsibleButtonGroupSchemaContractTest::lodAvailabilityDefaultsMembersAndEqualityRemainStable()
{
    using Data = KisLodAvailabilityData;

    static_assert(std::is_class_v<Data>);
    static_assert(std::is_same_v<decltype(&Data::isLodUserAllowed), bool Data::*>);
    static_assert(std::is_same_v<decltype(&Data::isLodSizeThresholdSupported), bool Data::*>);
    static_assert(std::is_same_v<decltype(&Data::lodSizeThreshold), qreal Data::*>);

    Data first;
    Data second;
    QVERIFY(first.isLodUserAllowed);
    QVERIFY(first.isLodSizeThresholdSupported);
    QCOMPARE(first.lodSizeThreshold, 100.0);
    QVERIFY(first == second);

    second.isLodUserAllowed = false;
    QVERIFY(first != second);
    second = first;
    second.isLodSizeThresholdSupported = false;
    QVERIFY(first != second);
    second = first;
    second.lodSizeThreshold = 50.0;
    QVERIFY(first != second);
}

void KisCollapsibleButtonGroupSchemaContractTest::lodAvailabilityPersistenceSignaturesRemainStable()
{
    using Data = KisLodAvailabilityData;

    static_assert(std::is_same_v<decltype(&Data::read), bool (Data::*)(const KisPropertiesConfiguration *)>);
    static_assert(std::is_same_v<decltype(&Data::write), void (Data::*)(KisPropertiesConfiguration *) const>);
}

void KisCollapsibleButtonGroupSchemaContractTest::toolButtonTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisToolButton>);
    static_assert(std::is_base_of_v<QToolButton, KisToolButton>);
    static_assert(std::is_default_constructible_v<KisToolButton>);
    static_assert(std::is_constructible_v<KisToolButton, QWidget *>);
}

void KisCollapsibleButtonGroupSchemaContractTest::memoryReportButtonTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisMemoryReportButton>);
    static_assert(std::is_base_of_v<QPushButton, KisMemoryReportButton>);
    static_assert(std::is_default_constructible_v<KisMemoryReportButton>);
    static_assert(std::is_constructible_v<KisMemoryReportButton, QWidget *>);
}

void KisCollapsibleButtonGroupSchemaContractTest::memoryReportButtonStateAndPaintingSignaturesRemainStable()
{
    using Button = KisMemoryReportButton;

    static_assert(std::is_same_v<decltype(&Button::setMaximumMemory), void (Button::*)(qint64)>);
    static_assert(std::is_same_v<decltype(&Button::setCurrentMemory), void (Button::*)(qint64)>);
    static_assert(std::is_same_v<decltype(&Button::setImageWeight), void (Button::*)(qint64)>);
    static_assert(std::is_same_v<decltype(&Button::paintEvent), void (Button::*)(QPaintEvent *)>);
}

void KisCollapsibleButtonGroupSchemaContractTest::colorSamplerPreviewTypeAndConstructionSchemaRemainStable()
{
    using Preview = KisColorSamplerPreviewPreview;

    static_assert(std::is_class_v<Preview>);
    static_assert(std::is_base_of_v<QLabel, Preview>);
    static_assert(std::is_constructible_v<Preview, QWidget *>);
}

void KisCollapsibleButtonGroupSchemaContractTest::colorSamplerPreviewStateAndPaintingSignaturesRemainStable()
{
    using Preview = KisColorSamplerPreviewPreview;

    static_assert(std::is_same_v<decltype(&Preview::setDiameter), void (Preview::*)(int)>);
    static_assert(std::is_same_v<decltype(&Preview::setOutlineEnabled), void (Preview::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Preview::setThickness), void (Preview::*)(qreal)>);
    static_assert(std::is_same_v<decltype(&Preview::paintEvent), void (Preview::*)(QPaintEvent *)>);
}

void KisCollapsibleButtonGroupSchemaContractTest::dockerHudTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Hud = KisDockerHud;

    static_assert(std::is_class_v<Hud>);
    static_assert(std::is_base_of_v<QWidget, Hud>);
    static_assert(std::is_constructible_v<Hud, QString, QString>);
    static_assert(std::has_virtual_destructor_v<Hud>);
}

void KisCollapsibleButtonGroupSchemaContractTest::dockerHudStateAndBorrowingSignaturesRemainStable()
{
    using Hud = KisDockerHud;

    static_assert(std::is_same_v<decltype(&Hud::slotUpdateIcons), void (Hud::*)()>);
    static_assert(std::is_same_v<decltype(&Hud::borrowOrReturnDocker), void (Hud::*)()>);
    static_assert(std::is_same_v<decltype(&Hud::returnDocker), void (Hud::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Hud::borrowDocker), void (Hud::*)()>);
    static_assert(std::is_same_v<decltype(&Hud::setIsShown), void (Hud::*)(bool)>);
}

QTEST_GUILESS_MAIN(KisCollapsibleButtonGroupSchemaContractTest)

#include "KisCollapsibleButtonGroupSchemaContractTest.moc"
