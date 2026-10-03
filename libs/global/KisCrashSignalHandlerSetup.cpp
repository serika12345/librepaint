/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisCrashSignalHandlerSetup_p.h"

#include <cstdlib>
#include <qlist.h>
#include <qmap.h>
#include <signal.h>
#include <sys/signal.h>

namespace KisCrashSignalHandlerSetup
{

bool installAlternateStack(const QList<int> &signals, Callback callback, QMap<int, struct sigaction> *previousActions)
{
    stack_t alternateStack = {};
    alternateStack.ss_flags = 0;
    alternateStack.ss_size = SIGSTKSZ;
    if ((alternateStack.ss_sp = malloc(SIGSTKSZ)) == nullptr) {
        return false;
    }

    struct sigaction action = {};
    action.sa_sigaction = callback;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigaltstack(&alternateStack, nullptr);

    for (const int signal : signals) {
        sigaction(signal, &action, &(*previousActions)[signal]);
    }

    return true;
}

} // namespace KisCrashSignalHandlerSetup
