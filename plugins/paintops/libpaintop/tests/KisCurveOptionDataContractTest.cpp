/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisCurveOptionData.h"
#include "KisStandardOptionData.h"

#include <kis_properties_configuration.h>

#include <QHash>
#include <QMap>
#include <QTest>

#include <type_traits>

namespace
{
using PropertyStore = QMap<QString, QVariant>;
QHash<const KisPropertiesConfiguration *, PropertyStore> &stores()
{
    static QHash<const KisPropertiesConfiguration *, PropertyStore> result;
    return result;
}

PropertyStore &properties(KisPropertiesConfiguration *value)
{
    return stores()[value];
}
const PropertyStore &properties(const KisPropertiesConfiguration *value)
{
    return stores()[value];
}

template<typename Data>
void verifyStandardCurve(const Data &data, const QString &id)
{
    QCOMPARE(data.id.id(), id);
    QVERIFY(!data.id.name().isEmpty());
    QVERIFY(data.prefix.isEmpty());
    QVERIFY(data.isCheckable);
    QVERIFY(!data.isChecked);
    QCOMPARE(data.strengthMinValue, 0.0);
    QCOMPARE(data.strengthMaxValue, 1.0);
}

template<typename Data>
void verifyPrefixedStandardCurve(const QString &id)
{
    const Data data(QStringLiteral("前置/"));
    QCOMPARE(data.id.id(), id);
    QVERIFY(!data.id.name().isEmpty());
    QCOMPARE(data.prefix, QStringLiteral("前置/"));
    QVERIFY(data.isCheckable);
    QVERIFY(!data.isChecked);
    QCOMPARE(data.strengthMinValue, 0.0);
    QCOMPARE(data.strengthMaxValue, 1.0);
}
} // namespace

void kis_assert_exception(const char *, const char *, int)
{
}
void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

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
    stores().insert(this, {});
}
KisPropertiesConfiguration::~KisPropertiesConfiguration()
{
    stores().remove(this);
    delete d;
}
KisPropertiesConfiguration::KisPropertiesConfiguration(const KisPropertiesConfiguration &rhs)
    : KisSerializableConfiguration(rhs)
    , d(new Private)
{
    stores().insert(this, properties(&rhs));
}
KisPropertiesConfiguration &KisPropertiesConfiguration::operator=(const KisPropertiesConfiguration &rhs)
{
    if (this != &rhs)
        properties(this) = properties(&rhs);
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
    const auto it = properties(this).constFind(name);
    if (it == properties(this).constEnd())
        return false;
    value = *it;
    return true;
}
QVariant KisPropertiesConfiguration::getProperty(const QString &name) const
{
    return properties(this).value(name);
}
int KisPropertiesConfiguration::getInt(const QString &name, int value) const
{
    const QVariant v = getProperty(name);
    return v.isValid() ? v.toInt() : value;
}
double KisPropertiesConfiguration::getDouble(const QString &name, double value) const
{
    const QVariant v = getProperty(name);
    return v.isValid() ? v.toDouble() : value;
}
bool KisPropertiesConfiguration::getBool(const QString &name, bool value) const
{
    const QVariant v = getProperty(name);
    return v.isValid() ? v.toBool() : value;
}
QString KisPropertiesConfiguration::getString(const QString &name, const QString &value) const
{
    const QVariant v = getProperty(name);
    return v.isValid() ? v.toString() : value;
}
QMap<QString, QVariant> KisPropertiesConfiguration::getProperties() const
{
    return properties(this);
}
QList<QString> KisPropertiesConfiguration::getPropertiesKeys() const
{
    return properties(this).keys();
}
void KisPropertiesConfiguration::getPrefixedProperties(const QString &prefix,
                                                       KisPropertiesConfiguration *configuration) const
{
    for (auto it = properties(this).constBegin(); it != properties(this).constEnd(); ++it) {
        if (it.key().startsWith(prefix))
            configuration->setProperty(it.key().mid(prefix.size()), it.value());
    }
}
void KisPropertiesConfiguration::setPrefixedProperties(const QString &prefix,
                                                       const KisPropertiesConfiguration *configuration)
{
    for (auto it = properties(configuration).constBegin(); it != properties(configuration).constEnd(); ++it) {
        setProperty(prefix + it.key(), it.value());
    }
}
QString KisPropertiesConfiguration::extractedPrefixKey()
{
    return QStringLiteral("__extractedFromPrefix");
}
bool KisPropertiesConfiguration::compareTo(const KisPropertiesConfiguration *rhs) const
{
    return properties(this) == properties(rhs);
}
void KisPropertiesConfiguration::dump() const
{
}

class KisCurveOptionDataContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void publicTypeAndDefaultConstructorValues();
    void prefixedCheckabilityFollowsAllThreeModes();
    void explicitCheckedStateOverridesDeduction();
    void valueRangeIsForwardedByBothConstructors();
    void sensorViewsExposeTheOwnedPackState();
    void opacityAndFlowRemainFixedCurves();
    void ratioAndRotationForwardPrefixes();
    void softnessKeepsItsRestrictedRange();
    void colorAdjustmentTypesKeepIndependentIds();
    void rateAndStrengthTypesKeepIndependentIds();
};

void KisCurveOptionDataContractTest::publicTypeAndDefaultConstructorValues()
{
    static_assert(std::is_same_v<KisCurveOptionData::Checkability, KisKritaSensorPack::Checkability>);
    const KisCurveOptionData data(KoID(QStringLiteral("id"), QStringLiteral("name")));
    QVERIFY(data.prefix.isEmpty());
    QVERIFY(data.isCheckable);
    QVERIFY(!data.isChecked);
}

void KisCurveOptionDataContractTest::prefixedCheckabilityFollowsAllThreeModes()
{
    const KoID id(QStringLiteral("curve"), QStringLiteral("Curve"));
    const KisCurveOptionData fixed(QStringLiteral("前置/"), id, KisCurveOptionData::Checkability::NotCheckable);
    const KisCurveOptionData checkable(QStringLiteral("前置/"), id, KisCurveOptionData::Checkability::Checkable);
    const KisCurveOptionData conditionalWithoutPrefix(QString(),
                                                      id,
                                                      KisCurveOptionData::Checkability::CheckableIfHasPrefix);
    const KisCurveOptionData conditionalWithPrefix(QStringLiteral("前置/"),
                                                   id,
                                                   KisCurveOptionData::Checkability::CheckableIfHasPrefix);
    QVERIFY(!fixed.isCheckable);
    QVERIFY(fixed.isChecked);
    QVERIFY(checkable.isCheckable);
    QVERIFY(!checkable.isChecked);
    QVERIFY(!conditionalWithoutPrefix.isCheckable);
    QVERIFY(!conditionalWithoutPrefix.isChecked);
    QVERIFY(conditionalWithPrefix.isCheckable);
    QVERIFY(!conditionalWithPrefix.isChecked);
}

void KisCurveOptionDataContractTest::explicitCheckedStateOverridesDeduction()
{
    const KoID id(QStringLiteral("curve"), QStringLiteral("Curve"));
    const KisCurveOptionData forcedOn(id, KisCurveOptionData::Checkability::Checkable, true);
    const KisCurveOptionData forcedOff(id, KisCurveOptionData::Checkability::NotCheckable, false);
    QVERIFY(forcedOn.isChecked);
    QVERIFY(!forcedOff.isChecked);
}

void KisCurveOptionDataContractTest::valueRangeIsForwardedByBothConstructors()
{
    const KoID id(QStringLiteral("curve"), QStringLiteral("Curve"));
    const KisCurveOptionData plain(id, KisCurveOptionData::Checkability::Checkable, std::nullopt, {-17.5, 42.25});
    const KisCurveOptionData prefixed(QStringLiteral("値/"),
                                      id,
                                      KisCurveOptionData::Checkability::Checkable,
                                      std::nullopt,
                                      {3.5, 9.75});
    QCOMPARE(plain.strengthMinValue, -17.5);
    QCOMPARE(plain.strengthMaxValue, 42.25);
    QCOMPARE(prefixed.strengthMinValue, 3.5);
    QCOMPARE(prefixed.strengthMaxValue, 9.75);
}

