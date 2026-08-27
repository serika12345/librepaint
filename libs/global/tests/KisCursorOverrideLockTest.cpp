/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisCursorOverrideLock.h>

#include <QGuiApplication>
#include <QTest>

class KisCursorOverrideLockTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cleanup();
    void constructionHasNoCursorSideEffect();
    void lockAndUnlockRestorePreviousCursor();
};

void KisCursorOverrideLockTest::cleanup()
{
    while (QGuiApplication::overrideCursor()) {
        QGuiApplication::restoreOverrideCursor();
    }
}

void KisCursorOverrideLockTest::constructionHasNoCursorSideEffect()
{
    QVERIFY(!QGuiApplication::overrideCursor());

    {
        KisCursorOverrideLockAdapter adapter{QCursor(Qt::WaitCursor)};
        QVERIFY(!QGuiApplication::overrideCursor());
    }

    QVERIFY(!QGuiApplication::overrideCursor());
}

void KisCursorOverrideLockTest::lockAndUnlockRestorePreviousCursor()
{
    QGuiApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
    QVERIFY(QGuiApplication::overrideCursor());
    QCOMPARE(QGuiApplication::overrideCursor()->shape(), Qt::CrossCursor);

    KisCursorOverrideLockAdapter adapter{QCursor(Qt::WaitCursor)};
    adapter.lock();
    QVERIFY(QGuiApplication::overrideCursor());
    QCOMPARE(QGuiApplication::overrideCursor()->shape(), Qt::WaitCursor);

    adapter.unlock();
    QVERIFY(QGuiApplication::overrideCursor());
    QCOMPARE(QGuiApplication::overrideCursor()->shape(), Qt::CrossCursor);
}

QTEST_MAIN(KisCursorOverrideLockTest)

#include "KisCursorOverrideLockTest.moc"
