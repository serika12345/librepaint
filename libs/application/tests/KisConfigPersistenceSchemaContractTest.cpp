/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_config.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KisConfigPersistenceSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void lifetimeAndDiagnosticSchemaRemainStable();
    void vectorImportAndColorHistorySignaturesRemainStable();
    void koColorPersistenceSignaturesRemainStable();
    void genericEntryPersistenceSignaturesRemainStable();
    void saveMessagePolicySignaturesRemainStable();
    void widgetAndSnapConfigurationSignaturesRemainStable();
    void sessionAndBannerPolicySignaturesRemainStable();
    void androidInputWorkaroundSignaturesRemainStable();
    void androidScalingSignaturesRemainStable();
    void firstRunAndAssistantColorSignaturesRemainStable();
};

#define ASSERT_CONFIG_SIGNATURE(Method, Signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<Signature>(&KisConfig::Method)), Signature>)

void KisConfigPersistenceSchemaContractTest::lifetimeAndDiagnosticSchemaRemainStable()
{
    using BitDepthDebug = QDebug (*)(QDebug, const KisConfig::CanvasSurfaceBitDepthMode &);
    using ModeDebug = QDebug (*)(QDebug, const KisConfig::CanvasSurfaceMode &);
    using VoidConst = void (KisConfig::*)() const;

    static_assert(std::is_constructible_v<KisConfig, bool>);
    static_assert(!std::is_default_constructible_v<KisConfig>);
    static_assert(std::is_destructible_v<KisConfig>);
    ASSERT_CONFIG_SIGNATURE(logImportantSettings, VoidConst);
    static_assert(std::is_same_v<decltype(static_cast<ModeDebug>(&operator<<)), ModeDebug>);
    static_assert(std::is_same_v<decltype(static_cast<BitDepthDebug>(&operator<<)), BitDepthDebug>);
}

void KisConfigPersistenceSchemaContractTest::vectorImportAndColorHistorySignaturesRemainStable()
{
    using BooleanReader = bool (KisConfig::*)(bool) const;
    using IntegerReader = int (KisConfig::*)(bool) const;
    using IntegerWriterConst = void (KisConfig::*)(int) const;

    ASSERT_CONFIG_SIGNATURE(preferredVectorImportResolutionPPI, IntegerReader);
    ASSERT_CONFIG_SIGNATURE(setPreferredVectorImportResolutionPPI, IntegerWriterConst);
    ASSERT_CONFIG_SIGNATURE(colorHistoryPerDocument, BooleanReader);
}

void KisConfigPersistenceSchemaContractTest::koColorPersistenceSignaturesRemainStable()
{
    using ColorReader = KoColor (KisConfig::*)(const QString &, const KoColor &) const;
    using ColorsReader = QList<KoColor> (KisConfig::*)(const QString &) const;
    using ColorWriter = void (KisConfig::*)(const QString &, const KoColor &) const;
    using ColorsWriter = void (KisConfig::*)(const QString &, const QList<KoColor> &) const;

    ASSERT_CONFIG_SIGNATURE(readKoColor, ColorReader);
    ASSERT_CONFIG_SIGNATURE(readKoColors, ColorsReader);
    ASSERT_CONFIG_SIGNATURE(writeKoColor, ColorWriter);
    ASSERT_CONFIG_SIGNATURE(writeKoColors, ColorsWriter);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().readKoColor(QString())), KoColor>);
}

void KisConfigPersistenceSchemaContractTest::genericEntryPersistenceSignaturesRemainStable()
{
    using EntryReader = int (KisConfig::*)(const QString &, const int &);
    using EntryWriter = void (KisConfig::*)(const QString &, const int &);
    using ListReader = QList<int> (KisConfig::*)(const QString &, const QList<int> &);
    using ListWriter = void (KisConfig::*)(const QString &, const QList<int> &);

    static_assert(std::is_same_v<decltype(static_cast<EntryReader>(&KisConfig::readEntry<int>)), EntryReader>);
    static_assert(std::is_same_v<decltype(static_cast<EntryWriter>(&KisConfig::writeEntry<int>)), EntryWriter>);
    static_assert(std::is_same_v<decltype(static_cast<ListReader>(&KisConfig::readList<int>)), ListReader>);
    static_assert(std::is_same_v<decltype(static_cast<ListWriter>(&KisConfig::writeList<int>)), ListWriter>);
    static_assert(std::is_same_v<decltype(std::declval<KisConfig &>().readEntry<int>(QString())), int>);
    static_assert(std::is_same_v<decltype(std::declval<KisConfig &>().readList<int>(QString())), QList<int>>);
}

void KisConfigPersistenceSchemaContractTest::saveMessagePolicySignaturesRemainStable()
{
    using BooleanReader = bool (KisConfig::*)(bool) const;
    using BooleanWriterConst = void (KisConfig::*)(bool) const;

    ASSERT_CONFIG_SIGNATURE(forceShowSaveMessages, BooleanReader);
    ASSERT_CONFIG_SIGNATURE(setForceShowSaveMessages, BooleanWriterConst);
    ASSERT_CONFIG_SIGNATURE(forceShowAutosaveMessages, BooleanReader);
    ASSERT_CONFIG_SIGNATURE(setForceShowAutosaveMessages, BooleanWriterConst);
}

