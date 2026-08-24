/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QAction>
#include <QKeyEvent>
#include <QStandardPaths>
#include <QTest>

#include <cmath>

#include <KConfigGroup>
#include <KSharedConfig>

#include <KisToolPaintFactoryBase.h>
#include <kis_delegated_tool.h>
#include <kis_delegated_tool_policies.h>
#include <kis_selection_modifier_mapping.h>
#include <kis_rectangle_interaction.h>
#include <kis_smoothing_options.h>
#include <kis_tool_select_base.h>
#include <kis_tool_paint_interaction.h>
#include <kis_tool_utils.h>

class TestToolFactory : public KisToolPaintFactoryBase
{
public:
    TestToolFactory()
        : KisToolPaintFactoryBase(QStringLiteral("TestToolFactory"))
    {
    }

    QList<QAction *> actions()
    {
        return createActionsImpl();
    }

    KoToolBase *createTool(KoCanvasBase *) override
    {
        return nullptr;
    }
};

class TestToolCoreContract : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void defaultSmoothingSettings();
    void smoothingSettingsRoundTrip();
    void standardBrushSizes();
    void factoryActions();
    void noOpActivationPolicy();
    void selectionModifierMapping();
    void rectangleConstraints();
    void rectangleModifierModes();
    void rectangleRotation();

private:
    void clearSmoothingSettings();
};

void TestToolCoreContract::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    clearSmoothingSettings();
}

void TestToolCoreContract::cleanupTestCase()
{
    clearSmoothingSettings();
}

void TestToolCoreContract::clearSmoothingSettings()
{
    KConfigGroup cfg = KSharedConfig::openConfig()->group(QString());
    const QStringList keys {
        QStringLiteral("LineSmoothingType"),
        QStringLiteral("LineSmoothingDistanceMin"),
        QStringLiteral("LineSmoothingDistanceMax"),
        QStringLiteral("LineSmoothingDistanceKeepAspectRatio"),
        QStringLiteral("LineSmoothingTailAggressiveness"),
        QStringLiteral("LineSmoothingSmoothPressure"),
        QStringLiteral("LineSmoothingScalableDistance"),
        QStringLiteral("LineSmoothingDelayDistance"),
        QStringLiteral("LineSmoothingUseDelayDistance"),
        QStringLiteral("LineSmoothingFinishStabilizedCurve"),
        QStringLiteral("LineSmoothingStabilizeSensors")
    };

    for (const QString &key : keys) {
        cfg.deleteEntry(key);
    }
    cfg.sync();
}

void TestToolCoreContract::defaultSmoothingSettings()
{
    KisSmoothingOptions options(false);

    QCOMPARE(options.smoothingType(), KisSmoothingOptions::SIMPLE_SMOOTHING);
    QCOMPARE(options.smoothnessDistanceMin(), 50.0);
    QCOMPARE(options.smoothnessDistanceMax(), 50.0);
    QVERIFY(options.smoothnessDistanceKeepAspectRatio());
    QCOMPARE(options.tailAggressiveness(), 0.15);
    QVERIFY(!options.smoothPressure());
    QVERIFY(options.useScalableDistance());
    QCOMPARE(options.delayDistance(), 50.0);
    QVERIFY(options.useDelayDistance());
    QVERIFY(options.finishStabilizedCurve());
    QVERIFY(options.stabilizeSensors());
}

void TestToolCoreContract::smoothingSettingsRoundTrip()
{
    {
        KisSmoothingOptions options(false);
        options.setSmoothingType(KisSmoothingOptions::WEIGHTED_SMOOTHING);
        options.setSmoothnessDistanceMin(18.0);
        options.setSmoothnessDistanceMax(41.0);
        options.setSmoothnessDistanceKeepAspectRatio(false);
        options.setTailAggressiveness(0.42);
        options.setSmoothPressure(true);
        options.setUseScalableDistance(false);
        options.setDelayDistance(23.0);
        options.setUseDelayDistance(false);
        options.setFinishStabilizedCurve(false);
        options.setStabilizeSensors(false);

        QTRY_COMPARE_WITH_TIMEOUT(
            KSharedConfig::openConfig()->group(QString()).readEntry("LineSmoothingDistanceMin", -1.0),
            18.0,
            1000);
    }

    KisSmoothingOptions restored(true);
    QCOMPARE(restored.smoothingType(), KisSmoothingOptions::WEIGHTED_SMOOTHING);
    QCOMPARE(restored.smoothnessDistanceMin(), 18.0);
    QCOMPARE(restored.smoothnessDistanceMax(), 41.0);
    QVERIFY(!restored.smoothnessDistanceKeepAspectRatio());
    QCOMPARE(restored.tailAggressiveness(), 0.42);
    QVERIFY(restored.smoothPressure());
    QVERIFY(!restored.useScalableDistance());
    QCOMPARE(restored.delayDistance(), 23.0);
    QVERIFY(!restored.useDelayDistance());
    QVERIFY(!restored.finishStabilizedCurve());
    QVERIFY(!restored.stabilizeSensors());
}

void TestToolCoreContract::standardBrushSizes()
{
    KisToolUtils::StandardBrushSizes sizes(1, 1000);

    QCOMPARE(sizes.increaseBrushSize(1.0), 2);
    QCOMPARE(sizes.decreaseBrushSize(2.0), 1);
    QCOMPARE(sizes.increaseBrushSize(1000.0), 1000);
    QCOMPARE(sizes.decreaseBrushSize(1.0), 1);
}

