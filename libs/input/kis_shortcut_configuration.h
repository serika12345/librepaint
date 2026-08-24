/*
 * This file is part of the KDE project
 * SPDX-FileCopyrightText: 2013 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISSHORTCUTCONFIGURATION_H
#define KISSHORTCUTCONFIGURATION_H

#include <QList>
#include <QMetaType>
#include <QString>

#include "kritainput_export.h"

/**
 * Input-owned values for one persisted shortcut.
 *
 * The action identifier is stable profile data. Concrete UI action objects
 * are resolved only when a profile is presented or installed in a matcher.
 */
class KRITAINPUT_EXPORT KisShortcutConfiguration
{
public:
    enum ShortcutType {
        UnknownType,
        KeyCombinationType,
        MouseButtonType,
        MouseWheelType,
        GestureType,
        MacOSGestureType,
    };

    enum MouseWheelMovement {
        NoMovement,
        WheelUp,
        WheelDown,
        WheelLeft,
        WheelRight,
        WheelTrackpad,
    };

    enum GestureAction {
        NoGesture,
#ifdef Q_OS_MACOS
        PinchGesture,
        PanGesture,
        RotateGesture,
        SmartZoomGesture,
#else
        OneFingerTap,
        TwoFingerTap,
        ThreeFingerTap,
        FourFingerTap,
        FiveFingerTap,
        OneFingerDrag,
        TwoFingerDrag,
        ThreeFingerDrag,
        FourFingerDrag,
        FiveFingerDrag,
        OneFingerHold,
#endif
        MaxGesture,
    };

    KisShortcutConfiguration();
    KisShortcutConfiguration(const KisShortcutConfiguration &other);
    KisShortcutConfiguration &operator=(const KisShortcutConfiguration &other);
    virtual ~KisShortcutConfiguration();

    bool operator==(const KisShortcutConfiguration &other) const;

    QString serialize() const;
    bool unserialize(const QString &serialized);

    QString actionId() const;
    void setActionId(const QString &actionId);

    ShortcutType type() const;
    void setType(ShortcutType newType);

    uint mode() const;
    void setMode(uint newMode);

    QList<Qt::Key> keys() const;
    void setKeys(const QList<Qt::Key> &newKeys);

    Qt::MouseButtons buttons() const;
    void setButtons(Qt::MouseButtons newButtons);

    MouseWheelMovement wheel() const;
    void setWheel(MouseWheelMovement type);

    GestureAction gesture() const;
    void setGesture(GestureAction type);

    bool isNoOp() const;

private:
    class Private;
    Private *const d;
};

Q_DECLARE_METATYPE(KisShortcutConfiguration *)

#endif // KISSHORTCUTCONFIGURATION_H
