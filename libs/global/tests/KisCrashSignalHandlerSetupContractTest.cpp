/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisAndroidCrashHandler.h"
#include "KisCrashSignalHandlerSetup_p.h"

#include <QTest>

#include <cstdlib>
#include <type_traits>

namespace
{

void testSignalCallback(int, siginfo_t *, void *)
{
}

bool hasSameAction(const struct sigaction &first, const struct sigaction &second)
{
    if (first.sa_flags != second.sa_flags) {
        return false;
    }

    if ((first.sa_flags & SA_SIGINFO) != 0) {
        return first.sa_sigaction == second.sa_sigaction;
    }

    return first.sa_handler == second.sa_handler;
}

} // namespace

class KisCrashSignalHandlerSetupContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void androidCrashHandlerKeepsItsEntryPointAndInstallsAlternateSignalHandling();
};

void KisCrashSignalHandlerSetupContractTest::androidCrashHandlerKeepsItsEntryPointAndInstallsAlternateSignalHandling()
{
    static_assert(std::is_same_v<decltype(&KisAndroidCrashHandler::handler_init), void (*)()>);

    const QList<int> signals{SIGUSR1, SIGUSR2};
    QMap<int, struct sigaction> actionsBeforeInstall;
    struct sigaction action = {};
    for (const int signal : signals) {
        QCOMPARE(sigaction(signal, nullptr, &action), 0);
        actionsBeforeInstall.insert(signal, action);
    }

    stack_t stackBeforeInstall = {};
    QCOMPARE(sigaltstack(nullptr, &stackBeforeInstall), 0);

    QMap<int, struct sigaction> previousActions;
    QVERIFY(KisCrashSignalHandlerSetup::installAlternateStack(signals, testSignalCallback, &previousActions));

    stack_t installedStack = {};
    const bool capturedInstalledStack = sigaltstack(nullptr, &installedStack) == 0;

    bool restoresSavedActions = previousActions.size() == signals.size();
    bool installsExpectedActions = true;
    for (const int signal : signals) {
        struct sigaction installedAction = {};
        if (sigaction(signal, nullptr, &installedAction) != 0) {
            installsExpectedActions = false;
            continue;
        }

        restoresSavedActions =
            restoresSavedActions && hasSameAction(actionsBeforeInstall.value(signal), previousActions.value(signal));
        installsExpectedActions = installsExpectedActions && (installedAction.sa_flags & SA_SIGINFO) != 0
            && (installedAction.sa_flags & SA_ONSTACK) != 0 && installedAction.sa_sigaction == testSignalCallback;
    }

    bool restoredSignals = true;
    for (const int signal : signals) {
        if (!previousActions.contains(signal)) {
            restoredSignals = false;
            continue;
        }

        const struct sigaction previousAction = previousActions.value(signal);
        restoredSignals = sigaction(signal, &previousAction, nullptr) == 0 && restoredSignals;
    }
    const bool restoredStack = sigaltstack(&stackBeforeInstall, nullptr) == 0;
    free(installedStack.ss_sp);

    QVERIFY(capturedInstalledStack);
    QVERIFY(installedStack.ss_sp != nullptr);
    QCOMPARE(installedStack.ss_size, size_t(SIGSTKSZ));
    QVERIFY((installedStack.ss_flags & SS_DISABLE) == 0);
    QVERIFY(restoresSavedActions);
    QVERIFY(installsExpectedActions);
    QVERIFY(restoredSignals);
    QVERIFY(restoredStack);
}

QTEST_APPLESS_MAIN(KisCrashSignalHandlerSetupContractTest)

#include "KisCrashSignalHandlerSetupContractTest.moc"
