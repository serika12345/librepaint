/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisAngleSelector.h"
#include "kis_aspect_ratio_locker.h"

#include <QTest>

#include <type_traits>
#include <utility>

class AngleBoxProbe;
class SpinBoxProbe;

class KisAngleSelectorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void angleSelectorFlipPresentationSchemaRemainsStable();
    void angleSelectorPrimaryAngleSchemaRemainsStable();
    void angleSelectorRangePrecisionAndWrappingSchemaRemainsStable();
    void angleSelectorLocalPresentationSchemaRemainsStable();
    void angleSelectorStaticTransformSignaturesRemainStable();
    void angleSelectorLifetimeAndNotificationSchemaRemainStable();
    void angleSelectorSpinBoxTypeAndLifetimeSchemaRemainStable();
    void angleSelectorSpinBoxPresentationSchemaRemainStable();
    void aspectRatioLockerTypeLifetimeAndConnectionSchemaRemainStable();
    void aspectRatioLockerUpdateAndNotificationSchemaRemainStable();
};

void KisAngleSelectorSchemaContractTest::angleSelectorFlipPresentationSchemaRemainsStable()
{
    using Selector = KisAngleSelector;
    using Mode = Selector::FlipOptionsMode;
    using Getter = Mode (Selector::*)() const;
    using Setter = void (Selector::*)(Mode);

    static_assert(std::is_class_v<Selector>);
    static_assert(std::is_base_of_v<QWidget, Selector>);
    static_assert(std::is_enum_v<Mode>);
    static_assert(std::is_same_v<decltype(static_cast<Getter>(&Selector::flipOptionsMode)), Getter>);
    static_assert(std::is_same_v<decltype(static_cast<Setter>(&Selector::setFlipOptionsMode)), Setter>);

    QCOMPARE(static_cast<int>(Mode::FlipOptionsMode_NoFlipOptions), 0);
    QCOMPARE(static_cast<int>(Mode::FlipOptionsMode_MenuButton), 1);
    QCOMPARE(static_cast<int>(Mode::FlipOptionsMode_Buttons), 2);
    QCOMPARE(static_cast<int>(Mode::FlipOptionsMode_ContextMenu), 3);
}

void KisAngleSelectorSchemaContractTest::angleSelectorPrimaryAngleSchemaRemainsStable()
{
    using Selector = KisAngleSelector;
    using Getter = qreal (Selector::*)() const;
    using Setter = void (Selector::*)(qreal);

    static_assert(std::is_same_v<decltype(static_cast<Getter>(&Selector::angle)), Getter>);
    static_assert(std::is_same_v<decltype(static_cast<Setter>(&Selector::setAngle)), Setter>);
    static_assert(std::is_same_v<decltype(static_cast<Getter>(&Selector::snapAngle)), Getter>);
    static_assert(std::is_same_v<decltype(static_cast<Setter>(&Selector::setSnapAngle)), Setter>);
    static_assert(std::is_same_v<decltype(static_cast<Getter>(&Selector::resetAngle)), Getter>);
    static_assert(std::is_same_v<decltype(static_cast<Setter>(&Selector::setResetAngle)), Setter>);
}

void KisAngleSelectorSchemaContractTest::angleSelectorRangePrecisionAndWrappingSchemaRemainsStable()
{
    using Selector = KisAngleSelector;
    using DecimalsGetter = int (Selector::*)() const;
    using DecimalsSetter = void (Selector::*)(int);
    using LimitGetter = qreal (Selector::*)() const;
    using LimitSetter = void (Selector::*)(qreal);
    using RangeSetter = void (Selector::*)(qreal, qreal);
    using WrappingGetter = bool (Selector::*)() const;
    using WrappingSetter = void (Selector::*)(bool);

    static_assert(std::is_same_v<decltype(static_cast<DecimalsGetter>(&Selector::decimals)), DecimalsGetter>);
    static_assert(std::is_same_v<decltype(static_cast<DecimalsSetter>(&Selector::setDecimals)), DecimalsSetter>);
    static_assert(std::is_same_v<decltype(static_cast<LimitGetter>(&Selector::maximum)), LimitGetter>);
    static_assert(std::is_same_v<decltype(static_cast<LimitSetter>(&Selector::setMaximum)), LimitSetter>);
    static_assert(std::is_same_v<decltype(static_cast<LimitGetter>(&Selector::minimum)), LimitGetter>);
    static_assert(std::is_same_v<decltype(static_cast<LimitSetter>(&Selector::setMinimum)), LimitSetter>);
    static_assert(std::is_same_v<decltype(static_cast<RangeSetter>(&Selector::setRange)), RangeSetter>);
    static_assert(std::is_same_v<decltype(static_cast<WrappingGetter>(&Selector::wrapping)), WrappingGetter>);
    static_assert(std::is_same_v<decltype(static_cast<WrappingSetter>(&Selector::setWrapping)), WrappingSetter>);
}

