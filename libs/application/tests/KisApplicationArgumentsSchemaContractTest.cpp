/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <canvas/KisAndroidScaling.h>
#include <ui/orchestration/KisApplication.h>
#include <ui/orchestration/KisApplicationArguments.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisApplicationArguments::method)), signature>)
#define ASSERT_APPLICATION_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisApplication::method)), signature>)
#define ASSERT_ANDROID_SCALING_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAndroidScaling::method)), signature>)
} // namespace

class KisApplicationArgumentsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void applicationArgumentsTypeLifetimeAndValueSemanticsSchemaRemainStable();
    void applicationArgumentsSerializationSignaturesRemainStable();
    void applicationArgumentsDocumentInputSignaturesRemainStable();
    void applicationArgumentsTemplateAndExportSignaturesRemainStable();
    void applicationArgumentsPresentationAndWorkspaceSignaturesRemainStable();
    void applicationTypeConstructionAndLifetimeSchemaRemainStable();
    void applicationStartupAndEventSignaturesRemainStable();
    void applicationResourceAndPluginInitializationSignaturesRemainStable();
    void applicationSplashAndExternalInterfaceSignaturesRemainStable();
    void applicationRemoteArgumentAndFileNotificationSignaturesRemainStable();
    void androidScalingTypeConstructionAndSingletonSchemaRemainStable();
    void androidScalingSupportAndDialogSignaturesRemainStable();
    void androidScalingInterfaceNotificationSignaturesRemainStable();
    void androidScalingPersistenceNotificationSignaturesRemainStable();
    void androidScalingDialogEntryPointSignaturesRemainStable();
};

void KisApplicationArgumentsSchemaContractTest::applicationArgumentsTypeLifetimeAndValueSemanticsSchemaRemainStable()
{
    static_assert(std::is_class_v<KisApplicationArguments>);
    static_assert(std::is_constructible_v<KisApplicationArguments, const QApplication &>);
    static_assert(std::is_copy_constructible_v<KisApplicationArguments>);
    static_assert(std::is_destructible_v<KisApplicationArguments>);
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(operator=,
                                           void (KisApplicationArguments::*)(const KisApplicationArguments &));
}

void KisApplicationArgumentsSchemaContractTest::applicationArgumentsSerializationSignaturesRemainStable()
{
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(deserialize, KisApplicationArguments (*)(QByteArray &));
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(serialize, QByteArray (KisApplicationArguments::*)());
}

void KisApplicationArgumentsSchemaContractTest::applicationArgumentsDocumentInputSignaturesRemainStable()
{
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(createDocumentFromArguments,
                                           KisDocument * (KisApplicationArguments::*)() const);
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(doNewImage, bool (KisApplicationArguments::*)() const);
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(fileLayer, QString (KisApplicationArguments::*)() const);
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(filenames, QStringList (KisApplicationArguments::*)() const);
}

void KisApplicationArgumentsSchemaContractTest::applicationArgumentsTemplateAndExportSignaturesRemainStable()
{
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(doTemplate, bool (KisApplicationArguments::*)() const);
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(exportAs, bool (KisApplicationArguments::*)() const);
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(exportFileName, QString (KisApplicationArguments::*)() const);
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(exportSequence, bool (KisApplicationArguments::*)() const);
}

void KisApplicationArgumentsSchemaContractTest::applicationArgumentsPresentationAndWorkspaceSignaturesRemainStable()
{
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(canvasOnly, bool (KisApplicationArguments::*)() const);
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(fullScreen, bool (KisApplicationArguments::*)() const);
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(noSplash, bool (KisApplicationArguments::*)() const);
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(session, QString (KisApplicationArguments::*)() const);
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(windowLayout, QString (KisApplicationArguments::*)() const);
    ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(workspace, QString (KisApplicationArguments::*)() const);
}

void KisApplicationArgumentsSchemaContractTest::applicationTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisApplication>);
    static_assert(std::is_base_of_v<QtSingleApplication, KisApplication>);
    static_assert(std::is_constructible_v<KisApplication, const QString &, int &, char **>);
    static_assert(std::has_virtual_destructor_v<KisApplication>);
}

void KisApplicationArgumentsSchemaContractTest::applicationStartupAndEventSignaturesRemainStable()
{
    ASSERT_APPLICATION_SIGNATURE(start, bool (KisApplication::*)(const KisApplicationArguments &));
    ASSERT_APPLICATION_SIGNATURE(event, bool (KisApplication::*)(QEvent *));
    ASSERT_APPLICATION_SIGNATURE(notify, bool (KisApplication::*)(QObject *, QEvent *));
    ASSERT_APPLICATION_SIGNATURE(askResetConfig, void (KisApplication::*)());
}

