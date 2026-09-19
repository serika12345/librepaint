/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <functional>

#include "KisCurveOptionDataCommon.h"

#include <kis_properties_configuration.h>

#include <QHash>
#include <QMap>
#include <QTest>

#include <utility>

struct KisSensorData {
    int marker;
};

namespace
{
using PropertyStore = QMap<QString, QVariant>;

struct ConfigurationState {
    PropertyStore properties;
};

QHash<const KisPropertiesConfiguration *, ConfigurationState> &configurationStates()
{
    static QHash<const KisPropertiesConfiguration *, ConfigurationState> states;
    return states;
}

ConfigurationState &state(KisPropertiesConfiguration *configuration)
{
    return configurationStates()[configuration];
}

const ConfigurationState &state(const KisPropertiesConfiguration *configuration)
{
    return configurationStates()[configuration];
}

struct SensorPackObserver {
    int destructionCount = 0;
    bool readResult = true;
    bool compareResult = true;
    PropertyStore readProperties;
    int writePayload = 0;
};

class SensorPackProbe final : public KisSensorPackInterface
{
public:
    SensorPackProbe(SensorPackObserver *observer, std::vector<KisSensorData *> sensors = {})
        : m_observer(observer)
        , m_sensors(std::move(sensors))
    {
    }

    ~SensorPackProbe() override
    {
        if (m_observer) {
            ++m_observer->destructionCount;
        }
    }

    KisSensorPackInterface *clone() const override
    {
        return new SensorPackProbe(*this);
    }

    std::vector<const KisSensorData *> constSensors() const override
    {
        return {m_sensors.cbegin(), m_sensors.cend()};
    }

    std::vector<KisSensorData *> sensors() override
    {
        return m_sensors;
    }

    bool compare(const KisSensorPackInterface *) const override
    {
        if (!m_observer) {
            return false;
        }

        return m_observer->compareResult;
    }

    bool read(KisCurveOptionDataCommon &, const KisPropertiesConfiguration *setting) const override
    {
        if (!m_observer) {
            return false;
        }

        if (setting) {
            m_observer->readProperties = setting->getProperties();
        }
        return m_observer->readResult;
    }

