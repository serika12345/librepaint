/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisAndroidLogHandler.h>
#include <KisMessageBoxWrapper.h>
#include <KisUsageLogger.h>
#include <kis_assert.h>

#include <QTest>

#include <type_traits>

class KisGlobalDiagnosticsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void usageLoggerTypeLifetimeAndConstructionSchemaRemainStable();
    void usageLoggerLifecycleAndInformationSchemaRemainStable();
    void usageLoggerRecordingSchemaRemainsStable();
    void assertionEntrypointSchemaRemainsStable();
    void messageAndAndroidLogEntrypointSchemaRemainStable();
};

void KisGlobalDiagnosticsSchemaContractTest::usageLoggerTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisUsageLogger>);
    static_assert(std::is_default_constructible_v<KisUsageLogger>);
    static_assert(std::is_destructible_v<KisUsageLogger>);
}

void KisGlobalDiagnosticsSchemaContractTest::usageLoggerLifecycleAndInformationSchemaRemainStable()
{
    using LifecycleSignature = void (*)();
    using InformationSignature = QString (*)();

    static_assert(std::is_same_v<decltype(&KisUsageLogger::initialize), LifecycleSignature>);
    static_assert(std::is_same_v<decltype(&KisUsageLogger::close), LifecycleSignature>);
    static_assert(std::is_same_v<decltype(&KisUsageLogger::basicSystemInfo), InformationSignature>);
    static_assert(std::is_same_v<decltype(&KisUsageLogger::screenInformation), InformationSignature>);
}

void KisGlobalDiagnosticsSchemaContractTest::usageLoggerRecordingSchemaRemainsStable()
{
    using MessageSignature = void (*)(const QString &);
    using NoArgumentSignature = void (*)();

    static_assert(std::is_same_v<decltype(&KisUsageLogger::log), MessageSignature>);
    static_assert(std::is_same_v<decltype(&KisUsageLogger::writeSysInfo), MessageSignature>);
    static_assert(std::is_same_v<decltype(&KisUsageLogger::writeHeader), NoArgumentSignature>);
    static_assert(std::is_same_v<decltype(&KisUsageLogger::writeLocaleSysInfo), NoArgumentSignature>);
}

void KisGlobalDiagnosticsSchemaContractTest::assertionEntrypointSchemaRemainsStable()
{
    using AssertionSignature = void (*)(const char *, const char *, int);
    using ContextualAssertionSignature = void (*)(const char *, const char *, const char *, const char *, int);

    static_assert(std::is_same_v<decltype(&kis_assert_exception), AssertionSignature>);
    static_assert(std::is_same_v<decltype(&kis_assert_recoverable), AssertionSignature>);
    static_assert(std::is_same_v<decltype(&kis_safe_assert_recoverable), AssertionSignature>);
    static_assert(std::is_same_v<decltype(&kis_assert_x_exception), ContextualAssertionSignature>);
}

void KisGlobalDiagnosticsSchemaContractTest::messageAndAndroidLogEntrypointSchemaRemainStable()
{
    using MessageBoxSignature = int (*)(QMessageBox *, const QString &);
    using AndroidLogSignature = void (*)();

    static_assert(std::is_same_v<decltype(&KisMessageBoxWrapper::doNotAskAgainMessageBoxWrapper), MessageBoxSignature>);
    static_assert(std::is_same_v<decltype(&KisAndroidLogHandler::handler_init), AndroidLogSignature>);
}

QTEST_GUILESS_MAIN(KisGlobalDiagnosticsSchemaContractTest)

#include "KisGlobalDiagnosticsSchemaContractTest.moc"
