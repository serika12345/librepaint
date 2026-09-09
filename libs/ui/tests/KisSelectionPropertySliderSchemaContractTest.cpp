/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/KisSelectionPropertySlider.h"
#include "widgets/kis_color_space_selector.h"
#include "widgets/kis_floating_message.h"

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
#define ASSERT_COLOR_SPACE_SELECTOR_SIGNATURE(method, ...)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisColorSpaceSelector::method)), __VA_ARGS__>)
#define ASSERT_FLOATING_MESSAGE_SIGNATURE(method, ...)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisFloatingMessage::method)), __VA_ARGS__>)

} // namespace

class KisSelectionPropertySliderSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseTypeConstructionAndLifetimeSchemaRemainStable();
    void textTemplateAndDeletedAffixSchemaRemainStable();
    void genericSelectionAndValueGetterSchemaRemainStable();
    void shapeSliderTypeAndConstructionSchemaRemainStable();
    void colorSpaceSelectorTypeConstructionAndLifetimeSchemaRemainStable();
    void colorSpaceSelectorStateSignaturesRemainStable();
    void colorSpaceSelectorPresentationAndNotificationSignaturesRemainStable();
    void floatingMessageTypeAndPrioritySchemaRemainStable();
    void floatingMessageConstructionAndConfigurationSignaturesRemainStable();
    void floatingMessagePresentationSignaturesRemainStable();
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

void KisSelectionPropertySliderSchemaContractTest::colorSpaceSelectorTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisColorSpaceSelector>);
    static_assert(std::is_base_of_v<QWidget, KisColorSpaceSelector>);
    static_assert(std::is_constructible_v<KisColorSpaceSelector, QWidget *>);
    static_assert(std::has_virtual_destructor_v<KisColorSpaceSelector>);

    QVERIFY(true);
}

void KisSelectionPropertySliderSchemaContractTest::colorSpaceSelectorStateSignaturesRemainStable()
{
    ASSERT_COLOR_SPACE_SELECTOR_SIGNATURE(currentColorSpace, const KoColorSpace *(KisColorSpaceSelector::*)());
    ASSERT_COLOR_SPACE_SELECTOR_SIGNATURE(setCurrentColorModel, void (KisColorSpaceSelector::*)(const KoID &));
    ASSERT_COLOR_SPACE_SELECTOR_SIGNATURE(setCurrentColorDepth, void (KisColorSpaceSelector::*)(const KoID &));
    ASSERT_COLOR_SPACE_SELECTOR_SIGNATURE(setCurrentProfile, void (KisColorSpaceSelector::*)(const QString &));
    ASSERT_COLOR_SPACE_SELECTOR_SIGNATURE(setCurrentColorSpace, void (KisColorSpaceSelector::*)(const KoColorSpace *));
}

// clang-format off
void KisSelectionPropertySliderSchemaContractTest::colorSpaceSelectorPresentationAndNotificationSignaturesRemainStable()
// clang-format on
{
    ASSERT_COLOR_SPACE_SELECTOR_SIGNATURE(showColorBrowserButton, void (KisColorSpaceSelector::*)(bool));
    ASSERT_COLOR_SPACE_SELECTOR_SIGNATURE(showDepth, void (KisColorSpaceSelector::*)(bool));
    ASSERT_COLOR_SPACE_SELECTOR_SIGNATURE(selectionChanged, void (KisColorSpaceSelector::*)(bool));
    ASSERT_COLOR_SPACE_SELECTOR_SIGNATURE(colorSpaceChanged, void (KisColorSpaceSelector::*)(const KoColorSpace *));
}

void KisSelectionPropertySliderSchemaContractTest::floatingMessageTypeAndPrioritySchemaRemainStable()
{
    static_assert(std::is_class_v<KisFloatingMessage>);
    static_assert(std::is_base_of_v<QWidget, KisFloatingMessage>);
    static_assert(std::is_enum_v<KisFloatingMessage::Priority>);

    QCOMPARE(int(KisFloatingMessage::High), 0);
    QCOMPARE(int(KisFloatingMessage::Medium), 1);
    QCOMPARE(int(KisFloatingMessage::Low), 2);
}

// clang-format off
void KisSelectionPropertySliderSchemaContractTest::floatingMessageConstructionAndConfigurationSignaturesRemainStable()
// clang-format on
{
    static_assert(std::is_constructible_v<KisFloatingMessage,
                                          const QString &,
                                          QWidget *,
                                          bool,
                                          int,
                                          KisFloatingMessage::Priority>);
    ASSERT_FLOATING_MESSAGE_SIGNATURE(setShowOverParent, void (KisFloatingMessage::*)(bool));
    ASSERT_FLOATING_MESSAGE_SIGNATURE(setIcon, void (KisFloatingMessage::*)(const QIcon &));
    ASSERT_FLOATING_MESSAGE_SIGNATURE(
        tryOverrideMessage,
        void (KisFloatingMessage::*)(QString, const QIcon &, int, KisFloatingMessage::Priority, int));
}

void KisSelectionPropertySliderSchemaContractTest::floatingMessagePresentationSignaturesRemainStable()
{
    ASSERT_FLOATING_MESSAGE_SIGNATURE(showMessage, void (KisFloatingMessage::*)());
    ASSERT_FLOATING_MESSAGE_SIGNATURE(removeMessage, void (KisFloatingMessage::*)());
}

#undef ASSERT_FLOATING_MESSAGE_SIGNATURE
#undef ASSERT_COLOR_SPACE_SELECTOR_SIGNATURE
#undef ASSERT_SELECTION_SLIDER_BASE_SIGNATURE

QTEST_APPLESS_MAIN(KisSelectionPropertySliderSchemaContractTest)

#include "KisSelectionPropertySliderSchemaContractTest.moc"