    void write(const KisCurveOptionDataCommon &, KisPropertiesConfiguration *setting) const override
    {
        if (!m_observer) {
            return;
        }

        if (setting) {
            setting->setProperty(QStringLiteral("written"), m_observer->writePayload);
        }
    }

private:
    SensorPackObserver *m_observer;
    std::vector<KisSensorData *> m_sensors;
};

KisCurveOptionDataCommon
makeData(SensorPackObserver *observer, const QString &prefix = QString(), std::vector<KisSensorData *> sensors = {})
{
    return KisCurveOptionDataCommon(prefix,
                                    KoID(QStringLiteral("curve-id"), QString::fromUtf8("曲線・識別")),
                                    false,
                                    true,
                                    -0.25,
                                    1.75,
                                    new SensorPackProbe(observer, std::move(sensors)));
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
    configurationStates().insert(this, {});
}

KisPropertiesConfiguration::~KisPropertiesConfiguration()
{
    configurationStates().remove(this);
    delete d;
}

KisPropertiesConfiguration::KisPropertiesConfiguration(const KisPropertiesConfiguration &rhs)
    : KisSerializableConfiguration(rhs)
    , d(new Private)
{
    configurationStates().insert(this, state(&rhs));
}

KisPropertiesConfiguration &KisPropertiesConfiguration::operator=(const KisPropertiesConfiguration &rhs)
{
    if (this != &rhs) {
        state(this) = state(&rhs);
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
    return state(this).properties.contains(name);
}

void KisPropertiesConfiguration::setProperty(const QString &name, const QVariant &value)
{
    state(this).properties.insert(name, value);
}

bool KisPropertiesConfiguration::getProperty(const QString &name, QVariant &value) const
{
    const auto item = state(this).properties.constFind(name);
    if (item == state(this).properties.constEnd()) {
        return false;
    }
    value = *item;
    return true;
}

QVariant KisPropertiesConfiguration::getProperty(const QString &name) const
{
    return state(this).properties.value(name);
}

QMap<QString, QVariant> KisPropertiesConfiguration::getProperties() const
{
    return state(this).properties;
}

QList<QString> KisPropertiesConfiguration::getPropertiesKeys() const
{
    return state(this).properties.keys();
}

void KisPropertiesConfiguration::getPrefixedProperties(const QString &prefix,
                                                       KisPropertiesConfiguration *configuration) const
{
    const qsizetype prefixSize = prefix.size();
    for (auto item = state(this).properties.constBegin(); item != state(this).properties.constEnd(); ++item) {
        if (item.key().startsWith(prefix)) {
            configuration->setProperty(item.key().mid(prefixSize), item.value());
        }
    }
}

void KisPropertiesConfiguration::setPrefixedProperties(const QString &prefix,
                                                       const KisPropertiesConfiguration *configuration)
{
    for (auto item = state(configuration).properties.constBegin(); item != state(configuration).properties.constEnd();
         ++item) {
        setProperty(prefix + item.key(), item.value());
    }
}

bool KisPropertiesConfiguration::compareTo(const KisPropertiesConfiguration *rhs) const
{
    return state(this).properties == state(rhs).properties;
}

void KisPropertiesConfiguration::dump() const
{
}

class KisCurveOptionDataCommonContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionPreservesValues();
    void sensorViewsExposeProvidedSensors();
    void readRejectsNullAndPropagatesFailure();
    void prefixedReadWritePreservesUnrelatedSettings();
    void equalityIncludesValueAndSensorStateButIgnoresCallbacks();
};

void KisCurveOptionDataCommonContractTest::constructionPreservesValues()
{
    SensorPackObserver observer;
    {
        const QString prefix = QString::fromUtf8("覆面/β・曲線/");
        KisCurveOptionDataCommon data(prefix,
                                      KoID(QStringLiteral("筆圧/Δ"), QString::fromUtf8("筆圧・曲線")),
                                      false,
                                      true,
                                      -0.5,
                                      2.25,
                                      new SensorPackProbe(&observer));

        QCOMPARE(data.id.id(), QStringLiteral("筆圧/Δ"));
        QCOMPARE(data.id.name(), QString::fromUtf8("筆圧・曲線"));
        QCOMPARE(data.prefix, prefix);
        QVERIFY(!data.isCheckable);
        QVERIFY(data.isChecked);
        QCOMPARE(data.strengthMinValue, -0.5);
        QCOMPARE(data.strengthMaxValue, 2.25);
        QCOMPARE(data.strengthValue, 2.25);
        QVERIFY(data.useCurve);
        QVERIFY(data.useSameCurve);
        QCOMPARE(data.curveMode, 0);
        QCOMPARE(data.commonCurve, QStringLiteral("0,0;1,1;"));
        QVERIFY(data.sensorData);
        QVERIFY(!data.valueFixUpReadCallback);
        QVERIFY(!data.valueFixUpWriteCallback);
    }
    QCOMPARE(observer.destructionCount, 1);

    SensorPackObserver noPrefixObserver;
    const KisCurveOptionDataCommon noPrefix(KoID(QStringLiteral("size"), QStringLiteral("Size")),
                                            true,
                                            false,
                                            0.25,
                                            0.75,
                                            new SensorPackProbe(&noPrefixObserver));
    QVERIFY(noPrefix.prefix.isEmpty());
    QVERIFY(noPrefix.isCheckable);
    QVERIFY(!noPrefix.isChecked);
    QCOMPARE(noPrefix.strengthMinValue, 0.25);
    QCOMPARE(noPrefix.strengthMaxValue, 0.75);
    QCOMPARE(noPrefix.strengthValue, 0.75);
}

void KisCurveOptionDataCommonContractTest::sensorViewsExposeProvidedSensors()
{
    SensorPackObserver observer;
    KisSensorData first{17};
    KisSensorData second{29};
    KisCurveOptionDataCommon data = makeData(&observer, QString(), {&first, &second});

    const std::vector<KisSensorData *> mutableSensors = data.sensors();
    QCOMPARE(mutableSensors, std::vector<KisSensorData *>({&first, &second}));

    const KisCurveOptionDataCommon &constData = data;
    const std::vector<const KisSensorData *> constSensors = constData.sensors();
    QCOMPARE(constSensors, std::vector<const KisSensorData *>({&first, &second}));
}

void KisCurveOptionDataCommonContractTest::readRejectsNullAndPropagatesFailure()
{
    SensorPackObserver observer;
    KisCurveOptionDataCommon data = makeData(&observer);

    QVERIFY(!data.read(nullptr));

    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("payload"), 43);
    observer.readResult = false;

    QVERIFY(!data.read(&setting));
    QCOMPARE(observer.readProperties.value(QStringLiteral("payload")).toInt(), 43);
    QCOMPARE(setting.getProperty(QStringLiteral("payload")).toInt(), 43);
}

