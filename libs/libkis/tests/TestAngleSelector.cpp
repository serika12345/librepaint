/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <AngleSelector.h>

#include <QTest>

class TestAngleSelector : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void flipOptionsModeRoundTripsScriptNames();
    void increasingDirectionRoundTripsScriptNames();
    void invalidNamesPreserveCurrentModes();
};

void TestAngleSelector::flipOptionsModeRoundTripsScriptNames()
{
    AngleSelector selector;

    const QStringList modes {
        QStringLiteral("NoFlipOptions"),
        QStringLiteral("MenuButton"),
        QStringLiteral("Buttons"),
        QStringLiteral("ContextMenu"),
    };
    for (const QString &mode : modes) {
        selector.setFlipOptionsMode(mode);
        QCOMPARE(selector.flipOptionsMode(), mode);
    }
}

void TestAngleSelector::increasingDirectionRoundTripsScriptNames()
{
    AngleSelector selector;

    const QStringList directions {
        QStringLiteral("CounterClockwise"),
        QStringLiteral("Clockwise"),
    };
    for (const QString &direction : directions) {
        selector.setIncreasingDirection(direction);
        QCOMPARE(selector.increasingDirection(), direction);
    }
}

void TestAngleSelector::invalidNamesPreserveCurrentModes()
{
    AngleSelector selector;
    selector.setFlipOptionsMode(QStringLiteral("MenuButton"));
    selector.setIncreasingDirection(QStringLiteral("Clockwise"));

    selector.setFlipOptionsMode(QStringLiteral("invalid"));
    selector.setIncreasingDirection(QStringLiteral("invalid"));

    QCOMPARE(selector.flipOptionsMode(), QStringLiteral("MenuButton"));
    QCOMPARE(selector.increasingDirection(), QStringLiteral("Clockwise"));
}

QTEST_MAIN(TestAngleSelector)

#include "TestAngleSelector.moc"
