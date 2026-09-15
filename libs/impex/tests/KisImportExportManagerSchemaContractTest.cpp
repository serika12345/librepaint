/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../ui/KisImportExportManager.h"
#include "KisExportCheckRegistry.h"
#include "KisImportExportColorSpaceDialog.h"
#include "KisImportExportFilter.h"

#include <QFuture>

#include <type_traits>
#include <utility>

#include <QTest>

namespace
{

#define ASSERT_MANAGER_MEMBER(method, signature)                                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisImportExportManager::method)), signature>)

} // namespace

class KisImportExportManagerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void exportCheckRegistryLifetimeSchemaRemainsStable();
    void importExportColorSpaceSelectionSignatureRemainsStable();
    void typeDirectionConstructionAndLifetimeSchemaRemainStable();
    void documentConversionSignaturesRemainStable();
    void filterDiscoveryAndConfigurationSignaturesRemainStable();
    void batchAndProgressSignaturesRemainStable();
    void additionalFileQuerySignaturesRemainStable();
    void importExportFilterLifetimeSchemaRemainsStable();
};

void KisImportExportManagerSchemaContractTest::exportCheckRegistryLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisExportCheckRegistry>);
    static_assert(std::is_base_of_v<QObject, KisExportCheckRegistry>);
    static_assert(std::has_virtual_destructor_v<KisExportCheckRegistry>);
}

void KisImportExportManagerSchemaContractTest::importExportColorSpaceSelectionSignatureRemainsStable()
{
    using SelectEditableColorSpaceSignature = bool (*)(QWidget *,
                                                       const KoColorSpace *,
                                                       const KoColorSpace **,
                                                       KoColorConversionTransformation::Intent *,
                                                       KoColorConversionTransformation::ConversionFlags *);

    static_assert(std::is_same_v<decltype(&KisImportExportColorSpaceDialog::selectEditableColorSpace),
                                 SelectEditableColorSpaceSignature>);
}

void KisImportExportManagerSchemaContractTest::typeDirectionConstructionAndLifetimeSchemaRemainStable()
{
    using Manager = KisImportExportManager;

    static_assert(std::is_class_v<Manager>);
    static_assert(std::is_base_of_v<QObject, Manager>);
    static_assert(std::is_enum_v<Manager::Direction>);
    static_assert(Manager::Import == static_cast<Manager::Direction>(1));
    static_assert(Manager::Export == static_cast<Manager::Direction>(2));
    static_assert(std::is_constructible_v<Manager, KisDocument *>);
    static_assert(std::has_virtual_destructor_v<Manager>);
    static_assert(!std::is_copy_constructible_v<Manager>);

    QVERIFY(true);
}

void KisImportExportManagerSchemaContractTest::importExportFilterLifetimeSchemaRemainsStable()
{
    static_assert(std::is_base_of_v<QObject, KisImportExportFilter>);
    static_assert(std::is_abstract_v<KisImportExportFilter>);
    static_assert(std::has_virtual_destructor_v<KisImportExportFilter>);
}

void KisImportExportManagerSchemaContractTest::documentConversionSignaturesRemainStable()
{
    using Manager = KisImportExportManager;
    using Export = KisImportExportErrorCode (
        Manager::*)(const QString &, const QString &, const QByteArray &, bool, KisPropertiesConfigurationSP, bool);
    using ExportAsync = QFuture<KisImportExportErrorCode> (Manager::*)(const QString &,
                                                                       const QString &,
                                                                       const QByteArray &,
                                                                       KisImportExportErrorCode &,
                                                                       bool,
                                                                       KisPropertiesConfigurationSP,
                                                                       bool);

    ASSERT_MANAGER_MEMBER(importDocument, KisImportExportErrorCode (Manager::*)(const QString &, const QString &));
    ASSERT_MANAGER_MEMBER(exportDocument, Export);
    ASSERT_MANAGER_MEMBER(exportDocumentAsync, ExportAsync);
    static_assert(std::is_same_v<decltype(std::declval<Manager &>().exportDocument(std::declval<const QString &>(),
                                                                                   std::declval<const QString &>(),
                                                                                   std::declval<const QByteArray &>())),
                                 KisImportExportErrorCode>);
    static_assert(std::is_same_v<decltype(std::declval<Manager &>().exportDocumentAsync(
                                     std::declval<const QString &>(),
                                     std::declval<const QString &>(),
                                     std::declval<const QByteArray &>(),
                                     std::declval<KisImportExportErrorCode &>())),
                                 QFuture<KisImportExportErrorCode>>);
}

void KisImportExportManagerSchemaContractTest::filterDiscoveryAndConfigurationSignaturesRemainStable()
{
    using Manager = KisImportExportManager;

    ASSERT_MANAGER_MEMBER(supportedMimeTypes, QStringList (*)(Manager::Direction));
    ASSERT_MANAGER_MEMBER(filterForMimeType, KisImportExportFilter * (*)(const QString &, Manager::Direction));
    ASSERT_MANAGER_MEMBER(fillStaticExportConfigurationProperties, void (*)(KisPropertiesConfigurationSP, KisImageSP));
}

void KisImportExportManagerSchemaContractTest::batchAndProgressSignaturesRemainStable()
{
    using Manager = KisImportExportManager;

    ASSERT_MANAGER_MEMBER(batchMode, bool (Manager::*)() const);
    ASSERT_MANAGER_MEMBER(setUpdater, void (Manager::*)(KoUpdaterPtr));
}

void KisImportExportManagerSchemaContractTest::additionalFileQuerySignaturesRemainStable()
{
    using FileQuery = QString (*)(const QString &, QWidget *);

    ASSERT_MANAGER_MEMBER(askForAudioFileName, FileQuery);
    ASSERT_MANAGER_MEMBER(getUriForAdditionalFile, FileQuery);
}

#undef ASSERT_MANAGER_MEMBER

QTEST_APPLESS_MAIN(KisImportExportManagerSchemaContractTest)

#include "KisImportExportManagerSchemaContractTest.moc"
