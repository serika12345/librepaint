/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_input_profile.h>
#include <kis_shortcut_configuration.h>

#include <QSignalSpy>
#include <QTest>

class KisInputProfileContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultProfileUsesQObjectParent();
    void nameChangesOnlyForDistinctValues();
    void indexesOwnedShortcutsByActionId();
    void removedShortcutRemainsCallerOwned();
    void destroysOwnedShortcuts();
};

void KisInputProfileContractTest::defaultProfileUsesQObjectParent()
{
    QObject parent;
    KisInputProfile profile(&parent);

    QCOMPARE(profile.parent(), &parent);
    QCOMPARE(profile.name(), QString());
    QVERIFY(profile.allShortcuts().isEmpty());
}

void KisInputProfileContractTest::nameChangesOnlyForDistinctValues()
{
    KisInputProfile profile;
    QSignalSpy nameChangedSpy(&profile, &KisInputProfile::nameChanged);

    profile.setName(QStringLiteral("Digital painting"));
    QCOMPARE(profile.name(), QStringLiteral("Digital painting"));
    QCOMPARE(nameChangedSpy.count(), 1);

    profile.setName(QStringLiteral("Digital painting"));
    QCOMPARE(nameChangedSpy.count(), 1);

    profile.setName(QStringLiteral("Line art"));
    QCOMPARE(profile.name(), QStringLiteral("Line art"));
    QCOMPARE(nameChangedSpy.count(), 2);
}

void KisInputProfileContractTest::indexesOwnedShortcutsByActionId()
{
    KisInputProfile profile;

    auto *primary = new KisShortcutConfiguration;
    primary->setActionId(QStringLiteral("freehand-brush"));
    profile.addShortcut(primary);

    auto *alternate = new KisShortcutConfiguration;
    alternate->setActionId(QStringLiteral("freehand-brush"));
    profile.addShortcut(alternate);

    auto *eraser = new KisShortcutConfiguration;
    eraser->setActionId(QStringLiteral("eraser-mode"));
    profile.addShortcut(eraser);

    const QList<KisShortcutConfiguration *> allShortcuts = profile.allShortcuts();
    QCOMPARE(allShortcuts.size(), 3);
    QVERIFY(allShortcuts.contains(primary));
    QVERIFY(allShortcuts.contains(alternate));
    QVERIFY(allShortcuts.contains(eraser));

    const QList<KisShortcutConfiguration *> brushShortcuts =
        profile.shortcutsForAction(QStringLiteral("freehand-brush"));
    QCOMPARE(brushShortcuts.size(), 2);
    QVERIFY(brushShortcuts.contains(primary));
    QVERIFY(brushShortcuts.contains(alternate));
    QCOMPARE(profile.shortcutsForAction(QStringLiteral("eraser-mode")),
             QList<KisShortcutConfiguration *>({eraser}));
    QVERIFY(profile.shortcutsForAction(QStringLiteral("missing")).isEmpty());
}

void KisInputProfileContractTest::removedShortcutRemainsCallerOwned()
{
    KisInputProfile profile;
    auto *shortcut = new KisShortcutConfiguration;
    shortcut->setActionId(QStringLiteral("temporary"));
    profile.addShortcut(shortcut);

    profile.removeShortcut(shortcut);

    QVERIFY(profile.allShortcuts().isEmpty());
    QVERIFY(profile.shortcutsForAction(QStringLiteral("temporary")).isEmpty());
    QCOMPARE(shortcut->actionId(), QStringLiteral("temporary"));
    delete shortcut;
}

void KisInputProfileContractTest::destroysOwnedShortcuts()
{
    class TrackedShortcut final : public KisShortcutConfiguration
    {
    public:
        explicit TrackedShortcut(bool *destroyed)
            : m_destroyed(destroyed)
        {
        }

        ~TrackedShortcut() override
        {
            *m_destroyed = true;
        }

    private:
        bool *m_destroyed;
    };

    bool destroyed = false;
    {
        KisInputProfile profile;
        auto *shortcut = new TrackedShortcut(&destroyed);
        shortcut->setActionId(QStringLiteral("owned"));
        profile.addShortcut(shortcut);
        QVERIFY(!destroyed);
    }

    QVERIFY(destroyed);
}

QTEST_GUILESS_MAIN(KisInputProfileContractTest)

#include "KisInputProfileContractTest.moc"
