/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_lockless_stack.h"

#include <QTest>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

class KisLocklessStackContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsEmptyAndReleasesStoredValues();
    void pushAndPopUseLifoOrder();
    void clearRemovesAllValues();
    void mergeMovesTheSourceChainToTheTop();
    void concurrentProducersAndConsumersPreserveValues();
};

void KisLocklessStackContractTest::startsEmptyAndReleasesStoredValues()
{
    std::weak_ptr<int> storedValue;
    {
        KisLocklessStack<std::shared_ptr<int>> stack;

        QCOMPARE(stack.size(), 0);
        QVERIFY(stack.isEmpty());

        auto value = std::make_shared<int>(42);
        storedValue = value;
        stack.push(value);
        value.reset();

        QVERIFY(!storedValue.expired());
    }

    QVERIFY(storedValue.expired());
}

void KisLocklessStackContractTest::pushAndPopUseLifoOrder()
{
    KisLocklessStack<int> stack;
    int value = -1;

    QVERIFY(!stack.pop(value));
    QCOMPARE(value, -1);

    stack.push(1);
    stack.push(2);
    QCOMPARE(stack.size(), 2);

    QVERIFY(stack.pop(value));
    QCOMPARE(value, 2);
    QVERIFY(stack.pop(value));
    QCOMPARE(value, 1);
    QVERIFY(stack.isEmpty());
}

void KisLocklessStackContractTest::clearRemovesAllValues()
{
    KisLocklessStack<int> stack;
    stack.push(1);
    stack.push(2);

    stack.clear();

    QCOMPARE(stack.size(), 0);
    QVERIFY(stack.isEmpty());

    stack.clear();
    QVERIFY(stack.isEmpty());
}

void KisLocklessStackContractTest::mergeMovesTheSourceChainToTheTop()
{
    KisLocklessStack<int> destination;
    destination.push(10);

    KisLocklessStack<int> source;
    source.push(1);
    source.push(2);

    destination.mergeFrom(source);

    QCOMPARE(source.size(), 0);
    QVERIFY(source.isEmpty());
    QCOMPARE(destination.size(), 3);

    int value = 0;
    QVERIFY(destination.pop(value));
    QCOMPARE(value, 2);
    QVERIFY(destination.pop(value));
    QCOMPARE(value, 1);
    QVERIFY(destination.pop(value));
    QCOMPARE(value, 10);
}

void KisLocklessStackContractTest::concurrentProducersAndConsumersPreserveValues()
{
    constexpr int producerCount = 4;
    constexpr int consumerCount = 4;
    constexpr int valuesPerProducer = 500;
    constexpr int totalValues = producerCount * valuesPerProducer;

    KisLocklessStack<int> stack;
    std::vector<std::thread> threads;

    for (int producer = 0; producer < producerCount; ++producer) {
        threads.emplace_back([&stack, producer] {
            const int firstValue = producer * valuesPerProducer;
            for (int offset = 0; offset < valuesPerProducer; ++offset) {
                stack.push(firstValue + offset);
            }
        });
    }
    for (std::thread &thread : threads) {
        thread.join();
    }

    QCOMPARE(stack.size(), totalValues);

    threads.clear();
    std::vector<std::atomic<int>> occurrences(totalValues);
    for (std::atomic<int> &occurrence : occurrences) {
        occurrence.store(0);
    }
    std::atomic<int> remainingValues(totalValues);
    std::atomic<bool> invalidValue(false);

    for (int consumer = 0; consumer < consumerCount; ++consumer) {
        threads.emplace_back([&] {
            while (remainingValues.load() > 0) {
                int value = -1;
                if (stack.pop(value)) {
                    if (value < 0 || value >= totalValues) {
                        invalidValue.store(true);
                    } else {
                        occurrences[value].fetch_add(1);
                    }
                    remainingValues.fetch_sub(1);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }
    for (std::thread &thread : threads) {
        thread.join();
    }

    QVERIFY(!invalidValue.load());
    QCOMPARE(remainingValues.load(), 0);
    QVERIFY(stack.isEmpty());
    for (const std::atomic<int> &occurrence : occurrences) {
        QCOMPARE(occurrence.load(), 1);
    }
}

QTEST_GUILESS_MAIN(KisLocklessStackContractTest)

#include "KisLocklessStackContractTest.moc"
