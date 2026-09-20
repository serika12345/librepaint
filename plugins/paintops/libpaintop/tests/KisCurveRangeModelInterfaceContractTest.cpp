/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <functional>
#include <utility>

#include <KisCurveRangeModelInterface.h>

#include <QTest>

#include <lager/state.hpp>

namespace
{

class CurveRangeModelProbe final : public KisCurveRangeModelInterface
{
public:
    CurveRangeModelProbe(lager::cursor<QString> curve,
                         lager::reader<QString> xMinLabel,
                         lager::reader<QString> xMaxLabel,
                         lager::reader<QString> yMinLabel,
                         lager::reader<QString> yMaxLabel,
                         lager::reader<qreal> yMinValue,
                         lager::reader<qreal> yMaxValue,
                         lager::reader<QString> yValueSuffix,
                         lager::reader<qreal> xMinValue,
                         lager::reader<qreal> xMaxValue,
                         lager::reader<QString> xValueSuffix,
                         bool *destroyed = nullptr)
        : m_curve(std::move(curve))
        , m_xMinLabel(std::move(xMinLabel))
        , m_xMaxLabel(std::move(xMaxLabel))
        , m_yMinLabel(std::move(yMinLabel))
        , m_yMaxLabel(std::move(yMaxLabel))
        , m_yMinValue(std::move(yMinValue))
        , m_yMaxValue(std::move(yMaxValue))
        , m_yValueSuffix(std::move(yValueSuffix))
        , m_xMinValue(std::move(xMinValue))
        , m_xMaxValue(std::move(xMaxValue))
        , m_xValueSuffix(std::move(xValueSuffix))
        , m_destroyed(destroyed)
    {
    }

    ~CurveRangeModelProbe() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

    lager::cursor<QString> curve() override
    {
        return m_curve;
    }

    lager::reader<QString> xMinLabel() override
    {
        return m_xMinLabel;
    }

    lager::reader<QString> xMaxLabel() override
    {
        return m_xMaxLabel;
    }

    lager::reader<QString> yMinLabel() override
    {
        return m_yMinLabel;
    }

    lager::reader<QString> yMaxLabel() override
    {
        return m_yMaxLabel;
    }

    lager::reader<qreal> yMinValue() override
    {
        return m_yMinValue;
    }

    lager::reader<qreal> yMaxValue() override
    {
        return m_yMaxValue;
    }

    lager::reader<QString> yValueSuffix() override
    {
        return m_yValueSuffix;
    }

    lager::reader<qreal> xMinValue() override
    {
        return m_xMinValue;
    }

    lager::reader<qreal> xMaxValue() override
    {
        return m_xMaxValue;
    }

    lager::reader<QString> xValueSuffix() override
    {
        return m_xValueSuffix;
    }

private:
    lager::cursor<QString> m_curve;
    lager::reader<QString> m_xMinLabel;
    lager::reader<QString> m_xMaxLabel;
    lager::reader<QString> m_yMinLabel;
    lager::reader<QString> m_yMaxLabel;
    lager::reader<qreal> m_yMinValue;
    lager::reader<qreal> m_yMaxValue;
    lager::reader<QString> m_yValueSuffix;
    lager::reader<qreal> m_xMinValue;
    lager::reader<qreal> m_xMaxValue;
    lager::reader<QString> m_xValueSuffix;
    bool *m_destroyed;
};

} // namespace

class KisCurveRangeModelInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void accessorsKeepIndependentValuesAndWritableCurve();
    void factoryForwardsInputsAndBaseLifetimeIsVirtual();
};

