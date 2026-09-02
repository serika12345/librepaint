/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tiles3/kis_memento_manager.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_MEMENTO_MANAGER_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisMementoManager::method)), signature>)

} // namespace

class KisMementoManagerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mementoManagerAliasAndOwnerSchemaRemainsStable();
    void mementoHistoryItemValueSchemaRemainsStable();
    void mementoManagerLifetimeAndCurrentRevisionSignaturesRemainStable();
    void mementoManagerTileRegistrationAndCommitSignaturesRemainStable();
    void mementoManagerHistoryTraversalAndMaintenanceSignaturesRemainStable();
};

void KisMementoManagerSchemaContractTest::mementoManagerAliasAndOwnerSchemaRemainsStable()
{
    static_assert(std::is_same_v<KisMementoItemList, QList<KisMementoItemSP>>);
    static_assert(std::is_same_v<KisMementoItemListIterator, QListIterator<KisMementoItemSP>>);
    static_assert(std::is_same_v<KisHistoryList, QList<KisHistoryItem>>);
    static_assert(std::is_same_v<KisMementoSP, KisSharedPtr<KisMemento>>);
#ifdef USE_LOCK_FREE_HASH_TABLE
    static_assert(std::is_same_v<KisMementoItemHashTable, KisTileHashTableTraits2<KisMementoItem>>);
    static_assert(std::is_same_v<KisMementoItemHashTableIterator, KisTileHashTableIteratorTraits2<KisMementoItem>>);
    static_assert(
        std::is_same_v<KisMementoItemHashTableIteratorConst, KisTileHashTableIteratorTraits2<KisMementoItem>>);
#else
    static_assert(std::is_same_v<KisMementoItemHashTable, KisTileHashTableTraits<KisMementoItem>>);
    static_assert(
        std::is_same_v<KisMementoItemHashTableIterator, KisTileHashTableIteratorTraits<KisMementoItem, QWriteLocker>>);
    static_assert(std::is_same_v<KisMementoItemHashTableIteratorConst,
                                 KisTileHashTableIteratorTraits<KisMementoItem, QReadLocker>>);
#endif
    static_assert(std::is_class_v<KisMementoManager>);
}

void KisMementoManagerSchemaContractTest::mementoHistoryItemValueSchemaRemainsStable()
{
    static_assert(std::is_aggregate_v<KisHistoryItem>);
    static_assert(std::is_same_v<decltype(KisHistoryItem::memento), KisMemento *>);
    static_assert(std::is_same_v<decltype(KisHistoryItem::itemList), KisMementoItemList>);
}

void KisMementoManagerSchemaContractTest::mementoManagerLifetimeAndCurrentRevisionSignaturesRemainStable()
{
    static_assert(std::is_default_constructible_v<KisMementoManager>);
    static_assert(std::is_copy_constructible_v<KisMementoManager>);
    static_assert(std::is_destructible_v<KisMementoManager>);
    ASSERT_MEMENTO_MANAGER_SIGNATURE(getMemento, KisMementoSP (KisMementoManager::*)());
    ASSERT_MEMENTO_MANAGER_SIGNATURE(hasCurrentMemento, bool (KisMementoManager::*)());
    ASSERT_MEMENTO_MANAGER_SIGNATURE(currentMemento, KisMementoSP (KisMementoManager::*)());
}

void KisMementoManagerSchemaContractTest::mementoManagerTileRegistrationAndCommitSignaturesRemainStable()
{
    ASSERT_MEMENTO_MANAGER_SIGNATURE(registerTileChange, void (KisMementoManager::*)(KisTile *));
    ASSERT_MEMENTO_MANAGER_SIGNATURE(registerTileDeleted, void (KisMementoManager::*)(KisTile *));
    ASSERT_MEMENTO_MANAGER_SIGNATURE(commit, void (KisMementoManager::*)());
    ASSERT_MEMENTO_MANAGER_SIGNATURE(getCommittedTile, KisTileSP (KisMementoManager::*)(qint32, qint32, bool &));
    ASSERT_MEMENTO_MANAGER_SIGNATURE(setDefaultTileData, void (KisMementoManager::*)(KisTileData *));
}

void KisMementoManagerSchemaContractTest::mementoManagerHistoryTraversalAndMaintenanceSignaturesRemainStable()
{
    ASSERT_MEMENTO_MANAGER_SIGNATURE(rollback, void (KisMementoManager::*)(KisTileHashTable *, KisMementoSP));
    ASSERT_MEMENTO_MANAGER_SIGNATURE(rollforward, void (KisMementoManager::*)(KisTileHashTable *, KisMementoSP));
    ASSERT_MEMENTO_MANAGER_SIGNATURE(purgeHistory, void (KisMementoManager::*)(KisMementoSP));
    ASSERT_MEMENTO_MANAGER_SIGNATURE(debugPrintInfo, void (KisMementoManager::*)());
}

#undef ASSERT_MEMENTO_MANAGER_SIGNATURE

QTEST_GUILESS_MAIN(KisMementoManagerSchemaContractTest)

#include "KisMementoManagerSchemaContractTest.moc"
