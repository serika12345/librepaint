/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/KisSelectionPropertySlider.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

class SelectionPropertySliderBaseProbe : public KisSelectionPropertySliderBase
{
public:
    explicit SelectionPropertySliderBaseProbe(QWidget *parent);

protected:
    bool hasSelection() const override;
    qreal getCommonValue() const override;
};

template<typename T, typename = void>
struct CanSetPrefix : std::false_type {
};

template<typename T>
struct CanSetPrefix<T, std::void_t<decltype(std::declval<T &>().setPrefix(std::declval<const QString &>()))>>
    : std::true_type {
};

template<typename T, typename = void>
struct CanSetSuffix : std::false_type {
};

template<typename T>
struct CanSetSuffix<T, std::void_t<decltype(std::declval<T &>().setSuffix(std::declval<const QString &>()))>>
    : std::true_type {
};

#define ASSERT_SELECTION_SLIDER_BASE_SIGNATURE(method, ...)                                                            \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisSelectionPropertySliderBase::method)), __VA_ARGS__>)

} // namespace

class KisSelectionPropertySliderSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseTypeConstructionAndLifetimeSchemaRemainStable();
    void textTemplateAndDeletedAffixSchemaRemainStable();
    void genericSelectionAndValueGetterSchemaRemainStable();
    void shapeSliderTypeAndConstructionSchemaRemainStable();
};

void KisSelectionPropertySliderSchemaContractTest::baseTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisSelectionPropertySliderBase>);
    static_assert(std::is_base_of_v<KisDoubleSliderSpinBox, KisSelectionPropertySliderBase>);
    static_assert(std::is_abstract_v<KisSelectionPropertySliderBase>);
    static_assert(std::is_constructible_v<SelectionPropertySliderBaseProbe, QWidget *>);
    static_assert(std::has_virtual_destructor_v<KisSelectionPropertySliderBase>);

    QVERIFY(true);
}

void KisSelectionPropertySliderSchemaContractTest::textTemplateAndDeletedAffixSchemaRemainStable()
{
    ASSERT_SELECTION_SLIDER_BASE_SIGNATURE(setTextTemplates,
                                           void (KisSelectionPropertySliderBase::*)(const QString &, const QString &));
    static_assert(!CanSetPrefix<KisSelectionPropertySliderBase>::value);
    static_assert(!CanSetSuffix<KisSelectionPropertySliderBase>::value);

    QVERIFY(true);
}

void KisSelectionPropertySliderSchemaContractTest::genericSelectionAndValueGetterSchemaRemainStable()
{
    using Slider = KisSelectionPropertySlider<int>;

    static_assert(std::is_class_v<Slider>);
    static_assert(std::is_base_of_v<KisSelectionPropertySliderBase, Slider>);
    static_assert(std::is_constructible_v<Slider, QWidget *>);
    static_assert(std::is_same_v<decltype(static_cast<void (Slider::*)(qreal (*)(int))>(&Slider::setValueGetter)),
                                 void (Slider::*)(qreal (*)(int))>);
    static_assert(std::is_same_v<decltype(static_cast<void (Slider::*)(QList<int>)>(&Slider::setSelection)),
                                 void (Slider::*)(QList<int>)>);
    static_assert(std::is_same_v<decltype(static_cast<QList<int> (Slider::*)() const>(&Slider::selection)),
                                 QList<int> (Slider::*)() const>);

    QVERIFY(true);
}

void KisSelectionPropertySliderSchemaContractTest::shapeSliderTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisShapePropertySlider>);
    static_assert(std::is_base_of_v<KisSelectionPropertySlider<KoShape *>, KisShapePropertySlider>);
    static_assert(std::is_constructible_v<KisShapePropertySlider, QWidget *>);

    QVERIFY(true);
}

#undef ASSERT_SELECTION_SLIDER_BASE_SIGNATURE

QTEST_APPLESS_MAIN(KisSelectionPropertySliderSchemaContractTest)

#include "KisSelectionPropertySliderSchemaContractTest.moc"
