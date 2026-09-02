/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_brush.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_BRUSH_SIGNATURE(method, signature)                                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisBrush::method)), signature>)
} // namespace

class KisBrushSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void brushIdentityAndTypeSchemaRemainsStable();
    void brushColoringConstantsAndMetadataSchemaRemainsStable();
    void brushSizeAndAngleSignaturesRemainStable();
    void brushSpacingPolicySignaturesRemainStable();
    void brushApplicationPolicySignaturesRemainStable();
};

void KisBrushSchemaContractTest::brushIdentityAndTypeSchemaRemainsStable()
{
    static_assert(std::is_same_v<KisBrushSP, QSharedPointer<KisBrush>>);
    static_assert(std::is_same_v<KoAbstractGradientSP, QSharedPointer<KoAbstractGradient>>);
    static_assert(std::is_class_v<KisBrush>);
    static_assert(std::is_class_v<KisBrush::ColoringInformation>);
    static_assert(std::is_enum_v<enumBrushType>);
    static_assert(INVALID == 0);
    static_assert(MASK == 1);
    static_assert(IMAGE == 2);
    static_assert(PIPE_MASK == 3);
    static_assert(PIPE_IMAGE == 4);
    static_assert(std::is_enum_v<enumBrushApplication>);
    static_assert(ALPHAMASK == 0);
    static_assert(IMAGESTAMP == 1);
    static_assert(LIGHTNESSMAP == 2);
    static_assert(GRADIENTMAP == 3);
}

void KisBrushSchemaContractTest::brushColoringConstantsAndMetadataSchemaRemainsStable()
{
    using ColoringInformation = KisBrush::ColoringInformation;

    static_assert(std::is_same_v<decltype(KisBrush::brushTypeMetaDataKey), const QString>);
    static_assert(std::is_same_v<decltype(DEFAULT_LIGHTNESS_STRENGTH), const qreal>);
    static_assert(std::is_same_v<decltype(DEFAULT_SOFTNESS_FACTOR), const qreal>);
    static_assert(std::has_virtual_destructor_v<ColoringInformation>);
    static_assert(
        std::is_same_v<decltype(&ColoringInformation::color), const quint8 *(ColoringInformation::*)() const>);
    static_assert(std::is_same_v<decltype(&ColoringInformation::nextColumn), void (ColoringInformation::*)()>);
    static_assert(std::is_same_v<decltype(&ColoringInformation::nextRow), void (ColoringInformation::*)()>);
}

void KisBrushSchemaContractTest::brushSizeAndAngleSignaturesRemainStable()
{
    ASSERT_BRUSH_SIGNATURE(width, qint32 (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(height, qint32 (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(userEffectiveSize, qreal (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(setUserEffectiveSize, void (KisBrush::*)(qreal));
    ASSERT_BRUSH_SIGNATURE(scale, qreal (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(setScale, void (KisBrush::*)(qreal));
    ASSERT_BRUSH_SIGNATURE(angle, qreal (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(setAngle, void (KisBrush::*)(qreal));
}

void KisBrushSchemaContractTest::brushSpacingPolicySignaturesRemainStable()
{
    ASSERT_BRUSH_SIGNATURE(spacing, double (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(setSpacing, void (KisBrush::*)(double));
    ASSERT_BRUSH_SIGNATURE(autoSpacingActive, bool (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(autoSpacingCoeff, qreal (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(setAutoSpacing, void (KisBrush::*)(bool, qreal));
}

void KisBrushSchemaContractTest::brushApplicationPolicySignaturesRemainStable()
{
    using ResourceTypeSignature = QPair<QString, QString> (KisBrush::*)() const;

    ASSERT_BRUSH_SIGNATURE(brushType, enumBrushType (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(brushApplication, enumBrushApplication (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(setBrushApplication, void (KisBrush::*)(enumBrushApplication));
    ASSERT_BRUSH_SIGNATURE(applyingGradient, bool (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(preserveLightness, bool (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(supportsCaching, bool (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(isPiercedApprox, bool (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(resourceType, ResourceTypeSignature);
}

QTEST_GUILESS_MAIN(KisBrushSchemaContractTest)

#include "KisBrushSchemaContractTest.moc"
