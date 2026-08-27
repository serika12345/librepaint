/*
 * SPDX-FileCopyrightText: 2026 LibrePaint Contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisFutureUtilsTest.h"

#include "KisFutureUtils.h"

#include <QTest>

#include <chrono>

void KisFutureUtilsTest::readyValueFuture()
{
    int sourceValue = 42;
    std::future<int> future = kismpl::make_ready_future(sourceValue);

    QVERIFY(future.valid());
    QVERIFY(future.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
    QCOMPARE(future.get(), sourceValue);
}

void KisFutureUtilsTest::readyVoidFuture()
{
    std::future<void> future = kismpl::make_ready_future();

    QVERIFY(future.valid());
    QVERIFY(future.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
    future.get();
}

void KisFutureUtilsTest::deferredContinuation()
{
    std::future<int> continuation = kismpl::then(kismpl::make_ready_future(21), [](std::future<int> &&input) {
        return input.get() * 2;
    });

    QVERIFY(continuation.valid());
    QVERIFY(continuation.wait_for(std::chrono::seconds(0)) == std::future_status::deferred);
    QCOMPARE(continuation.get(), 42);
}

QTEST_GUILESS_MAIN(KisFutureUtilsTest)
