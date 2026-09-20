/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisLevelsSlider.h>

#include <QPointer>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

namespace
{

bool approximatelyEqual(qreal lhs, qreal rhs)
{
    return qAbs(lhs - rhs) < 1e-12;
}

class LevelsSliderProbe final : public KisLevelsSlider
{
public:
    explicit LevelsSliderProbe(QWidget *parent = nullptr, bool *destroyed = nullptr)
        : KisLevelsSlider(parent)
        , m_destroyed(destroyed)
    {
        m_handles = {{0, 0.25, Qt::red}, {1, 0.75, Qt::blue}};
    }

    ~LevelsSliderProbe() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

protected:
    void paintGradient(QPainter &, const QRect &) override
    {
    }

private:
    bool *m_destroyed;
};

void comparePositionSignal(const QSignalSpy &spy, int signalIndex, int handleIndex, qreal position)
{
    QCOMPARE(spy.at(signalIndex).at(0).toInt(), handleIndex);
    QVERIFY(approximatelyEqual(spy.at(signalIndex).at(1).toDouble(), position));
}

} // namespace

class KisLevelsSliderContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseMaintainsGeometryValuesAndNotifications();
    void parentOwnsAllSliderTypesAndDestructionIsVirtual();
    void inputLevelsMaintainValuesConstraintsAndNotifications();
    void inputResetPreservesReversedRangeDefect();
    void gammaMaintainsValuePositionAndResets();
    void gammaEndpointNotificationPreservesReportedPositionDefect();
    void outputLevelsAllowCrossedPositions();
    void thresholdMaintainsUnifiedValueAndNotifications();
};

void KisLevelsSliderContractTest::baseMaintainsGeometryValuesAndNotifications()
{
    LevelsSliderProbe slider;

    QCOMPARE(slider.sizeHint(), QSize(267, 31));
    QCOMPARE(slider.minimumSizeHint(), QSize(139, 31));
    slider.resize(300, 40);
    QCOMPARE(slider.gradientRect(), QRect(5, 0, 290, 29));

    QVERIFY(approximatelyEqual(slider.handlePosition(0), 0.25));
    QVERIFY(approximatelyEqual(slider.handlePosition(1), 0.75));
    QCOMPARE(slider.handleColor(0), QColor(Qt::red));
    QCOMPARE(slider.handleColor(1), QColor(Qt::blue));

    QSignalSpy positionSpy(&slider, &KisLevelsSlider::handlePositionChanged);
    slider.setHandlePosition(0, 0.9);
    QVERIFY(approximatelyEqual(slider.handlePosition(0), 0.749));
    QCOMPARE(positionSpy.size(), 1);
    comparePositionSignal(positionSpy, 0, 0, 0.749);

    slider.setHandlePosition(0, 0.749);
    QCOMPARE(positionSpy.size(), 1);

    QSignalSpy colorSpy(&slider, &KisLevelsSlider::handleColorChanged);
    slider.setHandleColor(1, Qt::green);
    QCOMPARE(slider.handleColor(1), QColor(Qt::green));
    QCOMPARE(colorSpy.size(), 1);
    QCOMPARE(colorSpy.at(0).at(0).toInt(), 1);
    QCOMPARE(qvariant_cast<QColor>(colorSpy.at(0).at(1)), QColor(Qt::green));

    slider.setHandleColor(1, Qt::green);
    QCOMPARE(colorSpy.size(), 1);
}

void KisLevelsSliderContractTest::parentOwnsAllSliderTypesAndDestructionIsVirtual()
{
    bool probeDestroyed = false;
    auto *parent = new QWidget;
    QPointer<LevelsSliderProbe> base = new LevelsSliderProbe(parent, &probeDestroyed);
    QPointer<KisInputLevelsSlider> input = new KisInputLevelsSlider(parent);
    QPointer<KisInputLevelsSliderWithGamma> gamma = new KisInputLevelsSliderWithGamma(parent);
    QPointer<KisOutputLevelsSlider> output = new KisOutputLevelsSlider(parent);
    QPointer<KisThresholdSlider> threshold = new KisThresholdSlider(parent);

    QCOMPARE(base->parentWidget(), parent);
    QCOMPARE(input->parentWidget(), parent);
    QCOMPARE(gamma->parentWidget(), parent);
    QCOMPARE(output->parentWidget(), parent);
    QCOMPARE(threshold->parentWidget(), parent);

    delete parent;

    QVERIFY(probeDestroyed);
    QVERIFY(base.isNull());
    QVERIFY(input.isNull());
    QVERIFY(gamma.isNull());
    QVERIFY(output.isNull());
    QVERIFY(threshold.isNull());
}

