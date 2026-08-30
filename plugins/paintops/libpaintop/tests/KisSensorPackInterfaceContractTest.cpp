/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QString>

#include <memory>
#include <utility>
#include <vector>

#include <KisSensorPackInterface.h>

#include <QTest>

struct KisSensorData {
    int marker;
};

struct KisCurveOptionDataCommon {
    int marker;
};

class KisPropertiesConfiguration
{
public:
    int marker;
};

namespace
{

struct ProbeObserver {
    int cloneCount = 0;
    int destructionCount = 0;
    int mutableSensorsCount = 0;
    int constSensorsCount = 0;
    const KisSensorPackInterface *comparedPack = nullptr;
    const KisCurveOptionDataCommon *readData = nullptr;
    const KisPropertiesConfiguration *readSetting = nullptr;
    const KisCurveOptionDataCommon *writtenData = nullptr;
    KisPropertiesConfiguration *writtenSetting = nullptr;
    std::vector<QString> operationOrder;
};

class SensorPackProbe final : public KisSensorPackInterface
{
public:
    SensorPackProbe(ProbeObserver *observer, std::vector<KisSensorData *> sensors, int payload)
        : m_observer(observer)
        , m_sensors(std::move(sensors))
        , m_payload(payload)
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
        if (m_observer) {
            ++m_observer->cloneCount;
        }
        return new SensorPackProbe(*this);
    }

    std::vector<const KisSensorData *> constSensors() const override
    {
        if (m_observer) {
            ++m_observer->constSensorsCount;
            m_observer->operationOrder.push_back(QStringLiteral("constSensors"));
        }

        return {m_sensors.cbegin(), m_sensors.cend()};
    }

    std::vector<KisSensorData *> sensors() override
    {
        if (m_observer) {
            ++m_observer->mutableSensorsCount;
            m_observer->operationOrder.push_back(QStringLiteral("sensors"));
        }
        return m_sensors;
    }

    bool compare(const KisSensorPackInterface *rhs) const override
    {
        if (m_observer) {
            m_observer->comparedPack = rhs;
            m_observer->operationOrder.push_back(QStringLiteral("compare"));
        }
        return m_compareResult;
    }

    bool read(KisCurveOptionDataCommon &data, const KisPropertiesConfiguration *setting) const override
    {
        if (m_observer) {
            m_observer->readData = &data;
            m_observer->readSetting = setting;
            m_observer->operationOrder.push_back(QStringLiteral("read"));
        }
        return m_readResult;
    }

    void write(const KisCurveOptionDataCommon &data, KisPropertiesConfiguration *setting) const override
    {
        if (m_observer) {
            m_observer->writtenData = &data;
            m_observer->writtenSetting = setting;
            m_observer->operationOrder.push_back(QStringLiteral("write"));
        }
    }

    void setCompareResult(bool value)
    {
        m_compareResult = value;
    }

    void setReadResult(bool value)
    {
        m_readResult = value;
    }

    int payload() const
    {
        return m_payload;
    }

    void setPayload(int value)
    {
        m_payload = value;
    }

private:
    ProbeObserver *m_observer;
    std::vector<KisSensorData *> m_sensors;
    int m_payload;
    bool m_compareResult = false;
    bool m_readResult = false;
};

} // namespace

class KisSensorPackInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void operationsPreservePointersOrderAndResults();
    void cloningUsesVirtualCopyAndDetachesSharedData();
    void baseLifetimeIsVirtual();
};