void KisAngleSelectorSchemaContractTest::angleSelectorLocalPresentationSchemaRemainsStable()
{
    using Selector = KisAngleSelector;
    using PrefixGetter = QString (Selector::*)() const;
    using PrefixSetter = void (Selector::*)(const QString &);
    using HeightGetter = int (Selector::*)() const;
    using HeightSetter = void (Selector::*)(int);
    using Direction = KisAngleGauge::IncreasingDirection;
    using DirectionGetter = Direction (Selector::*)() const;
    using DirectionSetter = void (Selector::*)(Direction);
    using FlatGetter = bool (Selector::*)() const;
    using FlatSetter = void (Selector::*)(bool);

    static_assert(std::is_same_v<decltype(static_cast<PrefixGetter>(&Selector::prefix)), PrefixGetter>);
    static_assert(std::is_same_v<decltype(static_cast<PrefixSetter>(&Selector::setPrefix)), PrefixSetter>);
    static_assert(std::is_same_v<decltype(static_cast<HeightGetter>(&Selector::widgetsHeight)), HeightGetter>);
    static_assert(std::is_same_v<decltype(static_cast<HeightSetter>(&Selector::setWidgetsHeight)), HeightSetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<DirectionGetter>(&Selector::increasingDirection)), DirectionGetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<DirectionSetter>(&Selector::setIncreasingDirection)), DirectionSetter>);
    static_assert(std::is_same_v<decltype(static_cast<FlatGetter>(&Selector::isUsingFlatSpinBox)), FlatGetter>);
    static_assert(std::is_same_v<decltype(static_cast<FlatSetter>(&Selector::useFlatSpinBox)), FlatSetter>);
}

void KisAngleSelectorSchemaContractTest::angleSelectorStaticTransformSignaturesRemainStable()
{
    using Selector = KisAngleSelector;
    using ClosestCoterminal = qreal (*)(qreal, qreal, qreal, bool *);
    using Flip = qreal (*)(qreal, Qt::Orientations);
    using FlipInRange = qreal (*)(qreal, qreal, qreal, Qt::Orientations, bool *);

    static_assert(std::is_same_v<decltype(static_cast<ClosestCoterminal>(&Selector::closestCoterminalAngleInRange)),
                                 ClosestCoterminal>);
    static_assert(std::is_same_v<decltype(static_cast<Flip>(&Selector::flipAngle)), Flip>);
    static_assert(std::is_same_v<decltype(static_cast<FlipInRange>(&Selector::flipAngle)), FlipInRange>);
}

void KisAngleSelectorSchemaContractTest::angleSelectorLifetimeAndNotificationSchemaRemainStable()
{
    using Selector = KisAngleSelector;
    using ClosestCoterminal = qreal (Selector::*)(qreal, bool *) const;
    using Flip = void (Selector::*)(Qt::Orientations);
    using Reset = void (Selector::*)();
    using AngleChanged = void (Selector::*)(qreal);

    static_assert(std::is_default_constructible_v<Selector>);
    static_assert(std::is_constructible_v<Selector, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Selector>);
    static_assert(std::is_same_v<decltype(static_cast<ClosestCoterminal>(&Selector::closestCoterminalAngleInRange)),
                                 ClosestCoterminal>);
    static_assert(std::is_same_v<decltype(static_cast<Flip>(&Selector::flip)), Flip>);
    static_assert(std::is_same_v<decltype(static_cast<Reset>(&Selector::reset)), Reset>);
    static_assert(std::is_same_v<decltype(static_cast<AngleChanged>(&Selector::angleChanged)), AngleChanged>);

    using ClosestWithDefaultOk =
        decltype(std::declval<const Selector &>().closestCoterminalAngleInRange(std::declval<qreal>()));
    static_assert(std::is_same_v<ClosestWithDefaultOk, qreal>);
}

