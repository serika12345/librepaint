/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisColorOptionData.h"

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

struct BoolMapping {
    const QString *key;
    bool KisColorOptionData::*member;
};

const std::array<BoolMapping, 6> boolMappings{{
    {&COLOROP_USE_RANDOM_HSV, &KisColorOptionData::useRandomHSV},
    {&COLOROP_USE_RANDOM_OPACITY, &KisColorOptionData::useRandomOpacity},
    {&COLOROP_SAMPLE_COLOR, &KisColorOptionData::sampleInputColor},
    {&COLOROP_FILL_BG, &KisColorOptionData::fillBackground},
    {&COLOROP_COLOR_PER_PARTICLE, &KisColorOptionData::colorPerParticle},
    {&COLOROP_MIX_BG_COLOR, &KisColorOptionData::mixBgColor},
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

class KisColorOptionDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void persistenceKeysRemainStable();
    void defaultValuesAndMemberTypesRemainStable();
    void readAndWriteMapEveryMemberIndependently();
    void equalityDependsOnEveryMember();
};

void KisColorOptionDataContractTest::persistenceKeysRemainStable()
{
    QCOMPARE(COLOROP_HUE, QStringLiteral("ColorOption/hue"));
    QCOMPARE(COLOROP_SATURATION, QStringLiteral("ColorOption/saturation"));
    QCOMPARE(COLOROP_VALUE, QStringLiteral("ColorOption/value"));
    QCOMPARE(COLOROP_USE_RANDOM_HSV, QStringLiteral("ColorOption/useRandomHSV"));
    QCOMPARE(COLOROP_USE_RANDOM_OPACITY, QStringLiteral("ColorOption/useRandomOpacity"));
    QCOMPARE(COLOROP_SAMPLE_COLOR, QStringLiteral("ColorOption/sampleInputColor"));
    QCOMPARE(COLOROP_FILL_BG, QStringLiteral("ColorOption/fillBackground"));
    QCOMPARE(COLOROP_COLOR_PER_PARTICLE, QStringLiteral("ColorOption/colorPerParticle"));
    QCOMPARE(COLOROP_MIX_BG_COLOR, QStringLiteral("ColorOption/mixBgColor"));
}

void KisColorOptionDataContractTest::defaultValuesAndMemberTypesRemainStable()
{
    static_assert(std::is_class_v<KisColorOptionData>);
    static_assert(std::is_same_v<decltype(KisColorOptionData::useRandomHSV), bool>);
    static_assert(std::is_same_v<decltype(KisColorOptionData::useRandomOpacity), bool>);
    static_assert(std::is_same_v<decltype(KisColorOptionData::sampleInputColor), bool>);
    static_assert(std::is_same_v<decltype(KisColorOptionData::fillBackground), bool>);
    static_assert(std::is_same_v<decltype(KisColorOptionData::colorPerParticle), bool>);
    static_assert(std::is_same_v<decltype(KisColorOptionData::mixBgColor), bool>);
    static_assert(std::is_same_v<decltype(KisColorOptionData::hue), int>);
    static_assert(std::is_same_v<decltype(KisColorOptionData::saturation), int>);
    static_assert(std::is_same_v<decltype(KisColorOptionData::value), int>);

    const KisColorOptionData data;
    QVERIFY(!data.useRandomHSV);
    QVERIFY(!data.useRandomOpacity);
    QVERIFY(!data.sampleInputColor);
    QVERIFY(!data.fillBackground);
    QVERIFY(!data.colorPerParticle);
    QVERIFY(!data.mixBgColor);
    QCOMPARE(data.hue, 0);
    QCOMPARE(data.saturation, 0);
    QCOMPARE(data.value, 0);
}

void KisColorOptionDataContractTest::readAndWriteMapEveryMemberIndependently()
{
    KisPropertiesConfiguration missingSetting;
    KisColorOptionData missingData;

    QVERIFY(missingData.read(&missingSetting));
    QVERIFY(missingData == KisColorOptionData());
    QVERIFY(missingSetting.getProperties().isEmpty());

    KisPropertiesConfiguration integerSetting;
    integerSetting.setProperty(COLOROP_HUE, -37);
    integerSetting.setProperty(COLOROP_SATURATION, 58);
    integerSetting.setProperty(COLOROP_VALUE, 91);
    KisColorOptionData integerData;

    QVERIFY(integerData.read(&integerSetting));
    QCOMPARE(integerData.hue, -37);
    QCOMPARE(integerData.saturation, 58);
    QCOMPARE(integerData.value, 91);

    for (std::size_t activeIndex = 0; activeIndex < boolMappings.size(); ++activeIndex) {
        KisPropertiesConfiguration setting;
        setting.setProperty(*boolMappings[activeIndex].key, true);
        KisColorOptionData data;

        QVERIFY(data.read(&setting));
        for (std::size_t memberIndex = 0; memberIndex < boolMappings.size(); ++memberIndex) {
            QCOMPARE(data.*(boolMappings[memberIndex].member), memberIndex == activeIndex);
        }
    }

    for (std::size_t activeIndex = 0; activeIndex < boolMappings.size(); ++activeIndex) {
        KisPropertiesConfiguration setting;
        setting.setProperty(QStringLiteral("unrelated/保持"), QStringLiteral("残す"));
        KisColorOptionData data;
        data.*(boolMappings[activeIndex].member) = true;
        data.hue = -17;
        data.saturation = 43;
        data.value = 89;

        data.write(&setting);

        for (std::size_t keyIndex = 0; keyIndex < boolMappings.size(); ++keyIndex) {
            QCOMPARE(setting.getBool(*boolMappings[keyIndex].key), keyIndex == activeIndex);
        }
        QCOMPARE(setting.getInt(COLOROP_HUE), -17);
        QCOMPARE(setting.getInt(COLOROP_SATURATION), 43);
        QCOMPARE(setting.getInt(COLOROP_VALUE), 89);
        QCOMPARE(setting.getProperty(QStringLiteral("unrelated/保持")).toString(), QStringLiteral("残す"));
        QCOMPARE(setting.getProperties().size(), 10);
    }
}

void KisColorOptionDataContractTest::equalityDependsOnEveryMember()
{
    KisColorOptionData baseline;
    baseline.useRandomHSV = true;
    baseline.useRandomOpacity = false;
    baseline.sampleInputColor = true;
    baseline.fillBackground = false;
    baseline.colorPerParticle = true;
    baseline.mixBgColor = false;
    baseline.hue = -31;
    baseline.saturation = 47;
    baseline.value = 83;

    const KisColorOptionData same = baseline;
    QVERIFY(baseline == same);
    QVERIFY(!(baseline != same));

    const auto verifyDifference = [&baseline](auto mutate) {
        KisColorOptionData changed = baseline;
        mutate(changed);
        QVERIFY(baseline != changed);
        QVERIFY(!(baseline == changed));
    };

    verifyDifference([](KisColorOptionData &data) {
        data.useRandomHSV = !data.useRandomHSV;
    });
    verifyDifference([](KisColorOptionData &data) {
        data.useRandomOpacity = !data.useRandomOpacity;
    });
    verifyDifference([](KisColorOptionData &data) {
        data.sampleInputColor = !data.sampleInputColor;
    });
    verifyDifference([](KisColorOptionData &data) {
        data.fillBackground = !data.fillBackground;
    });
    verifyDifference([](KisColorOptionData &data) {
        data.colorPerParticle = !data.colorPerParticle;
    });
    verifyDifference([](KisColorOptionData &data) {
        data.mixBgColor = !data.mixBgColor;
    });
    verifyDifference([](KisColorOptionData &data) {
        ++data.hue;
    });
    verifyDifference([](KisColorOptionData &data) {
        ++data.saturation;
    });
    verifyDifference([](KisColorOptionData &data) {
        ++data.value;
    });
}

QTEST_GUILESS_MAIN(KisColorOptionDataContractTest)

#include "KisColorOptionDataContractTest.moc"
