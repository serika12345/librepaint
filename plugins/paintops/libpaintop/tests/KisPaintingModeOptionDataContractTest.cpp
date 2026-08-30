/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPaintingModeOptionData.h"

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

class KisPaintingModeOptionDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultValuesAndEnumeratorsRemainStable();
    void readMapsConfigurationState();
    void writeMapsModesAndPreservesOtherProperties();
    void equalityDependsOnlyOnPaintingMode();
};

void KisPaintingModeOptionDataContractTest::defaultValuesAndEnumeratorsRemainStable()
{
    static_assert(std::is_class_v<KisPaintingModeOptionData>);
    static_assert(std::is_same_v<decltype(KisPaintingModeOptionData::paintingMode), enumPaintingMode>);
    static_assert(std::is_same_v<decltype(KisPaintingModeOptionData::hasPaintingModeProperty), bool>);

    QCOMPARE(static_cast<int>(enumPaintingMode::BUILDUP), 0);
    QCOMPARE(static_cast<int>(enumPaintingMode::WASH), 1);

    const KisPaintingModeOptionData data;
    QCOMPARE(data.paintingMode, enumPaintingMode::BUILDUP);
    QVERIFY(!data.hasPaintingModeProperty);
}

void KisPaintingModeOptionDataContractTest::readMapsConfigurationState()
{
    KisPropertiesConfiguration missingSetting;
    KisPaintingModeOptionData missingData;

    QVERIFY(missingData.read(&missingSetting));
    QCOMPARE(missingData.paintingMode, enumPaintingMode::WASH);
    QVERIFY(!missingData.hasPaintingModeProperty);
    QVERIFY(missingSetting.getProperties().isEmpty());

    KisPropertiesConfiguration buildUpSetting;
    buildUpSetting.setProperty(QStringLiteral("PaintOpAction"), 1);
    KisPaintingModeOptionData buildUpData;

    QVERIFY(buildUpData.read(&buildUpSetting));
    QCOMPARE(buildUpData.paintingMode, enumPaintingMode::BUILDUP);
    QVERIFY(buildUpData.hasPaintingModeProperty);

    for (const int persistedValue : {0, 2, 17, -1}) {
        KisPropertiesConfiguration washSetting;
        washSetting.setProperty(QStringLiteral("PaintOpAction"), persistedValue);
        KisPaintingModeOptionData washData;

        QVERIFY(washData.read(&washSetting));
        QCOMPARE(washData.paintingMode, enumPaintingMode::WASH);
        QVERIFY(washData.hasPaintingModeProperty);
        QCOMPARE(washSetting.getInt(QStringLiteral("PaintOpAction")), persistedValue);
    }
}

void KisPaintingModeOptionDataContractTest::writeMapsModesAndPreservesOtherProperties()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("unrelated/保持"), 73);

    KisPaintingModeOptionData data;
    data.hasPaintingModeProperty = true;
    data.paintingMode = enumPaintingMode::BUILDUP;
    data.write(&setting);

    QCOMPARE(setting.getInt(QStringLiteral("PaintOpAction")), 1);
    QCOMPARE(setting.getInt(QStringLiteral("unrelated/保持")), 73);
    QVERIFY(data.hasPaintingModeProperty);

    data.hasPaintingModeProperty = false;
    data.paintingMode = enumPaintingMode::WASH;
    data.write(&setting);

    QCOMPARE(setting.getInt(QStringLiteral("PaintOpAction")), 2);
    QCOMPARE(setting.getInt(QStringLiteral("unrelated/保持")), 73);
    QVERIFY(!data.hasPaintingModeProperty);
    QCOMPARE(setting.getProperties().size(), 2);
}

void KisPaintingModeOptionDataContractTest::equalityDependsOnlyOnPaintingMode()
{
    KisPaintingModeOptionData lhs;
    KisPaintingModeOptionData rhs;
    rhs.hasPaintingModeProperty = true;

    QVERIFY(lhs == rhs);
    QVERIFY(!(lhs != rhs));

    rhs.paintingMode = enumPaintingMode::WASH;
    QVERIFY(!(lhs == rhs));
    QVERIFY(lhs != rhs);

    lhs.paintingMode = enumPaintingMode::WASH;
    lhs.hasPaintingModeProperty = false;
    QVERIFY(lhs == rhs);
}

QTEST_GUILESS_MAIN(KisPaintingModeOptionDataContractTest)

#include "KisPaintingModeOptionDataContractTest.moc"
