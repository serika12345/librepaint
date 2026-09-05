/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisSafeNodeProjectionStore.h"

#include <QTest>

#include <type_traits>

class KisSafeNodeProjectionStoreSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseTypeAndLifetimeSchemaRemainStable();
    void baseControlSchemaRemainStable();
    void paintDeviceStoreSchemaRemainStable();
    void selectionStoreTypeAndLifetimeSchemaRemainStable();
    void selectionStoreCopyAndDeviceSchemaRemainStable();
};

void KisSafeNodeProjectionStoreSchemaContractTest::baseTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_same_v<KisSafeNodeProjectionStoreBaseSP, KisSharedPtr<KisSafeNodeProjectionStoreBase>>);
    static_assert(std::is_class_v<KisSafeNodeProjectionStoreBase>);
    static_assert(std::is_base_of_v<QObject, KisSafeNodeProjectionStoreBase>);
    static_assert(std::is_copy_constructible_v<KisSafeNodeProjectionStoreBase>);
    static_assert(std::has_virtual_destructor_v<KisSafeNodeProjectionStoreBase>);
}

void KisSafeNodeProjectionStoreSchemaContractTest::baseControlSchemaRemainStable()
{
    using Base = KisSafeNodeProjectionStoreBase;

    static_assert(std::is_same_v<decltype(static_cast<void (Base::*)()>(&Base::internalInitiateProjectionsCleanup)),
                                 void (Base::*)()>);
    static_assert(std::is_same_v<decltype(static_cast<void (Base::*)()>(&Base::releaseDevice)), void (Base::*)()>);
    static_assert(std::is_same_v<decltype(static_cast<void (Base::*)(KisImageWSP)>(&Base::setImage)),
                                 void (Base::*)(KisImageWSP)>);
}

void KisSafeNodeProjectionStoreSchemaContractTest::paintDeviceStoreSchemaRemainStable()
{
    using Store = KisSafeNodeProjectionStore;

    static_assert(std::is_same_v<KisSafeNodeProjectionStoreSP, KisSharedPtr<Store>>);
    static_assert(std::is_same_v<KisSafeNodeProjectionStoreWSP, KisWeakSharedPtr<Store>>);
    static_assert(std::is_class_v<Store>);
    static_assert(std::is_base_of_v<KisSafeNodeProjectionStoreBase, Store>);
    static_assert(std::is_default_constructible_v<Store>);
    static_assert(std::is_copy_constructible_v<Store>);
    static_assert(
        std::is_same_v<decltype(static_cast<KisPaintDeviceSP (Store::*)(KisPaintDeviceSP)>(&Store::getDeviceLazy)),
                       KisPaintDeviceSP (Store::*)(KisPaintDeviceSP)>);
}

void KisSafeNodeProjectionStoreSchemaContractTest::selectionStoreTypeAndLifetimeSchemaRemainStable()
{
    using Store = KisSafeSelectionNodeProjectionStore;

    static_assert(std::is_same_v<KisSafeSelectionNodeProjectionStoreSP, KisSharedPtr<Store>>);
    static_assert(std::is_same_v<KisSafeSelectionNodeProjectionStoreWSP, KisWeakSharedPtr<Store>>);
    static_assert(std::is_class_v<Store>);
    static_assert(std::is_base_of_v<KisSafeNodeProjectionStoreBase, Store>);
    static_assert(std::is_default_constructible_v<Store>);
}

void KisSafeNodeProjectionStoreSchemaContractTest::selectionStoreCopyAndDeviceSchemaRemainStable()
{
    using Store = KisSafeSelectionNodeProjectionStore;

    static_assert(std::is_copy_constructible_v<Store>);
    static_assert(
        std::is_same_v<decltype(static_cast<KisSelectionSP (Store::*)(KisSelectionSP)>(&Store::getDeviceLazy)),
                       KisSelectionSP (Store::*)(KisSelectionSP)>);
}

QTEST_APPLESS_MAIN(KisSafeNodeProjectionStoreSchemaContractTest)

#include "KisSafeNodeProjectionStoreSchemaContractTest.moc"
