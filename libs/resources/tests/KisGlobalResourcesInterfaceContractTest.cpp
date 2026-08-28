/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisGlobalResourcesInterface.h>

#include <QTest>

#include <atomic>
#include <thread>
#include <vector>

namespace
{
std::atomic_int sourceCreateCount {0};

class EmptySource final : public KisResourcesInterface::ResourceSourceAdapter
{
public:
    explicit EmptySource(const QString &type)
        : ResourceSourceAdapter(type)
    {
    }

    QVector<KoResourceSP> resourcesForFilename(const QString &) const override
    {
        return {};
    }

    QVector<KoResourceSP> resourcesForName(const QString &) const override
    {
        return {};
    }

    QVector<KoResourceSP> resourcesForMD5(const QString &) const override
    {
        return {};
    }

    KoResourceSP fallbackResource() const override
    {
        return {};
    }
};
}

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("unexpected assertion: %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

namespace KisGlobalResourcesInterfaceDetail
{
KisResourcesInterface::ResourceSourceAdapter *createModelBackedSource(const QString &type)
{
    ++sourceCreateCount;
    return new EmptySource(type);
}
}

class KisGlobalResourcesInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void returnsOneTypedInstanceAcrossThreads();
};

void KisGlobalResourcesInterfaceContractTest::returnsOneTypedInstanceAcrossThreads()
{
    constexpr int threadCount = 8;
    std::atomic_bool start {false};
    std::vector<KisResourcesInterfaceSP> instances(threadCount);
    std::vector<std::thread> threads;
    threads.reserve(threadCount);
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&, i] {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            instances[i] = KisGlobalResourcesInterface::instance();
        });
    }

    start.store(true, std::memory_order_release);
    for (std::thread &thread : threads) {
        thread.join();
    }

    QVERIFY(instances.front());
    for (const KisResourcesInterfaceSP &instance : instances) {
        QCOMPARE(instance.data(), instances.front().data());
    }
    QCOMPARE(KisGlobalResourcesInterface::instance().data(), instances.front().data());
    QVERIFY(instances.front().dynamicCast<KisGlobalResourcesInterface>());

    auto *firstSource = &instances.front()->source(QStringLiteral("contract"));
    auto *secondSource = &instances.front()->source(QStringLiteral("contract"));
    QCOMPARE(firstSource, secondSource);
    QVERIFY(firstSource->fallbackResource().isNull());
    QCOMPARE(sourceCreateCount.load(), 1);
}

QTEST_GUILESS_MAIN(KisGlobalResourcesInterfaceContractTest)

#include "KisGlobalResourcesInterfaceContractTest.moc"
