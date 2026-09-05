/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <ui/orchestration/KisApplicationArguments.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_APPLICATION_ARGUMENTS_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisApplicationArguments::method)), signature>)
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

#undef ASSERT_APPLICATION_ARGUMENTS_SIGNATURE

QTEST_GUILESS_MAIN(KisApplicationArgumentsSchemaContractTest)

#include "KisApplicationArgumentsSchemaContractTest.moc"
