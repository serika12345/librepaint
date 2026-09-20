/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kundo2commandextradata.h"

#include <QTest>

#include <memory>

namespace
{

class PayloadExtraData : public KUndo2CommandExtraData
{
public:
    explicit PayloadExtraData(int value, int *destructionCount = nullptr)
        : m_value(value)
        , m_destructionCount(destructionCount)
    {
    }

    ~PayloadExtraData() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    KUndo2CommandExtraData *clone() const override
    {
        return new PayloadExtraData(m_value);
    }

    int value() const
    {
        return m_value;
    }

    void setValue(int value)
    {
        m_value = value;
    }

private:
    int m_value;
    int *m_destructionCount;
};

} // namespace

class KUndo2CommandExtraDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void clonePreservesDynamicValueWithoutAliasing();
    void baseDestructionDispatchesExactlyOnce();
};

void KUndo2CommandExtraDataContractTest::clonePreservesDynamicValueWithoutAliasing()
{
    PayloadExtraData source(42);
    const KUndo2CommandExtraData &base = source;

    std::unique_ptr<KUndo2CommandExtraData> clone(base.clone());
    QVERIFY(clone.get() != &source);

    auto *clonedPayload = dynamic_cast<PayloadExtraData *>(clone.get());
    QVERIFY(clonedPayload);
    QCOMPARE(clonedPayload->value(), 42);

    clonedPayload->setValue(7);
    QCOMPARE(source.value(), 42);
    QCOMPARE(clonedPayload->value(), 7);
}

void KUndo2CommandExtraDataContractTest::baseDestructionDispatchesExactlyOnce()
{
    int destructionCount = 0;
    KUndo2CommandExtraData *data = new PayloadExtraData(17, &destructionCount);

    delete data;

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KUndo2CommandExtraDataContractTest)

#include "KUndo2CommandExtraDataContractTest.moc"
