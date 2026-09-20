/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_config.h>
#include <kis_snap_config.h>

#include <QTest>

namespace
{
struct SnapValues {
    bool orthogonal = false;
    bool node = false;
    bool extension = false;
    bool intersection = false;
    bool boundingBox = false;
    bool imageBounds = true;
    bool imageCenter = false;
    bool toPixel = false;
};

struct ConfigProbe {
    int readOnlyConstructionCount = 0;
    int writableConstructionCount = 0;
    int destructionCount = 0;
    int loadCallCount = 0;
    int saveCallCount = 0;
    bool lastLoadDefaultValue = true;
    bool lastSaveReadOnly = true;
    SnapValues valuesToLoad;
    SnapValues savedValues;
};

ConfigProbe configProbe;

void resetProbe()
{
    configProbe = ConfigProbe{};
}

void applyValues(KisSnapConfig *config, const SnapValues &values)
{
    config->setOrthogonal(values.orthogonal);
    config->setNode(values.node);
    config->setExtension(values.extension);
    config->setIntersection(values.intersection);
    config->setBoundingBox(values.boundingBox);
    config->setImageBounds(values.imageBounds);
    config->setImageCenter(values.imageCenter);
    config->setToPixel(values.toPixel);
}

SnapValues readValues(const KisSnapConfig &config)
{
    return {
        config.orthogonal(),
        config.node(),
        config.extension(),
        config.intersection(),
        config.boundingBox(),
        config.imageBounds(),
        config.imageCenter(),
        config.toPixel(),
    };
}

void compareValues(const SnapValues &actual, const SnapValues &expected)
{
    QCOMPARE(actual.orthogonal, expected.orthogonal);
    QCOMPARE(actual.node, expected.node);
    QCOMPARE(actual.extension, expected.extension);
    QCOMPARE(actual.intersection, expected.intersection);
    QCOMPARE(actual.boundingBox, expected.boundingBox);
    QCOMPARE(actual.imageBounds, expected.imageBounds);
    QCOMPARE(actual.imageCenter, expected.imageCenter);
    QCOMPARE(actual.toPixel, expected.toPixel);
}
} // namespace

KisConfig::KisConfig(bool readOnly)
    : m_cfg()
    , m_readOnly(readOnly)
{
    if (readOnly) {
        ++configProbe.readOnlyConstructionCount;
    } else {
        ++configProbe.writableConstructionCount;
    }
}

KisConfig::~KisConfig()
{
    ++configProbe.destructionCount;
}

void KisConfig::loadSnapConfig(KisSnapConfig *config, bool defaultValue) const
{
    ++configProbe.loadCallCount;
    configProbe.lastLoadDefaultValue = defaultValue;
    applyValues(config, configProbe.valuesToLoad);
}

void KisConfig::saveSnapConfig(const KisSnapConfig &config)
{
    ++configProbe.saveCallCount;
    configProbe.lastSaveReadOnly = m_readOnly;
    configProbe.savedValues = readValues(config);
}

class KisSnapConfigContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void explicitConstructionUsesStableDefaultsAndHasLocalLifetime();
    void settersKeepEachSnapStrategyIndependent();
    void defaultConstructionLoadsThroughReadOnlyConfiguration();
    void explicitLoadReplacesCurrentValues();
    void saveDeliversCurrentValuesThroughWritableConfiguration();
};

void KisSnapConfigContractTest::explicitConstructionUsesStableDefaultsAndHasLocalLifetime()
{
    resetProbe();

    {
        const KisSnapConfig config(false);
        compareValues(readValues(config), SnapValues{});
    }

    QCOMPARE(configProbe.readOnlyConstructionCount, 0);
    QCOMPARE(configProbe.writableConstructionCount, 0);
    QCOMPARE(configProbe.destructionCount, 0);
}

void KisSnapConfigContractTest::settersKeepEachSnapStrategyIndependent()
{
    KisSnapConfig config(false);
    const SnapValues enabledStrategies{true, true, true, true, true, false, true, true};

    applyValues(&config, enabledStrategies);
    compareValues(readValues(config), enabledStrategies);

    applyValues(&config, SnapValues{});
    compareValues(readValues(config), SnapValues{});
}

void KisSnapConfigContractTest::defaultConstructionLoadsThroughReadOnlyConfiguration()
{
    resetProbe();
    configProbe.valuesToLoad = {true, false, true, false, true, false, true, false};

    const KisSnapConfig config;

    compareValues(readValues(config), configProbe.valuesToLoad);
    QCOMPARE(configProbe.readOnlyConstructionCount, 1);
    QCOMPARE(configProbe.writableConstructionCount, 0);
    QCOMPARE(configProbe.loadCallCount, 1);
    QCOMPARE(configProbe.destructionCount, 1);
    QVERIFY(!configProbe.lastLoadDefaultValue);
}

void KisSnapConfigContractTest::explicitLoadReplacesCurrentValues()
{
    KisSnapConfig config(false);
    applyValues(&config, {true, true, true, true, true, true, true, true});
    resetProbe();
    configProbe.valuesToLoad = {false, true, false, true, false, true, false, true};

    config.loadStaticData();

    compareValues(readValues(config), configProbe.valuesToLoad);
    QCOMPARE(configProbe.readOnlyConstructionCount, 1);
    QCOMPARE(configProbe.loadCallCount, 1);
    QCOMPARE(configProbe.destructionCount, 1);
    QVERIFY(!configProbe.lastLoadDefaultValue);
}

void KisSnapConfigContractTest::saveDeliversCurrentValuesThroughWritableConfiguration()
{
    KisSnapConfig config(false);
    const SnapValues currentValues{true, false, true, true, false, false, true, true};
    applyValues(&config, currentValues);
    resetProbe();

    config.saveStaticData();

    compareValues(configProbe.savedValues, currentValues);
    QCOMPARE(configProbe.readOnlyConstructionCount, 0);
    QCOMPARE(configProbe.writableConstructionCount, 1);
    QCOMPARE(configProbe.saveCallCount, 1);
    QCOMPARE(configProbe.destructionCount, 1);
    QVERIFY(!configProbe.lastSaveReadOnly);
}

QTEST_GUILESS_MAIN(KisSnapConfigContractTest)

#include "KisSnapConfigContractTest.moc"
