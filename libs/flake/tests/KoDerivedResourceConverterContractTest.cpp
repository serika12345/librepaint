/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoDerivedResourceConverter.h"

#include <QList>
#include <QPair>
#include <QTest>
#include <QVariant>

namespace
{
class OffsetConverter : public KoDerivedResourceConverter
{
public:
    OffsetConverter(int key, int sourceKey, int *destructionCount = nullptr)
        : KoDerivedResourceConverter(key, sourceKey)
        , m_destructionCount(destructionCount)
    {
    }

    ~OffsetConverter() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    QList<QVariant> fromInputs;
    QList<QPair<QVariant, QVariant>> toInputs;

protected:
    QVariant fromSource(const QVariant &value) override
    {
        fromInputs.append(value);
        return value.toInt() + 10;
    }

    QVariant toSource(const QVariant &value, const QVariant &sourceValue) override
    {
        toInputs.append(qMakePair(value, sourceValue));
        return value.toInt() - 10;
    }

private:
    int *m_destructionCount;
};

class NotifyOverrideConverter : public OffsetConverter
{
public:
    using OffsetConverter::OffsetConverter;

    bool notifySourceChanged(const QVariant &sourceValue) override
    {
        ++notifyCallCount;
        notifiedSource = sourceValue;
        return notifyResult;
    }

    int notifyCallCount = 0;
    QVariant notifiedSource;
    bool notifyResult = false;
};

class ImmutableConverter : public KoDerivedResourceConverter
{
public:
    ImmutableConverter()
        : KoDerivedResourceConverter(7, 11)
    {
    }

    int writeCallCount = 0;

protected:
    QVariant fromSource(const QVariant &) override
    {
        return 10;
    }

    QVariant toSource(const QVariant &, const QVariant &sourceValue) override
    {
        ++writeCallCount;
        return sourceValue;
    }
};
} // namespace

class KoDerivedResourceConverterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sharedAliasPreservesKeysAndVirtualLifetime();
    void tracksReadAndNotifyCacheTransitions();
    void writesSourceAndReportsCachedValueChanges();
};

void KoDerivedResourceConverterContractTest::sharedAliasPreservesKeysAndVirtualLifetime()
{
    int destructionCount = 0;
    KoDerivedResourceConverterSP owner(new OffsetConverter(-13, 4096, &destructionCount));
    KoDerivedResourceConverterSP copy = owner;

    QCOMPARE(copy->key(), -13);
    QCOMPARE(copy->sourceKey(), 4096);
    owner.clear();
    QCOMPARE(destructionCount, 0);

    copy.clear();
    QCOMPARE(destructionCount, 1);
}

void KoDerivedResourceConverterContractTest::tracksReadAndNotifyCacheTransitions()
{
    OffsetConverter converter(1, 2);

    QVERIFY(converter.notifySourceChanged(2));
    QVERIFY(!converter.notifySourceChanged(2));

    QCOMPARE(converter.readFromSource(2), QVariant(12));
    QVERIFY(!converter.notifySourceChanged(2));

    QCOMPARE(converter.readFromSource(3), QVariant(13));
    QVERIFY(converter.notifySourceChanged(3));
    QVERIFY(!converter.notifySourceChanged(3));

    NotifyOverrideConverter overrideConverter(3, 5);
    KoDerivedResourceConverter *base = &overrideConverter;
    overrideConverter.notifyResult = true;
    QVERIFY(base->notifySourceChanged(QStringLiteral("override")));
    QCOMPARE(overrideConverter.notifyCallCount, 1);
    QCOMPARE(overrideConverter.notifiedSource, QVariant(QStringLiteral("override")));
}

void KoDerivedResourceConverterContractTest::writesSourceAndReportsCachedValueChanges()
{
    OffsetConverter converter(1, 2);
    QVERIFY(converter.notifySourceChanged(2));

    bool changed = true;
    QCOMPARE(converter.writeToSource(12, 2, &changed), QVariant(2));
    QVERIFY(!changed);
    QVERIFY(converter.toInputs.isEmpty());

    QCOMPARE(converter.writeToSource(15, 2, &changed), QVariant(5));
    QVERIFY(changed);
    QCOMPARE(converter.toInputs.size(), 1);
    QCOMPARE(converter.toInputs.last().first, QVariant(15));
    QCOMPARE(converter.toInputs.last().second, QVariant(2));

    QCOMPARE(converter.writeToSource(15, 2, &changed), QVariant(5));
    QVERIFY(!changed);
    QCOMPARE(converter.toInputs.size(), 2);

    QCOMPARE(converter.writeToSource(16, 5, nullptr), QVariant(6));
    QCOMPARE(converter.toInputs.size(), 3);

    ImmutableConverter immutableConverter;
    changed = false;
    QCOMPARE(immutableConverter.writeToSource(150, 2, &changed), QVariant(2));
    QVERIFY(changed);
    QCOMPARE(immutableConverter.writeCallCount, 1);
    QCOMPARE(immutableConverter.readFromSource(2), QVariant(10));
}

QTEST_GUILESS_MAIN(KoDerivedResourceConverterContractTest)

#include "KoDerivedResourceConverterContractTest.moc"
