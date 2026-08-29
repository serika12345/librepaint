/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisSequentialIteratorProgress.h"

#include <KoFakeProgressProxy.h>

#include <QTest>

#include <memory>
#include <type_traits>

namespace
{

class RecordingProgressProxy final : public KoProgressProxy
{
public:
    enum Call {
        SetRangeCall,
        SetValueCall,
        MaximumCall,
    };

    RecordingProgressProxy(int maximum, bool *destroyed)
        : m_maximum(maximum)
        , m_destroyed(destroyed)
    {
    }

    ~RecordingProgressProxy() override
    {
        *m_destroyed = true;
    }

    int maximum() const override
    {
        calls.append(MaximumCall);
        return m_maximum;
    }

    void setValue(int value) override
    {
        calls.append(SetValueCall);
        values.append(value);
    }

    void setRange(int minimum, int maximum) override
    {
        calls.append(SetRangeCall);
        ranges.append(qMakePair(minimum, maximum));
    }

    void setFormat(const QString &format) override
    {
        lastFormat = format;
    }

    mutable QList<Call> calls;
    QList<int> values;
    QList<QPair<int, int>> ranges;
    QString lastFormat;

private:
    int m_maximum;
    bool *m_destroyed;
};

} // namespace

class KisSequentialIteratorProgressContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void borrowedProxyReceivesProgressOperationsInOrder();
    void nullProxyUsesTheSafeFallback();
    void aliasesSelectTheExpectedPoliciesAndAcceptAProxy();
};

void KisSequentialIteratorProgressContractTest::borrowedProxyReceivesProgressOperationsInOrder()
{
    bool destroyed = false;
    auto proxy = std::make_unique<RecordingProgressProxy>(73, &destroyed);

    {
        ProxyBasedProgressPolicy policy(proxy.get());
        policy.setRange(-9, 41);
        policy.setValue(17);
        policy.setFinished();
    }

    const QList<QPair<int, int>> expectedRanges({qMakePair(-9, 41)});
    QCOMPARE(proxy->ranges, expectedRanges);
    QCOMPARE(proxy->values, QList<int>({17, 73}));
    QCOMPARE(proxy->calls,
             QList<RecordingProgressProxy::Call>({RecordingProgressProxy::SetRangeCall,
                                                  RecordingProgressProxy::SetValueCall,
                                                  RecordingProgressProxy::MaximumCall,
                                                  RecordingProgressProxy::SetValueCall}));
    QVERIFY(!destroyed);

    proxy.reset();
    QVERIFY(destroyed);
}

void KisSequentialIteratorProgressContractTest::nullProxyUsesTheSafeFallback()
{
    KoProgressProxy *fallback = KoFakeProgressProxy::instance();
    QVERIFY(fallback);

    ProxyBasedProgressPolicy policy(nullptr);

    policy.setRange(-100, 250);
    policy.setValue(42);
    policy.setFinished();

    QCOMPARE(KoFakeProgressProxy::instance(), fallback);
    QCOMPARE(fallback->maximum(), 100);
}

void KisSequentialIteratorProgressContractTest::aliasesSelectTheExpectedPoliciesAndAcceptAProxy()
{
    using ExpectedReadOnly =
        KisSequentialIteratorBase<ReadOnlyIteratorPolicy<>, DevicePolicy, ProxyBasedProgressPolicy>;
    using ExpectedWritable =
        KisSequentialIteratorBase<WritableIteratorPolicy<>, DevicePolicy, ProxyBasedProgressPolicy>;

    QVERIFY((std::is_same_v<KisSequentialConstIteratorProgress, ExpectedReadOnly>));
    QVERIFY((std::is_same_v<KisSequentialIteratorProgress, ExpectedWritable>));
    QVERIFY(
        (std::is_constructible_v<KisSequentialConstIteratorProgress, DevicePolicy, const QRect &, KoProgressProxy *>));
    QVERIFY((std::is_constructible_v<KisSequentialIteratorProgress, DevicePolicy, const QRect &, KoProgressProxy *>));
}

QTEST_GUILESS_MAIN(KisSequentialIteratorProgressContractTest)

#include "KisSequentialIteratorProgressContractTest.moc"
