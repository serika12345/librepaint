/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisKritaSensorPack.h>

#include <KisCurveOptionDataCommon.h>
#include <kis_properties_configuration.h>

#include <QDomDocument>
#include <QHash>
#include <QMap>
#include <QTest>

#include <array>
#include <memory>
#include <type_traits>

namespace
{
using PropertyStore = QMap<QString, QVariant>;

int safeAssertCount = 0;

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

std::array<KisSensorData *, 16> declaredMembers(KisKritaSensorData &data)
{
    return {&data.sensorPressure,
            &data.sensorPressureIn,
            &data.sensorXTilt,
            &data.sensorYTilt,
            &data.sensorTiltDirection,
            &data.sensorTiltElevation,
            &data.sensorSpeed,
            &data.sensorDrawingAngle,
            &data.sensorRotation,
            &data.sensorDistance,
            &data.sensorTime,
            &data.sensorFuzzyPerDab,
            &data.sensorFuzzyPerStroke,
            &data.sensorFade,
            &data.sensorPerspective,
            &data.sensorTangentialPressure};
}

std::array<KisSensorData *, 16> packOrder(KisKritaSensorData &data)
{
    return {&data.sensorPressure,
            &data.sensorPressureIn,
            &data.sensorTangentialPressure,
            &data.sensorDrawingAngle,
            &data.sensorXTilt,
            &data.sensorYTilt,
            &data.sensorTiltDirection,
            &data.sensorTiltElevation,
            &data.sensorRotation,
            &data.sensorFuzzyPerDab,
            &data.sensorFuzzyPerStroke,
            &data.sensorSpeed,
            &data.sensorFade,
            &data.sensorDistance,
            &data.sensorTime,
            &data.sensorPerspective};
}

KisCurveOptionDataCommon optionData(KisKritaSensorPack::Checkability checkability, const QString &prefix = QString())
{
    return KisCurveOptionDataCommon(prefix,
                                    KoID(QStringLiteral("Size"), QStringLiteral("Size")),
                                    true,
                                    false,
                                    0.25,
                                    0.75,
                                    new KisKritaSensorPack(checkability));
}

KisKritaSensorPack *sensorPack(KisCurveOptionDataCommon &data)
{
    return static_cast<KisKritaSensorPack *>(data.sensorData.data());
}

class ForeignSensorPack final : public KisSensorPackInterface
{
public:
    KisSensorPackInterface *clone() const override
    {
        return new ForeignSensorPack;
    }

    std::vector<const KisSensorData *> constSensors() const override
    {
        return {};
    }

    std::vector<KisSensorData *> sensors() override
    {
        return {};
    }

    bool compare(const KisSensorPackInterface *) const override
    {
        return false;
    }

    bool read(KisCurveOptionDataCommon &, const KisPropertiesConfiguration *) const override
    {
        return false;
    }

    void write(const KisCurveOptionDataCommon &, KisPropertiesConfiguration *) const override
    {
    }
};

} // namespace

void kis_assert_exception(const char *, const char *, int)
{
}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    ++safeAssertCount;
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

KisCurveOptionDataCommon::KisCurveOptionDataCommon(const QString &_prefix,
                                                   const KoID &_id,
                                                   bool _isCheckable,
                                                   bool _isChecked,
                                                   qreal _minValue,
                                                   qreal _maxValue,
                                                   KisSensorPackInterface *sensorInterface)
    : id(_id)
    , prefix(_prefix)
    , isCheckable(_isCheckable)
    , strengthMinValue(_minValue)
    , strengthMaxValue(_maxValue)
    , isChecked(_isChecked)
    , strengthValue(_maxValue)
    , sensorData(sensorInterface)
{
}

std::vector<const KisSensorData *> KisCurveOptionDataCommon::sensors() const
{
    return sensorData->constSensors();
}

std::vector<KisSensorData *> KisCurveOptionDataCommon::sensors()
{
    return sensorData->sensors();
}

class KisKritaSensorPackContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsEnumeratorsAndMemberIdentity();
    void equalityCoversEverySensorMember();
    void sensorViewsPreserveOrderAndPointers();
    void copyCloneAndCompareRemainIndependent();
    void activeLengthUsesOnlyLengthSensors();
    void readMapsCheckabilityAndSensorConfiguration();
    void writePreservesConfigurationAndSensorOrder();
};

void KisKritaSensorPackContractTest::defaultsEnumeratorsAndMemberIdentity()
{
    static_assert(std::is_class_v<KisKritaSensorPack>);
    static_assert(std::is_class_v<KisKritaSensorData>);

    QCOMPARE(static_cast<int>(KisKritaSensorPack::Checkability::NotCheckable), 0);
    QCOMPARE(static_cast<int>(KisKritaSensorPack::Checkability::Checkable), 1);
    QCOMPARE(static_cast<int>(KisKritaSensorPack::Checkability::CheckableIfHasPrefix), 2);

    KisKritaSensorData data;
    const std::array<QString, 16> expectedIds = {QStringLiteral("pressure"),
                                                 QStringLiteral("pressurein"),
                                                 QStringLiteral("xtilt"),
                                                 QStringLiteral("ytilt"),
                                                 QStringLiteral("ascension"),
                                                 QStringLiteral("declination"),
                                                 QStringLiteral("speed"),
                                                 QStringLiteral("drawingangle"),
                                                 QStringLiteral("rotation"),
                                                 QStringLiteral("distance"),
                                                 QStringLiteral("time"),
                                                 QStringLiteral("fuzzy"),
                                                 QStringLiteral("fuzzystroke"),
                                                 QStringLiteral("fade"),
                                                 QStringLiteral("perspective"),
                                                 QStringLiteral("tangentialpressure")};
    const auto members = declaredMembers(data);

    for (std::size_t i = 0; i < members.size(); ++i) {
        QCOMPARE(members.at(i)->id.id(), expectedIds.at(i));
        QCOMPARE(members.at(i)->curve, QStringLiteral("0,0;1,1;"));
        QCOMPARE(members.at(i)->isActive, i == 0);
    }

    QCOMPARE(data.sensorFade.length, 1000);
    QCOMPARE(data.sensorDistance.length, 30);
    QCOMPARE(data.sensorTime.length, 30);
    QVERIFY(!data.sensorFade.isPeriodic);
    QVERIFY(!data.sensorDistance.isPeriodic);
    QVERIFY(!data.sensorTime.isPeriodic);

    QDomDocument document;
    QDomElement element = document.createElement(QStringLiteral("sensor"));
    document.appendChild(element);
    data.sensorTime.write(document, element);
    QCOMPARE(element.attribute(QStringLiteral("duration")).toInt(), 30);
    QVERIFY(!element.hasAttribute(QStringLiteral("length")));
}

void KisKritaSensorPackContractTest::equalityCoversEverySensorMember()
{
    const KisKritaSensorData baseline;
    KisKritaSensorData changed;

    QVERIFY(baseline == changed);
    QVERIFY(!(baseline != changed));

    for (std::size_t i = 0; i < declaredMembers(changed).size(); ++i) {
        changed = baseline;
        KisSensorData *member = declaredMembers(changed).at(i);
        member->isActive = !member->isActive;
        QVERIFY2(baseline != changed, qPrintable(QStringLiteral("member %1 was not compared").arg(i)));
    }
}

