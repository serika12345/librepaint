/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisCurveOptionModel.h"

#include <kis_properties_configuration.h>

#include <QTest>

namespace
{
[[noreturn]] void failUnexpectedConfigurationUse()
{
    qFatal("KisCurveOptionModelTest must not exercise configuration storage");
}

} // namespace

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected recoverable assertion %s at %s:%d", assertion, file, line);
}

struct KisPropertiesConfiguration::Private {
};
KisPropertiesConfiguration::KisPropertiesConfiguration()
    : d(new Private)
{
    failUnexpectedConfigurationUse();
}
KisPropertiesConfiguration::~KisPropertiesConfiguration()
{
    delete d;
}
bool KisPropertiesConfiguration::fromXML(const QString &, bool)
{
    failUnexpectedConfigurationUse();
}
void KisPropertiesConfiguration::fromXML(const QDomElement &)
{
    failUnexpectedConfigurationUse();
}
void KisPropertiesConfiguration::toXML(QDomDocument &, QDomElement &) const
{
    failUnexpectedConfigurationUse();
}
QString KisPropertiesConfiguration::toXML() const
{
    failUnexpectedConfigurationUse();
}
bool KisPropertiesConfiguration::hasProperty(const QString &) const
{
    failUnexpectedConfigurationUse();
}
void KisPropertiesConfiguration::setProperty(const QString &, const QVariant &)
{
    failUnexpectedConfigurationUse();
}
bool KisPropertiesConfiguration::getProperty(const QString &, QVariant &) const
{
    failUnexpectedConfigurationUse();
}
QVariant KisPropertiesConfiguration::getProperty(const QString &) const
{
    failUnexpectedConfigurationUse();
}
int KisPropertiesConfiguration::getInt(const QString &, int) const
{
    failUnexpectedConfigurationUse();
}
double KisPropertiesConfiguration::getDouble(const QString &, double) const
{
    failUnexpectedConfigurationUse();
}
bool KisPropertiesConfiguration::getBool(const QString &, bool) const
{
    failUnexpectedConfigurationUse();
}
QString KisPropertiesConfiguration::getString(const QString &, const QString &) const
{
    failUnexpectedConfigurationUse();
}
QMap<QString, QVariant> KisPropertiesConfiguration::getProperties() const
{
    failUnexpectedConfigurationUse();
}
QList<QString> KisPropertiesConfiguration::getPropertiesKeys() const
{
    failUnexpectedConfigurationUse();
}
void KisPropertiesConfiguration::getPrefixedProperties(const QString &prefix,
                                                       KisPropertiesConfiguration *configuration) const
{
    Q_UNUSED(prefix);
    Q_UNUSED(configuration);
    failUnexpectedConfigurationUse();
}
void KisPropertiesConfiguration::setPrefixedProperties(const QString &prefix,
                                                       const KisPropertiesConfiguration *configuration)
{
    Q_UNUSED(prefix);
    Q_UNUSED(configuration);
    failUnexpectedConfigurationUse();
}
QString KisPropertiesConfiguration::extractedPrefixKey()
{
    failUnexpectedConfigurationUse();
}
bool KisPropertiesConfiguration::compareTo(const KisPropertiesConfiguration *) const
{
    failUnexpectedConfigurationUse();
}
void KisPropertiesConfiguration::dump() const
{
    failUnexpectedConfigurationUse();
}

namespace
{
class RangeProbe final : public KisCurveRangeModelInterface
{
public:
    explicit RangeProbe(lager::cursor<QString> curve)
        : m_curve(std::move(curve))
    {
    }
    lager::cursor<QString> curve() override
    {
        return m_curve;
    }
    lager::reader<QString> xMinLabel() override
    {
        return xMin;
    }
    lager::reader<QString> xMaxLabel() override
    {
        return xMax;
    }
    lager::reader<QString> yMinLabel() override
    {
        return yMin;
    }
    lager::reader<QString> yMaxLabel() override
    {
        return yMax;
    }
    lager::reader<qreal> yMinValue() override
    {
        return value;
    }
    lager::reader<qreal> yMaxValue() override
    {
        return value;
    }
    lager::reader<QString> yValueSuffix() override
    {
        return suffix;
    }
    lager::reader<qreal> xMinValue() override
    {
        return value;
    }
    lager::reader<qreal> xMaxValue() override
    {
        return value;
    }
    lager::reader<QString> xValueSuffix() override
    {
        return suffix;
    }