void KisLevelsSliderContractTest::inputLevelsMaintainValuesConstraintsAndNotifications()
{
    QWidget parent;
    KisInputLevelsSlider slider(&parent);

    QVERIFY(approximatelyEqual(slider.blackPoint(), 0.0));
    QVERIFY(approximatelyEqual(slider.whitePoint(), 1.0));
    QCOMPARE(slider.handleColor(0), QColor(Qt::black));
    QCOMPARE(slider.handleColor(1), QColor(Qt::white));

    QSignalSpy positionSpy(&slider, &KisLevelsSlider::handlePositionChanged);
    QSignalSpy blackSpy(&slider, &KisInputLevelsSlider::blackPointChanged);
    QSignalSpy whiteSpy(&slider, &KisInputLevelsSlider::whitePointChanged);

    slider.setBlackPoint(0.2);
    QVERIFY(approximatelyEqual(slider.blackPoint(), 0.2));
    QCOMPARE(positionSpy.size(), 1);
    QCOMPARE(blackSpy.size(), 1);
    QCOMPARE(whiteSpy.size(), 0);
    comparePositionSignal(positionSpy, 0, 0, 0.2);
    QVERIFY(approximatelyEqual(blackSpy.at(0).at(0).toDouble(), 0.2));

    slider.setWhitePoint(0.1);
    QVERIFY(approximatelyEqual(slider.whitePoint(), 0.201));
    QCOMPARE(positionSpy.size(), 2);
    QCOMPARE(blackSpy.size(), 1);
    QCOMPARE(whiteSpy.size(), 1);
    comparePositionSignal(positionSpy, 1, 1, 0.201);
    QVERIFY(approximatelyEqual(whiteSpy.at(0).at(0).toDouble(), 0.201));

    slider.reset(0.25, 0.75);
    QVERIFY(approximatelyEqual(slider.blackPoint(), 0.25));
    QVERIFY(approximatelyEqual(slider.whitePoint(), 0.75));
    QCOMPARE(blackSpy.size(), 2);
    QCOMPARE(whiteSpy.size(), 2);

    slider.reset(0.25, 0.75);
    QCOMPARE(blackSpy.size(), 2);
    QCOMPARE(whiteSpy.size(), 2);
}

void KisLevelsSliderContractTest::inputResetPreservesReversedRangeDefect()
{
    KisInputLevelsSlider slider;

    slider.reset(0.4, 0.3);

    QVERIFY(approximatelyEqual(slider.blackPoint(), 0.999));
    QVERIFY(approximatelyEqual(slider.whitePoint(), 1.0));
}

void KisLevelsSliderContractTest::gammaMaintainsValuePositionAndResets()
{
    KisInputLevelsSliderWithGamma slider;

    QVERIFY(approximatelyEqual(slider.gamma(), 1.0));
    QVERIFY(approximatelyEqual(slider.handlePosition(0), 0.0));
    QVERIFY(approximatelyEqual(slider.handlePosition(1), 0.5));
    QVERIFY(approximatelyEqual(slider.handlePosition(2), 1.0));
    QCOMPARE(slider.handleColor(1), QColor(Qt::gray));

    QSignalSpy gammaSpy(&slider, &KisInputLevelsSliderWithGamma::gammaChanged);
    QSignalSpy positionSpy(&slider, &KisLevelsSlider::handlePositionChanged);

    slider.setGamma(2.0);
    QVERIFY(approximatelyEqual(slider.gamma(), 2.0));
    QVERIFY(slider.handlePosition(1) > slider.blackPoint());
    QVERIFY(slider.handlePosition(1) < 0.5);
    QCOMPARE(gammaSpy.size(), 1);
    QCOMPARE(positionSpy.size(), 1);
    comparePositionSignal(positionSpy, 0, 1, slider.handlePosition(1));

    slider.setHandlePosition(1, 0.4);
    QVERIFY(approximatelyEqual(slider.handlePosition(1), 0.4));
    QVERIFY(slider.gamma() > 1.0);
    QVERIFY(slider.gamma() < 10.0);
    QCOMPARE(gammaSpy.size(), 2);
    QCOMPARE(positionSpy.size(), 2);

    const qreal gammaBeforeRangeReset = slider.gamma();
    slider.reset(0.2, 0.8);
    QVERIFY(approximatelyEqual(slider.blackPoint(), 0.2));
    QVERIFY(approximatelyEqual(slider.whitePoint(), 0.8));
    QVERIFY(approximatelyEqual(slider.gamma(), gammaBeforeRangeReset));
    QVERIFY(slider.handlePosition(1) > slider.blackPoint());
    QVERIFY(slider.handlePosition(1) < slider.whitePoint());

    slider.reset(0.1, 0.9, 0.5);
    QVERIFY(approximatelyEqual(slider.blackPoint(), 0.1));
    QVERIFY(approximatelyEqual(slider.whitePoint(), 0.9));
    QVERIFY(approximatelyEqual(slider.gamma(), 0.5));

    slider.setGamma(0.0);
    QVERIFY(approximatelyEqual(slider.gamma(), 0.1));
    slider.setGamma(20.0);
    QVERIFY(approximatelyEqual(slider.gamma(), 10.0));
}

