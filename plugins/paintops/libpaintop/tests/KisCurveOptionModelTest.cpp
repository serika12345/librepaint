/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisCurveOptionModel.h"

#include <kis_properties_configuration.h>

#include <QHash>
#include <QMap>
#include <QPointer>
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

namespace
{
class RangeProbe final : public KisCurveRangeModelInterface
{
public:
    RangeProbe(lager::cursor<QString> curve, bool *destroyed)
        : m_curve(std::move(curve))
        , m_destroyed(destroyed)
    {
    }
    ~RangeProbe() override
    {
        *m_destroyed = true;
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
    bool *m_destroyed;
};

struct Environment {
    bool destroyed = false;
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
            return new RangeProbe(curveArg, &destroyed);
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
    void typesConstructorAndRangeFactoryPreserveOwnership();
    void checkabilityAndExternalEnablementDriveEffectiveState();
    void strengthRangeClampsScalesAndBakes();
    void curveControlsRouteCommonAndPerSensorState();
    void labelsAndLengthFollowInjectedRangeAndActiveSensor();
};

void KisCurveOptionModelTest::typesConstructorAndRangeFactoryPreserveOwnership()
{
    static_assert(std::is_same_v<RangeState, std::tuple<qreal, qreal>>);
    static_assert(std::is_same_v<StrengthState, std::tuple<qreal, qreal, qreal>>);
    static_assert(std::is_same_v<LabelsState, std::tuple<QString, int>>);

    const KisCurveOptionData initial = makeData();
    auto state = lager::make_state(static_cast<KisCurveOptionDataCommon>(initial), lager::automatic_tag{});
    auto enabled = lager::make_state(true, lager::automatic_tag{});
    Environment environment;
    QPointer<KisCurveOptionModel> model =
        new KisCurveOptionModel(state, enabled, std::nullopt, 100.0, environment.factory());

    QCOMPARE(model->optionData.get(), static_cast<KisCurveOptionDataCommon>(initial));
    QCOMPARE(model->strengthRangeNorm.get(), RangeState(0.2, 0.8));
    QCOMPARE(model->activeSensorIdData.get(), initial.sensors().front()->id.id());
    QVERIFY(model->rangeModel);
    QCOMPARE(environment.curve, QStringLiteral("common-curve"));
    QCOMPARE(environment.range, initial.sensors().front()->baseCurveRange());
    QCOMPARE(environment.sensorId, initial.sensors().front()->id.id());
    QCOMPARE(environment.sensorLength, -1);

    QObject *base = model;
    delete base;
    QVERIFY(model.isNull());
    QVERIFY(environment.destroyed);
}

void KisCurveOptionModelTest::checkabilityAndExternalEnablementDriveEffectiveState()
{
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
}

void KisCurveOptionModelTest::strengthRangeClampsScalesAndBakes()
{
    KisCurveOptionData data = makeData();
    data.strengthValue = 1.25;
    auto state = lager::make_state(static_cast<KisCurveOptionDataCommon>(data), lager::automatic_tag{});
    auto enabled = lager::make_state(true, lager::automatic_tag{});
    auto range = lager::make_state(RangeState(0.25, 0.75), lager::automatic_tag{});
    Environment environment;
    KisCurveOptionModel model(state, enabled, range, 100.0, environment.factory());

    QCOMPARE(model.effectiveStrengthValueNorm(), 0.75);
    QCOMPARE(model.strengthValueDenorm(), 125.0);
    QCOMPARE(model.effectiveStrengthStateDenorm(), StrengthState(75.0, 25.0, 75.0));
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
    QCOMPARE(model.labelsState(), LabelsState(timeId, 73));
    QCOMPARE(model.activeCurve(), QStringLiteral("time-curve"));
}

QTEST_GUILESS_MAIN(KisCurveOptionModelTest)
#include "KisCurveOptionModelTest.moc"
