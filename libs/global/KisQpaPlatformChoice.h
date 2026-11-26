/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KIS_QPA_PLATFORM_CHOICE_H
#define KIS_QPA_PLATFORM_CHOICE_H

#include <QByteArray>

/**
 * The Qt platform plugin LibrePaint asks for on Linux when the user has not
 * set QT_QPA_PLATFORM.
 *
 * A Wayland session gets the native Wayland plugin with xcb as the fallback
 * Qt tries when the Wayland connection fails: through XWayland the ink took
 * 47.5 ms from input to the screen and natively 18.0 ms in a GNOME Wayland
 * session (Issue #108, L3, 2026-11-26). Every other session, an Xorg session
 * among them, has no Wayland compositor and keeps xcb.
 *
 * The session counts as Wayland when XDG_SESSION_TYPE says so or, for a
 * session started without logind, when WAYLAND_DISPLAY is set.
 */
inline QByteArray kisDefaultQpaPlatform(const QByteArray &sessionType, const QByteArray &waylandDisplay)
{
    const QByteArray type = sessionType.trimmed().toLower();
    const bool wayland = type == "wayland" || (type != "x11" && !waylandDisplay.trimmed().isEmpty());
    return wayland ? QByteArrayLiteral("wayland;xcb") : QByteArrayLiteral("xcb");
}

#endif // KIS_QPA_PLATFORM_CHOICE_H
