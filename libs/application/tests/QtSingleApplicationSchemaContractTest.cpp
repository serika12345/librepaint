/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qtsingleapplication/qtsingleapplication.h"

#include <QTest>

#include <type_traits>
#include <utility>

class QtSingleApplicationSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeAndConstructionSchemaRemainStable();
    void processStateSignaturesRemainStable();
    void activationWindowSignaturesRemainStable();
    void messagingAndCompatibilitySignaturesRemainStable();
};

void QtSingleApplicationSchemaContractTest::typeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<QtSingleApplication>);
    static_assert(std::is_base_of_v<QApplication, QtSingleApplication>);
    static_assert(std::is_constructible_v<QtSingleApplication, int &, char **, bool>);
    static_assert(std::is_same_v<decltype(QtSingleApplication(std::declval<int &>(), static_cast<char **>(nullptr))),
                                 QtSingleApplication>);
    static_assert(std::is_constructible_v<QtSingleApplication, const QString &, int &, char **>);
}

void QtSingleApplicationSchemaContractTest::processStateSignaturesRemainStable()
{
    using Application = QtSingleApplication;

    static_assert(std::is_same_v<decltype(&Application::isRunning), bool (Application::*)()>);
    static_assert(std::is_same_v<decltype(&Application::id), QString (Application::*)() const>);
}

void QtSingleApplicationSchemaContractTest::activationWindowSignaturesRemainStable()
{
    using Application = QtSingleApplication;

    static_assert(std::is_same_v<decltype(&Application::setActivationWindow), void (Application::*)(QWidget *, bool)>);
    static_assert(
        std::is_same_v<decltype(std::declval<Application &>().setActivationWindow(static_cast<QWidget *>(nullptr))),
                       void>);
    static_assert(std::is_same_v<decltype(&Application::activationWindow), QWidget *(Application::*)() const>);
    static_assert(std::is_same_v<decltype(&Application::activateWindow), void (Application::*)()>);
}

void QtSingleApplicationSchemaContractTest::messagingAndCompatibilitySignaturesRemainStable()
{
    using Application = QtSingleApplication;

    static_assert(std::is_same_v<decltype(&Application::sendMessage), bool (Application::*)(const QString &, int)>);
    static_assert(
        std::is_same_v<decltype(std::declval<Application &>().sendMessage(std::declval<const QString &>())), bool>);
    static_assert(std::is_same_v<decltype(&Application::messageReceived), void (Application::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Application::initialize), void (Application::*)(bool)>);
    static_assert(std::is_same_v<decltype(std::declval<Application &>().initialize()), void>);
}

QTEST_GUILESS_MAIN(QtSingleApplicationSchemaContractTest)

#include "QtSingleApplicationSchemaContractTest.moc"
