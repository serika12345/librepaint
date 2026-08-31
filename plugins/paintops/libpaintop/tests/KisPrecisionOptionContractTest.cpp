/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_precision_option.h"

#include <kis_properties_configuration.h>

#include <QHash>
#include <QMap>
#include <QTest>

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

int KisPropertiesConfiguration::getInt(const QString &name, int defaultValue) const
{
    const QVariant value = getProperty(name);
    return value.isValid() ? value.toInt() : defaultValue;
}

bool KisPropertiesConfiguration::getBool(const QString &name, bool defaultValue) const
{
    const QVariant value = getProperty(name);
    return value.isValid() ? value.toBool() : defaultValue;
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

class KisPrecisionOptionContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void persistenceKeysAndDefaultDataRemainStable();
    void precisionDataReadsWritesAndComparesIndependently();
    void optionConstructionAndMutatorsPreserveIndependentState();
    void effectivePrecisionUsesAutoModeAndExactThreshold();
};

void KisPrecisionOptionContractTest::persistenceKeysAndDefaultDataRemainStable()
{
    static_assert(std::is_class_v<KisPrecisionOption>);
    static_assert(std::is_class_v<KisBrushModel::PrecisionData>);
    static_assert(std::is_same_v<decltype(KisBrushModel::PrecisionData::precisionLevel), int>);
    static_assert(std::is_same_v<decltype(KisBrushModel::PrecisionData::useAutoPrecision), bool>);

    QCOMPARE(PRECISION_LEVEL, QStringLiteral("KisPrecisionOption/precisionLevel"));
    QCOMPARE(AUTO_PRECISION_ENABLED, QStringLiteral("KisPrecisionOption/AutoPrecisionEnabled"));
    QCOMPARE(STARTING_SIZE, QStringLiteral("KisPrecisionOption/SizeToStartFrom"));
    QCOMPARE(DELTA_VALUE, QStringLiteral("KisPrecisionOption/DeltaValue"));

    const KisBrushModel::PrecisionData data;
    QCOMPARE(data.precisionLevel, 5);
    QVERIFY(!data.useAutoPrecision);
}

void KisPrecisionOptionContractTest::precisionDataReadsWritesAndComparesIndependently()
{
    KisPropertiesConfiguration missingSetting;
    const KisBrushModel::PrecisionData missingData = KisBrushModel::PrecisionData::read(&missingSetting);

    QCOMPARE(missingData.precisionLevel, 5);
    QVERIFY(!missingData.useAutoPrecision);
    QVERIFY(missingSetting.getProperties().isEmpty());

    KisPropertiesConfiguration populatedSetting;
    populatedSetting.setProperty(PRECISION_LEVEL, -7);
    populatedSetting.setProperty(AUTO_PRECISION_ENABLED, true);
    populatedSetting.setProperty(QString::fromUtf8("保持/備考"), QString::fromUtf8("精度・設定"));

    const KisBrushModel::PrecisionData populatedData = KisBrushModel::PrecisionData::read(&populatedSetting);
    QCOMPARE(populatedData.precisionLevel, -7);
    QVERIFY(populatedData.useAutoPrecision);
    QCOMPARE(populatedSetting.getProperties().size(), 3);

    KisPropertiesConfiguration writtenSetting;
    writtenSetting.setProperty(QString::fromUtf8("保持/備考"), QString::fromUtf8("既存値"));
    populatedData.write(&writtenSetting);

    QCOMPARE(writtenSetting.getInt(PRECISION_LEVEL), -7);
    QVERIFY(writtenSetting.getBool(AUTO_PRECISION_ENABLED));
    QCOMPARE(writtenSetting.getProperty(QString::fromUtf8("保持/備考")).toString(), QString::fromUtf8("既存値"));
    QCOMPARE(writtenSetting.getProperties().size(), 3);

    KisBrushModel::PrecisionData peer = populatedData;
    QVERIFY(populatedData == peer);

    peer.precisionLevel = 11;
    QVERIFY(!(populatedData == peer));

    peer = populatedData;
    peer.useAutoPrecision = false;
    QVERIFY(!(populatedData == peer));
}

void KisPrecisionOptionContractTest::optionConstructionAndMutatorsPreserveIndependentState()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(PRECISION_LEVEL, 9);
    setting.setProperty(AUTO_PRECISION_ENABLED, true);

    KisPrecisionOption option(&setting);
    QCOMPARE(option.precisionLevel(), 9);
    QVERIFY(option.autoPrecisionEnabled());
    QVERIFY(!option.hasImprecisePositionOptions());

    option.setPrecisionLevel(-4);
    QCOMPARE(option.precisionLevel(), -4);
    QVERIFY(option.autoPrecisionEnabled());
    QVERIFY(!option.hasImprecisePositionOptions());

    option.setAutoPrecisionEnabled(0);
    QVERIFY(!option.autoPrecisionEnabled());
    QCOMPARE(option.precisionLevel(), -4);
    QVERIFY(!option.hasImprecisePositionOptions());

    option.setAutoPrecisionEnabled(-7);
    QVERIFY(option.autoPrecisionEnabled());
    QCOMPARE(option.precisionLevel(), -4);

    option.setHasImprecisePositionOptions(true);
    QVERIFY(option.hasImprecisePositionOptions());
    QCOMPARE(option.precisionLevel(), -4);
    QVERIFY(option.autoPrecisionEnabled());

    option.setHasImprecisePositionOptions(false);
    QVERIFY(!option.hasImprecisePositionOptions());
    QCOMPARE(option.precisionLevel(), -4);
    QVERIFY(option.autoPrecisionEnabled());
}

void KisPrecisionOptionContractTest::effectivePrecisionUsesAutoModeAndExactThreshold()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(PRECISION_LEVEL, 8);
    setting.setProperty(AUTO_PRECISION_ENABLED, false);

    KisPrecisionOption option(&setting);
    QCOMPARE(option.effectivePrecisionLevel(0.0), 8);
    QCOMPARE(option.effectivePrecisionLevel(29.999), 8);
    QCOMPARE(option.effectivePrecisionLevel(30.0), 8);
    QCOMPARE(option.effectivePrecisionLevel(300.0), 8);

    option.setAutoPrecisionEnabled(1);
    QCOMPARE(option.effectivePrecisionLevel(29.999), 5);
    QCOMPARE(option.effectivePrecisionLevel(30.0), 5);
    QCOMPARE(option.effectivePrecisionLevel(30.001), 5);

    option.setHasImprecisePositionOptions(true);
    QCOMPARE(option.effectivePrecisionLevel(29.999), 5);
    QCOMPARE(option.effectivePrecisionLevel(30.0), 3);
    QCOMPARE(option.effectivePrecisionLevel(30.001), 3);

    option.setAutoPrecisionEnabled(0);
    QCOMPARE(option.effectivePrecisionLevel(30.0), 8);
}

QTEST_GUILESS_MAIN(KisPrecisionOptionContractTest)

#include "KisPrecisionOptionContractTest.moc"
