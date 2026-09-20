/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "color/kis_display_color_filter.h"

#include <QTest>

#include <memory>

namespace
{
class RecordingDisplayColorFilter final : public KisDisplayColorFilter
{
public:
    explicit RecordingDisplayColorFilter(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~RecordingDisplayColorFilter() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void filter(quint8 *, quint32) override
    {
    }

    bool useInternalColorManagement() const override
    {
        return false;
    }

    void approximateInverseTransformation(quint8 *pixels, quint32 numPixels) override
    {
        lastInversePixels = pixels;
        lastInverseNumPixels = numPixels;
        ++inverseCalls;

        for (quint32 index = 0; index < numPixels; ++index) {
            pixels[index] = quint8(index + 20);
        }
    }

    void approximateForwardTransformation(quint8 *pixels, quint32 numPixels) override
    {
        lastForwardPixels = pixels;
        lastForwardNumPixels = numPixels;
        ++forwardCalls;

        for (quint32 index = 0; index < numPixels; ++index) {
            pixels[index] = quint8(index + 30);
        }
    }

    quint8 *lastInversePixels{nullptr};
    quint8 *lastForwardPixels{nullptr};
    quint32 lastInverseNumPixels{0};
    quint32 lastForwardNumPixels{0};
    int inverseCalls{0};
    int forwardCalls{0};

private:
    int *m_destructionCount{nullptr};
};
} // namespace

class KisDisplayColorFilterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void inverseTransformationDispatchesMutableBufferAndCount();
    void forwardTransformationDispatchesMutableBufferAndCount();
    void baseOwnershipDestroysDerivedOnce();
};

void KisDisplayColorFilterContractTest::inverseTransformationDispatchesMutableBufferAndCount()
{
    RecordingDisplayColorFilter filter;
    KisDisplayColorFilter &interface = filter;
    quint8 pixels[]{1, 2, 3, 4};

    interface.approximateInverseTransformation(pixels, 4);

    QCOMPARE(filter.lastInversePixels, pixels);
    QCOMPARE(filter.lastInverseNumPixels, quint32(4));
    QCOMPARE(filter.inverseCalls, 1);
    for (quint32 index = 0; index < 4; ++index) {
        QCOMPARE(pixels[index], quint8(index + 20));
    }
}

void KisDisplayColorFilterContractTest::forwardTransformationDispatchesMutableBufferAndCount()
{
    RecordingDisplayColorFilter filter;
    KisDisplayColorFilter &interface = filter;
    quint8 pixels[]{5, 6, 7, 8, 9};

    interface.approximateForwardTransformation(pixels, 5);

    QCOMPARE(filter.lastForwardPixels, pixels);
    QCOMPARE(filter.lastForwardNumPixels, quint32(5));
    QCOMPARE(filter.forwardCalls, 1);
    for (quint32 index = 0; index < 5; ++index) {
        QCOMPARE(pixels[index], quint8(index + 30));
    }
}

void KisDisplayColorFilterContractTest::baseOwnershipDestroysDerivedOnce()
{
    int destructionCount = 0;
    {
        std::unique_ptr<KisDisplayColorFilter> filter =
            std::make_unique<RecordingDisplayColorFilter>(&destructionCount);
        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisDisplayColorFilterContractTest)

#include "KisDisplayColorFilterContractTest.moc"
