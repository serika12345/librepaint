/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <kis_input_profile.h>
#include <kis_shortcut_configuration.h>

class TestInputProfile : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void roundTripsShortcutValuesByActionId();
    void indexesShortcutsByActionId();
};

void TestInputProfile::roundTripsShortcutValuesByActionId()
{
    KisShortcutConfiguration shortcut;
    shortcut.setActionId(QStringLiteral("tool-invocation"));
    shortcut.setType(KisShortcutConfiguration::MouseButtonType);
    shortcut.setMode(2);
    shortcut.setButtons(Qt::LeftButton);

    const QString serialized = shortcut.serialize();
    QCOMPARE(serialized, QStringLiteral("{2;2;[];1;0;0}"));

    KisShortcutConfiguration restored;
    restored.setActionId(shortcut.actionId());
    QVERIFY(restored.unserialize(serialized));
    QCOMPARE(restored.actionId(), QStringLiteral("tool-invocation"));
    QCOMPARE(restored.type(), KisShortcutConfiguration::MouseButtonType);
    QCOMPARE(restored.mode(), 2U);
    QCOMPARE(restored.buttons(), Qt::MouseButtons(Qt::LeftButton));
    QVERIFY(restored == shortcut);
}

void TestInputProfile::indexesShortcutsByActionId()
{
    KisInputProfile profile;
    profile.setName(QStringLiteral("Test profile"));

    auto *primary = new KisShortcutConfiguration;
    primary->setActionId(QStringLiteral("tool-invocation"));
    primary->setType(KisShortcutConfiguration::KeyCombinationType);
    primary->setKeys({Qt::Key_B});
    profile.addShortcut(primary);

    auto *secondary = new KisShortcutConfiguration;
    secondary->setActionId(QStringLiteral("alternate-invocation"));
    secondary->setType(KisShortcutConfiguration::KeyCombinationType);
    secondary->setKeys({Qt::Key_E});
    profile.addShortcut(secondary);

    QCOMPARE(profile.name(), QStringLiteral("Test profile"));
    QCOMPARE(profile.allShortcuts().size(), 2);
    QCOMPARE(profile.shortcutsForAction(QStringLiteral("tool-invocation")),
             QList<KisShortcutConfiguration *>({primary}));
    QCOMPARE(profile.shortcutsForAction(QStringLiteral("alternate-invocation")),
             QList<KisShortcutConfiguration *>({secondary}));
    QVERIFY(profile.shortcutsForAction(QStringLiteral("missing")).isEmpty());
}

QTEST_MAIN(TestInputProfile)

#include "TestInputProfile.moc"
