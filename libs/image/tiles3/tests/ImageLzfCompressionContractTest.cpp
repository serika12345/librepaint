/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <tiles3/swap/kis_abstract_compression.h>
#include <tiles3/swap/kis_lzf_compression.h>

#include <QByteArray>
#include <QTest>

#include <cstring>
#include <memory>
#include <utility>

namespace
{
class AbstractCompressionProbe final : public KisAbstractCompression
{
public:
    explicit AbstractCompressionProbe(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~AbstractCompressionProbe() override
    {
        *m_destroyed = true;
    }

    qint32 compress(const quint8 *input, qint32 inputLength, quint8 *output, qint32 outputLength) override
    {
        ++compressCallCount;
        const qint32 copiedBytes = qMin(inputLength, outputLength);
        std::memcpy(output, input, static_cast<std::size_t>(copiedBytes));
        return copiedBytes;
    }

    qint32 decompress(const quint8 *input, qint32 inputLength, quint8 *output, qint32 outputLength) override
    {
        ++decompressCallCount;
        const qint32 copiedBytes = qMin(inputLength, outputLength);
        std::memcpy(output, input, static_cast<std::size_t>(copiedBytes));
        return copiedBytes;
    }

    qint32 outputBufferSize(qint32 dataSize) override
    {
        ++outputBufferSizeCallCount;
        return dataSize + 5;
    }

    int compressCallCount{0};
    int decompressCallCount{0};
    int outputBufferSizeCallCount{0};

private:
    bool *m_destroyed;
};

class LzfCompressionProbe final : public KisLzfCompression
{
public:
    explicit LzfCompressionProbe(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~LzfCompressionProbe() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};

quint8 *bytes(QByteArray &data)
{
    return reinterpret_cast<quint8 *>(data.data());
}

const quint8 *bytes(const QByteArray &data)
{
    return reinterpret_cast<const quint8 *>(data.constData());
}
} // namespace

class ImageLzfCompressionContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void abstractInterfaceDispatchesAndUsesDefaultAdjustment();
    void colorLinearizationRoundTripsComponentPlanes();
    void lzfCompressionIsDeterministicBoundedAndReversible();
};

void ImageLzfCompressionContractTest::abstractInterfaceDispatchesAndUsesDefaultAdjustment()
{
    bool destroyed = false;

    {
        auto probe = std::make_unique<AbstractCompressionProbe>(&destroyed);
        AbstractCompressionProbe *observation = probe.get();
        std::unique_ptr<KisAbstractCompression> compression(std::move(probe));
        const QByteArray input = QByteArray::fromHex("00ff1020");
        QByteArray compressed(input.size(), '\0');
        QByteArray restored(input.size(), '\0');

        QCOMPARE(compression->outputBufferSize(input.size()), input.size() + 5);
        QCOMPARE(compression->compress(bytes(input), input.size(), bytes(compressed), compressed.size()), input.size());
        QCOMPARE(compressed, input);
        QCOMPARE(compression->decompress(bytes(compressed), compressed.size(), bytes(restored), restored.size()),
                 input.size());
        QCOMPARE(restored, input);

        compression->adjustForDataSize(4096);
        QCOMPARE(compression->outputBufferSize(input.size()), input.size() + 5);
        QCOMPARE(observation->compressCallCount, 1);
        QCOMPARE(observation->decompressCallCount, 1);
        QCOMPARE(observation->outputBufferSizeCallCount, 2);
        QVERIFY(!destroyed);
    }

    QVERIFY(destroyed);
}

void ImageLzfCompressionContractTest::colorLinearizationRoundTripsComponentPlanes()
{
    const QByteArray interleaved = QByteArray::fromHex("000102031011121320212223");
    const QByteArray expectedLinearized = QByteArray::fromHex("001020011121021222031323");
    QByteArray linearized(interleaved.size(), '\0');
    QByteArray restored(interleaved.size(), '\0');
    QByteArray mutableInput = interleaved;

    KisAbstractCompression::linearizeColors(bytes(mutableInput), bytes(linearized), mutableInput.size(), 4);
    QCOMPARE(mutableInput, interleaved);
    QCOMPARE(linearized, expectedLinearized);

    KisAbstractCompression::delinearizeColors(bytes(linearized), bytes(restored), linearized.size(), 4);
    QCOMPARE(restored, interleaved);
}

void ImageLzfCompressionContractTest::lzfCompressionIsDeterministicBoundedAndReversible()
{
    QByteArray input;
    const QByteArray pattern = QByteArray::fromHex("0001020304050607ff80402010080402");
    for (int repetition = 0; repetition < 64; ++repetition) {
        input.append(pattern);
    }

    bool destroyed = false;
    {
        std::unique_ptr<KisAbstractCompression> compression(new LzfCompressionProbe(&destroyed));
        compression->adjustForDataSize(input.size());

        const qint32 dataSizes[] = {0, 1, 15, 16, 17, 255, 1024};
        for (const qint32 dataSize : dataSizes) {
            QCOMPARE(compression->outputBufferSize(dataSize), dataSize + dataSize / 16 + 64 + 3);
        }

        const qint32 outputSize = compression->outputBufferSize(input.size());
        QCOMPARE(outputSize, input.size() + input.size() / 16 + 64 + 3);

        QByteArray firstCompressed(outputSize, '\0');
        QByteArray secondCompressed(outputSize, '\0');
        const qint32 firstSize =
            compression->compress(bytes(input), input.size(), bytes(firstCompressed), firstCompressed.size());
        const qint32 secondSize =
            compression->compress(bytes(input), input.size(), bytes(secondCompressed), secondCompressed.size());

        QVERIFY(firstSize > 0);
        QVERIFY(firstSize < input.size());
        QVERIFY(firstSize <= outputSize);
        QCOMPARE(secondSize, firstSize);
        QCOMPARE(firstCompressed.first(firstSize), secondCompressed.first(secondSize));

        QByteArray restored(input.size(), '\0');
        QCOMPARE(compression->decompress(bytes(firstCompressed), firstSize, bytes(restored), restored.size()),
                 input.size());
        QCOMPARE(restored, input);

        QByteArray undersizedOutput(input.size() + 1, char(0x5a));
        const qint32 undersizedCapacity = input.size() - 1;
        QCOMPARE(
            compression->decompress(bytes(firstCompressed), firstSize, bytes(undersizedOutput), undersizedCapacity),
            0);
        QCOMPARE(quint8(undersizedOutput.at(undersizedCapacity)), quint8(0x5a));
        QVERIFY(!destroyed);
    }

    QVERIFY(destroyed);
}

QTEST_GUILESS_MAIN(ImageLzfCompressionContractTest)

#include "ImageLzfCompressionContractTest.moc"