void KisAngleSelectorSchemaContractTest::angleSelectorSpinBoxTypeAndLifetimeSchemaRemainStable()
{
    using SpinBox = KisAngleSelectorSpinBox;

    static_assert(std::is_class_v<SpinBox>);
    static_assert(std::is_base_of_v<KisDoubleParseSpinBox, SpinBox>);
    static_assert(std::is_default_constructible_v<SpinBox>);
    static_assert(std::is_constructible_v<SpinBox, QWidget *>);
    static_assert(std::has_virtual_destructor_v<SpinBox>);
}

void KisAngleSelectorSchemaContractTest::angleSelectorSpinBoxPresentationSchemaRemainStable()
{
    using SpinBox = KisAngleSelectorSpinBox;
    using FlatGetter = bool (SpinBox::*)() const;
    using FlatSetter = void (SpinBox::*)(bool);
    using RangeSetter = void (SpinBox::*)(double, double);
    using SizeGetter = QSize (SpinBox::*)() const;
    using TextParser = double (SpinBox::*)(const QString &) const;
    using Refresher = void (SpinBox::*)();

    static_assert(std::is_same_v<decltype(static_cast<FlatGetter>(&SpinBox::isFlat)), FlatGetter>);
    static_assert(std::is_same_v<decltype(static_cast<FlatSetter>(&SpinBox::setFlat)), FlatSetter>);
    static_assert(std::is_same_v<decltype(static_cast<RangeSetter>(&SpinBox::setRange)), RangeSetter>);
    static_assert(std::is_same_v<decltype(static_cast<SizeGetter>(&SpinBox::minimumSizeHint)), SizeGetter>);
    static_assert(std::is_same_v<decltype(static_cast<SizeGetter>(&SpinBox::sizeHint)), SizeGetter>);
    static_assert(std::is_same_v<decltype(static_cast<TextParser>(&SpinBox::valueFromText)), TextParser>);
    static_assert(std::is_same_v<decltype(static_cast<Refresher>(&SpinBox::refreshStyle)), Refresher>);
}

void KisAngleSelectorSchemaContractTest::aspectRatioLockerTypeLifetimeAndConnectionSchemaRemainStable()
{
    using Locker = KisAspectRatioLocker;
    using SpinConnection = void (Locker::*)(SpinBoxProbe *, SpinBoxProbe *, KoAspectButton *);
    using AngleConnection = void (Locker::*)(AngleBoxProbe *, AngleBoxProbe *, KoAspectButton *);

    static_assert(std::is_class_v<Locker>);
    static_assert(std::is_base_of_v<QObject, Locker>);
    static_assert(std::is_default_constructible_v<Locker>);
    static_assert(std::is_constructible_v<Locker, QObject *>);
    static_assert(std::has_virtual_destructor_v<Locker>);
    static_assert(
        std::is_same_v<decltype(static_cast<SpinConnection>(&Locker::connectSpinBoxes<SpinBoxProbe>)), SpinConnection>);
    static_assert(std::is_same_v<decltype(static_cast<AngleConnection>(&Locker::connectAngleBoxes<AngleBoxProbe>)),
                                 AngleConnection>);
}

void KisAngleSelectorSchemaContractTest::aspectRatioLockerUpdateAndNotificationSchemaRemainStable()
{
    using Locker = KisAspectRatioLocker;
    using BooleanSetter = void (Locker::*)(bool);
    using Notification = void (Locker::*)();

    static_assert(
        std::is_same_v<decltype(static_cast<BooleanSetter>(&Locker::setBlockUpdateSignalOnDrag)), BooleanSetter>);
    static_assert(std::is_same_v<decltype(static_cast<Notification>(&Locker::updateAspect)), Notification>);
    static_assert(std::is_same_v<decltype(static_cast<Notification>(&Locker::sliderValueChanged)), Notification>);
    static_assert(std::is_same_v<decltype(static_cast<Notification>(&Locker::aspectButtonChanged)), Notification>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanSetter>(&Locker::aspectButtonToggled)), BooleanSetter>);
}

QTEST_APPLESS_MAIN(KisAngleSelectorSchemaContractTest)

#include "KisAngleSelectorSchemaContractTest.moc"
