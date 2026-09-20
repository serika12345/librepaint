/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoAlphaMaskApplicatorBase.h>

#include <QTest>

#include <array>
#include <memory>

class RecordingAlphaMaskApplicator : public KoAlphaMaskApplicatorBase
{
public:
    explicit RecordingAlphaMaskApplicator(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~RecordingAlphaMaskApplicator() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void applyInverseNormedFloatMask(quint8 *pixels, const float *alpha, qint32 nPixels) const override
    {
        ++applyCallCount;
        applyPixels = pixels;
        applyAlpha = alpha;
        applyLength = nPixels;
        pixels[0] = static_cast<quint8>(alpha[0] * 100.0f);
        pixels[nPixels - 1] = static_cast<quint8>(alpha[nPixels - 1] * 100.0f);
    }

    void fillInverseAlphaNormedFloatMaskWithColor(quint8 *pixels,
                                                  const float *alpha,
                                                  const quint8 *brushColor,
                                                  qint32 nPixels) const override
    {
        ++fillInverseCallCount;
        fillInversePixels = pixels;
        fillInverseAlpha = alpha;
        fillInverseBrushColor = brushColor;
        fillInverseLength = nPixels;
        pixels[0] = static_cast<quint8>(alpha[0] * brushColor[0]);
        pixels[nPixels - 1] = static_cast<quint8>(alpha[nPixels - 1] * brushColor[1]);
    }

    void fillGrayBrushWithColor(quint8 *dst, const QRgb *brush, quint8 *brushColor, qint32 nPixels) const override
    {
        ++fillGrayCallCount;
        fillGrayDestination = dst;
        fillGrayBrush = brush;
        fillGrayBrushColor = brushColor;
        fillGrayLength = nPixels;
        dst[0] = static_cast<quint8>(qRed(brush[0]));
        dst[nPixels - 1] = brushColor[0];
    }

    mutable int applyCallCount = 0;
    mutable quint8 *applyPixels = nullptr;
    mutable const float *applyAlpha = nullptr;
    mutable qint32 applyLength = -1;

    mutable int fillInverseCallCount = 0;
    mutable quint8 *fillInversePixels = nullptr;
    mutable const float *fillInverseAlpha = nullptr;
    mutable const quint8 *fillInverseBrushColor = nullptr;
    mutable qint32 fillInverseLength = -1;

    mutable int fillGrayCallCount = 0;
    mutable quint8 *fillGrayDestination = nullptr;
    mutable const QRgb *fillGrayBrush = nullptr;
    mutable quint8 *fillGrayBrushColor = nullptr;
    mutable qint32 fillGrayLength = -1;

private:
    int *m_destructionCount;
};

class KoAlphaMaskApplicatorBaseContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void virtualMethodsPreserveArgumentsAndWrites();
    void baseOwnershipDestroysDerivedExactlyOnce();
};

void KoAlphaMaskApplicatorBaseContractTest::virtualMethodsPreserveArgumentsAndWrites()
{
    RecordingAlphaMaskApplicator applicator;
    const KoAlphaMaskApplicatorBase &interface = applicator;

    std::array<quint8, 3> inversePixels{{9, 9, 9}};
    const std::array<float, 3> inverseAlpha{{0.25f, 0.5f, 0.75f}};
    interface.applyInverseNormedFloatMask(inversePixels.data(), inverseAlpha.data(), 3);

    QCOMPARE(applicator.applyCallCount, 1);
    QCOMPARE(applicator.applyPixels, inversePixels.data());
    QCOMPARE(applicator.applyAlpha, inverseAlpha.data());
    QCOMPARE(applicator.applyLength, 3);
    QCOMPARE(inversePixels, (std::array<quint8, 3>{{25, 9, 75}}));

    std::array<quint8, 2> coloredPixels{{0, 0}};
    const std::array<float, 2> coloredAlpha{{0.5f, 0.25f}};
    const std::array<quint8, 2> brushColor{{60, 80}};
    interface.fillInverseAlphaNormedFloatMaskWithColor(coloredPixels.data(), coloredAlpha.data(), brushColor.data(), 2);

    QCOMPARE(applicator.fillInverseCallCount, 1);
    QCOMPARE(applicator.fillInversePixels, coloredPixels.data());
    QCOMPARE(applicator.fillInverseAlpha, coloredAlpha.data());
    QCOMPARE(applicator.fillInverseBrushColor, brushColor.data());
    QCOMPARE(applicator.fillInverseLength, 2);
    QCOMPARE(coloredPixels, (std::array<quint8, 2>{{30, 20}}));

    std::array<quint8, 4> grayDestination{{0, 0, 0, 0}};
    const std::array<QRgb, 4> grayBrush{{qRgb(11, 12, 13), qRgb(21, 22, 23), qRgb(31, 32, 33), qRgb(41, 42, 43)}};
    std::array<quint8, 4> grayBrushColor{{91, 92, 93, 94}};
    interface.fillGrayBrushWithColor(grayDestination.data(), grayBrush.data(), grayBrushColor.data(), 4);

    QCOMPARE(applicator.fillGrayCallCount, 1);
    QCOMPARE(applicator.fillGrayDestination, grayDestination.data());
    QCOMPARE(applicator.fillGrayBrush, grayBrush.data());
    QCOMPARE(applicator.fillGrayBrushColor, grayBrushColor.data());
    QCOMPARE(applicator.fillGrayLength, 4);
    QCOMPARE(grayDestination, (std::array<quint8, 4>{{11, 0, 0, 91}}));
}

void KoAlphaMaskApplicatorBaseContractTest::baseOwnershipDestroysDerivedExactlyOnce()
{
    int destructionCount = 0;

    {
        std::unique_ptr<KoAlphaMaskApplicatorBase> applicator =
            std::make_unique<RecordingAlphaMaskApplicator>(&destructionCount);
        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KoAlphaMaskApplicatorBaseContractTest)

#include "KoAlphaMaskApplicatorBaseContractTest.moc"
