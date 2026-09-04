/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <animation/kis_frame_cache_store.h>

#include <QTest>

#include <type_traits>

class KisFrameCacheStoreSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void frameCacheStoreTypeLifetimeAndConstructionSchemaRemainStable();
    void frameCacheStoreFrameValueSemanticsSchemaRemainStable();
    void frameCacheStoreFrameDataSchemaRemainStable();
    void frameCacheStorePersistenceSignaturesRemainStable();
    void frameCacheStoreMutationAndQuerySignaturesRemainStable();
};

void KisFrameCacheStoreSchemaContractTest::frameCacheStoreTypeLifetimeAndConstructionSchemaRemainStable()
{
    using Store = KisFrameCacheStore;
    using Frame = Store::Frame;

    static_assert(std::is_class_v<Store>);
    static_assert(std::is_class_v<Frame>);
    static_assert(std::is_default_constructible_v<Store>);
    static_assert(std::is_constructible_v<Store, const QString &>);
    static_assert(std::is_destructible_v<Store>);
}

void KisFrameCacheStoreSchemaContractTest::frameCacheStoreFrameValueSemanticsSchemaRemainStable()
{
    using Frame = KisFrameCacheStore::Frame;

    static_assert(std::is_default_constructible_v<Frame>);
    static_assert(std::is_move_constructible_v<Frame>);
    static_assert(!std::is_copy_constructible_v<Frame>);
    static_assert(std::is_same_v<decltype(&Frame::isValid), bool (Frame::*)() const>);
    static_assert(std::is_move_assignable_v<Frame>);
    static_assert(!std::is_copy_assignable_v<Frame>);
}

void KisFrameCacheStoreSchemaContractTest::frameCacheStoreFrameDataSchemaRemainStable()
{
    using Frame = KisFrameCacheStore::Frame;

    static_assert(std::is_same_v<decltype(&Frame::data), KisFrameDataSerializer::Frame Frame::*>);
    static_assert(std::is_same_v<decltype(&Frame::dirtyImageRect), QRect Frame::*>);
    static_assert(std::is_same_v<decltype(&Frame::imageBounds), QRect Frame::*>);
    static_assert(std::is_same_v<decltype(&Frame::levelOfDetail), int Frame::*>);
}

void KisFrameCacheStoreSchemaContractTest::frameCacheStorePersistenceSignaturesRemainStable()
{
    using Store = KisFrameCacheStore;
    using Frame = Store::Frame;

    static_assert(std::is_same_v<decltype(&Store::loadFrame), Frame (Store::*)(int, KisTileDataPoolSP)>);
    static_assert(std::is_same_v<decltype(&Store::saveFrame), void (Store::*)(int, Frame)>);
}

void KisFrameCacheStoreSchemaContractTest::frameCacheStoreMutationAndQuerySignaturesRemainStable()
{
    using Store = KisFrameCacheStore;

    static_assert(std::is_same_v<decltype(&Store::forgetFrame), void (Store::*)(int)>);
    static_assert(std::is_same_v<decltype(&Store::frameDirtyRect), QRect (Store::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Store::frameLevelOfDetail), int (Store::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Store::hasFrame), bool (Store::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Store::moveFrame), void (Store::*)(int, int)>);
}

QTEST_APPLESS_MAIN(KisFrameCacheStoreSchemaContractTest)

#include "KisFrameCacheStoreSchemaContractTest.moc"