void KisCurveOptionDataCommonContractTest::prefixedReadWritePreservesUnrelatedSettings()
{
    const QString prefix = QString::fromUtf8("覆面/γ・曲線/");
    KisPropertiesConfiguration setting;
    setting.setProperty(prefix + QStringLiteral("payload"), 67);
    setting.setProperty(QStringLiteral("unrelated/保持"), QString::fromUtf8("残す"));

    SensorPackObserver observer;
    observer.readResult = false;
    observer.writePayload = 83;
    KisCurveOptionDataCommon data = makeData(&observer, prefix);

    QVERIFY(!data.read(&setting));
    QCOMPARE(observer.readProperties.value(QStringLiteral("payload")).toInt(), 67);
    QVERIFY(!observer.readProperties.contains(QStringLiteral("unrelated/保持")));

    data.write(&setting);

    QCOMPARE(setting.getProperty(prefix + QStringLiteral("written")).toInt(), 83);
    QCOMPARE(setting.getProperty(QStringLiteral("unrelated/保持")).toString(), QString::fromUtf8("残す"));
}

void KisCurveOptionDataCommonContractTest::equalityIncludesValueAndSensorStateButIgnoresCallbacks()
{
    SensorPackObserver baselineObserver;
    auto *baselinePack = new SensorPackProbe(&baselineObserver);
    KisCurveOptionDataCommon baseline(QStringLiteral("prefix/"),
                                      KoID(QStringLiteral("curve-id"), QStringLiteral("Curve")),
                                      false,
                                      true,
                                      -0.25,
                                      1.75,
                                      baselinePack);
    baseline.useCurve = false;
    baseline.useSameCurve = false;
    baseline.curveMode = 3;
    baseline.commonCurve = QStringLiteral("0,0;0.5,0.75;1,1;");
    baseline.strengthValue = 1.25;

    SensorPackObserver peerObserver;
    auto *peerPack = new SensorPackProbe(&peerObserver);
    KisCurveOptionDataCommon peer(QStringLiteral("prefix/"),
                                  KoID(QStringLiteral("curve-id"), QStringLiteral("Other display name")),
                                  false,
                                  true,
                                  -0.25,
                                  1.75,
                                  peerPack);
    peer.useCurve = false;
    peer.useSameCurve = false;
    peer.curveMode = 3;
    peer.commonCurve = QStringLiteral("0,0;0.5,0.75;1,1;");
    peer.strengthValue = 1.25;

    QVERIFY(baseline == peer);

    const auto verifyDifference = [&baseline](auto mutate) {
        KisCurveOptionDataCommon changed = baseline;
        mutate(changed);
        QVERIFY(!(baseline == changed));
    };

    verifyDifference([](KisCurveOptionDataCommon &data) {
        data.id = KoID(QStringLiteral("other-id"), QStringLiteral("Other"));
    });
    verifyDifference([](KisCurveOptionDataCommon &data) {
        data.prefix.append(QStringLiteral("other/"));
    });
    verifyDifference([](KisCurveOptionDataCommon &data) {
        data.isCheckable = !data.isCheckable;
    });
    verifyDifference([](KisCurveOptionDataCommon &data) {
        data.isChecked = !data.isChecked;
    });
    verifyDifference([](KisCurveOptionDataCommon &data) {
        data.useCurve = !data.useCurve;
    });
    verifyDifference([](KisCurveOptionDataCommon &data) {
        data.useSameCurve = !data.useSameCurve;
    });
    verifyDifference([](KisCurveOptionDataCommon &data) {
        ++data.curveMode;
    });
    verifyDifference([](KisCurveOptionDataCommon &data) {
        data.commonCurve.append(QStringLiteral("0.75,0.5;"));
    });
    verifyDifference([](KisCurveOptionDataCommon &data) {
        data.strengthValue += 0.125;
    });
    verifyDifference([](KisCurveOptionDataCommon &data) {
        data.strengthMinValue -= 0.125;
    });
    verifyDifference([](KisCurveOptionDataCommon &data) {
        data.strengthMaxValue += 0.125;
    });

    baselineObserver.compareResult = false;
    QVERIFY(!(baseline == peer));
    baselineObserver.compareResult = true;

    KisCurveOptionDataCommon callbacksChanged = baseline;
    callbacksChanged.valueFixUpReadCallback = [](KisCurveOptionDataCommon *, const KisPropertiesConfiguration *) { };
    callbacksChanged.valueFixUpWriteCallback = [](qreal, KisPropertiesConfiguration *) { };
    QVERIFY(baseline == callbacksChanged);
}

QTEST_GUILESS_MAIN(KisCurveOptionDataCommonContractTest)

#include "KisCurveOptionDataCommonContractTest.moc"
