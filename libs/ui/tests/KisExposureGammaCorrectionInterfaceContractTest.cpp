/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "canvas/kis_exposure_gamma_correction_interface.h"

#include <QTest>

#include <memory>

class ExposureGammaProbe : public KisExposureGammaCorrectionInterface
{
public:
    explicit ExposureGammaProbe(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~ExposureGammaProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    bool canChangeExposureAndGamma() const override
    {
        ++canChangeCallCount;
        return canChangeResult;
    }

    qreal currentExposure() const override
    {
        ++currentExposureCallCount;
        return exposureResult;
    }

    void setCurrentExposure(qreal value) override
    {
        ++setExposureCallCount;
        lastExposure = value;
    }

    qreal currentGamma() const override
    {
        ++currentGammaCallCount;
        return gammaResult;
    }

    void setCurrentGamma(qreal value) override
    {
        ++setGammaCallCount;
        lastGamma = value;
    }

    mutable int canChangeCallCount = 0;
    mutable int currentExposureCallCount = 0;
    mutable int currentGammaCallCount = 0;
    int setExposureCallCount = 0;
    int setGammaCallCount = 0;
    bool canChangeResult = true;
    qreal exposureResult = -2.5;
    qreal gammaResult = 2.25;
    qreal lastExposure = 0.0;
    qreal lastGamma = 0.0;

private:
    int *m_destructionCount;
};

class KisExposureGammaCorrectionInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void abstractInterfaceDispatchesValuesAndResults();
    void baseOwnershipDestroysDerivedExactlyOnce();
    void dumbInstanceKeepsNeutralValuesAndIgnoresSetters();
};

void KisExposureGammaCorrectionInterfaceContractTest::abstractInterfaceDispatchesValuesAndResults()
{
    ExposureGammaProbe probe;
    KisExposureGammaCorrectionInterface &interface = probe;

    QCOMPARE(interface.canChangeExposureAndGamma(), probe.canChangeResult);
    QCOMPARE(interface.currentExposure(), probe.exposureResult);
    interface.setCurrentExposure(3.5);
    QCOMPARE(interface.currentGamma(), probe.gammaResult);
    interface.setCurrentGamma(0.625);

    QCOMPARE(probe.canChangeCallCount, 1);
    QCOMPARE(probe.currentExposureCallCount, 1);
    QCOMPARE(probe.setExposureCallCount, 1);
    QCOMPARE(probe.lastExposure, 3.5);
    QCOMPARE(probe.currentGammaCallCount, 1);
    QCOMPARE(probe.setGammaCallCount, 1);
    QCOMPARE(probe.lastGamma, 0.625);
}

void KisExposureGammaCorrectionInterfaceContractTest::baseOwnershipDestroysDerivedExactlyOnce()
{
    int destructionCount = 0;

    {
        std::unique_ptr<KisExposureGammaCorrectionInterface> interface =
            std::make_unique<ExposureGammaProbe>(&destructionCount);
        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

void KisExposureGammaCorrectionInterfaceContractTest::dumbInstanceKeepsNeutralValuesAndIgnoresSetters()
{
    KisDumbExposureGammaCorrectionInterface *first = KisDumbExposureGammaCorrectionInterface::instance();
    KisDumbExposureGammaCorrectionInterface *second = KisDumbExposureGammaCorrectionInterface::instance();

    QVERIFY(first);
    QCOMPARE(first, second);

    KisExposureGammaCorrectionInterface *interface = first;
    QVERIFY(!interface->canChangeExposureAndGamma());
    QCOMPARE(interface->currentExposure(), 0.0);
    QCOMPARE(interface->currentGamma(), 1.0);

    interface->setCurrentExposure(-7.5);
    interface->setCurrentGamma(4.25);

    QVERIFY(!interface->canChangeExposureAndGamma());
    QCOMPARE(interface->currentExposure(), 0.0);
    QCOMPARE(interface->currentGamma(), 1.0);
}

QTEST_GUILESS_MAIN(KisExposureGammaCorrectionInterfaceContractTest)

#include "KisExposureGammaCorrectionInterfaceContractTest.moc"