void KisCurveRangeModelInterfaceContractTest::accessorsKeepIndependentValuesAndWritableCurve()
{
    auto curve = lager::make_state(QStringLiteral("曲線-α"), lager::automatic_tag{});
    auto xMinLabel = lager::make_state(QStringLiteral("x最小"), lager::automatic_tag{});
    auto xMaxLabel = lager::make_state(QStringLiteral("x最大"), lager::automatic_tag{});
    auto yMinLabel = lager::make_state(QStringLiteral("y下限"), lager::automatic_tag{});
    auto yMaxLabel = lager::make_state(QStringLiteral("y上限"), lager::automatic_tag{});
    auto yMinValue = lager::make_state(qreal(-12.5), lager::automatic_tag{});
    auto yMaxValue = lager::make_state(qreal(87.25), lager::automatic_tag{});
    auto yValueSuffix = lager::make_state(QStringLiteral("縦単位"), lager::automatic_tag{});
    auto xMinValue = lager::make_state(qreal(-31.75), lager::automatic_tag{});
    auto xMaxValue = lager::make_state(qreal(142.5), lager::automatic_tag{});
    auto xValueSuffix = lager::make_state(QStringLiteral("横単位"), lager::automatic_tag{});

    CurveRangeModelProbe model(curve,
                               xMinLabel,
                               xMaxLabel,
                               yMinLabel,
                               yMaxLabel,
                               yMinValue,
                               yMaxValue,
                               yValueSuffix,
                               xMinValue,
                               xMaxValue,
                               xValueSuffix);

    QCOMPARE(model.curve().get(), QStringLiteral("曲線-α"));
    QCOMPARE(model.xMinLabel().get(), QStringLiteral("x最小"));
    QCOMPARE(model.xMaxLabel().get(), QStringLiteral("x最大"));
    QCOMPARE(model.yMinLabel().get(), QStringLiteral("y下限"));
    QCOMPARE(model.yMaxLabel().get(), QStringLiteral("y上限"));
    QCOMPARE(model.yMinValue().get(), qreal(-12.5));
    QCOMPARE(model.yMaxValue().get(), qreal(87.25));
    QCOMPARE(model.yValueSuffix().get(), QStringLiteral("縦単位"));
    QCOMPARE(model.xMinValue().get(), qreal(-31.75));
    QCOMPARE(model.xMaxValue().get(), qreal(142.5));
    QCOMPARE(model.xValueSuffix().get(), QStringLiteral("横単位"));

    model.curve().set(QStringLiteral("更新後-β"));
    QCOMPARE(curve.get(), QStringLiteral("更新後-β"));
}

void KisCurveRangeModelInterfaceContractTest::factoryForwardsInputsAndBaseLifetimeIsVirtual()
{
    auto resultCurve = lager::make_state(QStringLiteral("result-curve"), lager::automatic_tag{});
    auto resultString = lager::make_state(QStringLiteral("result-label"), lager::automatic_tag{});
    auto resultValue = lager::make_state(qreal(17.0), lager::automatic_tag{});
    bool destroyed = false;
    auto *expectedModel = new CurveRangeModelProbe(resultCurve,
                                                   resultString,
                                                   resultString,
                                                   resultString,
                                                   resultString,
                                                   resultValue,
                                                   resultValue,
                                                   resultString,
                                                   resultValue,
                                                   resultValue,
                                                   resultString,
                                                   &destroyed);

    auto curve = lager::make_state(QStringLiteral("工場曲線"), lager::automatic_tag{});
    auto range = lager::make_state(QRectF(-4.0, 6.0, 15.0, 23.0), lager::automatic_tag{});
    auto activeSensorId = lager::make_state(QStringLiteral("筆圧センサー"), lager::automatic_tag{});
    auto activeSensorLength = lager::make_state(4096, lager::automatic_tag{});
    QString receivedCurve;
    QRectF receivedRange;
    QString receivedSensorId;
    int receivedSensorLength = 0;

    const QString updatedCurve = QStringLiteral("工場曲線-更新");
    const QRectF updatedRange(-8.0, 3.0, 21.0, 34.0);
    KisCurveRangeModelFactory factory = [&](lager::cursor<QString> curveArgument,
                                            lager::cursor<QRectF> rangeArgument,
                                            lager::reader<QString> sensorIdArgument,
                                            lager::reader<int> sensorLengthArgument) {
        receivedCurve = curveArgument.get();
        receivedRange = rangeArgument.get();
        receivedSensorId = sensorIdArgument.get();
        receivedSensorLength = sensorLengthArgument.get();
        curveArgument.set(updatedCurve);
        rangeArgument.set(updatedRange);
        return expectedModel;
    };

    KisCurveRangeModelInterface *actualModel = factory(curve, range, activeSensorId, activeSensorLength);

    QCOMPARE(actualModel, expectedModel);
    QCOMPARE(receivedCurve, QStringLiteral("工場曲線"));
    QCOMPARE(receivedRange, QRectF(-4.0, 6.0, 15.0, 23.0));
    QCOMPARE(receivedSensorId, QStringLiteral("筆圧センサー"));
    QCOMPARE(receivedSensorLength, 4096);
    QCOMPARE(curve.get(), updatedCurve);
    QCOMPARE(range.get(), updatedRange);

    delete actualModel;
    QVERIFY(destroyed);
}

QTEST_GUILESS_MAIN(KisCurveRangeModelInterfaceContractTest)

#include "KisCurveRangeModelInterfaceContractTest.moc"
