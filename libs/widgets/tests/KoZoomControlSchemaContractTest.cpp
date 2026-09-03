/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoZoomAction.h"
#include "KoZoomWidget.h"

#include <QTest>

#include <type_traits>

class KoZoomControlSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void zoomActionTypeLifetimeAndWidgetSchemaRemainStable();
    void zoomActionStateControlSignaturesRemainStable();
    void zoomActionNotificationSignaturesRemainStable();
    void zoomWidgetTypeLifetimeAndPresentationSchemaRemainStable();
    void zoomWidgetStateAndNotificationSignaturesRemainStable();
};

void KoZoomControlSchemaContractTest::zoomActionTypeLifetimeAndWidgetSchemaRemainStable()
{
    using Action = KoZoomAction;

    static_assert(std::is_class_v<Action>);
    static_assert(std::is_base_of_v<KSelectAction, Action>);
    static_assert(std::is_constructible_v<Action, const QString &, QObject *>);
    static_assert(std::has_virtual_destructor_v<Action>);
    static_assert(std::is_same_v<decltype(&Action::createWidget), QWidget *(Action::*)(QWidget *)>);
}

void KoZoomControlSchemaContractTest::zoomActionStateControlSignaturesRemainStable()
{
    using Action = KoZoomAction;

    static_assert(std::is_same_v<decltype(&Action::slotZoomStateChanged), void (Action::*)(const KoZoomState &)>);
    static_assert(std::is_same_v<decltype(&Action::setUsePrintResolutionMode), void (Action::*)(bool)>);
}

void KoZoomControlSchemaContractTest::zoomActionNotificationSignaturesRemainStable()
{
    using Action = KoZoomAction;

    static_assert(std::is_same_v<decltype(&Action::zoomChanged), void (Action::*)(KoZoomMode::Mode, qreal)>);
    static_assert(std::is_same_v<decltype(&Action::sigUsePrintResolutionModeChanged), void (Action::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Action::sigInternalUpdateZoomLevelsComboState),
                                 void (Action::*)(const QStringList &, int, const QString &)>);
    static_assert(
        std::is_same_v<decltype(&Action::sigInternalUpdateZoomLevelsSliderState), void (Action::*)(int, int)>);
    static_assert(std::is_same_v<decltype(&Action::sigInternalUpdateUsePrintResolutionMode), void (Action::*)(bool)>);
}

void KoZoomControlSchemaContractTest::zoomWidgetTypeLifetimeAndPresentationSchemaRemainStable()
{
    using Widget = KoZoomWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<QWidget, Widget>);
    static_assert(std::is_constructible_v<Widget, QWidget *, int>);
    static_assert(std::has_virtual_destructor_v<Widget>);
    static_assert(std::is_same_v<decltype(&Widget::isZoomInputFlat), bool (Widget::*)() const>);
    static_assert(std::is_same_v<decltype(&Widget::setZoomInputFlat), void (Widget::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Widget::setUsePrintResolutionMode), void (Widget::*)(bool)>);
}

void KoZoomControlSchemaContractTest::zoomWidgetStateAndNotificationSignaturesRemainStable()
{
    using Widget = KoZoomWidget;
    using StringZoomLevelSetter = void (Widget::*)(const QString &);
    using IndexZoomLevelSetter = void (Widget::*)(int);

    static_assert(std::is_same_v<decltype(static_cast<StringZoomLevelSetter>(&Widget::setCurrentZoomLevel)),
                                 StringZoomLevelSetter>);
    static_assert(std::is_same_v<decltype(static_cast<IndexZoomLevelSetter>(&Widget::setCurrentZoomLevel)),
                                 IndexZoomLevelSetter>);
    static_assert(std::is_same_v<decltype(&Widget::setSliderState), void (Widget::*)(int, int)>);
    static_assert(std::is_same_v<decltype(&Widget::setSliderValue), void (Widget::*)(int)>);
    static_assert(std::is_same_v<decltype(&Widget::setZoomLevelsState),
                                 void (Widget::*)(const QStringList &, int, const QString &)>);
    static_assert(std::is_same_v<decltype(&Widget::sigUsePrintResolutionModeChanged), void (Widget::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Widget::sliderValueChanged), void (Widget::*)(int)>);
    static_assert(std::is_same_v<decltype(&Widget::zoomLevelChanged), void (Widget::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Widget::zoomLevelChangedIndex), void (Widget::*)(int)>);
}

QTEST_APPLESS_MAIN(KoZoomControlSchemaContractTest)

#include "KoZoomControlSchemaContractTest.moc"