void KisConfigPersistenceSchemaContractTest::widgetAndSnapConfigurationSignaturesRemainStable()
{
    using LoadSnap = void (KisConfig::*)(KisSnapConfig *, bool) const;
    using SaveSnap = void (KisConfig::*)(const KisSnapConfig &);
    using WidgetReader = QString (KisConfig::*)(bool);
    using WidgetWriter = void (KisConfig::*)(QString);

    ASSERT_CONFIG_SIGNATURE(widgetStyle, WidgetReader);
    ASSERT_CONFIG_SIGNATURE(setWidgetStyle, WidgetWriter);
    ASSERT_CONFIG_SIGNATURE(loadSnapConfig, LoadSnap);
    ASSERT_CONFIG_SIGNATURE(saveSnapConfig, SaveSnap);
}

void KisConfigPersistenceSchemaContractTest::sessionAndBannerPolicySignaturesRemainStable()
{
    using BooleanReader = bool (KisConfig::*)(bool) const;
    using BooleanWriter = void (KisConfig::*)(bool);

    ASSERT_CONFIG_SIGNATURE(saveSessionOnQuit, BooleanReader);
    ASSERT_CONFIG_SIGNATURE(setSaveSessionOnQuit, BooleanWriter);
    ASSERT_CONFIG_SIGNATURE(hideDevFundBanner, BooleanReader);
    ASSERT_CONFIG_SIGNATURE(setHideDevFundBanner, BooleanWriter);
    static_assert(std::is_same_v<decltype(std::declval<KisConfig &>().setHideDevFundBanner()), void>);
}

void KisConfigPersistenceSchemaContractTest::androidInputWorkaroundSignaturesRemainStable()
{
#ifdef Q_OS_ANDROID
    using BooleanReader = bool (KisConfig::*)(bool) const;
    using BooleanWriter = void (KisConfig::*)(bool);

    ASSERT_CONFIG_SIGNATURE(usePageUpDownMouseButtonEmulationWorkaround, BooleanReader);
    ASSERT_CONFIG_SIGNATURE(setUsePageUpDownMouseButtonEmulationWorkaround, BooleanWriter);
    ASSERT_CONFIG_SIGNATURE(useHighFunctionKeyMouseButtonEmulationWorkaround, BooleanReader);
    ASSERT_CONFIG_SIGNATURE(setUseHighFunctionKeyMouseButtonEmulationWorkaround, BooleanWriter);
    ASSERT_CONFIG_SIGNATURE(useIgnoreHistoricTabletEventsWorkaround, BooleanReader);
    ASSERT_CONFIG_SIGNATURE(setUseIgnoreHistoricTabletEventsWorkaround, BooleanWriter);
#endif
}

void KisConfigPersistenceSchemaContractTest::androidScalingSignaturesRemainStable()
{
#ifdef Q_OS_ANDROID
    using BooleanReader = bool (KisConfig::*)(bool) const;
    using BooleanWriter = void (KisConfig::*)(bool);
    using ScaleReader = qreal (KisConfig::*)(bool) const;
    using ScaleWriter = void (KisConfig::*)(qreal);

    ASSERT_CONFIG_SIGNATURE(androidScalingLastInitialScale, ScaleReader);
    ASSERT_CONFIG_SIGNATURE(setAndroidScalingLastInitialScale, ScaleWriter);
    ASSERT_CONFIG_SIGNATURE(androidScalingTargetScale, ScaleReader);
    ASSERT_CONFIG_SIGNATURE(setAndroidScalingTargetScale, ScaleWriter);
    ASSERT_CONFIG_SIGNATURE(androidScalingAskOnStartup, BooleanReader);
    ASSERT_CONFIG_SIGNATURE(setAndroidScalingAskOnStartup, BooleanWriter);
#endif
}

void KisConfigPersistenceSchemaContractTest::firstRunAndAssistantColorSignaturesRemainStable()
{
    using BooleanReader = bool (KisConfig::*)(bool) const;
    using BooleanWriterConst = void (KisConfig::*)(bool) const;
    using ColorReader = QColor (KisConfig::*)(bool) const;
    using ColorWriterConst = void (KisConfig::*)(const QColor &) const;

    ASSERT_CONFIG_SIGNATURE(firstRun, BooleanReader);
    ASSERT_CONFIG_SIGNATURE(setFirstRun, BooleanWriterConst);
    ASSERT_CONFIG_SIGNATURE(defaultAssistantsColor, ColorReader);
    ASSERT_CONFIG_SIGNATURE(setDefaultAssistantsColor, ColorWriterConst);
}

#undef ASSERT_CONFIG_SIGNATURE

QTEST_APPLESS_MAIN(KisConfigPersistenceSchemaContractTest)

#include "KisConfigPersistenceSchemaContractTest.moc"
