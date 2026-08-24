/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QFont>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <KSharedConfig>

#include <KoAspectButton.h>

#include <kis_paintop_option.h>
#include <kis_rectangle_constraint_widget.h>
#include <kis_selection_tool_config_widget_helper.h>
#include <kis_tool_options_popup.h>

class TestToolSettingsUiContract : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void optionStateAndNotifications();
    void rectangleConstraintSettingsRoundTrip();
    void selectionSettingsRoundTrip();
    void toolOptionsPopupLayout();
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

void TestToolSettingsUiContract::toolOptionsPopupLayout()
{
    KisToolOptionsPopup popup(QFont {});
    QWidget first;
    first.setObjectName(QStringLiteral("first-options"));
    first.setWindowTitle(QStringLiteral("First"));
    first.setLayout(new QVBoxLayout);
    QWidget second;
    second.setObjectName(QStringLiteral("second-options"));
    second.setWindowTitle(QStringLiteral("Second"));

    popup.newOptionWidgets({QPointer<QWidget>(&first),
                            QPointer<QWidget>(&second)});

    QCOMPARE(first.parentWidget(), &popup);
    QCOMPARE(second.parentWidget(), &popup);
    const QList<QLabel *> initialLabels = popup.findChildren<QLabel *>();
    QCOMPARE(initialLabels.size(), 2);
    QCOMPARE(initialLabels.at(0)->text(), QStringLiteral("First"));
    QCOMPARE(initialLabels.at(1)->text(), QStringLiteral("Second"));

    popup.newOptionWidgets({QPointer<QWidget>(&second)});

    QVERIFY(first.parentWidget() != &popup);
    QCOMPARE(second.parentWidget(), &popup);
    const QList<QLabel *> updatedLabels = popup.findChildren<QLabel *>();
    QCOMPARE(updatedLabels.size(), 1);
    QCOMPARE(updatedLabels.at(0)->text(), QStringLiteral("Second"));
}

void TestToolSettingsUiContract::rectangleConstraintSettingsRoundTrip()
{
    QStandardPaths::setTestModeEnabled(true);
    const QString configGroup =
        QStringLiteral("TestToolSettingsUiContractRectangle");
    KConfigGroup config = KSharedConfig::openConfig()->group(configGroup);
    config.deleteGroup();
    config.writeEntry("roundCornersX", 7);
    config.writeEntry("roundCornersY", 11);
    config.writeEntry("roundCornersAspectLocked", false);
    config.sync();

    KisRectangleConstraintWidget writer(configGroup, true);
    QSignalSpy writerCornersSpy(
        &writer, &KisRectangleConstraintWidget::roundCornersChanged);
    writer.reloadConfig();

    QCOMPARE(writerCornersSpy.count(), 1);
    QCOMPARE(writerCornersSpy.at(0).at(0).toInt(), 7);
    QCOMPARE(writerCornersSpy.at(0).at(1).toInt(), 11);

    auto *roundX = writer.findChild<QSpinBox *>(
        QStringLiteral("intRoundCornersX"));
    auto *roundY = writer.findChild<QSpinBox *>(
        QStringLiteral("intRoundCornersY"));
    auto *aspectButton = writer.findChild<KoAspectButton *>(
        QStringLiteral("cornersAspectButton"));
    QVERIFY(roundX);
    QVERIFY(roundY);
    QVERIFY(aspectButton);
    QVERIFY(!roundX->isHidden());
    QVERIFY(!roundY->isHidden());
    QVERIFY(!aspectButton->isHidden());

    roundX->setValue(13);
    roundY->setValue(21);
    aspectButton->setKeepAspectRatio(true);

    KisRectangleConstraintWidget reader(configGroup, false);
    QSignalSpy readerCornersSpy(
        &reader, &KisRectangleConstraintWidget::roundCornersChanged);
    reader.reloadConfig();

    QCOMPARE(readerCornersSpy.count(), 1);
    QCOMPARE(readerCornersSpy.at(0).at(0).toInt(), 13);
    QCOMPARE(readerCornersSpy.at(0).at(1).toInt(), 21);
    QVERIFY(reader.findChild<KoAspectButton *>(
        QStringLiteral("cornersAspectButton"))->keepAspectRatio());
    QVERIFY(reader.findChild<QSpinBox *>(
        QStringLiteral("intRoundCornersX"))->isHidden());
    QVERIFY(reader.findChild<QSpinBox *>(
        QStringLiteral("intRoundCornersY"))->isHidden());
    QVERIFY(reader.findChild<KoAspectButton *>(
        QStringLiteral("cornersAspectButton"))->isHidden());

    config.deleteGroup();
    config.sync();
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
