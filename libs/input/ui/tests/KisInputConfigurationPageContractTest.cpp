/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config/kis_input_configuration_page.h>
#include <kis_abstract_input_action.h>
#include <kis_input_profile.h>
#include <kis_input_profile_manager.h>
#include <kis_shortcut_configuration.h>

#include <QPushButton>
#include <QTest>
#include <QToolButton>
#include <QTreeView>

namespace
{
class ConfigurableInputAction final : public KisAbstractInputAction
{
public:
    ConfigurableInputAction()
        : KisAbstractInputAction(QStringLiteral("touch-configurable-action"))
    {
        setName(QStringLiteral("Touch configurable action"));
        setDescription(QStringLiteral("An action used to verify touch-accessible configuration."));
    }
};
} // namespace

class KisInputConfigurationPageContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void removesSelectedShortcutWithoutAContextMenu();
};

void KisInputConfigurationPageContractTest::removesSelectedShortcutWithoutAContextMenu()
{
    KisInputProfileManager *manager = KisInputProfileManager::instance();
    auto *action = new ConfigurableInputAction;
    manager->setActions({action});

    KisInputProfile *profile = manager->addProfile(QStringLiteral("Touch deletion contract"));
    auto *shortcut = new KisShortcutConfiguration;
    shortcut->setActionId(action->id());
    shortcut->setType(KisShortcutConfiguration::MouseButtonType);
    shortcut->setButtons(Qt::RightButton);
    profile->addShortcut(shortcut);
    manager->setCurrentProfile(profile);

    {
        KisInputConfigurationPage page;
        page.show();

        auto *collapseButton = page.findChild<QToolButton *>(QStringLiteral("collapseButton"));
        auto *shortcutsView = page.findChild<QTreeView *>(QStringLiteral("shortcutsView"));
        auto *deleteButton = page.findChild<QPushButton *>(QStringLiteral("deleteShortcutButton"));
        QVERIFY(collapseButton);
        QVERIFY(shortcutsView);
        QVERIFY2(deleteButton, "Canvas input shortcuts need a visible delete control for touch-only devices");

        QTest::mouseClick(collapseButton, Qt::LeftButton);
        shortcutsView->setCurrentIndex(shortcutsView->model()->index(0, 0));

        QTRY_VERIFY(deleteButton->isVisible());
        QTRY_VERIFY(deleteButton->isEnabled());
        deleteButton->click();

        QVERIFY(profile->shortcutsForAction(action->id()).isEmpty());
        QCOMPARE(shortcutsView->model()->rowCount(), 1);
        QVERIFY(!deleteButton->isEnabled());
    }

    manager->setActions({});
}

QTEST_MAIN(KisInputConfigurationPageContractTest)

#include "KisInputConfigurationPageContractTest.moc"
