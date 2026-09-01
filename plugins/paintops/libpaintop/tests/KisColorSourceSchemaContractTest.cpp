/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_color_source.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_COLOR_SOURCE_SIGNATURE(type, method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

class UniformColorSourceConstructionProbe final : public KisUniformColorSource
{
public:
    void selectColor(double, const KisPaintInformation &) override;
};

} // namespace

class KisColorSourceSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorSourceHierarchyAndBaseProtocolSignaturesRemainStable();
    void uniformColorSourceTransformationSignaturesRemainStable();
    void selectedColorSourceVariantSignaturesRemainStable();
    void totalRandomColorSourceSignaturesRemainStable();
    void patternColorSourceSignaturesRemainStable();
};

void KisColorSourceSchemaContractTest::colorSourceHierarchyAndBaseProtocolSignaturesRemainStable()
{
    static_assert(std::is_class_v<KisColorSource>);
    static_assert(std::is_class_v<KisUniformColorSource>);
    static_assert(std::is_class_v<KisPlainColorSource>);
    static_assert(std::is_class_v<KisGradientColorSource>);
    static_assert(std::is_class_v<KisUniformRandomColorSource>);
    static_assert(std::is_class_v<KisTotalRandomColorSource>);
    static_assert(std::is_class_v<KoPatternColorSource>);

    static_assert(std::is_abstract_v<KisColorSource>);
    static_assert(std::is_abstract_v<KisUniformColorSource>);
    static_assert(std::is_base_of_v<KisColorSource, KisUniformColorSource>);
    static_assert(std::is_base_of_v<KisUniformColorSource, KisPlainColorSource>);
    static_assert(std::is_base_of_v<KisUniformColorSource, KisGradientColorSource>);
    static_assert(std::is_base_of_v<KisUniformColorSource, KisUniformRandomColorSource>);
    static_assert(std::is_base_of_v<KisColorSource, KisTotalRandomColorSource>);
    static_assert(std::is_base_of_v<KisColorSource, KoPatternColorSource>);
    static_assert(std::is_destructible_v<KisColorSource>);
    static_assert(std::has_virtual_destructor_v<KisColorSource>);

    ASSERT_COLOR_SOURCE_SIGNATURE(KisColorSource,
                                  selectColor,
                                  void (KisColorSource::*)(double, const KisPaintInformation &));
    ASSERT_COLOR_SOURCE_SIGNATURE(KisColorSource,
                                  applyColorTransformation,
                                  void (KisColorSource::*)(const KoColorTransformation *));
    ASSERT_COLOR_SOURCE_SIGNATURE(KisColorSource, colorSpace, const KoColorSpace *(KisColorSource::*)() const);
    ASSERT_COLOR_SOURCE_SIGNATURE(KisColorSource,
                                  colorize,
                                  void (KisColorSource::*)(KisPaintDeviceSP, const QRect &, const QPoint &) const);
}

void KisColorSourceSchemaContractTest::uniformColorSourceTransformationSignaturesRemainStable()
{
    static_assert(std::is_default_constructible_v<UniformColorSourceConstructionProbe>);
    static_assert(std::is_destructible_v<KisUniformColorSource>);
    static_assert(std::has_virtual_destructor_v<KisUniformColorSource>);

    ASSERT_COLOR_SOURCE_SIGNATURE(KisUniformColorSource, rotate, void (KisUniformColorSource::*)(double));
    ASSERT_COLOR_SOURCE_SIGNATURE(KisUniformColorSource, resize, void (KisUniformColorSource::*)(double, double));
    ASSERT_COLOR_SOURCE_SIGNATURE(KisUniformColorSource,
                                  applyColorTransformation,
                                  void (KisUniformColorSource::*)(const KoColorTransformation *));
    ASSERT_COLOR_SOURCE_SIGNATURE(KisUniformColorSource,
                                  colorSpace,
                                  const KoColorSpace *(KisUniformColorSource::*)() const);
    ASSERT_COLOR_SOURCE_SIGNATURE(KisUniformColorSource,
                                  colorize,
                                  void (KisUniformColorSource::*)(KisPaintDeviceSP, const QRect &, const QPoint &)
                                      const);
    ASSERT_COLOR_SOURCE_SIGNATURE(KisUniformColorSource,
                                  uniformColor,
                                  const KoColor &(KisUniformColorSource::*)() const);
}

