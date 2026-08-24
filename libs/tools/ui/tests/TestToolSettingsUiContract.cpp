/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <kis_paintop_option.h>
#include <kis_selection_tool_config_widget_helper.h>

class TestToolSettingsUiContract : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void optionStateAndNotifications();
    void selectionSettingsRoundTrip();
};

void TestToolSettingsUiContract::optionStateAndNotifications()
{
    KisPaintOpOption option(QStringLiteral("Opacity"), KisPaintOpOption::GENERAL, true);
    QSignalSpy checkedSpy(&option, &KisPaintOpOption::sigCheckedChanged);
    QSignalSpy settingSpy(&option, &KisPaintOpOption::sigSettingChanged);

    QCOMPARE(option.label(), QStringLiteral("Opacity"));
    QCOMPARE(option.category(), KisPaintOpOption::GENERAL);
    QVERIFY(option.isCheckable());
    QVERIFY(option.isChecked());

    option.setChecked(false);

    QVERIFY(!option.isChecked());
    QCOMPARE(checkedSpy.count(), 1);
    QCOMPARE(settingSpy.count(), 1);
}

void TestToolSettingsUiContract::selectionSettingsRoundTrip()
{
    QStandardPaths::setTestModeEnabled(true);
    const QString configGroup =
        QStringLiteral("TestToolSettingsUiContractSelection");

    KisSelectionToolConfigWidgetHelper writer(QStringLiteral("Selection"));
    writer.createOptionWidget(QStringLiteral("test-selection"));
    writer.setConfigGroupForExactTool(configGroup);

    KisSelectionOptions *options = writer.optionWidget();
    options->setMode(PIXEL_SELECTION);
    options->setAction(SELECTION_INTERSECT);
    options->setAntiAliasSelection(false);
    options->setGrowSelection(17);
    options->setStopGrowingAtDarkestPixel(true);
    options->setFeatherSelection(9);
    options->setReferenceLayers(KisSelectionOptions::ColorLabeledLayers);
    options->setSelectedColorLabels({1, 4, 7});

    KisSelectionToolConfigWidgetHelper reader(QStringLiteral("Selection"));
    reader.createOptionWidget(QStringLiteral("test-selection-reader"));
    reader.setConfigGroupForExactTool(configGroup);

    QCOMPARE(reader.selectionMode(), PIXEL_SELECTION);
    QCOMPARE(reader.selectionAction(), SELECTION_INTERSECT);
    QVERIFY(!reader.antiAliasSelection());
    QCOMPARE(reader.growSelection(), 17);
    QVERIFY(reader.stopGrowingAtDarkestPixel());
    QCOMPARE(reader.featherSelection(), 9);
    QCOMPARE(reader.referenceLayers(),
             KisSelectionOptions::ColorLabeledLayers);
    QCOMPARE(reader.selectedColorLabels(), QList<int>({1, 4, 7}));

    delete writer.optionWidget();
    delete reader.optionWidget();
}

QTEST_MAIN(TestToolSettingsUiContract)

#include "TestToolSettingsUiContract.moc"
