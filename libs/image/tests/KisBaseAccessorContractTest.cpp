/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_base_accessor.h"
#include "kis_shared_ptr.h"

#include <QTest>

#include <array>
#include <memory>

namespace
{

class ConstAccessorSentinel final : public KisBaseConstAccessor
{
public:
    ConstAccessorSentinel(const quint8 *oldData, const quint8 *currentData, qint32 x, qint32 y, int *destructionCount)
        : m_oldData(oldData)
        , m_currentData(currentData)
        , m_x(x)
        , m_y(y)
        , m_destructionCount(destructionCount)
    {
    }

    ~ConstAccessorSentinel() override
    {
        ++*m_destructionCount;
    }

    const quint8 *oldRawData() const override
    {
        return m_oldData;
    }

    const quint8 *rawDataConst() const override
    {
        return m_currentData;
    }

    qint32 x() const override
    {
        return m_x;
    }

    qint32 y() const override
    {
        return m_y;
    }

private:
    const quint8 *m_oldData;
    const quint8 *m_currentData;
    qint32 m_x;
    qint32 m_y;
    int *m_destructionCount;
};

class WritableAccessorSentinel final : public KisBaseAccessor
{
public:
    WritableAccessorSentinel(quint8 *data, int *destructionCount)
        : m_data(data)
        , m_destructionCount(destructionCount)
    {
    }

    ~WritableAccessorSentinel() override
    {
        ++*m_destructionCount;
    }

    quint8 *rawData() override
    {
        return m_data;
    }

private:
    quint8 *m_data;
    int *m_destructionCount;
};

} // namespace

class KisBaseAccessorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constAccessorReportsSnapshotPointersAndCoordinates();
    void writableAccessorExposesLiveStorage();
    void baseOwnershipPreservesVirtualLifetime();
};

void KisBaseAccessorContractTest::constAccessorReportsSnapshotPointersAndCoordinates()
{
    const std::array<quint8, 4> oldData{{0x11, 0x22, 0x33, 0x44}};
    const std::array<quint8, 4> currentData{{0xa1, 0xb2, 0xc3, 0xd4}};
    int destructionCount = 0;
    ConstAccessorSentinel sentinel(oldData.data(), currentData.data(), 13, -8, &destructionCount);
    const KisBaseConstAccessor &accessor = sentinel;

    QCOMPARE(accessor.oldRawData(), oldData.data());
    QCOMPARE(accessor.rawDataConst(), currentData.data());
    QCOMPARE(accessor.oldRawData()[2], quint8(0x33));
    QCOMPARE(accessor.rawDataConst()[2], quint8(0xc3));
    QCOMPARE(accessor.x(), 13);
    QCOMPARE(accessor.y(), -8);
}

void KisBaseAccessorContractTest::writableAccessorExposesLiveStorage()
{
    std::array<quint8, 4> data{{0x10, 0x20, 0x30, 0x40}};
    int destructionCount = 0;
    WritableAccessorSentinel sentinel(data.data(), &destructionCount);
    KisBaseAccessor &accessor = sentinel;

    quint8 *const returnedData = accessor.rawData();
    QCOMPARE(returnedData, data.data());

    returnedData[1] = 0x7f;
    QCOMPARE(data[1], quint8(0x7f));
}

void KisBaseAccessorContractTest::baseOwnershipPreservesVirtualLifetime()
{
    std::array<quint8, 1> data{{0x5a}};
    int writableDestructionCount = 0;
    {
        std::unique_ptr<KisBaseAccessor> accessor(new WritableAccessorSentinel(data.data(), &writableDestructionCount));
        QCOMPARE(writableDestructionCount, 0);
    }
    QCOMPARE(writableDestructionCount, 1);

    int constDestructionCount = 0;
    KisSharedPtr<KisBaseConstAccessor> firstOwner(
        new ConstAccessorSentinel(data.data(), data.data(), 13, -8, &constDestructionCount));
    KisSharedPtr<KisBaseConstAccessor> finalOwner(firstOwner);

    firstOwner.clear();
    QCOMPARE(constDestructionCount, 0);

    finalOwner.clear();
    QCOMPARE(constDestructionCount, 1);
}

QTEST_GUILESS_MAIN(KisBaseAccessorContractTest)

#include "KisBaseAccessorContractTest.moc"