void KisColorSourceSchemaContractTest::selectedColorSourceVariantSignaturesRemainStable()
{
    static_assert(std::is_constructible_v<KisPlainColorSource, const KoColor &, const KoColor &>);
    static_assert(std::is_constructible_v<KisGradientColorSource, KoAbstractGradientSP, const KoColorSpace *>);
    static_assert(std::is_default_constructible_v<KisUniformRandomColorSource>);
    static_assert(std::is_destructible_v<KisPlainColorSource>);
    static_assert(std::is_destructible_v<KisGradientColorSource>);
    static_assert(std::is_destructible_v<KisUniformRandomColorSource>);
    static_assert(std::has_virtual_destructor_v<KisPlainColorSource>);
    static_assert(std::has_virtual_destructor_v<KisGradientColorSource>);
    static_assert(std::has_virtual_destructor_v<KisUniformRandomColorSource>);

    ASSERT_COLOR_SOURCE_SIGNATURE(KisPlainColorSource,
                                  selectColor,
                                  void (KisPlainColorSource::*)(double, const KisPaintInformation &));
    ASSERT_COLOR_SOURCE_SIGNATURE(KisGradientColorSource,
                                  selectColor,
                                  void (KisGradientColorSource::*)(double, const KisPaintInformation &));
    ASSERT_COLOR_SOURCE_SIGNATURE(KisUniformRandomColorSource,
                                  selectColor,
                                  void (KisUniformRandomColorSource::*)(double, const KisPaintInformation &));
}

void KisColorSourceSchemaContractTest::totalRandomColorSourceSignaturesRemainStable()
{
    static_assert(std::is_default_constructible_v<KisTotalRandomColorSource>);
    static_assert(std::is_destructible_v<KisTotalRandomColorSource>);
    static_assert(std::has_virtual_destructor_v<KisTotalRandomColorSource>);

    ASSERT_COLOR_SOURCE_SIGNATURE(KisTotalRandomColorSource,
                                  selectColor,
                                  void (KisTotalRandomColorSource::*)(double, const KisPaintInformation &));
    ASSERT_COLOR_SOURCE_SIGNATURE(KisTotalRandomColorSource,
                                  applyColorTransformation,
                                  void (KisTotalRandomColorSource::*)(const KoColorTransformation *));
    ASSERT_COLOR_SOURCE_SIGNATURE(KisTotalRandomColorSource,
                                  colorSpace,
                                  const KoColorSpace *(KisTotalRandomColorSource::*)() const);
    ASSERT_COLOR_SOURCE_SIGNATURE(KisTotalRandomColorSource,
                                  colorize,
                                  void (KisTotalRandomColorSource::*)(KisPaintDeviceSP, const QRect &, const QPoint &)
                                      const);
    ASSERT_COLOR_SOURCE_SIGNATURE(KisTotalRandomColorSource, rotate, void (KisTotalRandomColorSource::*)(double));
    ASSERT_COLOR_SOURCE_SIGNATURE(KisTotalRandomColorSource,
                                  resize,
                                  void (KisTotalRandomColorSource::*)(double, double));
}

void KisColorSourceSchemaContractTest::patternColorSourceSignaturesRemainStable()
{
    static_assert(std::is_constructible_v<KoPatternColorSource, KisPaintDeviceSP, int, int, bool>);
    static_assert(std::is_destructible_v<KoPatternColorSource>);
    static_assert(std::has_virtual_destructor_v<KoPatternColorSource>);

    ASSERT_COLOR_SOURCE_SIGNATURE(KoPatternColorSource,
                                  selectColor,
                                  void (KoPatternColorSource::*)(double, const KisPaintInformation &));
    ASSERT_COLOR_SOURCE_SIGNATURE(KoPatternColorSource,
                                  applyColorTransformation,
                                  void (KoPatternColorSource::*)(const KoColorTransformation *));
    ASSERT_COLOR_SOURCE_SIGNATURE(KoPatternColorSource,
                                  colorSpace,
                                  const KoColorSpace *(KoPatternColorSource::*)() const);
    ASSERT_COLOR_SOURCE_SIGNATURE(KoPatternColorSource,
                                  colorize,
                                  void (KoPatternColorSource::*)(KisPaintDeviceSP, const QRect &, const QPoint &)
                                      const);
    ASSERT_COLOR_SOURCE_SIGNATURE(KoPatternColorSource, rotate, void (KoPatternColorSource::*)(double));
    ASSERT_COLOR_SOURCE_SIGNATURE(KoPatternColorSource, resize, void (KoPatternColorSource::*)(double, double));
}

#undef ASSERT_COLOR_SOURCE_SIGNATURE

QTEST_GUILESS_MAIN(KisColorSourceSchemaContractTest)

#include "KisColorSourceSchemaContractTest.moc"
