/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_projection_updates_filter.h"

#include <QRect>
#include <QTest>

namespace
{

class ProjectionUpdatesFilterProbe final : public KisProjectionUpdatesFilter
{
public:
    explicit ProjectionUpdatesFilterProbe(bool *destroyed = nullptr)
        : destroyed(destroyed)
    {
    }

    ~ProjectionUpdatesFilterProbe() override
    {
        if (destroyed) {
            *destroyed = true;
        }
    }

    bool filter(KisImage *image, KisNode *node, const QVector<QRect> &rects, KisProjectionUpdateFlags flags) override
    {
        ++filterCalls;
        filteredImage = image;
        filteredNode = node;
        filteredRects = rects;
        filteredFlags = flags;
        return filterResult;
    }

    bool filterRefreshGraph(KisImage *image,
                            KisNode *node,
                            const QVector<QRect> &rects,
                            const QRect &cropRect,
                            KisProjectionUpdateFlags flags) override
    {
        ++refreshCalls;
        refreshedImage = image;
        refreshedNode = node;
        refreshedRects = rects;
        refreshedCropRect = cropRect;
        refreshedFlags = flags;
        return refreshResult;
    }

    bool *destroyed = nullptr;
    int filterCalls = 0;
    int refreshCalls = 0;
    bool filterResult = false;
    bool refreshResult = false;
    KisImage *filteredImage = nullptr;
    KisNode *filteredNode = nullptr;
    QVector<QRect> filteredRects;
    KisProjectionUpdateFlags filteredFlags;
    KisImage *refreshedImage = nullptr;
    KisNode *refreshedNode = nullptr;
    QVector<QRect> refreshedRects;
    QRect refreshedCropRect;
    KisProjectionUpdateFlags refreshedFlags;
};

} // namespace

class KisProjectionUpdatesFilterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseDispatchesArgumentsResultsAndLifetime();
    void dropAllRejectsDirectAndRefreshUpdates();
};

void KisProjectionUpdatesFilterContractTest::baseDispatchesArgumentsResultsAndLifetime()
{
    alignas(void *) unsigned char imageStorage = 0;
    alignas(void *) unsigned char nodeStorage = 0;
    auto *image = reinterpret_cast<KisImage *>(&imageStorage);
    auto *node = reinterpret_cast<KisNode *>(&nodeStorage);
    const QVector<QRect> rects{QRect(1, 2, 30, 40), QRect(-5, 6, 7, 8)};
    const QRect cropRect(-10, -20, 100, 200);
    const KisProjectionUpdateFlags flags =
        KisProjectionUpdateFlag::NoFilthy | KisProjectionUpdateFlag::DontInvalidateFrames;

    ProjectionUpdatesFilterProbe probe;
    probe.filterResult = true;
    probe.refreshResult = false;
    KisProjectionUpdatesFilter *filter = &probe;

    QVERIFY(filter->filter(image, node, rects, flags));
    QCOMPARE(probe.filterCalls, 1);
    QCOMPARE(probe.filteredImage, image);
    QCOMPARE(probe.filteredNode, node);
    QCOMPARE(probe.filteredRects, rects);
    QCOMPARE(probe.filteredFlags, flags);

    QVERIFY(!filter->filterRefreshGraph(image, node, rects, cropRect, flags));
    QCOMPARE(probe.refreshCalls, 1);
    QCOMPARE(probe.refreshedImage, image);
    QCOMPARE(probe.refreshedNode, node);
    QCOMPARE(probe.refreshedRects, rects);
    QCOMPARE(probe.refreshedCropRect, cropRect);
    QCOMPARE(probe.refreshedFlags, flags);

    bool destroyed = false;
    filter = new ProjectionUpdatesFilterProbe(&destroyed);
    delete filter;
    QVERIFY(destroyed);
}

void KisProjectionUpdatesFilterContractTest::dropAllRejectsDirectAndRefreshUpdates()
{
    alignas(void *) unsigned char imageStorage = 0;
    alignas(void *) unsigned char nodeStorage = 0;
    auto *image = reinterpret_cast<KisImage *>(&imageStorage);
    auto *node = reinterpret_cast<KisNode *>(&nodeStorage);
    const QVector<QRect> rects{QRect(3, 4, 50, 60)};
    const QRect cropRect(1, 2, 100, 200);

    KisDropAllProjectionUpdatesFilter filter;

    QVERIFY(filter.filter(image, node, rects, KisProjectionUpdateFlag::NoFilthy));
    QVERIFY(filter.filterRefreshGraph(image, node, rects, cropRect, KisProjectionUpdateFlag::DontInvalidateFrames));
}

QTEST_GUILESS_MAIN(KisProjectionUpdatesFilterContractTest)

#include "KisProjectionUpdatesFilterContractTest.moc"
