/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "AngleSelector.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_ANGLE_SELECTOR_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&AngleSelector::method)), signature>)
} // namespace

class AngleSelectorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void angleSelectorTypeAndViewSchemaRemainsStable();
    void angleSelectorPrimaryAngleSchemaRemainsStable();
    void angleSelectorRangePrecisionAndWrappingSchemaRemainsStable();
    void angleSelectorPresentationSchemaRemainsStable();
    void angleSelectorTransformSchemaRemainsStable();
};

void AngleSelectorSchemaContractTest::angleSelectorTypeAndViewSchemaRemainsStable()
{
    static_assert(std::is_class_v<AngleSelector>);
    static_assert(std::is_base_of_v<QObject, AngleSelector>);
    static_assert(std::is_default_constructible_v<AngleSelector>);
    static_assert(std::has_virtual_destructor_v<AngleSelector>);

    ASSERT_ANGLE_SELECTOR_SIGNATURE(widget, QWidget * (AngleSelector::*)() const);
    ASSERT_ANGLE_SELECTOR_SIGNATURE(angleChanged, void (AngleSelector::*)(qreal));

    QVERIFY(true);
}

void AngleSelectorSchemaContractTest::angleSelectorPrimaryAngleSchemaRemainsStable()
{
    ASSERT_ANGLE_SELECTOR_SIGNATURE(angle, qreal (AngleSelector::*)() const);
    ASSERT_ANGLE_SELECTOR_SIGNATURE(setAngle, void (AngleSelector::*)(qreal));
    ASSERT_ANGLE_SELECTOR_SIGNATURE(reset, void (AngleSelector::*)());
    ASSERT_ANGLE_SELECTOR_SIGNATURE(snapAngle, qreal (AngleSelector::*)() const);
    ASSERT_ANGLE_SELECTOR_SIGNATURE(setSnapAngle, void (AngleSelector::*)(qreal));
    ASSERT_ANGLE_SELECTOR_SIGNATURE(resetAngle, qreal (AngleSelector::*)() const);
    ASSERT_ANGLE_SELECTOR_SIGNATURE(setResetAngle, void (AngleSelector::*)(qreal));

    QVERIFY(true);
}

void AngleSelectorSchemaContractTest::angleSelectorRangePrecisionAndWrappingSchemaRemainsStable()
{
    ASSERT_ANGLE_SELECTOR_SIGNATURE(decimals, int (AngleSelector::*)() const);
    ASSERT_ANGLE_SELECTOR_SIGNATURE(setDecimals, void (AngleSelector::*)(int));
    ASSERT_ANGLE_SELECTOR_SIGNATURE(maximum, qreal (AngleSelector::*)() const);
    ASSERT_ANGLE_SELECTOR_SIGNATURE(setMaximum, void (AngleSelector::*)(qreal));
    ASSERT_ANGLE_SELECTOR_SIGNATURE(minimum, qreal (AngleSelector::*)() const);
    ASSERT_ANGLE_SELECTOR_SIGNATURE(setMinimum, void (AngleSelector::*)(qreal));
    ASSERT_ANGLE_SELECTOR_SIGNATURE(setRange, void (AngleSelector::*)(qreal, qreal));
    ASSERT_ANGLE_SELECTOR_SIGNATURE(wrapping, bool (AngleSelector::*)() const);
    ASSERT_ANGLE_SELECTOR_SIGNATURE(setWrapping, void (AngleSelector::*)(bool));

    QVERIFY(true);
}

void AngleSelectorSchemaContractTest::angleSelectorPresentationSchemaRemainsStable()
{
    ASSERT_ANGLE_SELECTOR_SIGNATURE(prefix, QString (AngleSelector::*)() const);
    ASSERT_ANGLE_SELECTOR_SIGNATURE(setPrefix, void (AngleSelector::*)(const QString &));
    ASSERT_ANGLE_SELECTOR_SIGNATURE(flipOptionsMode, QString (AngleSelector::*)() const);
    ASSERT_ANGLE_SELECTOR_SIGNATURE(setFlipOptionsMode, void (AngleSelector::*)(QString));
    ASSERT_ANGLE_SELECTOR_SIGNATURE(widgetsHeight, int (AngleSelector::*)() const);
    ASSERT_ANGLE_SELECTOR_SIGNATURE(setWidgetsHeight, void (AngleSelector::*)(int));
    ASSERT_ANGLE_SELECTOR_SIGNATURE(increasingDirection, QString (AngleSelector::*)() const);
    ASSERT_ANGLE_SELECTOR_SIGNATURE(setIncreasingDirection, void (AngleSelector::*)(QString));
    ASSERT_ANGLE_SELECTOR_SIGNATURE(isUsingFlatSpinBox, bool (AngleSelector::*)() const);
    ASSERT_ANGLE_SELECTOR_SIGNATURE(useFlatSpinBox, void (AngleSelector::*)(bool));

    QVERIFY(true);
}

void AngleSelectorSchemaContractTest::angleSelectorTransformSchemaRemainsStable()
{
    ASSERT_ANGLE_SELECTOR_SIGNATURE(closestCoterminalAngleInRange, qreal (AngleSelector::*)(qreal, bool *) const);
    ASSERT_ANGLE_SELECTOR_SIGNATURE(closestCoterminalAngleInRange, qreal (*)(qreal, qreal, qreal, bool *));
    ASSERT_ANGLE_SELECTOR_SIGNATURE(flipAngle, qreal (*)(qreal, Qt::Orientations));
    ASSERT_ANGLE_SELECTOR_SIGNATURE(flipAngle, qreal (*)(qreal, qreal, qreal, Qt::Orientations, bool *));
    ASSERT_ANGLE_SELECTOR_SIGNATURE(flip, void (AngleSelector::*)(Qt::Orientations));

    static_assert(
        std::is_same_v<decltype(std::declval<const AngleSelector &>().closestCoterminalAngleInRange(qreal{})), qreal>);
    static_assert(
        std::is_same_v<decltype(AngleSelector::closestCoterminalAngleInRange(qreal{}, qreal{}, qreal{})), qreal>);
    static_assert(std::is_same_v<decltype(AngleSelector::flipAngle(qreal{}, qreal{}, qreal{}, Qt::Horizontal)), qreal>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(AngleSelectorSchemaContractTest)

#include "AngleSelectorSchemaContractTest.moc"
