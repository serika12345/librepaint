/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisAirbrushOptionData.h"

#include <kis_paintop_settings.h>
#include <kis_properties_configuration.h>

#include <QHash>
#include <QMap>
#include <QTest>

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

double KisPropertiesConfiguration::getDouble(const QString &name, double defaultValue) const
{
    const QVariant value = getProperty(name);
    return value.isValid() ? value.toDouble() : defaultValue;
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

class KisAirbrushOptionDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsAndEqualityReflectEveryValue();
    void writePersistsOnlyAirbrushProperties();
    void readRestoresEveryPersistedProperty();
    void missingPropertiesUseReadDefaults();
};

void KisAirbrushOptionDataContractTest::defaultsAndEqualityReflectEveryValue()
{
    const KisAirbrushOptionData defaults;
    QVERIFY(!defaults.isChecked);
    QCOMPARE(defaults.airbrushRate, 50.0);
    QVERIFY(!defaults.ignoreSpacing);

    KisAirbrushOptionData same;
    QCOMPARE(defaults, same);

    same.isChecked = true;
    QVERIFY(defaults != same);
    same = defaults;
    same.airbrushRate = 75.0;
    QVERIFY(defaults != same);
    same = defaults;
    same.ignoreSpacing = true;
    QVERIFY(defaults != same);
}

void KisAirbrushOptionDataContractTest::writePersistsOnlyAirbrushProperties()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("unrelated"), 91);

    KisAirbrushOptionData data;
    data.isChecked = true;
    data.airbrushRate = 37.5;
    data.ignoreSpacing = true;
    data.write(&setting);

    QCOMPARE(properties(&setting).size(), 4);
    QCOMPARE(setting.getBool(AIRBRUSH_ENABLED), true);
    QCOMPARE(setting.getDouble(AIRBRUSH_RATE), 37.5);
    QCOMPARE(setting.getBool(AIRBRUSH_IGNORE_SPACING), true);
    QCOMPARE(setting.getProperty(QStringLiteral("unrelated")).toInt(), 91);
}

void KisAirbrushOptionDataContractTest::readRestoresEveryPersistedProperty()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(AIRBRUSH_ENABLED, true);
    setting.setProperty(AIRBRUSH_RATE, 12.25);
    setting.setProperty(AIRBRUSH_IGNORE_SPACING, true);

    KisAirbrushOptionData data;
    QVERIFY(data.read(&setting));
    QVERIFY(data.isChecked);
    QCOMPARE(data.airbrushRate, 12.25);
    QVERIFY(data.ignoreSpacing);
    QCOMPARE(properties(&setting).size(), 3);
}

void KisAirbrushOptionDataContractTest::missingPropertiesUseReadDefaults()
{
    KisPropertiesConfiguration setting;
    KisAirbrushOptionData data;
    data.isChecked = true;
    data.airbrushRate = 88.0;
    data.ignoreSpacing = true;

    QVERIFY(data.read(&setting));
    QVERIFY(!data.isChecked);
    QCOMPARE(data.airbrushRate, 20.0);
    QVERIFY(!data.ignoreSpacing);
    QVERIFY(properties(&setting).isEmpty());
}

QTEST_GUILESS_MAIN(KisAirbrushOptionDataContractTest)

#include "KisAirbrushOptionDataContractTest.moc"
