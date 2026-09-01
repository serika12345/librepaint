/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoRuler.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_RULER_SIGNATURE(method, signature)                                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoRuler::method)), signature>)
} // namespace

class KoRulerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rulerTypeAndMeasurementSchemaRemainsStable();
    void rulerRangeAndIndentSchemaRemainsStable();
    void rulerPointerAndGuideSchemaRemainsStable();
    void rulerTabAndPopupSchemaRemainsStable();
    void rulerHotSpotSchemaRemainsStable();
};

void KoRulerSchemaContractTest::rulerTypeAndMeasurementSchemaRemainsStable()
{
    using SizeGetter = QSize (KoRuler::*)() const;
    using UnitGetter = KoUnit (KoRuler::*)() const;
    using UnitSetter = void (KoRuler::*)(const KoUnit &);
    using OrientationGetter = Qt::Orientation (KoRuler::*)() const;
    using RealGetter = qreal (KoRuler::*)() const;
    using RealSetter = void (KoRuler::*)(qreal);
    using OffsetSetter = void (KoRuler::*)(int);
    using ToggleSetter = void (KoRuler::*)(bool);

    static_assert(std::is_class_v<KoRuler>);
    static_assert(std::is_base_of_v<QWidget, KoRuler>);
    static_assert(std::is_constructible_v<KoRuler, QWidget *, Qt::Orientation, const KoViewConverter *>);
    static_assert(std::is_destructible_v<KoRuler>);
    static_assert(std::has_virtual_destructor_v<KoRuler>);

    ASSERT_RULER_SIGNATURE(minimumSizeHint, SizeGetter);
    ASSERT_RULER_SIGNATURE(sizeHint, SizeGetter);
    ASSERT_RULER_SIGNATURE(unit, UnitGetter);
    ASSERT_RULER_SIGNATURE(setUnit, UnitSetter);
    ASSERT_RULER_SIGNATURE(orientation, OrientationGetter);
    ASSERT_RULER_SIGNATURE(rulerLength, RealGetter);
    ASSERT_RULER_SIGNATURE(setRulerLength, RealSetter);
    ASSERT_RULER_SIGNATURE(setOffset, OffsetSetter);
    ASSERT_RULER_SIGNATURE(setUnitPixelMultiple2, ToggleSetter);
}

void KoRulerSchemaContractTest::rulerRangeAndIndentSchemaRemainsStable()
{
    using RealRangeSetter = void (KoRuler::*)(qreal, qreal);
    using ToggleSetter = void (KoRuler::*)(bool);
    using RealGetter = qreal (KoRuler::*)() const;
    using RealSetter = void (KoRuler::*)(qreal);
    using NoArgumentSignal = void (KoRuler::*)();

    ASSERT_RULER_SIGNATURE(setActiveRange, RealRangeSetter);
    ASSERT_RULER_SIGNATURE(setOverrideActiveRange, RealRangeSetter);
    ASSERT_RULER_SIGNATURE(setRightToLeft, ToggleSetter);
    ASSERT_RULER_SIGNATURE(setShowIndents, ToggleSetter);
    ASSERT_RULER_SIGNATURE(firstLineIndent, RealGetter);
    ASSERT_RULER_SIGNATURE(paragraphIndent, RealGetter);
    ASSERT_RULER_SIGNATURE(endIndent, RealGetter);
    ASSERT_RULER_SIGNATURE(setFirstLineIndent, RealSetter);
    ASSERT_RULER_SIGNATURE(setParagraphIndent, RealSetter);
    ASSERT_RULER_SIGNATURE(setEndIndent, RealSetter);
    ASSERT_RULER_SIGNATURE(indentsChanged, ToggleSetter);
    ASSERT_RULER_SIGNATURE(aboutToChange, NoArgumentSignal);
}

