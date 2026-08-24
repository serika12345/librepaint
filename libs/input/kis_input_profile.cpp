/*
 * This file is part of the KDE project
 * SPDX-FileCopyrightText: 2013 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_input_profile.h"

#include <QMultiHash>

#include "kis_shortcut_configuration.h"

class KisInputProfile::Private
{
public:
    Private() { }
    ~Private()
    {
        qDeleteAll(shortcuts);
    }

    QString name;
    QMultiHash<QString, KisShortcutConfiguration *> shortcuts;
};

KisInputProfile::KisInputProfile(QObject *parent)
    : QObject(parent), d(new Private())
{

}

KisInputProfile::~KisInputProfile()
{
    delete d;
}

QString KisInputProfile::name() const
{
    return d->name;
}
void KisInputProfile::setName(const QString &name)
{
    if (d->name != name) {
        d->name = name;
        Q_EMIT nameChanged();
    }
}

QList< KisShortcutConfiguration * > KisInputProfile::allShortcuts() const
{
    return d->shortcuts.values();
}

QList<KisShortcutConfiguration *> KisInputProfile::shortcutsForAction(const QString &actionId) const
{
    if (d->shortcuts.contains(actionId)) {
        return d->shortcuts.values(actionId);
    }

    return QList<KisShortcutConfiguration *>();
}

void KisInputProfile::addShortcut(KisShortcutConfiguration *shortcut)
{
    Q_ASSERT(shortcut);
    Q_ASSERT(!shortcut->actionId().isEmpty());
    d->shortcuts.insert(shortcut->actionId(), shortcut);
}

void KisInputProfile::removeShortcut(KisShortcutConfiguration *shortcut)
{
    Q_ASSERT(shortcut);
    Q_ASSERT(!shortcut->actionId().isEmpty());
    d->shortcuts.remove(shortcut->actionId(), shortcut);
}