void KisApplicationArgumentsSchemaContractTest::applicationResourceAndPluginInitializationSignaturesRemainStable()
{
    ASSERT_APPLICATION_SIGNATURE(addResourceTypes, void (KisApplication::*)());
    ASSERT_APPLICATION_SIGNATURE(registerResources, bool (KisApplication::*)());
    ASSERT_APPLICATION_SIGNATURE(loadPlugins, void (KisApplication::*)());
    ASSERT_APPLICATION_SIGNATURE(initializeGlobals, void (KisApplication::*)(const KisApplicationArguments &));
    ASSERT_APPLICATION_SIGNATURE(processPostponedSynchronizationEvents, void (KisApplication::*)());
    static_assert(std::is_same_v<decltype(&KisApplication::verifyMetatypeRegistration), void (*)()>);
    static_assert(std::is_same_v<decltype(&KisApplication::setWidgetStyle), void (*)(const QString &)>);
}

void KisApplicationArgumentsSchemaContractTest::applicationSplashAndExternalInterfaceSignaturesRemainStable()
{
    ASSERT_APPLICATION_SIGNATURE(setSplashScreen, void (KisApplication::*)(QWidget *));
    ASSERT_APPLICATION_SIGNATURE(hideSplashScreen, void (KisApplication::*)());
    ASSERT_APPLICATION_SIGNATURE(setSplashScreenLoadingText, void (KisApplication::*)(const QString &));
    ASSERT_APPLICATION_SIGNATURE(extendedModifiersPluginInterface,
                                 KisExtendedModifiersMapperPluginInterface * (KisApplication::*)());
#ifdef Q_OS_ANDROID
    ASSERT_APPLICATION_SIGNATURE(androidSplash, KisAndroidSplash * (KisApplication::*)());
    ASSERT_APPLICATION_SIGNATURE(androidScaling, KisAndroidScaling * (KisApplication::*)());
#endif
}

void KisApplicationArgumentsSchemaContractTest::applicationRemoteArgumentAndFileNotificationSignaturesRemainStable()
{
    ASSERT_APPLICATION_SIGNATURE(executeRemoteArguments, void (KisApplication::*)(QByteArray, KisMainWindow *));
    ASSERT_APPLICATION_SIGNATURE(remoteArguments, void (KisApplication::*)(const QString &));
    ASSERT_APPLICATION_SIGNATURE(fileOpenRequested, void (KisApplication::*)(const QString &));
}

void KisApplicationArgumentsSchemaContractTest::androidScalingTypeConstructionAndSingletonSchemaRemainStable()
{
    static_assert(std::is_class_v<KisAndroidScaling>);
    static_assert(std::is_base_of_v<QObject, KisAndroidScaling>);
    static_assert(std::is_constructible_v<KisAndroidScaling, KisConfig &, KisApplication *>);
    static_assert(std::is_same_v<decltype(&KisAndroidScaling::instance), KisAndroidScaling *(*)()>);
}

void KisApplicationArgumentsSchemaContractTest::androidScalingSupportAndDialogSignaturesRemainStable()
{
    ASSERT_ANDROID_SCALING_SIGNATURE(isSupported, bool (KisAndroidScaling::*)() const);
    ASSERT_ANDROID_SCALING_SIGNATURE(showDialog, void (KisAndroidScaling::*)());
}

void KisApplicationArgumentsSchemaContractTest::androidScalingInterfaceNotificationSignaturesRemainStable()
{
    ASSERT_ANDROID_SCALING_SIGNATURE(sigInterfaceScaleChanged, void (KisAndroidScaling::*)());
    ASSERT_ANDROID_SCALING_SIGNATURE(sigJniSetPrimaryScreenScale, void (KisAndroidScaling::*)(qreal));
}

void KisApplicationArgumentsSchemaContractTest::androidScalingPersistenceNotificationSignaturesRemainStable()
{
    ASSERT_ANDROID_SCALING_SIGNATURE(sigJniSaveInterfaceScale, void (KisAndroidScaling::*)(bool));
    ASSERT_ANDROID_SCALING_SIGNATURE(sigJniScalingDialogActive, void (KisAndroidScaling::*)(bool));
}

void KisApplicationArgumentsSchemaContractTest::androidScalingDialogEntryPointSignaturesRemainStable()
{
    static_assert(std::is_same_v<decltype(&KisAndroidScalingDialog::init), void (*)()>);
    static_assert(std::is_same_v<decltype(&KisAndroidScalingDialog::setInitialScale), void (*)(KisConfig &)>);
    static_assert(std::is_same_v<decltype(&KisAndroidScalingDialog::show), void (*)()>);
    static_assert(std::is_same_v<decltype(&KisAndroidScalingDialog::maybeShowOnStartup), void (*)()>);
    static_assert(std::is_same_v<decltype(&KisAndroidScalingDialog::isSupported), bool (*)()>);
}

#undef ASSERT_APPLICATION_ARGUMENTS_SIGNATURE
#undef ASSERT_APPLICATION_SIGNATURE
#undef ASSERT_ANDROID_SCALING_SIGNATURE

QTEST_GUILESS_MAIN(KisApplicationArgumentsSchemaContractTest)

#include "KisApplicationArgumentsSchemaContractTest.moc"
