/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_meta_data_schema_registry.h"

#include <QTest>

#include <type_traits>

class KisMetaDataSchemaRegistrySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void schemaCreationAndLookupSignaturesRemainStable();
    void singletonSignatureRemainsStable();
};

void KisMetaDataSchemaRegistrySchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Registry = KisMetaData::SchemaRegistry;

    static_assert(std::is_class_v<Registry>);
    static_assert(std::is_default_constructible_v<Registry>);
    static_assert(std::is_destructible_v<Registry>);
}

void KisMetaDataSchemaRegistrySchemaContractTest::schemaCreationAndLookupSignaturesRemainStable()
{
    using Registry = KisMetaData::SchemaRegistry;
    using Schema = KisMetaData::Schema;
    using Lookup = const Schema *(Registry::*)(const QString &) const;

    static_assert(
        std::is_same_v<decltype(&Registry::create), const Schema *(Registry::*)(const QString &, const QString &)>);
    static_assert(std::is_same_v<decltype(&Registry::schemaFromUri), Lookup>);
    static_assert(std::is_same_v<decltype(&Registry::schemaFromPrefix), Lookup>);
}

void KisMetaDataSchemaRegistrySchemaContractTest::singletonSignatureRemainsStable()
{
    using Registry = KisMetaData::SchemaRegistry;

    static_assert(std::is_same_v<decltype(&Registry::instance), Registry *(*)()>);
}

QTEST_GUILESS_MAIN(KisMetaDataSchemaRegistrySchemaContractTest)

#include "KisMetaDataSchemaRegistrySchemaContractTest.moc"
