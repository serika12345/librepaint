/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_meta_data_backend_registry.h"
#include "kis_meta_data_filter_registry.h"
#include "kis_meta_data_merge_strategy_registry.h"

#include <QTest>

#include <type_traits>

class KisMetaDataRegistrySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void filterRegistryTypeAndLifetimeSchemaRemainStable();
    void filterRegistrySingletonSignatureRemainsStable();
    void mergeStrategyRegistryTypeAndLifetimeSchemaRemainStable();
    void mergeStrategyRegistrySingletonSignatureRemainsStable();
    void backendRegistrySignaturesRemainStable();
};

void KisMetaDataRegistrySchemaContractTest::filterRegistryTypeAndLifetimeSchemaRemainStable()
{
    using Registry = KisMetaData::FilterRegistry;

    static_assert(std::is_class_v<Registry>);
    static_assert(std::is_default_constructible_v<Registry>);
    static_assert(std::is_destructible_v<Registry>);
}

void KisMetaDataRegistrySchemaContractTest::filterRegistrySingletonSignatureRemainsStable()
{
    using Registry = KisMetaData::FilterRegistry;

    static_assert(std::is_same_v<decltype(&Registry::instance), Registry *(*)()>);
}

void KisMetaDataRegistrySchemaContractTest::mergeStrategyRegistryTypeAndLifetimeSchemaRemainStable()
{
    using Registry = KisMetaData::MergeStrategyRegistry;

    static_assert(std::is_class_v<Registry>);
    static_assert(std::is_default_constructible_v<Registry>);
    static_assert(std::is_destructible_v<Registry>);
}

void KisMetaDataRegistrySchemaContractTest::mergeStrategyRegistrySingletonSignatureRemainsStable()
{
    using Registry = KisMetaData::MergeStrategyRegistry;

    static_assert(std::is_same_v<decltype(&Registry::instance), Registry *(*)()>);
}

void KisMetaDataRegistrySchemaContractTest::backendRegistrySignaturesRemainStable()
{
    using Registry = KisMetadataBackendRegistry;
    using Init = void (Registry::*)();

    static_assert(std::is_class_v<Registry>);
    static_assert(std::is_default_constructible_v<Registry>);
    static_assert(std::is_destructible_v<Registry>);
    static_assert(std::is_same_v<decltype(&Registry::init), Init>);
    static_assert(std::is_same_v<decltype(&Registry::instance), Registry *(*)()>);
}

QTEST_GUILESS_MAIN(KisMetaDataRegistrySchemaContractTest)

#include "KisMetaDataRegistrySchemaContractTest.moc"
