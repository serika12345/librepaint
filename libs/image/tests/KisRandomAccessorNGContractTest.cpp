/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_random_accessor_ng.h"

#include <QTest>

#include <array>
#include <memory>

namespace
{

class RandomConstAccessorSentinel final : public KisRandomConstAccessorNG
{
public:
    RandomConstAccessorSentinel(const quint8 *oldData,
                                const quint8 *currentData,
                                qint32 contiguousColumns,
                                qint32 contiguousRows,
                                qint32 stride,
                                int *destructionCount)
        : m_oldData(oldData)
        , m_currentData(currentData)
        , m_contiguousColumns(contiguousColumns)
        , m_contiguousRows(contiguousRows)
        , m_stride(stride)
        , m_destructionCount(destructionCount)
    {
    }

    ~RandomConstAccessorSentinel() override
    {
        ++*m_destructionCount;
    }

    void moveTo(qint32 x, qint32 y) override
    {
        m_x = x;
        m_y = y;
        ++m_moveCalls;
    }

    qint32 numContiguousColumns(qint32 x) const override
    {
        m_lastColumnQuery = x;
        return m_contiguousColumns;
    }

    qint32 numContiguousRows(qint32 y) const override
    {
        m_lastRowQuery = y;
        return m_contiguousRows;
    }

    qint32 rowStride(qint32 x, qint32 y) const override
    {
        m_lastStrideX = x;
        m_lastStrideY = y;
        return m_stride;
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

    int moveCalls() const
    {
        return m_moveCalls;
    }

    qint32 lastColumnQuery() const
    {
        return m_lastColumnQuery;
    }

    qint32 lastRowQuery() const
    {
        return m_lastRowQuery;
    }

    qint32 lastStrideX() const
    {
        return m_lastStrideX;
    }

    qint32 lastStrideY() const
    {
        return m_lastStrideY;
    }

private:
    const quint8 *m_oldData;
    const quint8 *m_currentData;
    qint32 m_contiguousColumns;
    qint32 m_contiguousRows;
    qint32 m_stride;
    int *m_destructionCount;
    qint32 m_x = 0;
    qint32 m_y = 0;
    int m_moveCalls = 0;
    mutable qint32 m_lastColumnQuery = 0;
    mutable qint32 m_lastRowQuery = 0;
    mutable qint32 m_lastStrideX = 0;
    mutable qint32 m_lastStrideY = 0;
};

class RandomAccessorSentinel final : public KisRandomAccessorNG
{
public:
    RandomAccessorSentinel(const quint8 *oldData, quint8 *currentData, int *destructionCount)
        : m_oldData(oldData)
        , m_currentData(currentData)
        , m_destructionCount(destructionCount)
    {
    }

    ~RandomAccessorSentinel() override
    {
        ++*m_destructionCount;
    }

    void moveTo(qint32 x, qint32 y) override
    {
        m_x = x;
        m_y = y;
    }

    qint32 numContiguousColumns(qint32) const override
    {
        return 31;
    }

    qint32 numContiguousRows(qint32) const override
    {
        return 19;
    }

