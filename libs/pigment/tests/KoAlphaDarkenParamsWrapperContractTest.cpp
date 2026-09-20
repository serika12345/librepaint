/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "compositeops/KoAlphaDarkenParamsWrapper.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QStandardPaths>
#include <QTest>

KoCompositeOp::ParameterInfo::ParameterInfo()
    : opacity(1.0f)
    , flow(1.0f)
    , lastOpacity(&opacity)
{
}

namespace
{
const QString configKey = QStringLiteral("useCreamyAlphaDarken");

void initializeParams(KoCompositeOp::ParameterInfo &params)
{
    params.opacity = 0.8f;
    params.flow = 0.25f;
    params._lastOpacityData = 0.15f;
    params.lastOpacity = &params._lastOpacityData;
}
} // namespace

class KoAlphaDarkenParamsWrapperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void hardWrapperMapsFlowWeightedParameters();
    void creamyWrapperPreservesParameters();
    void hardZeroFlowCombinesSourceAndDestination();
    void creamyZeroFlowPreservesDestination();
    void configurationIsReadOnce();
};

void KoAlphaDarkenParamsWrapperContractTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.writeEntry(configKey, false);
    config.sync();
}

void KoAlphaDarkenParamsWrapperContractTest::cleanupTestCase()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.deleteEntry(configKey);
    config.sync();
}

void KoAlphaDarkenParamsWrapperContractTest::hardWrapperMapsFlowWeightedParameters()
{
    KoCompositeOp::ParameterInfo params;
    initializeParams(params);
    const KoAlphaDarkenParamsWrapperHard wrapper(params);

    QCOMPARE(wrapper.opacity, 0.2f);
    QCOMPARE(wrapper.flow, 0.25f);
    QCOMPARE(wrapper.averageOpacity, 0.0375f);
}

void KoAlphaDarkenParamsWrapperContractTest::creamyWrapperPreservesParameters()
{
    KoCompositeOp::ParameterInfo params;
    initializeParams(params);
    const KoAlphaDarkenParamsWrapperCreamy wrapper(params);

    QCOMPARE(wrapper.opacity, 0.8f);
    QCOMPARE(wrapper.flow, 0.25f);
    QCOMPARE(wrapper.averageOpacity, 0.15f);
}

void KoAlphaDarkenParamsWrapperContractTest::hardZeroFlowCombinesSourceAndDestination()
{
    QCOMPARE(KoAlphaDarkenParamsWrapperHard::calculateZeroFlowAlpha(0.25f, 0.5f, 1.0f), 0.625f);
    QCOMPARE(KoAlphaDarkenParamsWrapperHard::calculateZeroFlowAlpha(0.25f, 0.5f), 0.625f);
    QCOMPARE(KoAlphaDarkenParamsWrapperHard::calculateZeroFlowAlphaLegacy(quint8(64), quint8(128)), quint8(160));

    QCOMPARE(KoAlphaDarkenParamsWrapperHard::calculateZeroFlowAlpha(0.0f, 0.5f), 0.5f);
    QCOMPARE(KoAlphaDarkenParamsWrapperHard::calculateZeroFlowAlpha(1.0f, 0.5f), 1.0f);
}

void KoAlphaDarkenParamsWrapperContractTest::creamyZeroFlowPreservesDestination()
{
    QCOMPARE(KoAlphaDarkenParamsWrapperCreamy::calculateZeroFlowAlpha(0.25f, 0.75f, 0.5f), 0.75f);
    QCOMPARE(KoAlphaDarkenParamsWrapperCreamy::calculateZeroFlowAlpha(0.25f, 0.75f), 0.75f);
    QCOMPARE(KoAlphaDarkenParamsWrapperCreamy::calculateZeroFlowAlphaLegacy(quint8(64), quint8(192)), quint8(192));
}

void KoAlphaDarkenParamsWrapperContractTest::configurationIsReadOnce()
{
    QVERIFY(!useCreamyAlphaDarken());

    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.writeEntry(configKey, true);
    config.sync();

    QVERIFY(!useCreamyAlphaDarken());
}

QTEST_GUILESS_MAIN(KoAlphaDarkenParamsWrapperContractTest)

#include "KoAlphaDarkenParamsWrapperContractTest.moc"