void KoRulerSchemaContractTest::rulerPointerAndGuideSchemaRemainsStable()
{
    using ToggleSetter = void (KoRuler::*)(bool);
    using ToggleGetter = bool (KoRuler::*)() const;
    using CoordinateSetter = void (KoRuler::*)(int);
    using SelectionBorderSetter = void (KoRuler::*)(qreal, qreal);
    using GuideConnector = void (KoRuler::*)(KoCanvasBase *);
    using GuideLineSignal = void (KoRuler::*)(Qt::Orientation, qreal);
    using GuidePositionSignal = void (KoRuler::*)(Qt::Orientation, const QPoint &);

    ASSERT_RULER_SIGNATURE(setShowMousePosition, ToggleSetter);
    ASSERT_RULER_SIGNATURE(showMousePosition, ToggleGetter);
    ASSERT_RULER_SIGNATURE(updateMouseCoordinate, CoordinateSetter);
    ASSERT_RULER_SIGNATURE(setShowSelectionBorders, ToggleSetter);
    ASSERT_RULER_SIGNATURE(updateSelectionBorders, SelectionBorderSetter);
    ASSERT_RULER_SIGNATURE(createGuideToolConnection, GuideConnector);
    ASSERT_RULER_SIGNATURE(guideLineCreated, GuideLineSignal);
    ASSERT_RULER_SIGNATURE(guideCreationInProgress, GuidePositionSignal);
    ASSERT_RULER_SIGNATURE(guideCreationFinished, GuidePositionSignal);
}

void KoRulerSchemaContractTest::rulerTabAndPopupSchemaRemainsStable()
{
    using TabPositionMember = qreal KoRuler::Tab::*;
    using TabTypeMember = QTextOption::TabType KoRuler::Tab::*;
    using ToggleSetter = void (KoRuler::*)(bool);
    using TabUpdater = void (KoRuler::*)(const QList<KoRuler::Tab> &, qreal);
    using TabsGetter = QList<KoRuler::Tab> (KoRuler::*)() const;
    using TabChooserGetter = QWidget *(KoRuler::*)();
    using TabChangedSignal = void (KoRuler::*)(int, KoRuler::Tab *);
    using PopupSetter = void (KoRuler::*)(const QList<QAction *> &);
    using PopupGetter = QList<QAction *> (KoRuler::*)() const;

    static_assert(std::is_class_v<KoRuler::Tab>);
    static_assert(std::is_same_v<decltype(&KoRuler::Tab::position), TabPositionMember>);
    static_assert(std::is_same_v<decltype(&KoRuler::Tab::type), TabTypeMember>);
    constexpr KoRuler::Tab defaultTab{};
    static_assert(defaultTab.position == qreal{0.0});
    static_assert(defaultTab.type == QTextOption::LeftTab);

    ASSERT_RULER_SIGNATURE(setShowTabs, ToggleSetter);
    ASSERT_RULER_SIGNATURE(setRelativeTabs, ToggleSetter);
    ASSERT_RULER_SIGNATURE(updateTabs, TabUpdater);
    ASSERT_RULER_SIGNATURE(tabs, TabsGetter);
    ASSERT_RULER_SIGNATURE(tabChooser, TabChooserGetter);
    ASSERT_RULER_SIGNATURE(tabChanged, TabChangedSignal);
    ASSERT_RULER_SIGNATURE(setPopupActionList, PopupSetter);
    ASSERT_RULER_SIGNATURE(popupActionList, PopupGetter);
}

void KoRulerSchemaContractTest::rulerHotSpotSchemaRemainsStable()
{
    using ClearHotSpots = void (KoRuler::*)();
    using HotSpotSetter = void (KoRuler::*)(qreal, int);
    using HotSpotRemover = bool (KoRuler::*)(int);
    using HotSpotChangedSignal = void (KoRuler::*)(int, qreal);

    ASSERT_RULER_SIGNATURE(clearHotSpots, ClearHotSpots);
    ASSERT_RULER_SIGNATURE(setHotSpot, HotSpotSetter);
    ASSERT_RULER_SIGNATURE(removeHotSpot, HotSpotRemover);
    ASSERT_RULER_SIGNATURE(hotSpotChanged, HotSpotChangedSignal);
    static_assert(std::is_same_v<decltype(std::declval<KoRuler &>().setHotSpot(qreal{})), void>);
}

#undef ASSERT_RULER_SIGNATURE

QTEST_GUILESS_MAIN(KoRulerSchemaContractTest)

#include "KoRulerSchemaContractTest.moc"