void KisKritaSensorPackContractTest::sensorViewsPreserveOrderAndPointers()
{
    KisKritaSensorPack pack(KisKritaSensorPack::Checkability::NotCheckable);
    KisKritaSensorData &data = pack.sensorsStruct();
    const auto expected = packOrder(data);

    const std::vector<KisSensorData *> mutableSensors = pack.sensors();
    QCOMPARE(mutableSensors.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        QCOMPARE(mutableSensors.at(i), expected.at(i));
    }

    const KisKritaSensorPack &constPack = pack;
    const KisKritaSensorData &constData = constPack.constSensorsStruct();
    QCOMPARE(&constData, &data);

    const std::vector<const KisSensorData *> constSensors = constPack.constSensors();
    QCOMPARE(constSensors.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        QCOMPARE(constSensors.at(i), expected.at(i));
    }

    mutableSensors.at(2)->curve = QStringLiteral("0,0;0.5,0.73;1,1;");
    QCOMPARE(data.sensorTangentialPressure.curve, QStringLiteral("0,0;0.5,0.73;1,1;"));
}

void KisKritaSensorPackContractTest::copyCloneAndCompareRemainIndependent()
{
    safeAssertCount = 0;

    KisKritaSensorPack source(KisKritaSensorPack::Checkability::Checkable);
    source.sensorsStruct().sensorFade.length = 137;

    KisKritaSensorPack copy(source);
    QVERIFY(source.compare(&copy));
    copy.sensorsStruct().sensorFade.length = 211;
    QCOMPARE(source.sensorsStruct().sensorFade.length, 137);
    QVERIFY(!source.compare(&copy));

    std::unique_ptr<KisSensorPackInterface> clone(source.clone());
    QVERIFY(clone.get() != &source);
    auto *typedClone = dynamic_cast<KisKritaSensorPack *>(clone.get());
    QVERIFY(typedClone);
    QVERIFY(source.compare(typedClone));
    typedClone->sensorsStruct().sensorDistance.length = 83;
    QCOMPARE(source.sensorsStruct().sensorDistance.length, 30);
    QVERIFY(!source.compare(typedClone));

    KisKritaSensorPack differentCheckability(KisKritaSensorPack::Checkability::CheckableIfHasPrefix);
    QVERIFY(!source.compare(&differentCheckability));
    QCOMPARE(safeAssertCount, 1);

    ForeignSensorPack foreign;
    QVERIFY(!source.compare(&foreign));
    QCOMPARE(safeAssertCount, 2);
}

void KisKritaSensorPackContractTest::activeLengthUsesOnlyLengthSensors()
{
    KisKritaSensorPack pack(KisKritaSensorPack::Checkability::Checkable);
    pack.sensorsStruct().sensorFade.length = 137;
    pack.sensorsStruct().sensorDistance.length = 251;
    pack.sensorsStruct().sensorTime.length = 367;

    QCOMPARE(pack.calcActiveSensorLength(QStringLiteral("fade")), 137);
    QCOMPARE(pack.calcActiveSensorLength(QStringLiteral("distance")), 251);
    QCOMPARE(pack.calcActiveSensorLength(QStringLiteral("time")), 367);
    QCOMPARE(pack.calcActiveSensorLength(QStringLiteral("pressure")), -1);
    QCOMPARE(pack.calcActiveSensorLength(QStringLiteral("未知/Δ")), -1);
}

