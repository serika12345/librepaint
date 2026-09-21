/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisSpacingOptionWidget.h>

#include <QCheckBox>
#include <QString>
#include <QWidget>
#include <QtCore/qtmetamacros.h>
#include <QtTest/qtest.h>
#include <QtTest/qtestcase.h>

#include <KLocalizedString>

#include <kis_properties_configuration.h>
#include <kis_types.h>
#include <lager/state.hpp>
#include <lager/tags.hpp>

namespace
{

QCheckBox *spacingControl(QWidget *page, const QString &label)
{
    const auto controls = page->findChildren<QCheckBox *>();
    for (QCheckBox *control : controls) {
        if (control->text() == label) {
            return control;
        }
    }

    return nullptr;
}

} // namespace

class KisSpacingOptionWidgetContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void savesSpacingControlsToPresetSettings();
};

void KisSpacingOptionWidgetContractTest::savesSpacingControlsToPresetSettings()
{
    auto optionData = lager::make_state(KisSpacingOptionData(), lager::automatic_tag{});
    KisSpacingOptionWidget widget(optionData);

    QCheckBox *isotropicSpacing = spacingControl(widget.configurationPage(), i18n("Isotropic Spacing"));
    QCheckBox *useSpacingUpdates = spacingControl(widget.configurationPage(), i18n("Update Between Dabs"));
    QVERIFY(isotropicSpacing);
    QVERIFY(useSpacingUpdates);

    isotropicSpacing->setChecked(true);
    useSpacingUpdates->setChecked(true);

    KisPropertiesConfigurationSP setting = new KisPropertiesConfiguration;
    widget.writeOptionSetting(setting);

    QVERIFY(setting->getBool(QStringLiteral("Spacing/Isotropic")));
    QVERIFY(setting->getBool(QStringLiteral("PaintOpSettings/updateSpacingBetweenDabs")));
}

QTEST_MAIN(KisSpacingOptionWidgetContractTest)

#include "KisSpacingOptionWidgetContractTest.moc"