    qint32 rowStride(qint32, qint32) const override
    {
        return 512;
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

    quint8 *rawData() override
    {
        return m_currentData;
    }

private:
    const quint8 *m_oldData;
    quint8 *m_currentData;
    int *m_destructionCount;
    qint32 m_x = 0;
    qint32 m_y = 0;
};

} // namespace

class KisRandomAccessorNGContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constAccessorRoutesCoordinatesAndBaseViews();
    void constAccessorRoutesContiguousShapeAndStride();
    void writableAccessorExposesMutableAndConstViews();
    void randomAccessorBasesPreserveVirtualLifetime();
};

void KisRandomAccessorNGContractTest::constAccessorRoutesCoordinatesAndBaseViews()
{
    const std::array<quint8, 3> oldData{{0x11, 0x22, 0x33}};
    const std::array<quint8, 3> currentData{{0xa1, 0xb2, 0xc3}};
    int destructionCount = 0;
    RandomConstAccessorSentinel sentinel(oldData.data(), currentData.data(), 23, 17, 256, &destructionCount);
    KisRandomConstAccessorNG &accessor = sentinel;

    accessor.moveTo(13, -8);

    QCOMPARE(sentinel.moveCalls(), 1);
    QCOMPARE(accessor.x(), 13);
    QCOMPARE(accessor.y(), -8);
    QCOMPARE(accessor.oldRawData(), oldData.data());
    QCOMPARE(accessor.rawDataConst(), currentData.data());
    QCOMPARE(accessor.oldRawData()[1], quint8(0x22));
    QCOMPARE(accessor.rawDataConst()[1], quint8(0xb2));
}

void KisRandomAccessorNGContractTest::constAccessorRoutesContiguousShapeAndStride()
{
    const std::array<quint8, 1> data{{0x5a}};
    int destructionCount = 0;
    RandomConstAccessorSentinel sentinel(data.data(), data.data(), 23, 17, 256, &destructionCount);
    const KisRandomConstAccessorNG &accessor = sentinel;

    QCOMPARE(accessor.numContiguousColumns(5), 23);
    QCOMPARE(accessor.numContiguousRows(-7), 17);
    QCOMPARE(accessor.rowStride(11, -13), 256);
    QCOMPARE(sentinel.lastColumnQuery(), 5);
    QCOMPARE(sentinel.lastRowQuery(), -7);
    QCOMPARE(sentinel.lastStrideX(), 11);
    QCOMPARE(sentinel.lastStrideY(), -13);
}

void KisRandomAccessorNGContractTest::writableAccessorExposesMutableAndConstViews()
{
    const std::array<quint8, 3> oldData{{0x10, 0x20, 0x30}};
    std::array<quint8, 3> currentData{{0x40, 0x50, 0x60}};
    int destructionCount = 0;
    RandomAccessorSentinel sentinel(oldData.data(), currentData.data(), &destructionCount);
    KisRandomAccessorNG &accessor = sentinel;
    KisBaseAccessor &writableView = accessor;
    const KisRandomConstAccessorNG &constView = accessor;

    QCOMPARE(writableView.rawData(), currentData.data());
    QCOMPARE(constView.rawDataConst(), currentData.data());
    QCOMPARE(constView.oldRawData(), oldData.data());

    writableView.rawData()[2] = 0x7f;
    QCOMPARE(currentData[2], quint8(0x7f));
    QCOMPARE(constView.rawDataConst()[2], quint8(0x7f));
    QCOMPARE(constView.oldRawData()[2], quint8(0x30));
}

void KisRandomAccessorNGContractTest::randomAccessorBasesPreserveVirtualLifetime()
{
    std::array<quint8, 1> data{{0x5a}};

    int constDestructionCount = 0;
    {
        std::unique_ptr<KisRandomConstAccessorNG> accessor(
            new RandomConstAccessorSentinel(data.data(), data.data(), 23, 17, 256, &constDestructionCount));
    }
    QCOMPARE(constDestructionCount, 1);

    int randomThroughConstDestructionCount = 0;
    {
        std::unique_ptr<KisRandomConstAccessorNG> accessor(
            new RandomAccessorSentinel(data.data(), data.data(), &randomThroughConstDestructionCount));
    }
    QCOMPARE(randomThroughConstDestructionCount, 1);

    int randomThroughWritableDestructionCount = 0;
    {
        std::unique_ptr<KisBaseAccessor> accessor(
            new RandomAccessorSentinel(data.data(), data.data(), &randomThroughWritableDestructionCount));
    }
    QCOMPARE(randomThroughWritableDestructionCount, 1);
}

QTEST_GUILESS_MAIN(KisRandomAccessorNGContractTest)

#include "KisRandomAccessorNGContractTest.moc"
