/*
 * This file is part of the KDE project
 * SPDX-FileCopyrightText: 2013 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_shortcut_configuration.h"

#include <QStringList>

class KisShortcutConfiguration::Private
{
public:
    Private()
        : type(UnknownType)
        , mode(0)
        , buttons(Qt::NoButton)
        , wheel(NoMovement)
        , gesture(NoGesture)
    {
    }

    QString actionId;
    ShortcutType type;
    uint mode;

    QList<Qt::Key> keys;
    Qt::MouseButtons buttons;
    MouseWheelMovement wheel;
    GestureAction gesture;
};

KisShortcutConfiguration::KisShortcutConfiguration()
    : d(new Private)
{
}

KisShortcutConfiguration::KisShortcutConfiguration(const KisShortcutConfiguration &other)
    : d(new Private)
{
    d->actionId = other.actionId();
    d->type = other.type();
    d->mode = other.mode();
    d->keys = other.keys();
    d->buttons = other.buttons();
    d->wheel = other.wheel();
    d->gesture = other.gesture();
}

KisShortcutConfiguration &KisShortcutConfiguration::operator=(const KisShortcutConfiguration &other)
{
    d->actionId = other.actionId();
    d->type = other.type();
    d->mode = other.mode();
    d->keys = other.keys();
    d->buttons = other.buttons();
    d->wheel = other.wheel();
    d->gesture = other.gesture();

    return *this;
}

bool KisShortcutConfiguration::operator==(const KisShortcutConfiguration &other) const
{
    return d->type == other.d->type && d->keys == other.d->keys && d->buttons == other.d->buttons
        && d->wheel == other.d->wheel && d->gesture == other.d->gesture;
}

KisShortcutConfiguration::~KisShortcutConfiguration()
{
    delete d;
}

QString KisShortcutConfiguration::serialize() const
{
    QString serialized("{");

    serialized.append(QString::number(d->mode, 16));
    serialized.append(';');
#ifdef Q_OS_MACOS
    if (d->type == GestureType) {
        serialized.append(QString::number(MacOSGestureType, 16));
    } else {
        serialized.append(QString::number(d->type, 16));
    }
#else
    serialized.append(QString::number(d->type, 16));
#endif
    serialized.append(";[");

    for (auto itr = d->keys.cbegin(); itr != d->keys.cend(); ++itr) {
        serialized.append(QString::number(*itr, 16));

        if (itr + 1 != d->keys.cend()) {
            serialized.append(',');
        }
    }

    serialized.append("];");

    serialized.append(QString::number(d->buttons, 16));
    serialized.append(';');
    serialized.append(QString::number(d->wheel, 16));
    serialized.append(';');
    serialized.append(QString::number(d->gesture, 16));
    serialized.append('}');

    return serialized;
}

bool KisShortcutConfiguration::unserialize(const QString &serialized)
{
    if (!serialized.startsWith('{')) {
        return false;
    }

    QString remainder = serialized;
    remainder.remove('{').remove('}');
    const QStringList parts = remainder.split(';');

    if (parts.size() < 6) {
        return false;
    }

    d->mode = parts.at(0).toUInt(nullptr, 16);
    d->type = static_cast<ShortcutType>(parts.at(1).toInt(nullptr, 16));

    if (d->type == UnknownType) {
        return false;
    }

#ifdef Q_OS_MACOS
    if (d->type == GestureType) {
        return false;
    }
    if (d->type == MacOSGestureType) {
        d->type = GestureType;
    }
#else
    if (d->type == MacOSGestureType) {
        return false;
    }
#endif

    QString serializedKeys = parts.at(2);
    serializedKeys.remove('[').remove(']');
    const QStringList keyList = serializedKeys.split(',');
    for (const QString &key : keyList) {
        if (!key.isEmpty()) {
            d->keys.append(static_cast<Qt::Key>(key.toUInt(nullptr, 16)));
        }
    }

    d->buttons = static_cast<Qt::MouseButtons>(parts.at(3).toInt(nullptr, 16));
    d->wheel = static_cast<MouseWheelMovement>(parts.at(4).toUInt(nullptr, 16));
    d->gesture = static_cast<GestureAction>(parts.at(5).toUInt(nullptr, 16));

    return true;
}

QString KisShortcutConfiguration::actionId() const
{
    return d->actionId;
}

void KisShortcutConfiguration::setActionId(const QString &actionId)
{
    d->actionId = actionId;
}

KisShortcutConfiguration::ShortcutType KisShortcutConfiguration::type() const
{
    return d->type;
}

void KisShortcutConfiguration::setType(ShortcutType newType)
{
    d->type = newType;
}

uint KisShortcutConfiguration::mode() const
{
    return d->mode;
}

void KisShortcutConfiguration::setMode(uint newMode)
{
    d->mode = newMode;
}

QList<Qt::Key> KisShortcutConfiguration::keys() const
{
    return d->keys;
}

void KisShortcutConfiguration::setKeys(const QList<Qt::Key> &newKeys)
{
    d->keys = newKeys;
}

Qt::MouseButtons KisShortcutConfiguration::buttons() const
{
    return d->buttons;
}

void KisShortcutConfiguration::setButtons(Qt::MouseButtons newButtons)
{
    d->buttons = newButtons;
}

KisShortcutConfiguration::MouseWheelMovement KisShortcutConfiguration::wheel() const
{
    return d->wheel;
}

void KisShortcutConfiguration::setWheel(MouseWheelMovement type)
{
    d->wheel = type;
}

KisShortcutConfiguration::GestureAction KisShortcutConfiguration::gesture() const
{
    return d->gesture;
}

void KisShortcutConfiguration::setGesture(GestureAction type)
{
    d->gesture = type;
}

bool KisShortcutConfiguration::isNoOp() const
{
    return d->type == UnknownType || (d->type == KeyCombinationType && d->keys.isEmpty())
        || (d->type == MouseButtonType && d->buttons.testFlag(Qt::NoButton))
        || (d->type == MouseWheelType && d->wheel == NoMovement)
        || ((d->type == GestureType || d->type == MacOSGestureType)
            && (d->gesture == NoGesture || d->gesture == MaxGesture));
}