void KisCurveOptionDataContractTest::sensorViewsExposeTheOwnedPackState()
{
    KisCurveOptionData data(KoID(QStringLiteral("curve"), QStringLiteral("Curve")));
    KisKritaSensorData &mutableView = data.sensorStruct();
    mutableView.sensorPressure.isActive = true;
    const KisCurveOptionData &constData = data;
    const KisKritaSensorData &constView = constData.sensorStruct();
    QCOMPARE(&mutableView, &constView);
    QVERIFY(constView.sensorPressure.isActive);
    QCOMPARE(data.sensors().size(), size_t(16));
}

void KisCurveOptionDataContractTest::opacityAndFlowRemainFixedCurves()
{
    static_assert(!std::is_same_v<KisOpacityOptionData, KisFlowOptionData>);

    const KisOpacityOptionData opacity(QStringLiteral("前置/"));
    QCOMPARE(opacity.id.id(), QStringLiteral("Opacity"));
    QVERIFY(!opacity.id.name().isEmpty());
    QCOMPARE(opacity.prefix, QStringLiteral("前置/"));
    QVERIFY(!opacity.isCheckable);
    QVERIFY(opacity.isChecked);
    QCOMPARE(opacity.strengthMinValue, 0.0);
    QCOMPARE(opacity.strengthMaxValue, 1.0);

    const KisFlowOptionData flow(QStringLiteral("流量/"));
    QCOMPARE(flow.id.id(), QStringLiteral("Flow"));
    QVERIFY(!flow.id.name().isEmpty());
    QCOMPARE(flow.prefix, QStringLiteral("流量/"));
    QVERIFY(!flow.isCheckable);
    QVERIFY(flow.isChecked);
    QCOMPARE(flow.strengthMinValue, 0.0);
    QCOMPARE(flow.strengthMaxValue, 1.0);
}

void KisCurveOptionDataContractTest::ratioAndRotationForwardPrefixes()
{
    static_assert(!std::is_same_v<KisRatioOptionData, KisRotationOptionData>);
    verifyPrefixedStandardCurve<KisRatioOptionData>(QStringLiteral("Ratio"));
    verifyPrefixedStandardCurve<KisRotationOptionData>(QStringLiteral("Rotation"));
}

void KisCurveOptionDataContractTest::softnessKeepsItsRestrictedRange()
{
    const KisSoftnessOptionData softness;
    QCOMPARE(softness.id.id(), QStringLiteral("Softness"));
    QVERIFY(!softness.id.name().isEmpty());
    QVERIFY(softness.prefix.isEmpty());
    QVERIFY(softness.isCheckable);
    QVERIFY(!softness.isChecked);
    QCOMPARE(softness.strengthMinValue, 0.1);
    QCOMPARE(softness.strengthMaxValue, 1.0);
}

void KisCurveOptionDataContractTest::colorAdjustmentTypesKeepIndependentIds()
{
    static_assert(!std::is_same_v<KisDarkenOptionData, KisMixOptionData>);
    static_assert(!std::is_same_v<KisMixOptionData, KisHueOptionData>);
    static_assert(!std::is_same_v<KisHueOptionData, KisSaturationOptionData>);
    static_assert(!std::is_same_v<KisSaturationOptionData, KisValueOptionData>);

    verifyStandardCurve(KisDarkenOptionData(), QStringLiteral("Darken"));
    verifyStandardCurve(KisMixOptionData(), QStringLiteral("Mix"));
    verifyStandardCurve(KisHueOptionData(), QStringLiteral("h"));
    verifyStandardCurve(KisSaturationOptionData(), QStringLiteral("s"));
    verifyStandardCurve(KisValueOptionData(), QStringLiteral("v"));
}

void KisCurveOptionDataContractTest::rateAndStrengthTypesKeepIndependentIds()
{
    static_assert(!std::is_same_v<KisRateOptionData, KisStrengthOptionData>);
    static_assert(!std::is_same_v<KisStrengthOptionData, KisLightnessStrengthOptionData>);

    verifyStandardCurve(KisRateOptionData(), QStringLiteral("Rate"));
    verifyStandardCurve(KisStrengthOptionData(), QStringLiteral("Texture/Strength/"));
    verifyStandardCurve(KisLightnessStrengthOptionData(), QStringLiteral("LightnessStrength"));
}

QTEST_MAIN(KisCurveOptionDataContractTest)
#include "KisCurveOptionDataContractTest.moc"
