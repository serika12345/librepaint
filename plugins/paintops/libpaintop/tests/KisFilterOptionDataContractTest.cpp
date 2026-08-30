/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisFilterOptionData.h"

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

bool KisPropertiesConfiguration::getBool(const QString &name, bool defaultValue) const
{
    const QVariant value = getProperty(name);
    return value.isValid() ? value.toBool() : defaultValue;
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

class KisFilterOptionDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultValuesAndTagsRemainStable();
    void readMapsUnicodeConfigurationState();
    void writeMapsAllMembersAndPreservesOtherProperties();
    void equalityDependsOnEveryMember();
};

void KisFilterOptionDataContractTest::defaultValuesAndTagsRemainStable()
{
    static_assert(std::is_class_v<KisFilterOptionData>);
    static_assert(std::is_same_v<decltype(KisFilterOptionData::filterId), QString>);
    static_assert(std::is_same_v<decltype(KisFilterOptionData::filterConfig), QString>);
    static_assert(std::is_same_v<decltype(KisFilterOptionData::smudgeMode), bool>);

    const KisFilterOptionData data;
    QVERIFY(data.filterId.isEmpty());
    QVERIFY(data.filterConfig.isEmpty());
    QVERIFY(!data.smudgeMode);
    QCOMPARE(KisFilterOptionData::filterIdTag(), QStringLiteral("Filter/id"));
    QCOMPARE(KisFilterOptionData::filterConfigTag(), QStringLiteral("Filter/configuration"));
}

void KisFilterOptionDataContractTest::readMapsUnicodeConfigurationState()
{
    KisPropertiesConfiguration missingSetting;
    KisFilterOptionData missingData;

    QVERIFY(missingData.read(&missingSetting));
    QVERIFY(missingData.filterId.isEmpty());
    QVERIFY(missingData.filterConfig.isEmpty());
    QVERIFY(!missingData.smudgeMode);
    QVERIFY(missingSetting.getProperties().isEmpty());

    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("Filter/id"), QStringLiteral("ぼかし/β"));
    setting.setProperty(QStringLiteral("Filter/configuration"), QStringLiteral("<設定 mode=\"β\">値</設定>"));
    setting.setProperty(QStringLiteral("Filter/smudgeMode"), true);
    KisFilterOptionData data;

    QVERIFY(data.read(&setting));
    QCOMPARE(data.filterId, QStringLiteral("ぼかし/β"));
    QCOMPARE(data.filterConfig, QStringLiteral("<設定 mode=\"β\">値</設定>"));
    QVERIFY(data.smudgeMode);
    QCOMPARE(setting.getProperties().size(), 3);
}

void KisFilterOptionDataContractTest::writeMapsAllMembersAndPreservesOtherProperties()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("unrelated/保持"), QStringLiteral("残す"));

    KisFilterOptionData data;
    data.filterId = QStringLiteral("輪郭/γ");
    data.filterConfig = QStringLiteral("<設定 strength=\"42\"/>");
    data.smudgeMode = true;
    data.write(&setting);

    QCOMPARE(setting.getString(QStringLiteral("Filter/id")), QStringLiteral("輪郭/γ"));
    QCOMPARE(setting.getString(QStringLiteral("Filter/configuration")), QStringLiteral("<設定 strength=\"42\"/>"));
    QVERIFY(setting.getBool(QStringLiteral("Filter/smudgeMode")));
    QCOMPARE(setting.getString(QStringLiteral("unrelated/保持")), QStringLiteral("残す"));
    QCOMPARE(setting.getProperties().size(), 4);

    QCOMPARE(data.filterId, QStringLiteral("輪郭/γ"));
    QCOMPARE(data.filterConfig, QStringLiteral("<設定 strength=\"42\"/>"));
    QVERIFY(data.smudgeMode);
}

void KisFilterOptionDataContractTest::equalityDependsOnEveryMember()
{
    KisFilterOptionData baseline;
    baseline.filterId = QStringLiteral("filter/α");
    baseline.filterConfig = QStringLiteral("<config>値</config>");
    baseline.smudgeMode = false;

    KisFilterOptionData same = baseline;
    QVERIFY(baseline == same);
    QVERIFY(!(baseline != same));

    same.filterId = QStringLiteral("filter/β");
    QVERIFY(baseline != same);

    same = baseline;
    same.filterConfig = QStringLiteral("<config>別</config>");
    QVERIFY(baseline != same);

    same = baseline;
    same.smudgeMode = true;
    QVERIFY(baseline != same);
}

QTEST_GUILESS_MAIN(KisFilterOptionDataContractTest)

#include "KisFilterOptionDataContractTest.moc"
