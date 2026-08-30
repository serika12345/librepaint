/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <array>
#include <memory>
#include <type_traits>

#include <QTest>

#include "strokes/KisMaskingBrushCompositeOpBase.h"

namespace
{

class RecordingCompositeOp final : public KisMaskingBrushCompositeOpBase
{
public:
    explicit RecordingCompositeOp(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~RecordingCompositeOp() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void composite(const quint8 *srcRowStart,
                   int srcRowStride,
                   quint8 *dstRowStart,
                   int dstRowStride,
                   int columns,
                   int rows) override
    {
        sourceRowStart = srcRowStart;
        sourceRowStride = srcRowStride;
        destinationRowStart = dstRowStart;
        destinationRowStride = dstRowStride;
        columnCount = columns;
        rowCount = rows;
        ++callCount;
    }

    const quint8 *sourceRowStart = nullptr;
    int sourceRowStride = 0;
    quint8 *destinationRowStart = nullptr;
    int destinationRowStride = 0;
    int columnCount = 0;
    int rowCount = 0;
    int callCount = 0;

private:
    int *m_destructionCount;
};

} // namespace

class KisMaskingBrushCompositeOpBaseContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void interfaceRemainsAbstractAndPolymorphic();
    void compositeForwardsBuffersStridesAndDimensions();
    void baseOwnershipUsesVirtualDestruction();
};

void KisMaskingBrushCompositeOpBaseContractTest::interfaceRemainsAbstractAndPolymorphic()
{
    static_assert(std::is_abstract_v<KisMaskingBrushCompositeOpBase>);
    static_assert(std::is_polymorphic_v<KisMaskingBrushCompositeOpBase>);
    static_assert(std::has_virtual_destructor_v<KisMaskingBrushCompositeOpBase>);
}

void KisMaskingBrushCompositeOpBaseContractTest::compositeForwardsBuffersStridesAndDimensions()
{
    std::array<quint8, 32> source{};
    std::array<quint8, 48> destination{};
    RecordingCompositeOp operation;
    KisMaskingBrushCompositeOpBase &interface = operation;

    const quint8 *const sourceStart = source.data() + 3;
    quint8 *const destinationStart = destination.data() + 5;

    interface.composite(sourceStart, 17, destinationStart, 23, 11, 7);

    QCOMPARE(operation.sourceRowStart, sourceStart);
    QCOMPARE(operation.sourceRowStride, 17);
    QCOMPARE(operation.destinationRowStart, destinationStart);
    QCOMPARE(operation.destinationRowStride, 23);
    QCOMPARE(operation.columnCount, 11);
    QCOMPARE(operation.rowCount, 7);
    QCOMPARE(operation.callCount, 1);
}

void KisMaskingBrushCompositeOpBaseContractTest::baseOwnershipUsesVirtualDestruction()
{
    int destructionCount = 0;

    {
        std::unique_ptr<KisMaskingBrushCompositeOpBase> operation =
            std::make_unique<RecordingCompositeOp>(&destructionCount);
    }

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisMaskingBrushCompositeOpBaseContractTest)

#include "KisMaskingBrushCompositeOpBaseContractTest.moc"
