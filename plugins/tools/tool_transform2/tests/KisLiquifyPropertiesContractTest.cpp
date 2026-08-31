/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_liquify_properties.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>
#include <QDomDocument>
#include <QStandardPaths>
#include <QTest>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

class KisLiquifyPropertiesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void defaultsAndModesPreserveStableValues();
    void settersAndEqualityPreserveIndependentState();
    void copiesAssignmentAndDebugPreserveValues();
    void xmlRoundTripPreservesValuesAndRejectsInvalidMode();
    void configurationRoundTripPreservesModeScopedValues();

private:
    void clearConfiguration();
};

namespace
{
void setDistinctValues(KisLiquifyProperties &properties)
{
    properties.setMode(KisLiquifyProperties::ROTATE);
    properties.setSize(143.5);
    properties.setAmount(0.73);
    properties.setSpacing(0.41);
    properties.setSizeHasPressure(true);
    properties.setAmountHasPressure(true);
    properties.setReverseDirection(true);
    properties.setUseWashMode(true);
    properties.setFlow(0.62);
}
} // namespace

void KisLiquifyPropertiesContractTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("LibrePaintContractTests"));
    QCoreApplication::setApplicationName(QStringLiteral("KisLiquifyPropertiesContractTest"));
    clearConfiguration();
}

void KisLiquifyPropertiesContractTest::cleanupTestCase()
{
    clearConfiguration();
}

void KisLiquifyPropertiesContractTest::clearConfiguration()
{
    const KSharedConfigPtr config = KSharedConfig::openConfig();
    const QStringList groups{QStringLiteral("LiquifyTool"),
                             QStringLiteral("LiquifyTool/Move"),
                             QStringLiteral("LiquifyTool/Scale"),
                             QStringLiteral("LiquifyTool/Rotate"),
                             QStringLiteral("LiquifyTool/Offset"),
                             QStringLiteral("LiquifyTool/Undo")};
    for (const QString &group : groups) {
        config->deleteGroup(group);
    }
    config->sync();
    config->reparseConfiguration();
}

void KisLiquifyPropertiesContractTest::defaultsAndModesPreserveStableValues()
{
    QCOMPARE(int(KisLiquifyProperties::MOVE), 0);
    QCOMPARE(int(KisLiquifyProperties::SCALE), 1);
    QCOMPARE(int(KisLiquifyProperties::ROTATE), 2);
    QCOMPARE(int(KisLiquifyProperties::OFFSET), 3);
    QCOMPARE(int(KisLiquifyProperties::UNDO), 4);
    QCOMPARE(int(KisLiquifyProperties::N_MODES), 5);

    const KisLiquifyProperties properties;
    QCOMPARE(properties.mode(), KisLiquifyProperties::MOVE);
    QCOMPARE(properties.size(), 60.0);
    QCOMPARE(properties.amount(), 0.05);
    QCOMPARE(properties.spacing(), 0.2);
    QVERIFY(!properties.sizeHasPressure());
    QVERIFY(!properties.amountHasPressure());
    QVERIFY(!properties.reverseDirection());
    QVERIFY(!properties.useWashMode());
    QCOMPARE(properties.flow(), 0.2);
    QCOMPARE(KisLiquifyProperties::minSize(), 5.0);
    QCOMPARE(KisLiquifyProperties::maxSize(), 1000.0);
}

void KisLiquifyPropertiesContractTest::settersAndEqualityPreserveIndependentState()
{
    KisLiquifyProperties baseline;
    KisLiquifyProperties changed;
    QVERIFY(baseline == changed);

    setDistinctValues(changed);
    QVERIFY(!(baseline == changed));
    QCOMPARE(changed.mode(), KisLiquifyProperties::ROTATE);
    QCOMPARE(changed.size(), 143.5);
    QCOMPARE(changed.amount(), 0.73);
    QCOMPARE(changed.spacing(), 0.41);
    QVERIFY(changed.sizeHasPressure());
    QVERIFY(changed.amountHasPressure());
    QVERIFY(changed.reverseDirection());
    QVERIFY(changed.useWashMode());
    QCOMPARE(changed.flow(), 0.62);

    baseline.setMode(KisLiquifyProperties::ROTATE);
    baseline.setSize(143.5);
    baseline.setAmount(0.73);
    baseline.setSpacing(0.41);
    baseline.setSizeHasPressure(true);
    baseline.setAmountHasPressure(true);
    baseline.setReverseDirection(true);
    baseline.setUseWashMode(true);
    baseline.setFlow(0.62);
    QVERIFY(baseline == changed);
}

void KisLiquifyPropertiesContractTest::copiesAssignmentAndDebugPreserveValues()
{
    KisLiquifyProperties original;
    setDistinctValues(original);
    KisLiquifyProperties copy(original);
    KisLiquifyProperties assigned;
    assigned = original;

    QVERIFY(copy == original);
    QVERIFY(assigned == original);
    original.setFlow(0.11);
    QCOMPARE(copy.flow(), 0.62);
    QCOMPARE(assigned.flow(), 0.62);

    QString output;
    QDebug debug(&output);
    debug << copy;
    QVERIFY(output.contains(QStringLiteral("KisLiquifyProperties")));
    QVERIFY(output.contains(QStringLiteral("143.5")));
    QVERIFY(output.contains(QStringLiteral("0.62")));
}

void KisLiquifyPropertiesContractTest::xmlRoundTripPreservesValuesAndRejectsInvalidMode()
{
    KisLiquifyProperties original;
    setDistinctValues(original);
    QDomDocument document;
    QDomElement root = document.createElement(QStringLiteral("root"));
    document.appendChild(root);
    original.toXML(&root);

    const KisLiquifyProperties restored = KisLiquifyProperties::fromXML(root);
    QVERIFY(restored == original);

    QDomElement mode =
        root.firstChildElement(QStringLiteral("liquify_properties")).firstChildElement(QStringLiteral("mode"));
    QVERIFY(!mode.isNull());
    mode.setAttribute(QStringLiteral("value"), int(KisLiquifyProperties::N_MODES));
    const KisLiquifyProperties invalid = KisLiquifyProperties::fromXML(root);
    QCOMPARE(invalid.mode(), KisLiquifyProperties::MOVE);
    QCOMPARE(invalid.size(), 143.5);
    QCOMPARE(invalid.flow(), 0.62);
}

void KisLiquifyPropertiesContractTest::configurationRoundTripPreservesModeScopedValues()
{
    clearConfiguration();
    KisLiquifyProperties saved;
    setDistinctValues(saved);
    saved.saveMode();

    KisLiquifyProperties restored;
    restored.setMode(KisLiquifyProperties::ROTATE);
    restored.loadMode();
    QVERIFY(restored == saved);

    KisLiquifyProperties selected;
    selected.loadAndResetMode();
    QVERIFY(selected == saved);
}

QTEST_GUILESS_MAIN(KisLiquifyPropertiesContractTest)

#include "KisLiquifyPropertiesContractTest.moc"