void KisKritaSensorPackContractTest::readMapsCheckabilityAndSensorConfiguration()
{
    KisPropertiesConfiguration emptySetting;
    KisCurveOptionDataCommon notCheckable = optionData(KisKritaSensorPack::Checkability::NotCheckable);
    QVERIFY(sensorPack(notCheckable)->read(notCheckable, &emptySetting));
    QVERIFY(!notCheckable.isCheckable);
    QVERIFY(notCheckable.isChecked);
    QCOMPARE(notCheckable.strengthValue, 0.75);
    QVERIFY(notCheckable.useCurve);
    QVERIFY(notCheckable.useSameCurve);
    QCOMPARE(notCheckable.curveMode, 0);
    QCOMPARE(notCheckable.commonCurve, QStringLiteral("0,0;1,1;"));
    QVERIFY(sensorPack(notCheckable)->sensorsStruct().sensorPressure.isActive);

    KisPropertiesConfiguration singleSetting;
    singleSetting.setProperty(QStringLiteral("PressureSize"), true);
    singleSetting.setProperty(
        QStringLiteral("SizeSensor"),
        QStringLiteral("<sensor id=\"fade\" length=\"137\" periodic=\"1\"><curve>0,0;0.5,0.8;1,1;</curve></sensor>"));
    singleSetting.setProperty(QStringLiteral("SizeUseCurve"), false);
    singleSetting.setProperty(QStringLiteral("SizeUseSameCurve"), false);
    singleSetting.setProperty(QStringLiteral("SizeValue"), 0.61);
    singleSetting.setProperty(QStringLiteral("SizecurveMode"), 3);
    KisCurveOptionDataCommon checkable = optionData(KisKritaSensorPack::Checkability::Checkable);

    QVERIFY(sensorPack(checkable)->read(checkable, &singleSetting));
    QVERIFY(checkable.isCheckable);
    QVERIFY(checkable.isChecked);
    QVERIFY(!checkable.useCurve);
    QVERIFY(!checkable.useSameCurve);
    QCOMPARE(checkable.strengthValue, 0.61);
    QCOMPARE(checkable.curveMode, 3);
    QCOMPARE(checkable.commonCurve, QStringLiteral("0,0;0.5,0.8;1,1;"));
    const KisKritaSensorData &singleSensors = sensorPack(checkable)->constSensorsStruct();
    QVERIFY(singleSensors.sensorFade.isActive);
    QCOMPARE(singleSensors.sensorFade.length, 137);
    QVERIFY(singleSensors.sensorFade.isPeriodic);
    QVERIFY(!singleSensors.sensorPressure.isActive);

    KisCurveOptionDataCommon directPrefix =
        optionData(KisKritaSensorPack::Checkability::CheckableIfHasPrefix, QStringLiteral("Mask/"));
    QVERIFY(sensorPack(directPrefix)->read(directPrefix, &emptySetting));
    QVERIFY(directPrefix.isCheckable);

    KisPropertiesConfiguration extractedSetting;
    extractedSetting.setProperty(KisPropertiesConfiguration::extractedPrefixKey(), QStringLiteral("抽出/β"));
    KisCurveOptionDataCommon extractedPrefix = optionData(KisKritaSensorPack::Checkability::CheckableIfHasPrefix);
    QVERIFY(sensorPack(extractedPrefix)->read(extractedPrefix, &extractedSetting));
    QVERIFY(extractedPrefix.isCheckable);

    KisCurveOptionDataCommon noPrefix = optionData(KisKritaSensorPack::Checkability::CheckableIfHasPrefix);
    QVERIFY(sensorPack(noPrefix)->read(noPrefix, &emptySetting));
    QVERIFY(!noPrefix.isCheckable);
    QVERIFY(noPrefix.isChecked);

    KisPropertiesConfiguration multipleSetting;
    multipleSetting.setProperty(
        QStringLiteral("SizeSensor"),
        QStringLiteral("<params id=\"sensorslist\"><ChildSensor "
                       "id=\"pressure\"><curve>0,0;0.4,0.6;1,1;</curve></ChildSensor><ChildSensor id=\"time\" "
                       "duration=\"311\" periodic=\"1\"><curve>0,0;0.7,0.9;1,1;</curve></ChildSensor></params>"));
    KisCurveOptionDataCommon multiple = optionData(KisKritaSensorPack::Checkability::Checkable);
    QVERIFY(sensorPack(multiple)->read(multiple, &multipleSetting));
    const KisKritaSensorData &multipleSensors = sensorPack(multiple)->constSensorsStruct();
    QVERIFY(multipleSensors.sensorPressure.isActive);
    QVERIFY(multipleSensors.sensorTime.isActive);
    QCOMPARE(multipleSensors.sensorTime.length, 311);
    QVERIFY(multipleSensors.sensorTime.isPeriodic);
    QCOMPARE(multiple.commonCurve, QStringLiteral("0,0;0.7,0.9;1,1;"));
}

