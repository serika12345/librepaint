/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "resources/KoCssStylePreset.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
using CssStylePreset = KoCssStylePreset;
using ResourceTypePair = QPair<QString, QString>;

#define ASSERT_CSS_STYLE_PRESET_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&CssStylePreset::method)), signature>)
} // namespace

class KoCssStylePresetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cssStylePresetTypeLifetimeAndConstructionSchemaRemainStable();
    void cssStylePresetPropertyAndResolutionSignaturesRemainStable();
    void cssStylePresetIdentityAndClassificationSignaturesRemainStable();
    void cssStylePresetSampleAndPresentationSignaturesRemainStable();
    void cssStylePresetPersistenceSignaturesRemainStable();
};

void KoCssStylePresetSchemaContractTest::cssStylePresetTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_same_v<KoCssStylePresetSP, QSharedPointer<CssStylePreset>>);
    static_assert(std::is_class_v<CssStylePreset>);
    static_assert(std::is_base_of_v<KoResource, CssStylePreset>);
    static_assert(std::has_virtual_destructor_v<CssStylePreset>);
    static_assert(std::is_constructible_v<CssStylePreset, const QString &>);
    static_assert(std::is_copy_constructible_v<CssStylePreset>);
    static_assert(!std::is_copy_assignable_v<CssStylePreset>);
}

void KoCssStylePresetSchemaContractTest::cssStylePresetPropertyAndResolutionSignaturesRemainStable()
{
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(properties, KoSvgTextProperties (CssStylePreset::*)(int, bool) const);
    static_assert(std::is_same_v<decltype(std::declval<const CssStylePreset &>().properties()), KoSvgTextProperties>);
    static_assert(
        std::is_same_v<decltype(std::declval<const CssStylePreset &>().properties(144)), KoSvgTextProperties>);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(setProperties, void (CssStylePreset::*)(const KoSvgTextProperties &));
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(storedPPIResolution, int (CssStylePreset::*)() const);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(setStoredPPIResolution, void (CssStylePreset::*)(int));
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(primaryFontFamily, QString (CssStylePreset::*)() const);
}

void KoCssStylePresetSchemaContractTest::cssStylePresetIdentityAndClassificationSignaturesRemainStable()
{
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(description, QString (CssStylePreset::*)() const);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(setDescription, void (CssStylePreset::*)(const QString &));
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(styleType, QString (CssStylePreset::*)() const);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(setStyleType, void (CssStylePreset::*)(const QString &));
}

void KoCssStylePresetSchemaContractTest::cssStylePresetSampleAndPresentationSignaturesRemainStable()
{
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(sampleText, QString (CssStylePreset::*)() const);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(setSampleText, void (CssStylePreset::*)(const QString &));
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(beforeText, QString (CssStylePreset::*)() const);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(setBeforeText, void (CssStylePreset::*)(const QString &));
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(afterText, QString (CssStylePreset::*)() const);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(setAfterText, void (CssStylePreset::*)(const QString &));
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(sampleSvg, QString (CssStylePreset::*)() const);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(paragraphSampleSize, QSizeF (CssStylePreset::*)() const);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(setParagraphSampleSize, void (CssStylePreset::*)(QSizeF));
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(generateSampleShape, KoShape * (CssStylePreset::*)() const);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(alignSample, Qt::Alignment (CssStylePreset::*)() const);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(updateAlignSample, void (CssStylePreset::*)());
}

void KoCssStylePresetSchemaContractTest::cssStylePresetPersistenceSignaturesRemainStable()
{
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(clone, KoResourceSP (CssStylePreset::*)() const);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(loadFromDevice, bool (CssStylePreset::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(saveToDevice, bool (CssStylePreset::*)(QIODevice *) const);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(defaultFileExtension, QString (CssStylePreset::*)() const);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(resourceType, ResourceTypePair (CssStylePreset::*)() const);
    ASSERT_CSS_STYLE_PRESET_SIGNATURE(updateThumbnail, void (CssStylePreset::*)());
}

QTEST_APPLESS_MAIN(KoCssStylePresetSchemaContractTest)

#include "KoCssStylePresetSchemaContractTest.moc"
