/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_projection_pixel_filter.h"

#include <QTest>

#include <memory>

namespace
{
class RecordingPixelFilter final : public KisProjectionPixelFilter
{
public:
    explicit RecordingPixelFilter(bool internalColorManagement = false, int *destructionCount = nullptr)
        : m_internalColorManagement(internalColorManagement)
        , m_destructionCount(destructionCount)
    {
    }

    ~RecordingPixelFilter() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void filter(quint8 *pixels, quint32 numPixels) override
    {
        lastPixels = pixels;
        lastNumPixels = numPixels;
        ++filterCalls;

        for (quint32 index = 0; index < numPixels; ++index) {
            pixels[index] = quint8(index + 10);
        }
    }

    bool useInternalColorManagement() const override
    {
        ++colorManagementCalls;
        return m_internalColorManagement;
    }

    quint8 *lastPixels{nullptr};
    quint32 lastNumPixels{0};
    int filterCalls{0};
    mutable int colorManagementCalls{0};

private:
    bool m_internalColorManagement{false};
    int *m_destructionCount{nullptr};
};
} // namespace

class KisProjectionPixelFilterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void filterDispatchesMutableBufferAndCount();
    void colorManagementModeDispatchesBothValues();
    void baseOwnershipDestroysDerivedOnce();
};

void KisProjectionPixelFilterContractTest::filterDispatchesMutableBufferAndCount()
{
    RecordingPixelFilter filter;
    KisProjectionPixelFilter &interface = filter;
    quint8 pixels[]{1, 2, 3, 4, 5};

    interface.filter(pixels, 5);

    QCOMPARE(filter.lastPixels, pixels);
    QCOMPARE(filter.lastNumPixels, quint32(5));
    QCOMPARE(filter.filterCalls, 1);
    for (quint32 index = 0; index < 5; ++index) {
        QCOMPARE(pixels[index], quint8(index + 10));
    }
}

void KisProjectionPixelFilterContractTest::colorManagementModeDispatchesBothValues()
{
    RecordingPixelFilter external(false);
    RecordingPixelFilter internal(true);
    const KisProjectionPixelFilter &externalInterface = external;
    const KisProjectionPixelFilter &internalInterface = internal;

    QVERIFY(!externalInterface.useInternalColorManagement());
    QVERIFY(internalInterface.useInternalColorManagement());
    QCOMPARE(external.colorManagementCalls, 1);
    QCOMPARE(internal.colorManagementCalls, 1);
}

void KisProjectionPixelFilterContractTest::baseOwnershipDestroysDerivedOnce()
{
    int destructionCount = 0;
    {
        std::unique_ptr<KisProjectionPixelFilter> filter =
            std::make_unique<RecordingPixelFilter>(false, &destructionCount);
        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisProjectionPixelFilterContractTest)

#include "KisProjectionPixelFilterContractTest.moc"
