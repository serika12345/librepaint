/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisSensorData.h>

#include <KisDynamicSensorIds.h>

#include <QDomDocument>
#include <QTest>

namespace
{
int assertCount = 0;
int safeAssertCount = 0;

const QString &defaultCurveString()
{
    static const QString value = QStringLiteral("0,0;1,1;");
    return value;
}

class DestructionProbe final : public KisSensorData
{
public:
    explicit DestructionProbe(bool *destroyed)
        : KisSensorData(KoID(QStringLiteral("probe")))
        , m_destroyed(destroyed)
    {
    }

    ~DestructionProbe() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};

QDomElement createSensorElement(QDomDocument &document, const QString &id)
{
    QDomElement element = document.createElement(QStringLiteral("sensor"));
    element.setAttribute(QStringLiteral("id"), id);
    document.appendChild(element);
    return element;
}
} // namespace

void kis_assert_exception(const char *, const char *, int)
{
    ++assertCount;
}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    ++safeAssertCount;
}

class KisSensorDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseDefaultsRangeResetAndEquality();
    void baseXmlStoresIdentityAndNonDefaultCurve();
    void lengthSensorsUseIdentifierSpecificDefaultsAndEquality();
    void lengthSensorXmlUsesConfiguredTagAndResetsMissingValues();
    void drawingAngleDefaultsResetAndEqualityCoverDerivedValues();
    void drawingAngleXmlRoundTripsDerivedValuesAndResetsMissingValues();
};

void KisSensorDataContractTest::baseDefaultsRangeResetAndEquality()
{
    assertCount = 0;
    safeAssertCount = 0;

    const KoID pressureId(QStringLiteral("pressure"), QStringLiteral("Pressure"));
    KisSensorData sensor(pressureId);
    QCOMPARE(sensor.id, pressureId);
    QCOMPARE(sensor.curve, defaultCurveString());
    QVERIFY(!sensor.isActive);
    QCOMPARE(sensor.baseCurveRange(), QRectF(0.0, 0.0, 1.0, 1.0));

    KisSensorData sameIdentifier(KoID(QStringLiteral("pressure"), QStringLiteral("Different display name")));
    QVERIFY(sensor == sameIdentifier);

    sameIdentifier.curve = QStringLiteral("0,0;0.5,0.8;1,1;");
    QVERIFY(!(sensor == sameIdentifier));
    sameIdentifier.curve = sensor.curve;
    sameIdentifier.isActive = true;
    QVERIFY(!(sensor == sameIdentifier));

    sensor.curve = QStringLiteral("0,0;0.5,0.8;1,1;");
    sensor.isActive = true;
    sensor.reset();
    QCOMPARE(sensor.id, pressureId);
    QCOMPARE(sensor.curve, defaultCurveString());
    QVERIFY(!sensor.isActive);

    sensor.setBaseCurveRange(QRectF(-1.0, -2.0, 4.0, 6.0));
    QCOMPARE(safeAssertCount, 1);
    QCOMPARE(sensor.baseCurveRange(), QRectF(0.0, 0.0, 1.0, 1.0));

    bool destroyed = false;
    KisSensorData *probe = new DestructionProbe(&destroyed);
    delete probe;
    QVERIFY(destroyed);
    QCOMPARE(assertCount, 0);
}

void KisSensorDataContractTest::baseXmlStoresIdentityAndNonDefaultCurve()
{
    assertCount = 0;

    const KoID pressureId(QStringLiteral("pressure"), QStringLiteral("Pressure"));
    KisSensorData defaultSensor(pressureId);
    QDomDocument defaultDocument;
    QDomElement defaultElement = createSensorElement(defaultDocument, QString());
    defaultSensor.write(defaultDocument, defaultElement);
    QCOMPARE(defaultElement.attribute(QStringLiteral("id")), QStringLiteral("pressure"));
    QVERIFY(defaultElement.firstChildElement(QStringLiteral("curve")).isNull());
    QVERIFY(!defaultElement.hasAttribute(QStringLiteral("isActive")));

    const QString customCurve = QStringLiteral("0,0;0.5,0.8;1,1;");
    KisSensorData source(pressureId);
    source.curve = customCurve;
    source.isActive = true;
    QDomDocument customDocument;
    QDomElement customElement = createSensorElement(customDocument, QString());
    source.write(customDocument, customElement);
    QCOMPARE(customElement.attribute(QStringLiteral("id")), QStringLiteral("pressure"));
    QCOMPARE(customElement.firstChildElement(QStringLiteral("curve")).text(), customCurve);
    QVERIFY(!customElement.hasAttribute(QStringLiteral("isActive")));

    KisSensorData restored(pressureId);
    restored.isActive = true;
    restored.read(customElement);
    QCOMPARE(restored.curve, customCurve);
    QVERIFY(restored.isActive);

    restored.read(defaultElement);
    QCOMPARE(restored.curve, defaultCurveString());
    QVERIFY(restored.isActive);
    QCOMPARE(assertCount, 0);
}

