/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_image_config.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisImageConfig::method)), signature>)
} // namespace

class KisImageConfigGeneralSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageConfigTypeAndLifetimeSchemaRemainStable();
    void imageConfigDiagnosticsAndMaskSignaturesRemainStable();
    void imageConfigPresentationAndBrushSignaturesRemainStable();
    void imageConfigProofingAndTransferSignaturesRemainStable();
    void imageConfigGenericStorageAndAlphaSignaturesRemainStable();
};

void KisImageConfigGeneralSchemaContractTest::imageConfigTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisImageConfig>);
    static_assert(std::is_constructible_v<KisImageConfig, bool>);
    static_assert(std::is_destructible_v<KisImageConfig>);

    QVERIFY(true);
}

void KisImageConfigGeneralSchemaContractTest::imageConfigDiagnosticsAndMaskSignaturesRemainStable()
{
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(enablePerfLog, bool (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(enableProgressReporting, bool (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setEnablePerfLog, void (KisImageConfig::*)(bool));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setEnableProgressReporting, void (KisImageConfig::*)(bool));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(transformMaskOffBoundsReadArea, qreal (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(useLodForColorizeMask, bool (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setUseLodForColorizeMask, void (KisImageConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().enablePerfLog()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().enableProgressReporting()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().useLodForColorizeMask()), bool>);

    QVERIFY(true);
}

void KisImageConfigGeneralSchemaContractTest::imageConfigPresentationAndBrushSignaturesRemainStable()
{
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(maxBrushSize, int (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(maxMaskingBrushSize, int (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setMaxBrushSize, void (KisImageConfig::*)(int));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(renameDuplicatedLayers, bool (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(renameMergedLayers, bool (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(selectionOutlineOpacity, qreal (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(selectionOverlayMaskColor, QColor (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setRenameDuplicatedLayers, void (KisImageConfig::*)(bool));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setRenameMergedLayers, void (KisImageConfig::*)(bool));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setSelectionOutlineOpacity, void (KisImageConfig::*)(qreal));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setSelectionOverlayMaskColor, void (KisImageConfig::*)(const QColor &));

    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().maxBrushSize()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().renameDuplicatedLayers()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().renameMergedLayers()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().selectionOutlineOpacity()), qreal>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().selectionOverlayMaskColor()), QColor>);

    QVERIFY(true);
}

void KisImageConfigGeneralSchemaContractTest::imageConfigProofingAndTransferSignaturesRemainStable()
{
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(defaultProofingconfiguration,
                                      KisProofingConfigurationSP (KisImageConfig::*)(bool));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setDefaultProofingConfig,
                                      void (KisImageConfig::*)(const KisProofingConfiguration &));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(exportConfiguration,
                                      KisPropertiesConfigurationSP (KisImageConfig::*)(const QString &, bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(exportConfigurationXML, QString (KisImageConfig::*)(const QString &, bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(hasExportConfiguration, bool (KisImageConfig::*)(const QString &));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(importConfiguration, QString (KisImageConfig::*)(const QString &, bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setExportConfiguration,
                                      void (KisImageConfig::*)(const QString &, KisPropertiesConfigurationSP));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setImportConfiguration,
                                      void (KisImageConfig::*)(const QString &, KisPropertiesConfigurationSP));

    static_assert(std::is_same_v<decltype(std::declval<KisImageConfig &>().defaultProofingconfiguration()),
                                 KisProofingConfigurationSP>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().exportConfiguration(
                                     std::declval<const QString &>())),
                                 KisPropertiesConfigurationSP>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().exportConfigurationXML(
                                     std::declval<const QString &>())),
                                 QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().importConfiguration(
                                     std::declval<const QString &>())),
                                 QString>);

    QVERIFY(true);
}

void KisImageConfigGeneralSchemaContractTest::imageConfigGenericStorageAndAlphaSignaturesRemainStable()
{
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(maxCollectAlpha, qreal (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(maxMergeAlpha, qreal (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(maxMergeCollectAlpha, qreal (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(resetConfig, void (*)());

    using ReadEntrySignature = int (KisImageConfig::*)(const QString &, const int &);
    using ReadListSignature = QList<int> (KisImageConfig::*)(const QString &, const QList<int> &);
    using WriteEntrySignature = void (KisImageConfig::*)(const QString &, const int &);
    using WriteListSignature = void (KisImageConfig::*)(const QString &, const QList<int> &);
    static_assert(
        std::is_same_v<decltype(static_cast<ReadEntrySignature>(&KisImageConfig::readEntry<int>)), ReadEntrySignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ReadListSignature>(&KisImageConfig::readList<int>)), ReadListSignature>);
    static_assert(std::is_same_v<decltype(static_cast<WriteEntrySignature>(&KisImageConfig::writeEntry<int>)),
                                 WriteEntrySignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<WriteListSignature>(&KisImageConfig::writeList<int>)), WriteListSignature>);

    static_assert(
        std::is_same_v<decltype(std::declval<KisImageConfig &>().readEntry<int>(std::declval<const QString &>())),
                       int>);
    static_assert(
        std::is_same_v<decltype(std::declval<KisImageConfig &>().readList<int>(std::declval<const QString &>())),
                       QList<int>>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisImageConfigGeneralSchemaContractTest)

#include "KisImageConfigGeneralSchemaContractTest.moc"
