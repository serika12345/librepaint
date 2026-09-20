/* This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <array>
#include <atomic>
#include <limits>
#include <thread>

#include "KoFakeProgressProxy.h"

class KoFakeProgressProxyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void instanceIsStableAcrossThreads();
    void updatesLeaveInstancesStateless();
};

void KoFakeProgressProxyContractTest::instanceIsStableAcrossThreads()
{
    constexpr std::size_t threadCount = 16;
    std::array<KoProgressProxy *, threadCount> instances{};
    std::array<std::thread, threadCount> threads;
    std::atomic<bool> start{false};

    for (std::size_t i = 0; i < threadCount; ++i) {
        threads[i] = std::thread([&instances, &start, i] {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            instances[i] = KoFakeProgressProxy::instance();
        });
    }

    start.store(true, std::memory_order_release);
    for (std::thread &thread : threads) {
        thread.join();
    }

    QVERIFY(instances.front());
    for (KoProgressProxy *instance : instances) {
        QCOMPARE(instance, instances.front());
        QVERIFY(dynamic_cast<KoFakeProgressProxy *>(instance));
    }
}

void KoFakeProgressProxyContractTest::updatesLeaveInstancesStateless()
{
    KoFakeProgressProxy first;
    KoFakeProgressProxy second;

    QCOMPARE(first.maximum(), 100);
    QCOMPARE(second.maximum(), 100);

    first.setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    first.setValue(std::numeric_limits<int>::min());
    first.setValue(std::numeric_limits<int>::max());
    first.setFormat(QStringLiteral("render %p% 描画"));
    first.setAutoNestedName(QStringLiteral("レイヤー"));

    QCOMPARE(first.maximum(), 100);
    QCOMPARE(second.maximum(), 100);
    QCOMPARE(KoFakeProgressProxy::instance()->maximum(), 100);
}

QTEST_MAIN(KoFakeProgressProxyContractTest)

#include "KoFakeProgressProxyContractTest.moc"
