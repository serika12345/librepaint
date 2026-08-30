/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisDitherOp.h>
#include <KoID.h>

#include <QTest>

#include <array>
#include <memory>
#include <utility>

class RecordingDitherOp final : public KisDitherOp
{
public:
    RecordingDitherOp(KoID sourceDepth, KoID destinationDepth, DitherType ditherType, int *destructionCount = nullptr)
        : m_sourceDepth(std::move(sourceDepth))
        , m_destinationDepth(std::move(destinationDepth))
        , m_ditherType(ditherType)
        , m_destructionCount(destructionCount)
    {
    }

    ~RecordingDitherOp() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void dither(const quint8 *src, quint8 *dst, int x, int y) const override
    {
        ++singleCallCount;
        singleSource = src;
        singleDestination = dst;
        singleX = x;
        singleY = y;

        dst[0] = quint8(src[0] + x + y);
    }

    void dither(const quint8 *srcRowStart,
                int srcRowStride,
                quint8 *dstRowStart,
                int dstRowStride,
                int x,
                int y,
                int columns,
                int rows) const override
    {
        ++rectangularCallCount;
        rectangularSource = srcRowStart;
        rectangularSourceStride = srcRowStride;
        rectangularDestination = dstRowStart;
        rectangularDestinationStride = dstRowStride;
        rectangularX = x;
        rectangularY = y;
        rectangularColumns = columns;
        rectangularRows = rows;

        dstRowStart[0] = quint8(srcRowStart[0] + x);
        dstRowStart[dstRowStride] = quint8(srcRowStart[srcRowStride] + y);
    }

    KoID sourceDepthId() const override
    {
        ++sourceDepthCallCount;
        return m_sourceDepth;
    }

    KoID destinationDepthId() const override
    {
        ++destinationDepthCallCount;
        return m_destinationDepth;
    }

    DitherType type() const override
    {
        ++typeCallCount;
        return m_ditherType;
    }

    mutable int singleCallCount = 0;
    mutable const quint8 *singleSource = nullptr;
    mutable quint8 *singleDestination = nullptr;
    mutable int singleX = -1;
    mutable int singleY = -1;

    mutable int rectangularCallCount = 0;
    mutable const quint8 *rectangularSource = nullptr;
    mutable int rectangularSourceStride = -1;
    mutable quint8 *rectangularDestination = nullptr;
    mutable int rectangularDestinationStride = -1;
    mutable int rectangularX = -1;
    mutable int rectangularY = -1;
    mutable int rectangularColumns = -1;
    mutable int rectangularRows = -1;

    mutable int sourceDepthCallCount = 0;
    mutable int destinationDepthCallCount = 0;
    mutable int typeCallCount = 0;

private:
    KoID m_sourceDepth;
    KoID m_destinationDepth;
    DitherType m_ditherType;
    int *m_destructionCount;
};

class KisDitherOpContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void enumValuesRemainStable();
    void singlePixelDitherPreservesArgumentsAndResult();
    void rectangularDitherPreservesArgumentsAndResult();
    void identifiersTypeAndBaseLifetimeRemainStable();
};

void KisDitherOpContractTest::enumValuesRemainStable()
{
    QCOMPARE(int(DITHER_NONE), 0);
    QCOMPARE(int(DITHER_FAST), 1);
    QCOMPARE(int(DITHER_BEST), 2);
    QCOMPARE(int(DITHER_BAYER), 3);
    QCOMPARE(int(DITHER_BLUE_NOISE), 4);
}

void KisDitherOpContractTest::singlePixelDitherPreservesArgumentsAndResult()
{
    RecordingDitherOp op(KoID(QStringLiteral("U8")), KoID(QStringLiteral("U16")), DITHER_FAST);
    const KisDitherOp &interface = op;
    const std::array<quint8, 3> source{13, 17, 19};
    std::array<quint8, 3> destination{0xaa, 0xaa, 0xaa};

    interface.dither(source.data(), destination.data(), 2, 7);

    QCOMPARE(op.singleCallCount, 1);
    QCOMPARE(op.singleSource, source.data());
    QCOMPARE(op.singleDestination, destination.data());
    QCOMPARE(op.singleX, 2);
    QCOMPARE(op.singleY, 7);
    QCOMPARE(destination[0], quint8(22));
    QCOMPARE(destination[1], quint8(0xaa));
    QCOMPARE(destination[2], quint8(0xaa));
}

void KisDitherOpContractTest::rectangularDitherPreservesArgumentsAndResult()
{
    RecordingDitherOp op(KoID(QStringLiteral("U8")), KoID(QStringLiteral("U16")), DITHER_BAYER);
    const KisDitherOp &interface = op;
    std::array<quint8, 20> source{};
    std::array<quint8, 24> destination{};
    source[0] = 11;
    source[7] = 29;
    destination.fill(0xcc);

    interface.dither(source.data(), 7, destination.data(), 9, 4, 5, 3, 2);

    QCOMPARE(op.rectangularCallCount, 1);
    QCOMPARE(op.rectangularSource, source.data());
    QCOMPARE(op.rectangularSourceStride, 7);
    QCOMPARE(op.rectangularDestination, destination.data());
    QCOMPARE(op.rectangularDestinationStride, 9);
    QCOMPARE(op.rectangularX, 4);
    QCOMPARE(op.rectangularY, 5);
    QCOMPARE(op.rectangularColumns, 3);
    QCOMPARE(op.rectangularRows, 2);
    QCOMPARE(destination[0], quint8(15));
    QCOMPARE(destination[9], quint8(34));
    QCOMPARE(destination[1], quint8(0xcc));
    QCOMPARE(destination[10], quint8(0xcc));
}

void KisDitherOpContractTest::identifiersTypeAndBaseLifetimeRemainStable()
{
    int destructionCount = 0;

    {
        auto recordingOp = std::make_unique<RecordingDitherOp>(
            KoID(QStringLiteral("source-depth"), QStringLiteral("Source")),
            KoID(QStringLiteral("destination-depth"), QStringLiteral("Destination")),
            DITHER_BLUE_NOISE,
            &destructionCount);
        RecordingDitherOp *probe = recordingOp.get();
        std::unique_ptr<KisDitherOp> interface = std::move(recordingOp);

        const KoID sourceDepth = interface->sourceDepthId();
        const KoID destinationDepth = interface->destinationDepthId();

        QCOMPARE(sourceDepth.id(), QStringLiteral("source-depth"));
        QCOMPARE(sourceDepth.name(), QStringLiteral("Source"));
        QCOMPARE(destinationDepth.id(), QStringLiteral("destination-depth"));
        QCOMPARE(destinationDepth.name(), QStringLiteral("Destination"));
        QCOMPARE(interface->type(), DITHER_BLUE_NOISE);
        QCOMPARE(probe->sourceDepthCallCount, 1);
        QCOMPARE(probe->destinationDepthCallCount, 1);
        QCOMPARE(probe->typeCallCount, 1);
        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisDitherOpContractTest)

#include "KisDitherOpContractTest.moc"