    lager::cursor<QString> xMin = lager::make_state(QStringLiteral("横最小"), lager::automatic_tag{});
    lager::cursor<QString> xMax = lager::make_state(QStringLiteral("横最大"), lager::automatic_tag{});
    lager::cursor<QString> yMin = lager::make_state(QStringLiteral("縦最小"), lager::automatic_tag{});
    lager::cursor<QString> yMax = lager::make_state(QStringLiteral("縦最大"), lager::automatic_tag{});

private:
    lager::cursor<QString> m_curve;
    lager::reader<qreal> value = lager::make_state(qreal(0), lager::automatic_tag{});
    lager::reader<QString> suffix = lager::make_state(QString(), lager::automatic_tag{});
};

struct Environment {
    QString curve;
    QRectF range;
    QString sensorId;
    int sensorLength = 0;

    KisCurveRangeModelFactory factory()
    {
        return [this](lager::cursor<QString> curveArg,
                      lager::cursor<QRectF> rangeArg,
                      lager::reader<QString> sensorIdArg,
                      lager::reader<int> sensorLengthArg) {
            curve = curveArg.get();
            range = rangeArg.get();
            sensorId = sensorIdArg.get();
            sensorLength = sensorLengthArg.get();
            return new RangeProbe(curveArg);
        };
    }
};

KisCurveOptionData makeData()
{
    KisCurveOptionData data(KoID(QStringLiteral("curve-id"), QStringLiteral("Curve")),
                            KisCurveOptionData::Checkability::Checkable,
                            true,
                            {0.2, 0.8});
    data.strengthValue = 0.6;
    data.commonCurve = QStringLiteral("common-curve");
    data.sensorStruct().sensorPressure.curve = QStringLiteral("pressure-curve");
    data.sensorStruct().sensorTime.curve = QStringLiteral("time-curve");
    data.sensorStruct().sensorTime.length = 73;
    return data;
}
} // namespace

class KisCurveOptionModelTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initialStateDisplaysCommonCurveForFirstSensor();
    void checkabilityAndExternalEnablementDriveEffectiveState();
    void strengthRangeClampsScalesAndBakes();
    void curveControlsRouteCommonAndPerSensorState();
    void labelsAndLengthFollowInjectedRangeAndActiveSensor();
};

void KisCurveOptionModelTest::initialStateDisplaysCommonCurveForFirstSensor()
{
    // Consumer: Brush option editors that present a curve for the selected input sensor.
    // Operation: The editor opens an option that uses a shared curve.
    // Observable result: The first sensor is selected and the shared curve and its range are displayed.
    // Failure impact: The curve editor opens on the wrong curve or with mismatched input bounds.
    const KisCurveOptionData initial = makeData();
    auto state = lager::make_state(static_cast<KisCurveOptionDataCommon>(initial), lager::automatic_tag{});
    auto enabled = lager::make_state(true, lager::automatic_tag{});
    Environment environment;
    auto *model = new KisCurveOptionModel(state, enabled, std::nullopt, 100.0, environment.factory());

    QCOMPARE(model->activeSensorId(), initial.sensors().front()->id.id());
    QCOMPARE(model->displayedCurve(), QStringLiteral("common-curve"));
    QCOMPARE(environment.curve, QStringLiteral("common-curve"));
    QCOMPARE(environment.range, initial.sensors().front()->baseCurveRange());
    QCOMPARE(environment.sensorId, initial.sensors().front()->id.id());
    QCOMPARE(environment.sensorLength, -1);

    delete model;
}

void KisCurveOptionModelTest::checkabilityAndExternalEnablementDriveEffectiveState()
{
    // Consumer: Paint-op option rows and preset saving.
    // Operation: An externally disabled option is enabled, then unchecked by the user.
    // Observable result: The row becomes effective only while both conditions are enabled, and saving records that state.
    // Failure impact: A disabled brush option is applied to strokes or saved as unexpectedly active.
    auto state = lager::make_state(static_cast<KisCurveOptionDataCommon>(makeData()), lager::automatic_tag{});
    auto enabled = lager::make_state(false, lager::automatic_tag{});
    Environment environment;
    KisCurveOptionModel model(state, enabled, std::nullopt, 100.0, environment.factory());

    QVERIFY(model.isCheckable());
    QVERIFY(model.isChecked());
    QVERIFY(!model.effectiveIsChecked());
    enabled.set(true);
    QVERIFY(model.effectiveIsChecked());
    model.setisChecked(false);
    QVERIFY(!state.get().isChecked);
    QVERIFY(!model.effectiveIsChecked());
    QVERIFY(!model.bakedOptionData().isChecked);
}

