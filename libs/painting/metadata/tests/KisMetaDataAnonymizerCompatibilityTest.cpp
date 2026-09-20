/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_meta_data_entry.h"
#include "kis_meta_data_filter.h"
#include "kis_meta_data_filter_registry.h"
#include "kis_meta_data_schema.h"
#include "kis_meta_data_schema_registry.h"
#include "kis_meta_data_store.h"
#include "kis_meta_data_value.h"

#include <kistest.h>

class KisMetaDataAnonymizerCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void anonymizerIdentifierRemovesPersonalMetadata();
};

void KisMetaDataAnonymizerCompatibilityTest::anonymizerIdentifierRemovesPersonalMetadata()
{
    // Compatibility requirement: libkis Document filters depend on the "Anonymizer" metadata filter identifier.
    // Consumer: document export callers requesting metadata anonymization.
    // Operation: apply the filter resolved from its public identifier to personal Dublin Core and Photoshop entries.
    // Observable result: the named entries are removed from the metadata store.
    // Failure impact: documents exported with anonymization can retain personally identifying metadata.
    const KisMetaData::Filter *const anonymizer = KisMetaData::FilterRegistry::instance()->get(QStringLiteral("Anonymizer"));
    const KisMetaData::Schema *const dcSchema =
        KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::DublinCoreSchemaUri);
    const KisMetaData::Schema *const photoshopSchema =
        KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::PhotoshopSchemaUri);
    QVERIFY(anonymizer);
    QVERIFY(dcSchema);
    QVERIFY(photoshopSchema);

    KisMetaData::Store store;
    const QList<QPair<const KisMetaData::Schema *, QString>> personalEntries{
        {dcSchema, QStringLiteral("contributor")},
        {dcSchema, QStringLiteral("creator")},
        {dcSchema, QStringLiteral("publisher")},
        {dcSchema, QStringLiteral("rights")},
        {photoshopSchema, QStringLiteral("AuthorsPosition")},
        {photoshopSchema, QStringLiteral("CaptionWriter")},
        {photoshopSchema, QStringLiteral("Credit")},
        {photoshopSchema, QStringLiteral("City")},
        {photoshopSchema, QStringLiteral("Country")},
    };
    for (const auto &entry : personalEntries) {
        QVERIFY(store.addEntry(KisMetaData::Entry(entry.first, entry.second, KisMetaData::Value(entry.second))));
    }

    store.applyFilters(QList<const KisMetaData::Filter *>{anonymizer});

    for (const auto &entry : personalEntries) {
        QVERIFY(!store.containsEntry(entry.first, entry.second));
    }
}

KISTEST_MAIN(KisMetaDataAnonymizerCompatibilityTest)

#include "KisMetaDataAnonymizerCompatibilityTest.moc"