void KisSensorDataContractTest::lengthSensorsUseIdentifierSpecificDefaultsAndEquality()
{
    KisSensorWithLengthData fade(FadeId);
    QCOMPARE(fade.id, FadeId);
    QCOMPARE(fade.length, 1000);
    QVERIFY(!fade.isPeriodic);

    KisSensorWithLengthData distance(DistanceId);
    QCOMPARE(distance.id, DistanceId);
    QCOMPARE(distance.length, 30);
    QVERIFY(!distance.isPeriodic);

    KisSensorWithLengthData time(TimeId, QLatin1String("duration"));
    QCOMPARE(time.id, TimeId);
    QCOMPARE(time.length, 30);
    QVERIFY(!time.isPeriodic);

    KisSensorWithLengthData copy(time);
    QVERIFY(time == copy);
    copy.length = 45;
    QVERIFY(!(time == copy));
    copy.length = time.length;
    copy.isPeriodic = true;
    QVERIFY(!(time == copy));

    KisSensorWithLengthData defaultTag(TimeId);
    QVERIFY(!(time == defaultTag));

    time.length = 120;
    time.isPeriodic = true;
    time.curve = QStringLiteral("0,0;0.25,0.7;1,1;");
    time.isActive = true;
    time.reset();
    QCOMPARE(time.id, TimeId);
    QCOMPARE(time.length, 30);
    QVERIFY(!time.isPeriodic);
    QCOMPARE(time.curve, defaultCurveString());
    QVERIFY(!time.isActive);

    QDomDocument document;
    QDomElement element = createSensorElement(document, QString());
    time.write(document, element);
    QVERIFY(element.hasAttribute(QStringLiteral("duration")));
    QVERIFY(!element.hasAttribute(QStringLiteral("length")));
}

void KisSensorDataContractTest::lengthSensorXmlUsesConfiguredTagAndResetsMissingValues()
{
    assertCount = 0;

    KisSensorWithLengthData source(TimeId, QLatin1String("duration"));
    source.curve = QStringLiteral("0,0;0.25,0.7;1,1;");
    source.length = 120;
    source.isPeriodic = true;

    QDomDocument document;
    QDomElement element = createSensorElement(document, QString());
    source.write(document, element);
    QCOMPARE(element.attribute(QStringLiteral("id")), QStringLiteral("time"));
    QCOMPARE(element.attribute(QStringLiteral("duration")).toInt(), 120);
    QCOMPARE(element.attribute(QStringLiteral("periodic")).toInt(), 1);
    QCOMPARE(element.firstChildElement(QStringLiteral("curve")).text(), source.curve);

    KisSensorWithLengthData restored(TimeId, QLatin1String("duration"));
    restored.length = 400;
    restored.isPeriodic = false;
    restored.isActive = true;
    restored.read(element);
    QVERIFY(restored == source);
    QVERIFY(!restored.isActive);

    QDomDocument missingDocument;
    QDomElement missingElement = createSensorElement(missingDocument, QStringLiteral("time"));
    restored.length = 400;
    restored.isPeriodic = true;
    restored.curve = QStringLiteral("0,0;0.2,0.9;1,1;");
    restored.read(missingElement);
    QCOMPARE(restored.length, 30);
    QVERIFY(!restored.isPeriodic);
    QCOMPARE(restored.curve, defaultCurveString());

    KisSensorWithLengthData fade(FadeId);
    QDomDocument fadeDocument;
    QDomElement fadeElement = createSensorElement(fadeDocument, QString());
    fade.write(fadeDocument, fadeElement);
    QCOMPARE(fadeElement.attribute(QStringLiteral("length")).toInt(), 1000);
    QVERIFY(!fadeElement.hasAttribute(QStringLiteral("duration")));
    QCOMPARE(assertCount, 0);
}

