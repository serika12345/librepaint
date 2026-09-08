/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_default_bounds_node_wrapper.h"

#include <QTest>

#include <type_traits>

#define ASSERT_DEFAULT_BOUNDS_WRAPPER_SIGNATURE(method, signature)                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisDefaultBoundsNodeWrapper::method)), signature>)

class KisDefaultBoundsNodeWrapperSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeOwnershipConstructionAndLifetimeSchemaRemainStable();
    void infiniteRectSchemaRemainsStable();
    void boundsSignaturesRemainStable();
    void wrapLodAndTimeSignaturesRemainStable();
    void externalFrameAndSourceSignaturesRemainStable();
};

void KisDefaultBoundsNodeWrapperSchemaContractTest::typeOwnershipConstructionAndLifetimeSchemaRemainStable()
{
    using Wrapper = KisDefaultBoundsNodeWrapper;

    static_assert(std::is_class_v<Wrapper>);
    static_assert(std::is_base_of_v<KisDefaultBoundsBase, Wrapper>);
    static_assert(std::is_same_v<KisDefaultBoundsNodeWrapperSP, KisSharedPtr<Wrapper>>);
    static_assert(std::is_default_constructible_v<Wrapper>);
    static_assert(std::is_constructible_v<Wrapper, KisBaseNodeWSP>);
    static_assert(std::is_constructible_v<Wrapper, Wrapper &>);
    static_assert(std::has_virtual_destructor_v<Wrapper>);
}

void KisDefaultBoundsNodeWrapperSchemaContractTest::infiniteRectSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(KisDefaultBoundsNodeWrapper::infiniteRect), const QRect>);
}

void KisDefaultBoundsNodeWrapperSchemaContractTest::boundsSignaturesRemainStable()
{
    ASSERT_DEFAULT_BOUNDS_WRAPPER_SIGNATURE(bounds, QRect (KisDefaultBoundsNodeWrapper::*)() const);
    ASSERT_DEFAULT_BOUNDS_WRAPPER_SIGNATURE(imageBorderRect, QRect (KisDefaultBoundsNodeWrapper::*)() const);
}

void KisDefaultBoundsNodeWrapperSchemaContractTest::wrapLodAndTimeSignaturesRemainStable()
{
    ASSERT_DEFAULT_BOUNDS_WRAPPER_SIGNATURE(wrapAroundMode, bool (KisDefaultBoundsNodeWrapper::*)() const);
    ASSERT_DEFAULT_BOUNDS_WRAPPER_SIGNATURE(wrapAroundModeAxis,
                                            WrapAroundAxis (KisDefaultBoundsNodeWrapper::*)() const);
    ASSERT_DEFAULT_BOUNDS_WRAPPER_SIGNATURE(currentLevelOfDetail, int (KisDefaultBoundsNodeWrapper::*)() const);
    ASSERT_DEFAULT_BOUNDS_WRAPPER_SIGNATURE(currentTime, int (KisDefaultBoundsNodeWrapper::*)() const);
}

void KisDefaultBoundsNodeWrapperSchemaContractTest::externalFrameAndSourceSignaturesRemainStable()
{
    ASSERT_DEFAULT_BOUNDS_WRAPPER_SIGNATURE(externalFrameActive, bool (KisDefaultBoundsNodeWrapper::*)() const);
    ASSERT_DEFAULT_BOUNDS_WRAPPER_SIGNATURE(sourceCookie, void *(KisDefaultBoundsNodeWrapper::*)() const);
}

QTEST_APPLESS_MAIN(KisDefaultBoundsNodeWrapperSchemaContractTest)

#include "KisDefaultBoundsNodeWrapperSchemaContractTest.moc"
