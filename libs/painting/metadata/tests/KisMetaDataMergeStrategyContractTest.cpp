/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_meta_data_entry.h"
#include "kis_meta_data_merge_strategy.h"
#include "kis_meta_data_merge_strategy_registry.h"
#include "kis_meta_data_schema.h"
#include "kis_meta_data_schema_registry.h"
#include "kis_meta_data_store.h"
#include "kis_meta_data_value.h"

#include <QVariant>

#include <kistest.h>

class KisMetaDataMergeStrategyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void layerMergeStrategiesResolveConflictingMetadata();
    void smartLayerMergeWeightsValuesAndCombinesCreators();

private:
    const KisMetaData::Schema *m_xmpSchema {nullptr};
    const KisMetaData::Schema *m_dublinCoreSchema {nullptr};
};

void KisMetaDataMergeStrategyContractTest::initTestCase()
{
    m_xmpSchema = KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::XMPSchemaUri);
    m_dublinCoreSchema = KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::DublinCoreSchemaUri);
    QVERIFY(m_xmpSchema);
    QVERIFY(m_dublinCoreSchema);
}

void KisMetaDataMergeStrategyContractTest::layerMergeStrategiesResolveConflictingMetadata()
{
    // Consumer: people merging paint layers in the Layers docker.
    // Operation: merge two layers using the drop, priority-to-first, and only-identical metadata strategies.
    // Observable result: the merged layer respectively keeps no metadata, prefers the bottom layer, or retains only shared values.
    // Failure impact: merging layers discards wanted metadata or retains conflicting metadata without the user's selected rule.
    KisMetaData::Store bottomLayer;
    KisMetaData::Store topLayer;
    QVERIFY(bottomLayer.addEntry({m_xmpSchema, QStringLiteral("Label"), KisMetaData::Value(QStringLiteral("bottom"))}));
    QVERIFY(bottomLayer.addEntry({m_xmpSchema, QStringLiteral("Nickname"), KisMetaData::Value(QStringLiteral("bottom-only"))}));
    QVERIFY(topLayer.addEntry({m_xmpSchema, QStringLiteral("Label"), KisMetaData::Value(QStringLiteral("top"))}));
    QVERIFY(topLayer.addEntry({m_xmpSchema, QStringLiteral("Nickname"), KisMetaData::Value(QStringLiteral("top-only"))}));

    const QList<const KisMetaData::Store *> sources{&bottomLayer, &topLayer};
    const QList<double> scores{0.5, 0.5};
    KisMetaData::MergeStrategyRegistry *const strategies = KisMetaData::MergeStrategyRegistry::instance();

    KisMetaData::Store dropped;
    const KisMetaData::MergeStrategy *const drop = strategies->get(QStringLiteral("Drop"));
    QVERIFY(drop);
    drop->merge(&dropped, sources, scores);
    QVERIFY(dropped.isEmpty());

    KisMetaData::Store priorityToFirst;
    const KisMetaData::MergeStrategy *const priority = strategies->get(QStringLiteral("PriorityToFirst"));
    QVERIFY(priority);
    priority->merge(&priorityToFirst, sources, scores);
    QCOMPARE(priorityToFirst.getValue(m_xmpSchema->uri(), QStringLiteral("Label")).asVariant(), QVariant(QStringLiteral("bottom")));
    QCOMPARE(priorityToFirst.getValue(m_xmpSchema->uri(), QStringLiteral("Nickname")).asVariant(),
             QVariant(QStringLiteral("bottom-only")));

    topLayer.getEntry(m_xmpSchema, QStringLiteral("Label")).value() = KisMetaData::Value(QStringLiteral("bottom"));
    KisMetaData::Store onlyIdentical;
    const KisMetaData::MergeStrategy *const identical = strategies->get(QStringLiteral("OnlyIdentical"));
    QVERIFY(identical);
    identical->merge(&onlyIdentical, sources, scores);
    QVERIFY(onlyIdentical.containsEntry(m_xmpSchema, QStringLiteral("Label")));
    QVERIFY(!onlyIdentical.containsEntry(m_xmpSchema, QStringLiteral("Nickname")));
}

void KisMetaDataMergeStrategyContractTest::smartLayerMergeWeightsValuesAndCombinesCreators()
{
    // Consumer: people merging paint layers with the Smart metadata strategy.
    // Operation: merge two layers with conflicting labels and ratings and different creator lists.
    // Observable result: the higher-weight label wins, the rating is weighted, and both creator lists remain available.
    // Failure impact: a merged document presents the wrong metadata rating, loses authors, or chooses the wrong layer label.
    KisMetaData::Store backgroundLayer;
    KisMetaData::Store foregroundLayer;
    QVERIFY(backgroundLayer.addEntry({m_xmpSchema, QStringLiteral("Label"), KisMetaData::Value(QStringLiteral("background"))}));
    QVERIFY(backgroundLayer.addEntry({m_xmpSchema, QStringLiteral("Rating"), KisMetaData::Value(2)}));
    QVERIFY(backgroundLayer.addEntry(
        {m_dublinCoreSchema,
         QStringLiteral("creator"),
         KisMetaData::Value(QList<KisMetaData::Value>{KisMetaData::Value(QStringLiteral("Ada"))},
                            KisMetaData::Value::OrderedArray)}));
    QVERIFY(foregroundLayer.addEntry({m_xmpSchema, QStringLiteral("Label"), KisMetaData::Value(QStringLiteral("foreground"))}));
    QVERIFY(foregroundLayer.addEntry({m_xmpSchema, QStringLiteral("Rating"), KisMetaData::Value(4)}));
    QVERIFY(foregroundLayer.addEntry(
        {m_dublinCoreSchema,
         QStringLiteral("creator"),
         KisMetaData::Value(QList<KisMetaData::Value>{KisMetaData::Value(QStringLiteral("Bea"))},
                            KisMetaData::Value::OrderedArray)}));

    KisMetaData::Store merged;
    const KisMetaData::MergeStrategy *const smart = KisMetaData::MergeStrategyRegistry::instance()->get(QStringLiteral("Smart"));
    QVERIFY(smart);
    smart->merge(&merged, {&backgroundLayer, &foregroundLayer}, {0.25, 0.75});

    QCOMPARE(merged.getValue(m_xmpSchema->uri(), QStringLiteral("Label")).asVariant(), QVariant(QStringLiteral("foreground")));
    QCOMPARE(merged.getValue(m_xmpSchema->uri(), QStringLiteral("Rating")).asVariant(), QVariant(3));
    const QList<KisMetaData::Value> creators = merged.getValue(m_dublinCoreSchema->uri(), QStringLiteral("creator")).asArray();
    QCOMPARE(creators.size(), 2);
    QVERIFY(creators.contains(KisMetaData::Value(QStringLiteral("Ada"))));
    QVERIFY(creators.contains(KisMetaData::Value(QStringLiteral("Bea"))));
}

KISTEST_MAIN(KisMetaDataMergeStrategyContractTest)

#include "KisMetaDataMergeStrategyContractTest.moc"