void KisKritaSensorPackContractTest::writePreservesConfigurationAndSensorOrder()
{
    KisCurveOptionDataCommon data = optionData(KisKritaSensorPack::Checkability::Checkable);
    KisKritaSensorPack *pack = sensorPack(data);
    data.isCheckable = true;
    data.isChecked = false;
    data.useCurve = false;
    data.useSameCurve = false;
    data.strengthValue = 0.625;
    data.curveMode = 4;
    data.commonCurve = QStringLiteral("0,0;0.6,0.9;1,1;");
    pack->sensorsStruct().sensorFade.isActive = true;
    pack->sensorsStruct().sensorFade.length = 173;

    int callbackCount = 0;
    qreal callbackValue = 0.0;
    KisPropertiesConfiguration *callbackSetting = nullptr;
    data.valueFixUpWriteCallback = [&](qreal value, KisPropertiesConfiguration *setting) {
        ++callbackCount;
        callbackValue = value;
        callbackSetting = setting;
        setting->setProperty(QStringLiteral("callback/値"), 89);
    };

    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("unrelated/保持"), QStringLiteral("残す"));
    pack->write(data, &setting);

    QVERIFY(!setting.getBool(QStringLiteral("PressureSize"), true));
    QVERIFY(!setting.getBool(QStringLiteral("SizeUseCurve"), true));
    QVERIFY(!setting.getBool(QStringLiteral("SizeUseSameCurve"), true));
    QCOMPARE(setting.getDouble(QStringLiteral("SizeValue")), 0.625);
    QCOMPARE(setting.getInt(QStringLiteral("SizecurveMode")), 4);
    QCOMPARE(setting.getString(QStringLiteral("SizecommonCurve")), QStringLiteral("0,0;0.6,0.9;1,1;"));
    QCOMPARE(setting.getString(QStringLiteral("unrelated/保持")), QStringLiteral("残す"));
    QCOMPARE(setting.getInt(QStringLiteral("callback/値")), 89);
    QCOMPARE(callbackCount, 1);
    QCOMPARE(callbackValue, 0.625);
    QCOMPARE(callbackSetting, &setting);

    QDomDocument multipleDocument;
    QVERIFY(multipleDocument.setContent(setting.getString(QStringLiteral("SizeSensor"))));
    const QDomElement root = multipleDocument.documentElement();
    QCOMPARE(root.attribute(QStringLiteral("id")), QStringLiteral("sensorslist"));
    const QDomNodeList children = root.elementsByTagName(QStringLiteral("ChildSensor"));
    QCOMPARE(children.size(), 2);
    QCOMPARE(children.at(0).toElement().attribute(QStringLiteral("id")), QStringLiteral("pressure"));
    QCOMPARE(children.at(1).toElement().attribute(QStringLiteral("id")), QStringLiteral("fade"));
    QCOMPARE(children.at(1).toElement().attribute(QStringLiteral("length")).toInt(), 173);

    pack->sensorsStruct().sensorFade.isActive = false;
    data.valueFixUpWriteCallback = {};
    KisPropertiesConfiguration singleSetting;
    pack->write(data, &singleSetting);

    QDomDocument singleDocument;
    QVERIFY(singleDocument.setContent(singleSetting.getString(QStringLiteral("SizeSensor"))));
    QCOMPARE(singleDocument.documentElement().attribute(QStringLiteral("id")), QStringLiteral("pressure"));
    QCOMPARE(singleDocument.elementsByTagName(QStringLiteral("ChildSensor")).size(), 0);
}

QTEST_GUILESS_MAIN(KisKritaSensorPackContractTest)

#include "KisKritaSensorPackContractTest.moc"
