/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KisQpaPlatformChoice.h"

#include <QTest>

// LibrePaint draws natively on Wayland sessions and through xcb elsewhere
// when the user has not chosen a Qt platform plugin (KisQpaPlatformChoice.h).
class KisQpaPlatformChoiceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aWaylandSessionDrawsNativelyWithXcbAsFallback()
    {
        QCOMPARE(kisDefaultQpaPlatform("wayland", "wayland-0"), QByteArray("wayland;xcb"));
        QCOMPARE(kisDefaultQpaPlatform("Wayland", ""), QByteArray("wayland;xcb"));
    }

    void anXorgSessionKeepsXcb()
    {
        QCOMPARE(kisDefaultQpaPlatform("x11", ""), QByteArray("xcb"));
        QCOMPARE(kisDefaultQpaPlatform("x11", "wayland-0"), QByteArray("xcb"));
    }

    void aSessionWithoutTypeFollowsTheWaylandSocket()
    {
        QCOMPARE(kisDefaultQpaPlatform("", "wayland-0"), QByteArray("wayland;xcb"));
        QCOMPARE(kisDefaultQpaPlatform("tty", "wayland-1"), QByteArray("wayland;xcb"));
        QCOMPARE(kisDefaultQpaPlatform("", ""), QByteArray("xcb"));
        QCOMPARE(kisDefaultQpaPlatform("tty", ""), QByteArray("xcb"));
    }
};

QTEST_GUILESS_MAIN(KisQpaPlatformChoiceContractTest)

#include "KisQpaPlatformChoiceContractTest.moc"