void TestToolCoreContract::factoryActions()
{
    TestToolFactory factory;
    const QList<QAction *> actions = factory.actions();
    QStringList actionIds;

    for (QAction *action : actions) {
        actionIds.append(action->objectName());
    }

    QCOMPARE(actions.size(), 6);
    QVERIFY(actionIds.contains(QStringLiteral("increase_brush_size")));
    QVERIFY(actionIds.contains(QStringLiteral("decrease_brush_size")));
    QVERIFY(actionIds.contains(QStringLiteral("rotate_brush_tip_clockwise")));
    QVERIFY(actionIds.contains(QStringLiteral("rotate_brush_tip_counter_clockwise_precise")));
}

void TestToolCoreContract::noOpActivationPolicy()
{
    NoopActivationPolicy::onActivate(nullptr);
}

void TestToolCoreContract::selectionModifierMapping()
{
    QCOMPARE(mapSelectionToolModifiers(Qt::NoModifier, false), SELECTION_DEFAULT);
    QCOMPARE(mapSelectionToolModifiers(Qt::ControlModifier, false), SELECTION_REPLACE);
    QCOMPARE(mapSelectionToolModifiers(Qt::AltModifier, false), SELECTION_SUBTRACT);
    QCOMPARE(mapSelectionToolModifiers(Qt::ShiftModifier, false), SELECTION_ADD);
    QCOMPARE(mapSelectionToolModifiers(Qt::AltModifier | Qt::ShiftModifier, false),
             SELECTION_INTERSECT);
    QCOMPARE(mapSelectionToolModifiers(Qt::ControlModifier | Qt::AltModifier, false),
             SELECTION_SYMMETRICDIFFERENCE);

    QCOMPARE(mapSelectionToolModifiers(Qt::AltModifier, true), SELECTION_REPLACE);
    QCOMPARE(mapSelectionToolModifiers(Qt::ControlModifier, true), SELECTION_SUBTRACT);
    QCOMPARE(mapSelectionToolModifiers(Qt::ControlModifier | Qt::ShiftModifier, true),
             SELECTION_INTERSECT);

    const QKeyEvent macAltEvent(QEvent::KeyPress,
                                Qt::Key_Meta,
                                Qt::ShiftModifier);
    QCOMPARE(normalizedSelectionModifierKey(&macAltEvent), Qt::Key_Alt);
}

void TestToolCoreContract::rectangleConstraints()
{
    KisRectangleInteraction interaction;
    interaction.setConstraints(true, false, false, 2.0, 0.0, 0.0);
    interaction.begin(QPointF(10.0, 20.0));
    interaction.update(QPointF(50.0, 50.0));

    QCOMPARE(interaction.rectangle(), QRectF(10.0, 20.0, 60.0, 30.0));

    interaction.setConstraints(false, true, true, 1.0, 40.0, 20.0);
    interaction.begin(QPointF(0.0, 0.0));
    QCOMPARE(interaction.rectangle(), QRectF(0.0, 0.0, 40.0, 20.0));

    interaction.update(QPointF(100.0, 200.0));
    QCOMPARE(interaction.rectangle(), QRectF(100.0, 200.0, 40.0, 20.0));
}

void TestToolCoreContract::rectangleModifierModes()
{
    KisRectangleInteraction interaction;
    interaction.begin(QPointF(10.0, 20.0));
    interaction.setModifier(Qt::ShiftModifier, true);
    interaction.update(QPointF(40.0, 60.0));
    QCOMPARE(interaction.rectangle(), QRectF(10.0, 20.0, 40.0, 40.0));

    interaction.setModifier(Qt::ShiftModifier, false);
    interaction.setModifier(Qt::AltModifier, true);
    interaction.update(QPointF(70.0, 80.0));
    QCOMPARE(interaction.rectangle(), QRectF(30.0, 40.0, 40.0, 40.0));
    QVERIFY(interaction.isTranslating());

    interaction.begin(QPointF(10.0, 20.0));
    interaction.setModifier(Qt::ControlModifier, true);
    interaction.update(QPointF(30.0, 50.0));
    QCOMPARE(interaction.start(), QPointF(0.0, 5.0));
    QCOMPARE(interaction.rectangle(), QRectF(0.0, 5.0, 20.0, 30.0));
    QCOMPARE(interaction.center(), QPointF(10.0, 20.0));
}

void TestToolCoreContract::rectangleRotation()
{
    KisRectangleInteraction interaction;
    interaction.setConstraints(false, true, true, 1.0, 10.0, 20.0);
    interaction.begin(QPointF(0.0, 0.0));
    interaction.setModifier(Qt::ControlModifier, true);
    interaction.setModifier(Qt::AltModifier, true);
    interaction.update(QPointF(0.0, 10.0));

    const qreal expectedAngle = std::atan2(10.0, 0.0) - std::atan2(20.0, 10.0);
    QVERIFY(qAbs(interaction.rotationAngle() - expectedAngle) < 0.000001);
}

QTEST_MAIN(TestToolCoreContract)

#include "TestToolCoreContract.moc"
