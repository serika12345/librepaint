/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisColorSourceOptionData.h"

#include <KoID.h>
#include <kis_properties_configuration.h>

#include <QTest>

#include <array>

namespace
{
struct TypeMapping {
    KisColorSourceOptionData::Type type;
    const char *id;
};

const std::array<TypeMapping, 6> typeMappings{{
    {KisColorSourceOptionData::PLAIN, "plain"},
    {KisColorSourceOptionData::GRADIENT, "gradient"},
    {KisColorSourceOptionData::UNIFORM_RANDOM, "uniform_random"},
    {KisColorSourceOptionData::TOTAL_RANDOM, "total_random"},
    {KisColorSourceOptionData::PATTERN, "pattern"},
    {KisColorSourceOptionData::PATTERN_LOCKED, "lockedpattern"},
}};
} // namespace

class KisColorSourceOptionDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultColorSource();
    void stableIdentifiersRoundTripEveryType();
    void readMapsEveryIdentifierAndUnknownFallback();
    void writeMapsEveryTypeAndPreservesOtherProperties();
    void equalityDependsOnlyOnType();
};

void KisColorSourceOptionDataContractTest::defaultColorSource()
{
    QCOMPARE(static_cast<int>(KisColorSourceOptionData::PLAIN), 0);
    QCOMPARE(static_cast<int>(KisColorSourceOptionData::GRADIENT), 1);
    QCOMPARE(static_cast<int>(KisColorSourceOptionData::UNIFORM_RANDOM), 2);
    QCOMPARE(static_cast<int>(KisColorSourceOptionData::TOTAL_RANDOM), 3);
    QCOMPARE(static_cast<int>(KisColorSourceOptionData::PATTERN), 4);
    QCOMPARE(static_cast<int>(KisColorSourceOptionData::PATTERN_LOCKED), 5);

    const KisColorSourceOptionData data;
    QCOMPARE(data.type, KisColorSourceOptionData::PLAIN);
}

void KisColorSourceOptionDataContractTest::stableIdentifiersRoundTripEveryType()
{
    const QVector<KoID> identifiers = KisColorSourceOptionData::colorSourceTypeIds();
    QCOMPARE(identifiers.size(), static_cast<qsizetype>(typeMappings.size()));

    for (std::size_t index = 0; index < typeMappings.size(); ++index) {
        const TypeMapping &mapping = typeMappings[index];
        const QString expectedId = QString::fromLatin1(mapping.id);

        QCOMPARE(identifiers[static_cast<qsizetype>(index)].id(), expectedId);
        QVERIFY(!identifiers[static_cast<qsizetype>(index)].name().isEmpty());

        const KoID identifier = KisColorSourceOptionData::type2Id(mapping.type);
        QCOMPARE(identifier.id(), expectedId);
        QCOMPARE(KisColorSourceOptionData::id2Type(KoID(expectedId, QStringLiteral("別名"))), mapping.type);
    }
}

void KisColorSourceOptionDataContractTest::readMapsEveryIdentifierAndUnknownFallback()
{
    KisPropertiesConfiguration missingSetting;
    KisColorSourceOptionData missingData;
    missingData.type = KisColorSourceOptionData::PATTERN_LOCKED;

    QVERIFY(missingData.read(&missingSetting));
    QCOMPARE(missingData.type, KisColorSourceOptionData::PLAIN);
    QVERIFY(missingSetting.getProperties().isEmpty());

    for (const TypeMapping &mapping : typeMappings) {
        KisPropertiesConfiguration setting;
        setting.setProperty(QStringLiteral("ColorSource/Type"), QString::fromLatin1(mapping.id));
        KisColorSourceOptionData data;
        data.type = KisColorSourceOptionData::PATTERN_LOCKED;

        QVERIFY(data.read(&setting));
        QCOMPARE(data.type, mapping.type);
        QCOMPARE(setting.getProperties().size(), 1);
    }

    KisPropertiesConfiguration unknownSetting;
    unknownSetting.setProperty(QStringLiteral("ColorSource/Type"), QStringLiteral("未知/色源"));
    KisColorSourceOptionData unknownData;
    unknownData.type = KisColorSourceOptionData::GRADIENT;

    QVERIFY(unknownData.read(&unknownSetting));
    QCOMPARE(unknownData.type, KisColorSourceOptionData::PLAIN);
    QCOMPARE(unknownSetting.getString(QStringLiteral("ColorSource/Type")), QStringLiteral("未知/色源"));
}

void KisColorSourceOptionDataContractTest::writeMapsEveryTypeAndPreservesOtherProperties()
{
    for (const TypeMapping &mapping : typeMappings) {
        KisPropertiesConfiguration setting;
        setting.setProperty(QStringLiteral("unrelated/保持"), QStringLiteral("残す"));
        KisColorSourceOptionData data;
        data.type = mapping.type;

        data.write(&setting);

        QCOMPARE(setting.getString(QStringLiteral("ColorSource/Type")), QString::fromLatin1(mapping.id));
        QCOMPARE(setting.getString(QStringLiteral("unrelated/保持")), QStringLiteral("残す"));
        QCOMPARE(setting.getProperties().size(), 2);
        QCOMPARE(data.type, mapping.type);
    }
}

void KisColorSourceOptionDataContractTest::equalityDependsOnlyOnType()
{
    for (const TypeMapping &lhsMapping : typeMappings) {
        for (const TypeMapping &rhsMapping : typeMappings) {
            KisColorSourceOptionData lhs;
            lhs.type = lhsMapping.type;
            KisColorSourceOptionData rhs;
            rhs.type = rhsMapping.type;

            QCOMPARE(lhs == rhs, lhsMapping.type == rhsMapping.type);
            QCOMPARE(lhs != rhs, lhsMapping.type != rhsMapping.type);
        }
    }
}

QTEST_GUILESS_MAIN(KisColorSourceOptionDataContractTest)

#include "KisColorSourceOptionDataContractTest.moc"
