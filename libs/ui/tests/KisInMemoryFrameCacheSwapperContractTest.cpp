/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "animation/cache/KisAbstractFrameCacheSwapper.h"
#include "animation/cache/KisInMemoryFrameCacheSwapper.h"
#include "opengl/kis_opengl_update_info.h"

#include <QRect>
#include <QTest>

#include <memory>

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

namespace
{


KisOpenGLUpdateInfoSP makeUpdateInfo(int levelOfDetail, const QRect &dirtyRect)
{
    KisOpenGLUpdateInfoSP info = new KisOpenGLUpdateInfo;
    info->assignLevelOfDetail(levelOfDetail);
    info->assignDirtyImageRect(dirtyRect);
    return info;
}

} // namespace

class KisInMemoryFrameCacheSwapperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeLifetimeAndEmptyStateRemainStable();
    void saveLoadAndPresenceRemainStable();
    void frameMetadataRemainsStable();
    void moveFrameTransfersIdentityAndMetadata();
    void forgetFrameDropsStoredIdentity();
};

void KisInMemoryFrameCacheSwapperContractTest::typeLifetimeAndEmptyStateRemainStable()
{
    using Abstract = KisAbstractFrameCacheSwapper;
    using Concrete = KisInMemoryFrameCacheSwapper;


    std::unique_ptr<Abstract> swapper = std::make_unique<Concrete>();
    QVERIFY(!swapper->hasFrame(4));
}

void KisInMemoryFrameCacheSwapperContractTest::saveLoadAndPresenceRemainStable()
{
    using Abstract = KisAbstractFrameCacheSwapper;
    using Concrete = KisInMemoryFrameCacheSwapper;


    Concrete concrete;
    Abstract &swapper = concrete;
    KisOpenGLUpdateInfoSP info = makeUpdateInfo(2, QRect(3, 5, 7, 11));
    const KisOpenGLUpdateInfo *const identity = info.data();

    QVERIFY(!swapper.hasFrame(17));
    swapper.saveFrame(17, info, QRect(0, 0, 40, 30));
    info.clear();
    QVERIFY(swapper.hasFrame(17));
    QCOMPARE(swapper.loadFrame(17).data(), identity);
}

void KisInMemoryFrameCacheSwapperContractTest::frameMetadataRemainsStable()
{
    using Abstract = KisAbstractFrameCacheSwapper;
    using Concrete = KisInMemoryFrameCacheSwapper;


    Concrete concrete;
    Abstract &swapper = concrete;
    const QRect dirtyRect(13, 17, 19, 23);
    swapper.saveFrame(5, makeUpdateInfo(4, dirtyRect), QRect(0, 0, 80, 60));

    QCOMPARE(swapper.frameLevelOfDetail(5), 4);
    QCOMPARE(swapper.frameDirtyRect(5), dirtyRect);
}

void KisInMemoryFrameCacheSwapperContractTest::moveFrameTransfersIdentityAndMetadata()
{
    using Abstract = KisAbstractFrameCacheSwapper;
    using Concrete = KisInMemoryFrameCacheSwapper;


    Concrete concrete;
    Abstract &swapper = concrete;
    const QRect dirtyRect(2, 3, 29, 31);
    KisOpenGLUpdateInfoSP info = makeUpdateInfo(6, dirtyRect);
    const KisOpenGLUpdateInfo *const identity = info.data();
    swapper.saveFrame(8, info, QRect(0, 0, 100, 90));
    info.clear();

    swapper.moveFrame(8, 12);

    QVERIFY(!swapper.hasFrame(8));
    QVERIFY(swapper.hasFrame(12));
    QCOMPARE(swapper.loadFrame(12).data(), identity);
    QCOMPARE(swapper.frameLevelOfDetail(12), 6);
    QCOMPARE(swapper.frameDirtyRect(12), dirtyRect);
}

void KisInMemoryFrameCacheSwapperContractTest::forgetFrameDropsStoredIdentity()
{
    using Abstract = KisAbstractFrameCacheSwapper;
    using Concrete = KisInMemoryFrameCacheSwapper;


    Concrete concrete;
    Abstract &swapper = concrete;
    swapper.saveFrame(21, makeUpdateInfo(1, QRect(1, 2, 3, 4)), QRect(0, 0, 20, 20));
    QVERIFY(swapper.hasFrame(21));

    swapper.forgetFrame(21);

    QVERIFY(!swapper.hasFrame(21));
}

QTEST_APPLESS_MAIN(KisInMemoryFrameCacheSwapperContractTest)

#include "KisInMemoryFrameCacheSwapperContractTest.moc"
