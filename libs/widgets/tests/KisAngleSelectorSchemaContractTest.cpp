/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisAngleSelector.h"

#include <QTest>

#include <type_traits>

class KisAngleSelectorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void angleSelectorFlipPresentationSchemaRemainsStable();
    void angleSelectorPrimaryAngleSchemaRemainsStable();
    void angleSelectorRangePrecisionAndWrappingSchemaRemainsStable();
    void angleSelectorLocalPresentationSchemaRemainsStable();
    void angleSelectorStaticTransformSignaturesRemainStable();
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

QTEST_APPLESS_MAIN(KisAngleSelectorSchemaContractTest)

#include "KisAngleSelectorSchemaContractTest.moc"