void KisSensorDataContractTest::drawingAngleDefaultsResetAndEqualityCoverDerivedValues()
{
    KisDrawingAngleSensorData sensor;
    QCOMPARE(sensor.id, DrawingAngleId);
    QVERIFY(!sensor.fanCornersEnabled);
    QCOMPARE(sensor.fanCornersStep, 30);
    QCOMPARE(sensor.angleOffset, 0.0);
    QVERIFY(!sensor.lockedAngleMode);

    KisDrawingAngleSensorData copy(sensor);
    QVERIFY(sensor == copy);
    copy.fanCornersEnabled = true;
    QVERIFY(!(sensor == copy));
    copy = sensor;
    copy.fanCornersStep = 45;
    QVERIFY(!(sensor == copy));
    copy = sensor;
    copy.angleOffset = 12.5;
    QVERIFY(!(sensor == copy));
    copy = sensor;
    copy.lockedAngleMode = true;
    QVERIFY(!(sensor == copy));

    sensor.fanCornersEnabled = true;
    sensor.fanCornersStep = 45;
    sensor.angleOffset = 12.5;
    sensor.lockedAngleMode = true;
    sensor.curve = QStringLiteral("0,0;0.4,0.8;1,1;");
    sensor.isActive = true;
    sensor.reset();
    QCOMPARE(sensor.id, DrawingAngleId);
    QVERIFY(!sensor.fanCornersEnabled);
    QCOMPARE(sensor.fanCornersStep, 30);
    QCOMPARE(sensor.angleOffset, 0.0);
    QVERIFY(!sensor.lockedAngleMode);
    QCOMPARE(sensor.curve, defaultCurveString());
    QVERIFY(!sensor.isActive);
}

void KisSensorDataContractTest::drawingAngleXmlRoundTripsDerivedValuesAndResetsMissingValues()
{
    assertCount = 0;

    KisDrawingAngleSensorData source;
    source.fanCornersEnabled = true;
    source.fanCornersStep = 45;
    source.angleOffset = 12.5;
    source.lockedAngleMode = true;
    source.curve = QStringLiteral("0,0;0.4,0.8;1,1;");

    QDomDocument document;
    QDomElement element = createSensorElement(document, QString());
    source.write(document, element);
    QCOMPARE(element.attribute(QStringLiteral("id")), QStringLiteral("drawingangle"));
    QCOMPARE(element.attribute(QStringLiteral("fanCornersEnabled")).toInt(), 1);
    QCOMPARE(element.attribute(QStringLiteral("fanCornersStep")).toInt(), 45);
    QCOMPARE(element.attribute(QStringLiteral("angleOffset")).toDouble(), 12.5);
    QCOMPARE(element.attribute(QStringLiteral("lockedAngleMode")).toInt(), 1);
    QCOMPARE(element.firstChildElement(QStringLiteral("curve")).text(), source.curve);
    QVERIFY(!element.hasAttribute(QStringLiteral("isActive")));

    KisDrawingAngleSensorData restored;
    restored.isActive = true;
    restored.read(element);
    QVERIFY(restored == source);
    QVERIFY(!restored.isActive);

    QDomDocument missingDocument;
    QDomElement missingElement = createSensorElement(missingDocument, QStringLiteral("drawingangle"));
    restored.fanCornersEnabled = true;
    restored.fanCornersStep = 90;
    restored.angleOffset = 20.0;
    restored.lockedAngleMode = true;
    restored.read(missingElement);
    QVERIFY(!restored.fanCornersEnabled);
    QCOMPARE(restored.fanCornersStep, 30);
    QCOMPARE(restored.angleOffset, 0.0);
    QVERIFY(!restored.lockedAngleMode);
    QCOMPARE(restored.curve, defaultCurveString());
    QCOMPARE(assertCount, 0);
}

QTEST_GUILESS_MAIN(KisSensorDataContractTest)

#include "KisSensorDataContractTest.moc"
