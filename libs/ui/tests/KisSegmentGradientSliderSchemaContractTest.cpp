/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <widgets/gradient/KisSegmentGradientSlider.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KisSegmentGradientSliderSchemaContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void segmentGradientSliderHandleSchemaRemainsStable();
    void segmentGradientSliderConstructionAndSelectionSchemaRemainsStable();
    void segmentGradientSliderNavigationAndNotificationSchemaRemainsStable();
    void segmentGradientSliderHandleMovementSchemaRemainsStable();
    void segmentGradientSliderMutationSchemaRemainsStable();
};

void KisSegmentGradientSliderSchemaContractTest::segmentGradientSliderHandleSchemaRemainsStable()
{
    using Slider = KisSegmentGradientSlider;
    using Handle = Slider::Handle;
    static_assert(std::is_class_v<Slider> && std::is_enum_v<Slider::HandleType> && std::is_class_v<Handle>);
    static_assert(Slider::HandleType_None == 0 && Slider::HandleType_Segment == 1);
    static_assert(Slider::HandleType_MidPoint == 2 && Slider::HandleType_Stop == 3);
    static_assert(std::is_same_v<decltype(&Handle::type), Slider::HandleType Handle::*>);
    static_assert(std::is_same_v<decltype(&Handle::index), int Handle::*>);
    static_assert(Slider::shrinkEpsilon == qreal(0.00001));
}

void KisSegmentGradientSliderSchemaContractTest::segmentGradientSliderConstructionAndSelectionSchemaRemainsStable()
{
    using Slider = KisSegmentGradientSlider;
    static_assert(std::is_constructible_v<Slider, QWidget *, const char *, Qt::WindowFlags>);
    static_assert(std::is_default_constructible_v<Slider>);
    static_assert(std::is_same_v<decltype(&Slider::selectedHandle), Slider::Handle (Slider::*)()>);
    static_assert(std::is_same_v<decltype(&Slider::sizeHint), QSize (Slider::*)() const>);
    static_assert(std::is_same_v<decltype(&Slider::minimumSizeHint), QSize (Slider::*)() const>);
    static_assert(std::is_same_v<decltype(&Slider::setGradientResource), void (Slider::*)(KoSegmentGradientSP)>);
}

void KisSegmentGradientSliderSchemaContractTest::segmentGradientSliderNavigationAndNotificationSchemaRemainsStable()
{
    using Slider = KisSegmentGradientSlider;
    using Action = void (Slider::*)();
    static_assert(std::is_same_v<decltype(&Slider::chooseSelectedStopColor), Action>);
    static_assert(std::is_same_v<decltype(&Slider::selectPreviousHandle), Action>);
    static_assert(std::is_same_v<decltype(&Slider::selectNextHandle), Action>);
    static_assert(std::is_same_v<decltype(&Slider::centerSelectedHandle), Action>);
    static_assert(std::is_same_v<decltype(&Slider::selectedHandleChanged), Action>);
    static_assert(std::is_same_v<decltype(&Slider::updateRequested), Action>);
}

void KisSegmentGradientSliderSchemaContractTest::segmentGradientSliderHandleMovementSchemaRemainsStable()
{
    using Slider = KisSegmentGradientSlider;
    using HandleMove = void (Slider::*)(Slider::Handle, qreal, bool);
    using SelectionMove = void (Slider::*)(qreal, bool);
    static_assert(std::is_same_v<decltype(&Slider::moveHandle), HandleMove>);
    static_assert(std::is_same_v<decltype(&Slider::moveHandleLeft), HandleMove>);
    static_assert(std::is_same_v<decltype(&Slider::moveHandleRight), HandleMove>);
    static_assert(std::is_same_v<decltype(&Slider::moveSelectedHandle), SelectionMove>);
    static_assert(std::is_same_v<decltype(&Slider::moveSelectedHandleLeft), SelectionMove>);
    static_assert(std::is_same_v<decltype(&Slider::moveSelectedHandleRight), SelectionMove>);
    static_assert(std::is_same_v<decltype(&Slider::deleteHandle), void (Slider::*)(Slider::Handle)>);
    static_assert(std::is_same_v<decltype(std::declval<Slider &>().moveHandle({}, qreal{})), void>);
    static_assert(std::is_same_v<decltype(std::declval<Slider &>().moveHandleLeft({}, qreal{})), void>);
    static_assert(std::is_same_v<decltype(std::declval<Slider &>().moveHandleRight({}, qreal{})), void>);
    static_assert(std::is_same_v<decltype(std::declval<Slider &>().moveSelectedHandle(qreal{})), void>);
    static_assert(std::is_same_v<decltype(std::declval<Slider &>().moveSelectedHandleLeft(qreal{})), void>);
    static_assert(std::is_same_v<decltype(std::declval<Slider &>().moveSelectedHandleRight(qreal{})), void>);
}

void KisSegmentGradientSliderSchemaContractTest::segmentGradientSliderMutationSchemaRemainsStable()
{
    using Slider = KisSegmentGradientSlider;
    using Action = void (Slider::*)();
    static_assert(std::is_same_v<decltype(&Slider::deleteSelectedHandle), Action>);
    static_assert(std::is_same_v<decltype(&Slider::collapseSelectedSegment), Action>);
    static_assert(std::is_same_v<decltype(&Slider::splitSelectedSegment), Action>);
    static_assert(std::is_same_v<decltype(&Slider::duplicateSelectedSegment), Action>);
    static_assert(std::is_same_v<decltype(&Slider::mirrorSelectedSegment), Action>);
    static_assert(std::is_same_v<decltype(&Slider::flipGradient), Action>);
    static_assert(std::is_same_v<decltype(&Slider::distributeStopsEvenly), Action>);
}

QTEST_GUILESS_MAIN(KisSegmentGradientSliderSchemaContractTest)
#include "KisSegmentGradientSliderSchemaContractTest.moc"