void KisLevelsSliderContractTest::gammaEndpointNotificationPreservesReportedPositionDefect()
{
    KisInputLevelsSliderWithGamma slider;
    QSignalSpy positionSpy(&slider, &KisLevelsSlider::handlePositionChanged);

    slider.setHandlePosition(0, 0.2);

    QVERIFY(approximatelyEqual(slider.blackPoint(), 0.2));
    QVERIFY(approximatelyEqual(slider.gamma(), 1.0));
    QVERIFY(approximatelyEqual(slider.handlePosition(1), 0.6));
    QCOMPARE(positionSpy.size(), 2);
    comparePositionSignal(positionSpy, 0, 0, 0.2);
    comparePositionSignal(positionSpy, 1, 1, 0.2);
    QVERIFY(!approximatelyEqual(positionSpy.at(1).at(1).toDouble(), slider.handlePosition(1)));
}

void KisLevelsSliderContractTest::outputLevelsAllowCrossedPositions()
{
    KisOutputLevelsSlider slider;

    slider.setBlackPoint(0.8);
    slider.setWhitePoint(0.2);
    QVERIFY(approximatelyEqual(slider.blackPoint(), 0.8));
    QVERIFY(approximatelyEqual(slider.whitePoint(), 0.2));

    slider.reset(0.9, 0.1);
    QVERIFY(approximatelyEqual(slider.blackPoint(), 0.9));
    QVERIFY(approximatelyEqual(slider.whitePoint(), 0.1));

    slider.setHandlePosition(0, 2.0);
    slider.setHandlePosition(1, -1.0);
    QVERIFY(approximatelyEqual(slider.blackPoint(), 1.0));
    QVERIFY(approximatelyEqual(slider.whitePoint(), 0.0));
}

void KisLevelsSliderContractTest::thresholdMaintainsUnifiedValueAndNotifications()
{
    KisThresholdSlider slider;

    QVERIFY(approximatelyEqual(slider.threshold(), 0.0));

    QSignalSpy thresholdSpy(&slider, &KisThresholdSlider::thresholdChanged);
    QSignalSpy positionSpy(&slider, &KisLevelsSlider::handlePositionChanged);

    slider.setThreshold(0.4);
    QVERIFY(approximatelyEqual(slider.threshold(), 0.4));
    QVERIFY(approximatelyEqual(slider.blackPoint(), 0.4));
    QVERIFY(approximatelyEqual(slider.whitePoint(), 0.4));
    QCOMPARE(thresholdSpy.size(), 1);
    QCOMPARE(positionSpy.size(), 2);
    comparePositionSignal(positionSpy, 0, 0, 0.4);
    comparePositionSignal(positionSpy, 1, 1, 0.4);

    slider.setBlackPoint(0.5);
    QVERIFY(approximatelyEqual(slider.threshold(), 0.5));
    QVERIFY(approximatelyEqual(slider.whitePoint(), 0.5));

    slider.setWhitePoint(0.6);
    QVERIFY(approximatelyEqual(slider.threshold(), 0.6));
    QVERIFY(approximatelyEqual(slider.whitePoint(), 0.6));

    KisLevelsSlider *base = &slider;
    base->setHandlePosition(1, 0.7);
    QVERIFY(approximatelyEqual(slider.threshold(), 0.7));
    QVERIFY(approximatelyEqual(slider.handlePosition(1), 0.7));

    slider.reset(0.8, 0.2);
    QVERIFY(approximatelyEqual(slider.threshold(), 0.8));
    QVERIFY(approximatelyEqual(slider.whitePoint(), 0.8));

    slider.setThreshold(2.0);
    QVERIFY(approximatelyEqual(slider.threshold(), 1.0));

    const qsizetype thresholdSignalCount = thresholdSpy.size();
    slider.setThreshold(1.0);
    QCOMPARE(thresholdSpy.size(), thresholdSignalCount);
}

QTEST_MAIN(KisLevelsSliderContractTest)

#include "KisLevelsSliderContractTest.moc"