void KisCurveOptionModelTest::strengthRangeClampsScalesAndBakes()
{
    // Consumer: The brush editor strength slider and the resulting preset.
    // Operation: The editor opens an out-of-range strength, then the user sets the displayed strength to 50 percent.
    // Observable result: The slider shows the permitted 25--75 percent range and saving uses the normalized value 0.5.
    // Failure impact: The editor shows an invalid strength or the saved brush produces a different stroke response.
    KisCurveOptionData data = makeData();
    data.strengthValue = 1.25;
    auto state = lager::make_state(static_cast<KisCurveOptionDataCommon>(data), lager::automatic_tag{});
    auto enabled = lager::make_state(true, lager::automatic_tag{});
    auto range = lager::make_state(std::tuple{qreal(0.25), qreal(0.75)}, lager::automatic_tag{});
    Environment environment;
    KisCurveOptionModel model(state, enabled, range, 100.0, environment.factory());

    const auto [displayedValue, displayedMinimum, displayedMaximum] = model.effectiveStrengthStateDenorm();
    QCOMPARE(displayedValue, 75.0);
    QCOMPARE(displayedMinimum, 25.0);
    QCOMPARE(displayedMaximum, 75.0);
    model.setstrengthValueDenorm(50.0);
    QCOMPARE(state.get().strengthValue, 0.5);

    const KisCurveOptionDataCommon baked = model.bakedOptionData();
    QCOMPARE(baked.strengthMinValue, 0.25);
    QCOMPARE(baked.strengthMaxValue, 0.75);
    QCOMPARE(baked.strengthValue, 0.5);
    QVERIFY(baked.isChecked);
}

void KisCurveOptionModelTest::curveControlsRouteCommonAndPerSensorState()
{
    // Consumer: Brush option editors that let the user share a curve or edit an individual sensor curve.
    // Operation: The user edits the shared curve, then selects the pressure-specific curve and edits it.
    // Observable result: Each edit reaches only the curve currently shown by the editor.
    // Failure impact: Editing one sensor changes the shared curve or a different sensor's stroke response.
    auto state = lager::make_state(static_cast<KisCurveOptionDataCommon>(makeData()), lager::automatic_tag{});
    auto enabled = lager::make_state(true, lager::automatic_tag{});
    Environment environment;
    KisCurveOptionModel model(state, enabled, std::nullopt, 1.0, environment.factory());

    QVERIFY(model.useCurve());
    QVERIFY(model.useSameCurve());
    QCOMPARE(model.curveMode(), 0);
    QCOMPARE(model.activeCurve(), QStringLiteral("common-curve"));
    QCOMPARE(model.displayedCurve(), QStringLiteral("common-curve"));
    model.setactiveCurve(QStringLiteral("common-updated"));
    QCOMPARE(state.get().commonCurve, QStringLiteral("common-updated"));

    model.setuseCurve(false);
    model.setcurveMode(3);
    model.setuseSameCurve(false);
    QVERIFY(!state.get().useCurve);
    QCOMPARE(state.get().curveMode, 3);
    QCOMPARE(model.activeCurve(), QStringLiteral("pressure-curve"));
    model.setdisplayedCurve(QStringLiteral("pressure-updated"));
    QCOMPARE(state.get().sensors().front()->curve, QStringLiteral("pressure-updated"));
}

void KisCurveOptionModelTest::labelsAndLengthFollowInjectedRangeAndActiveSensor()
{
    // Consumer: Curve editors with sensor-dependent labels and input limits.
    // Operation: The user selects the time sensor while using separate curves.
    // Observable result: The editor displays the time curve, its length, and the range labels supplied by the paint-op.
    // Failure impact: Sensor-specific editing uses the wrong curve or presents misleading input values.
    KisCurveOptionData initial = makeData();
    const QString timeId = initial.sensorStruct().sensorTime.id.id();
    auto state = lager::make_state(static_cast<KisCurveOptionDataCommon>(initial), lager::automatic_tag{});
    auto enabled = lager::make_state(true, lager::automatic_tag{});
    Environment environment;
    KisCurveOptionModel model(state, enabled, std::nullopt, 1.0, environment.factory());

    QCOMPARE(model.curveXMinLabel(), QStringLiteral("横最小"));
    QCOMPARE(model.curveXMaxLabel(), QStringLiteral("横最大"));
    QCOMPARE(model.curveYMinLabel(), QStringLiteral("縦最小"));
    QCOMPARE(model.curveYMaxLabel(), QStringLiteral("縦最大"));
    model.setuseSameCurve(false);
    model.setactiveSensorId(timeId);
    QCOMPARE(model.activeSensorId(), timeId);
    QCOMPARE(model.activeSensorLength(), 73);
    QCOMPARE(model.activeCurve(), QStringLiteral("time-curve"));
}

QTEST_GUILESS_MAIN(KisCurveOptionModelTest)
#include "KisCurveOptionModelTest.moc"
