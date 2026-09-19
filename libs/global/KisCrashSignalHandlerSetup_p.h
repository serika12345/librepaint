/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KIS_CRASH_SIGNAL_HANDLER_SETUP_P_H
#define KIS_CRASH_SIGNAL_HANDLER_SETUP_P_H

#include <QList>
#include <QMap>

#include <signal.h>

namespace KisCrashSignalHandlerSetup
{

using Callback = void (*)(int, siginfo_t *, void *);

bool installAlternateStack(const QList<int> &signals, Callback callback, QMap<int, struct sigaction> *previousActions);

} // namespace KisCrashSignalHandlerSetup

#endif // KIS_CRASH_SIGNAL_HANDLER_SETUP_P_H