void KisSensorPackInterfaceContractTest::operationsPreservePointersOrderAndResults()
{
    ProbeObserver observer;
    KisSensorData firstSensor{17};
    KisSensorData secondSensor{29};
    SensorPackProbe model(&observer, {&firstSensor, &secondSensor}, 67);
    SensorPackProbe peer(nullptr, {}, 71);

    const std::vector<KisSensorData *> mutableSensors = model.sensors();
    QCOMPARE(mutableSensors.size(), std::size_t(2));
    QCOMPARE(mutableSensors.at(0), &firstSensor);
    QCOMPARE(mutableSensors.at(1), &secondSensor);

    const KisSensorPackInterface &constModel = model;
    const std::vector<const KisSensorData *> constSensors = constModel.constSensors();
    QCOMPARE(constSensors.size(), std::size_t(2));
    QCOMPARE(constSensors.at(0), &firstSensor);
    QCOMPARE(constSensors.at(1), &secondSensor);

    model.setCompareResult(true);
    QVERIFY(model.compare(&peer));
    QCOMPARE(observer.comparedPack, &peer);
    model.setCompareResult(false);
    QVERIFY(!model.compare(nullptr));
    QCOMPARE(observer.comparedPack, nullptr);

    KisCurveOptionDataCommon data{41};
    KisPropertiesConfiguration setting{53};
    model.setReadResult(true);
    QVERIFY(model.read(data, &setting));
    QCOMPARE(observer.readData, &data);
    QCOMPARE(observer.readSetting, &setting);
    model.setReadResult(false);
    QVERIFY(!model.read(data, nullptr));
    QCOMPARE(observer.readData, &data);
    QCOMPARE(observer.readSetting, nullptr);

    model.write(data, &setting);
    QCOMPARE(observer.writtenData, &data);
    QCOMPARE(observer.writtenSetting, &setting);

    QCOMPARE(model.calcActiveSensorLength(QStringLiteral("筆圧/Δ")), -1);
    QCOMPARE(model.calcActiveSensorLength(QString()), -1);
    QCOMPARE(observer.mutableSensorsCount, 1);
    QCOMPARE(observer.constSensorsCount, 1);
    QCOMPARE(observer.operationOrder,
             std::vector<QString>({QStringLiteral("sensors"),
                                   QStringLiteral("constSensors"),
                                   QStringLiteral("compare"),
                                   QStringLiteral("compare"),
                                   QStringLiteral("read"),
                                   QStringLiteral("read"),
                                   QStringLiteral("write")}));
}

void KisSensorPackInterfaceContractTest::cloningUsesVirtualCopyAndDetachesSharedData()
{
    ProbeObserver observer;
    KisSensorData firstSensor{17};
    KisSensorData secondSensor{29};
    SensorPackProbe source(&observer, {&firstSensor, &secondSensor}, 67);

    std::unique_ptr<KisSensorPackInterface> directClone(source.clone());
    QCOMPARE(observer.cloneCount, 1);
    QVERIFY(directClone.get() != &source);
    auto *directProbe = dynamic_cast<SensorPackProbe *>(directClone.get());
    QVERIFY(directProbe);
    QCOMPARE(directProbe->payload(), 67);
    const std::vector<KisSensorData *> directSensors = directProbe->sensors();
    QCOMPARE(directSensors, std::vector<KisSensorData *>({&firstSensor, &secondSensor}));

    QSharedDataPointer<KisSensorPackInterface> original(
        new SensorPackProbe(&observer, {&firstSensor, &secondSensor}, 67));
    QSharedDataPointer<KisSensorPackInterface> detached(original);
    QCOMPARE(detached.constData(), original.constData());

    detached.detach();

    QCOMPARE(observer.cloneCount, 2);
    QVERIFY(detached.constData() != original.constData());
    auto *detachedProbe = dynamic_cast<SensorPackProbe *>(detached.data());
    const auto *originalProbe = dynamic_cast<const SensorPackProbe *>(original.constData());
    QVERIFY(detachedProbe);
    QVERIFY(originalProbe);
    QCOMPARE(detachedProbe->payload(), 67);
    QCOMPARE(originalProbe->payload(), 67);

    detachedProbe->setPayload(83);
    QCOMPARE(detachedProbe->payload(), 83);
    QCOMPARE(originalProbe->payload(), 67);
}

void KisSensorPackInterfaceContractTest::baseLifetimeIsVirtual()
{
    ProbeObserver observer;
    KisSensorPackInterface *pack = new SensorPackProbe(&observer, {}, 67);

    delete pack;

    QCOMPARE(observer.destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisSensorPackInterfaceContractTest)

#include "KisSensorPackInterfaceContractTest.moc"
