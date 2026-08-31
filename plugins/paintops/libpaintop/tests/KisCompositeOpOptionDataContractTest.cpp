/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisCompositeOpOptionData.h"

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

class KisCompositeOpOptionDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultValuesAndMemberTypesRemainStable();
    void readMapsMissingAndUnicodeConfiguration();
    void writeMapsBothMembersAndPreservesOtherProperties();
    void equalityDependsOnBothMembersIndependently();
};

void KisCompositeOpOptionDataContractTest::defaultValuesAndMemberTypesRemainStable()
{
    static_assert(std::is_class_v<KisCompositeOpOptionData>);
    static_assert(std::is_same_v<decltype(KisCompositeOpOptionData::compositeOpId), QString>);
    static_assert(std::is_same_v<decltype(KisCompositeOpOptionData::eraserMode), bool>);

    const KisCompositeOpOptionData data;
    QCOMPARE(data.compositeOpId, QStringLiteral("normal"));
    QVERIFY(!data.eraserMode);
}

void KisCompositeOpOptionDataContractTest::readMapsMissingAndUnicodeConfiguration()
{
    KisPropertiesConfiguration missingSetting;
    missingSetting.setProperty(QStringLiteral("unrelated/保持"), QStringLiteral("既存値"));
    KisCompositeOpOptionData missingData;

    QVERIFY(missingData.read(&missingSetting));
    QCOMPARE(missingData.compositeOpId, QStringLiteral("normal"));
    QVERIFY(!missingData.eraserMode);
    QCOMPARE(missingSetting.getString(QStringLiteral("unrelated/保持")), QStringLiteral("既存値"));
    QCOMPARE(missingSetting.getProperties().size(), 1);

    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("CompositeOp"), QStringLiteral("合成/🌿"));
    setting.setProperty(QStringLiteral("EraserMode"), true);
    setting.setProperty(QStringLiteral("unrelated/保持"), 73);
    KisCompositeOpOptionData data;

    QVERIFY(data.read(&setting));
    QCOMPARE(data.compositeOpId, QStringLiteral("合成/🌿"));
    QVERIFY(data.eraserMode);
    QCOMPARE(setting.getProperties().size(), 3);
}

void KisCompositeOpOptionDataContractTest::writeMapsBothMembersAndPreservesOtherProperties()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("unrelated/保持"), QStringLiteral("残す"));

    KisCompositeOpOptionData data;
    data.compositeOpId = QStringLiteral("描画/γ");
    data.eraserMode = true;
    data.write(&setting);

    QCOMPARE(setting.getString(QStringLiteral("CompositeOp")), QStringLiteral("描画/γ"));
    QVERIFY(setting.getBool(QStringLiteral("EraserMode")));
    QCOMPARE(setting.getString(QStringLiteral("unrelated/保持")), QStringLiteral("残す"));
    QCOMPARE(setting.getProperties().size(), 3);
    QCOMPARE(data.compositeOpId, QStringLiteral("描画/γ"));
    QVERIFY(data.eraserMode);
}

void KisCompositeOpOptionDataContractTest::equalityDependsOnBothMembersIndependently()
{
    KisCompositeOpOptionData baseline;
    baseline.compositeOpId = QStringLiteral("合成/α");
    baseline.eraserMode = false;

    KisCompositeOpOptionData peer = baseline;
    QVERIFY(baseline == peer);
    QVERIFY(!(baseline != peer));

    peer.compositeOpId = QStringLiteral("合成/β");
    QVERIFY(!(baseline == peer));
    QVERIFY(baseline != peer);

    peer = baseline;
    peer.eraserMode = true;
    QVERIFY(!(baseline == peer));
    QVERIFY(baseline != peer);
}

QTEST_GUILESS_MAIN(KisCompositeOpOptionDataContractTest)

#include "KisCompositeOpOptionDataContractTest.moc"
