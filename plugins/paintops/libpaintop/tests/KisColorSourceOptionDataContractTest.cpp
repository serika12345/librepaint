/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisColorSourceOptionData.h"

#include <KoID.h>
#include <kis_properties_configuration.h>

#include <QHash>
#include <QMap>
#include <QTest>

#include <array>
#include <type_traits>

namespace
{
using PropertyStore = QMap<QString, QVariant>;

QHash<const KisPropertiesConfiguration *, PropertyStore> &configurationStores()
{
    static QHash<const KisPropertiesConfiguration *, PropertyStore> stores;
    return stores;
}

PropertyStore &properties(KisPropertiesConfiguration *configuration)
{
    return configurationStores()[configuration];
}

const PropertyStore &properties(const KisPropertiesConfiguration *configuration)
{
    return configurationStores()[configuration];
}

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

KisShared::KisShared()
    : _ref(0)
    , _sharedWeakReference(nullptr)
{
}

KisShared::~KisShared()
{
    delete _sharedWeakReference;
}

KisSerializableConfiguration::KisSerializableConfiguration() = default;

KisSerializableConfiguration::KisSerializableConfiguration(const KisSerializableConfiguration &)
    : KisShared()
{
}

bool KisSerializableConfiguration::fromXML(const QString &, bool)
{
    return false;
}

QString KisSerializableConfiguration::toXML() const
{
    return {};
}

struct KisPropertiesConfiguration::Private {
};

KisPropertiesConfiguration::KisPropertiesConfiguration()
    : d(new Private)
{
    configurationStores().insert(this, {});
}

KisPropertiesConfiguration::~KisPropertiesConfiguration()
{
    configurationStores().remove(this);
    delete d;
}

KisPropertiesConfiguration::KisPropertiesConfiguration(const KisPropertiesConfiguration &rhs)
    : KisSerializableConfiguration(rhs)
    , d(new Private)
{
    configurationStores().insert(this, properties(&rhs));
}

KisPropertiesConfiguration &KisPropertiesConfiguration::operator=(const KisPropertiesConfiguration &rhs)
{
    if (this != &rhs) {
        properties(this) = properties(&rhs);
    }
    return *this;
}

bool KisPropertiesConfiguration::fromXML(const QString &, bool)
{
    return false;
}

void KisPropertiesConfiguration::fromXML(const QDomElement &)
{
}

void KisPropertiesConfiguration::toXML(QDomDocument &, QDomElement &) const
{
}

QString KisPropertiesConfiguration::toXML() const
{
    return {};
}

bool KisPropertiesConfiguration::hasProperty(const QString &name) const
{
    return properties(this).contains(name);
}

void KisPropertiesConfiguration::setProperty(const QString &name, const QVariant &value)
{
    properties(this).insert(name, value);
}

bool KisPropertiesConfiguration::getProperty(const QString &name, QVariant &value) const
{
    const auto item = properties(this).constFind(name);
    if (item == properties(this).constEnd()) {
        return false;
    }
    value = *item;
    return true;
}

QVariant KisPropertiesConfiguration::getProperty(const QString &name) const
{
    return properties(this).value(name);
}

QString KisPropertiesConfiguration::getString(const QString &name, const QString &defaultValue) const
{
    const QVariant value = getProperty(name);
    return value.isValid() ? value.toString() : defaultValue;
}

QMap<QString, QVariant> KisPropertiesConfiguration::getProperties() const
{
    return properties(this);
}

QList<QString> KisPropertiesConfiguration::getPropertiesKeys() const
{
    return properties(this).keys();
}

bool KisPropertiesConfiguration::compareTo(const KisPropertiesConfiguration *rhs) const
{
    return properties(this) == properties(rhs);
}

void KisPropertiesConfiguration::dump() const
{
}

class KisColorSourceOptionDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void enumDefaultsAndMemberTypeRemainStable();
    void stableIdentifiersRoundTripEveryType();
    void readMapsEveryIdentifierAndUnknownFallback();
    void writeMapsEveryTypeAndPreservesOtherProperties();
    void equalityDependsOnlyOnType();
};

void KisColorSourceOptionDataContractTest::enumDefaultsAndMemberTypeRemainStable()
{
    static_assert(std::is_class_v<KisColorSourceOptionData>);
    static_assert(std::is_enum_v<KisColorSourceOptionData::Type>);
    static_assert(std::is_same_v<decltype(KisColorSourceOptionData::type), KisColorSourceOptionData::Type>);

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
