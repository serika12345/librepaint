/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <QTest>
#include <View.h>
#include <type_traits>

#define ASSERT_VIEW_SIGNATURE(method, signature)                                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&View::method)), signature>)

class ViewSchemaContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void viewOwnershipLifetimeAndEqualitySchemaRemainsStable();
    void viewBrushGeometrySignaturesRemainStable();
    void viewDepositAndCompositeSignaturesRemainStable();
    void viewInputModePolicySignaturesRemainStable();
    void viewHdrPresentationSignaturesRemainStable();
};

void ViewSchemaContractTest::viewOwnershipLifetimeAndEqualitySchemaRemainsStable()
{
    static_assert(std::is_class_v<View>);
    static_assert(std::is_base_of_v<QObject, View>);
    static_assert(std::is_constructible_v<View, KisView *>);
    static_assert(std::is_constructible_v<View, KisView *, QObject *>);
    static_assert(std::has_virtual_destructor_v<View>);
    ASSERT_VIEW_SIGNATURE(operator==, bool (View::*)(const View &) const);
    ASSERT_VIEW_SIGNATURE(operator!=, bool (View::*)(const View &) const);
}

void ViewSchemaContractTest::viewBrushGeometrySignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(brushSize, qreal (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setBrushSize, void (View::*)(qreal));
    ASSERT_VIEW_SIGNATURE(brushRotation, qreal (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setBrushRotation, void (View::*)(qreal));
    ASSERT_VIEW_SIGNATURE(brushFade, qreal (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setBrushFade, void (View::*)(qreal));
    ASSERT_VIEW_SIGNATURE(patternSize, qreal (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setPatternSize, void (View::*)(qreal));
}

void ViewSchemaContractTest::viewDepositAndCompositeSignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(paintingFlow, qreal (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setPaintingFlow, void (View::*)(qreal));
    ASSERT_VIEW_SIGNATURE(paintingOpacity, qreal (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setPaintingOpacity, void (View::*)(qreal));
    ASSERT_VIEW_SIGNATURE(currentBlendingMode, QString (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setCurrentBlendingMode, void (View::*)(const QString &));
}

void ViewSchemaContractTest::viewInputModePolicySignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(disablePressure, bool (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setDisablePressure, void (View::*)(bool));
    ASSERT_VIEW_SIGNATURE(eraserMode, bool (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setEraserMode, void (View::*)(bool));
    ASSERT_VIEW_SIGNATURE(globalAlphaLock, bool (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setGlobalAlphaLock, void (View::*)(bool));
}

void ViewSchemaContractTest::viewHdrPresentationSignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(HDRExposure, float (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setHDRExposure, void (View::*)(float));
    ASSERT_VIEW_SIGNATURE(HDRGamma, float (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setHDRGamma, void (View::*)(float));
}

QTEST_GUILESS_MAIN(ViewSchemaContractTest)
#include "ViewSchemaContractTest.moc"
