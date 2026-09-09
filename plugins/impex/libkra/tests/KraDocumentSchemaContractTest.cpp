/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_kra_saver.h"
#include "kra_converter.h"

#include <QTest>

#include <type_traits>
#include <utility>

class KraDocumentSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void kraSaverTypeConstructionAndLifetimeSchemaRemainStable();
    void kraSaverDocumentContentSignaturesRemainStable();
    void kraSaverAnimationAudioAndDiagnosticSignaturesRemainStable();
    void kraConverterTypeConstructionAndLifetimeSchemaRemainStable();
    void kraConverterOperationAndResultSignaturesRemainStable();
};

void KraDocumentSchemaContractTest::kraSaverTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisKraSaver>);
    static_assert(std::is_constructible_v<KisKraSaver, KisDocument *, const QString &, bool>);
    static_assert(
        std::is_same_v<decltype(KisKraSaver(static_cast<KisDocument *>(nullptr), std::declval<const QString &>())),
                       KisKraSaver>);
    static_assert(std::is_destructible_v<KisKraSaver>);
}

void KraDocumentSchemaContractTest::kraSaverDocumentContentSignaturesRemainStable()
{
    using Saver = KisKraSaver;

    static_assert(std::is_same_v<decltype(&Saver::saveXML), QDomElement (Saver::*)(QDomDocument &, KisImageSP)>);
    static_assert(std::is_same_v<decltype(&Saver::saveKeyframes), bool (Saver::*)(KoStore *, const QString &, bool)>);
    static_assert(std::is_same_v<decltype(&Saver::saveBinaryData),
                                 bool (Saver::*)(KoStore *, KisImageSP, const QString &, bool, bool)>);
    static_assert(
        std::is_same_v<decltype(&Saver::saveResources), bool (Saver::*)(KoStore *, KisImageSP, const QString &)>);
    static_assert(
        std::is_same_v<decltype(&Saver::saveStoryboard), bool (Saver::*)(KoStore *, KisImageSP, const QString &)>);
}

void KraDocumentSchemaContractTest::kraSaverAnimationAudioAndDiagnosticSignaturesRemainStable()
{
    using Saver = KisKraSaver;

    static_assert(std::is_same_v<decltype(&Saver::saveAnimationMetadata),
                                 bool (Saver::*)(KoStore *, KisImageSP, const QString &)>);
    static_assert(std::is_same_v<decltype(&Saver::saveAudio), bool (Saver::*)(KoStore *)>);
    static_assert(std::is_same_v<decltype(&Saver::errorMessages), QStringList (Saver::*)() const>);
    static_assert(std::is_same_v<decltype(&Saver::warningMessages), QStringList (Saver::*)() const>);
}

void KraDocumentSchemaContractTest::kraConverterTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KraConverter>);
    static_assert(std::is_constructible_v<KraConverter, KisDocument *>);
    static_assert(std::is_constructible_v<KraConverter, KisDocument *, QPointer<KoUpdater>>);
    static_assert(std::has_virtual_destructor_v<KraConverter>);
}

void KraDocumentSchemaContractTest::kraConverterOperationAndResultSignaturesRemainStable()
{
    using Converter = KraConverter;

    static_assert(
        std::is_same_v<decltype(&Converter::buildImage), KisImportExportErrorCode (Converter::*)(QIODevice *)>);
    static_assert(std::is_same_v<decltype(&Converter::buildFile),
                                 KisImportExportErrorCode (Converter::*)(QIODevice *, const QString &, bool)>);
    static_assert(std::is_same_v<decltype(std::declval<Converter &>().buildFile(static_cast<QIODevice *>(nullptr),
                                                                                std::declval<const QString &>())),
                                 KisImportExportErrorCode>);
    static_assert(std::is_same_v<decltype(&Converter::image), KisImageSP (Converter::*)()>);
    static_assert(std::is_same_v<decltype(&Converter::activeNodes), vKisNodeSP (Converter::*)()>);
    static_assert(std::is_same_v<decltype(&Converter::assistants), QList<KisPaintingAssistantSP> (Converter::*)()>);
    static_assert(std::is_same_v<decltype(&Converter::storyboardItemList), StoryboardItemList (Converter::*)()>);
    static_assert(std::is_same_v<decltype(&Converter::storyboardCommentList), StoryboardCommentList (Converter::*)()>);
    static_assert(std::is_same_v<decltype(&Converter::cancel), void (Converter::*)()>);
}

QTEST_GUILESS_MAIN(KraDocumentSchemaContractTest)

#include "KraDocumentSchemaContractTest.moc"
