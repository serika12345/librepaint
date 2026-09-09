/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../ui/kis_clipboard.h"
#include "../ui/kis_mimedata.h"

#include <QImage>

#include <type_traits>
#include <utility>

#include <QTest>

namespace
{

#define ASSERT_CLIPBOARD_MEMBER(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisClipboard::method)), signature>)
#define ASSERT_MIME_MEMBER(method, signature)                                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisMimeData::method)), signature>)

} // namespace

class KisClipboardSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void clipboardTypeLifetimeAndSingletonSchemaRemainStable();
    void clipboardPastePolicySchemaRemainStable();
    void clipboardContentExtractionSignaturesRemainStable();
    void clipboardContentAvailabilitySignaturesRemainStable();
    void clipboardMutationLayerExchangeAndNotificationSignaturesRemainStable();
    void mimeDataTypeAndConstructionSchemaRemainStable();
    void mimeDataInsertionInterfaceSchemaRemainStable();
    void mimeDataStoredNodeSignaturesRemainStable();
    void mimeDataLoadingSignaturesRemainStable();
    void mimeDataLayerExchangeSignaturesRemainStable();
};

void KisClipboardSchemaContractTest::clipboardTypeLifetimeAndSingletonSchemaRemainStable()
{
    static_assert(std::is_class_v<KisClipboard>);
    static_assert(std::is_base_of_v<QObject, KisClipboard>);
    static_assert(std::is_default_constructible_v<KisClipboard>);
    static_assert(std::has_virtual_destructor_v<KisClipboard>);
    static_assert(!std::is_copy_constructible_v<KisClipboard>);
    ASSERT_CLIPBOARD_MEMBER(instance, KisClipboard * (*)());

    QVERIFY(true);
}

void KisClipboardSchemaContractTest::clipboardPastePolicySchemaRemainStable()
{
    static_assert(std::is_enum_v<KisClipboard::PasteBehaviour>);
    static_assert(std::is_enum_v<KisClipboard::PasteFormatBehaviour>);
    static_assert(KisClipboard::PasteBehaviour::PASTE_ASSUME_WEB == 0);
    static_assert(KisClipboard::PasteBehaviour::PASTE_ASSUME_MONITOR == 1);
    static_assert(KisClipboard::PasteBehaviour::PASTE_ASK == 2);
    static_assert(KisClipboard::PasteFormatBehaviour::PASTE_FORMAT_ASK == 0);
    static_assert(KisClipboard::PasteFormatBehaviour::PASTE_FORMAT_DOWNLOAD == 1);
    static_assert(KisClipboard::PasteFormatBehaviour::PASTE_FORMAT_LOCAL == 2);
    static_assert(KisClipboard::PasteFormatBehaviour::PASTE_FORMAT_CLIP == 3);

    QVERIFY(true);
}

void KisClipboardSchemaContractTest::clipboardContentExtractionSignaturesRemainStable()
{
    using PasteSource = QPair<bool, KisClipboard::PasteFormatBehaviour>;
    using Clipboard = KisClipboard;

    ASSERT_CLIPBOARD_MEMBER(askUserForSource, PasteSource (Clipboard::*)(const QMimeData *, bool) const);
    ASSERT_CLIPBOARD_MEMBER(askUserForSourceWithData,
                            PasteSource (Clipboard::*)(QImage, const QList<QUrl>, bool) const);
    ASSERT_CLIPBOARD_MEMBER(clip, KisPaintDeviceSP (Clipboard::*)(const QRect &, bool, int, KisTimeSpan *) const);
    ASSERT_CLIPBOARD_MEMBER(
        clipFromBoardContents,
        KisPaintDeviceSP (Clipboard::*)(const QMimeData *, const QRect &, bool, int, bool, PasteSource) const);
    ASSERT_CLIPBOARD_MEMBER(
        clipFromBoardContentsWithData,
        KisPaintDeviceSP (Clipboard::*)(QImage, const QList<QUrl>, const QRect &, bool, int, bool, PasteSource) const);
    ASSERT_CLIPBOARD_MEMBER(clipFromKritaLayers, KisPaintDeviceSP (Clipboard::*)(const KoColorSpace *) const);
    ASSERT_CLIPBOARD_MEMBER(
        clipFromMimeData,
        KisPaintDeviceSP (Clipboard::*)(const QMimeData *, const QRect &, bool, int, KisTimeSpan *, bool) const);
    ASSERT_CLIPBOARD_MEMBER(getImageWithFallback, QImage (Clipboard::*)(const QMimeData *, bool) const);

    static_assert(
        std::is_same_v<decltype(std::declval<const Clipboard &>().askUserForSource(std::declval<const QMimeData *>())),
                       PasteSource>);
    static_assert(std::is_same_v<decltype(std::declval<const Clipboard &>().askUserForSourceWithData(
                                     std::declval<QImage>(),
                                     std::declval<const QList<QUrl>>())),
                                 PasteSource>);
    static_assert(std::is_same_v<decltype(std::declval<const Clipboard &>().clip(std::declval<const QRect &>(),
                                                                                 std::declval<bool>())),
                                 KisPaintDeviceSP>);
    static_assert(std::is_same_v<decltype(std::declval<const Clipboard &>().clipFromBoardContents(
                                     std::declval<const QMimeData *>(),
                                     std::declval<const QRect &>(),
                                     std::declval<bool>())),
                                 KisPaintDeviceSP>);
    static_assert(std::is_same_v<decltype(std::declval<const Clipboard &>().clipFromBoardContentsWithData(
                                     std::declval<QImage>(),
                                     std::declval<const QList<QUrl>>(),
                                     std::declval<const QRect &>(),
                                     std::declval<bool>())),
                                 KisPaintDeviceSP>);
    static_assert(
        std::is_same_v<decltype(std::declval<const Clipboard &>().clipFromMimeData(std::declval<const QMimeData *>(),
                                                                                   std::declval<const QRect &>(),
                                                                                   std::declval<bool>())),
                       KisPaintDeviceSP>);
    static_assert(std::is_same_v<decltype(std::declval<const Clipboard &>().getImageWithFallback(
                                     std::declval<const QMimeData *>())),
                                 QImage>);

    QVERIFY(true);
}

void KisClipboardSchemaContractTest::clipboardContentAvailabilitySignaturesRemainStable()
{
    using Clipboard = KisClipboard;

    ASSERT_CLIPBOARD_MEMBER(clipSize, QSize (Clipboard::*)() const);
    ASSERT_CLIPBOARD_MEMBER(hasClip, bool (Clipboard::*)() const);
    ASSERT_CLIPBOARD_MEMBER(hasImage, bool (Clipboard::*)() const);
    ASSERT_CLIPBOARD_MEMBER(hasLayerStyles, bool (Clipboard::*)() const);
    ASSERT_CLIPBOARD_MEMBER(hasLayers, bool (Clipboard::*)() const);
    ASSERT_CLIPBOARD_MEMBER(hasUrls, bool (Clipboard::*)() const);

    QVERIFY(true);
}

void KisClipboardSchemaContractTest::clipboardMutationLayerExchangeAndNotificationSignaturesRemainStable()
{
    using Clipboard = KisClipboard;
    using SetClip = void (Clipboard::*)(KisPaintDeviceSP, const QPoint &);
    using SetClipWithRange = void (Clipboard::*)(KisPaintDeviceSP, const QPoint &, const KisTimeSpan &);

    ASSERT_CLIPBOARD_MEMBER(clipChanged, void (Clipboard::*)());
    ASSERT_CLIPBOARD_MEMBER(layersMimeData, const QMimeData *(Clipboard::*)() const);
    ASSERT_CLIPBOARD_MEMBER(setClip, SetClip);
    ASSERT_CLIPBOARD_MEMBER(setClip, SetClipWithRange);
    ASSERT_CLIPBOARD_MEMBER(setLayers, void (Clipboard::*)(KisNodeList, KisImageSP, bool));

    static_assert(std::is_same_v<decltype(std::declval<Clipboard &>().setLayers(std::declval<KisNodeList>(),
                                                                                std::declval<KisImageSP>())),
                                 void>);

    QVERIFY(true);
}

void KisClipboardSchemaContractTest::mimeDataTypeAndConstructionSchemaRemainStable()
{
    using MimeData = KisMimeData;

    static_assert(std::is_class_v<MimeData>);
    static_assert(std::is_base_of_v<QMimeData, MimeData>);
    static_assert(std::is_constructible_v<MimeData, QList<KisNodeSP>, KisImageSP, bool>);
    static_assert(
        std::is_same_v<decltype(MimeData(std::declval<QList<KisNodeSP>>(), std::declval<KisImageSP>())), MimeData>);

    QVERIFY(true);
}

void KisClipboardSchemaContractTest::mimeDataInsertionInterfaceSchemaRemainStable()
{
    using Insertion = KisMimeData::NodeInsertionInterface;

    static_assert(std::is_class_v<Insertion>);
    static_assert(std::is_abstract_v<Insertion>);
    static_assert(std::has_virtual_destructor_v<Insertion>);
    static_assert(
        std::is_same_v<decltype(&Insertion::moveNodes), void (Insertion::*)(KisNodeList, KisNodeSP, KisNodeSP)>);
    static_assert(
        std::is_same_v<decltype(&Insertion::addNodes), void (Insertion::*)(KisNodeList, KisNodeSP, KisNodeSP)>);

    QVERIFY(true);
}

void KisClipboardSchemaContractTest::mimeDataStoredNodeSignaturesRemainStable()
{
    using MimeData = KisMimeData;

    ASSERT_MIME_MEMBER(nodes, QList<KisNodeSP> (MimeData::*)() const);
    ASSERT_MIME_MEMBER(deepCopyNodes, void (MimeData::*)());
    ASSERT_MIME_MEMBER(formats, QStringList (MimeData::*)() const);

    QVERIFY(true);
}

void KisClipboardSchemaContractTest::mimeDataLoadingSignaturesRemainStable()
{
    ASSERT_MIME_MEMBER(displayConfigForMimePastes, KisDisplayConfig (*)());
    ASSERT_MIME_MEMBER(isNodeMimeDataFromSameImage, bool (*)(const QMimeData *, KisImageSP));
    ASSERT_MIME_MEMBER(loadNodesFast, KisNodeList (*)(const QMimeData *, KisImageSP, KisShapeController *, bool &));
    ASSERT_MIME_MEMBER(loadNodesFastAndRecenter,
                       KisNodeList (*)(const QPoint &, const QMimeData *, KisImageSP, KisShapeController *, bool &));

    QVERIFY(true);
}

void KisClipboardSchemaContractTest::mimeDataLayerExchangeSignaturesRemainStable()
{
    using MimeData = KisMimeData;
    using MimeFactory = QMimeData *(*)(const KisNodeList &, KisImageSP, bool);
    using Insert = bool (*)(const QMimeData *,
                            KisImageSP,
                            KisShapeController *,
                            KisNodeDummy *,
                            KisNodeDummy *,
                            bool,
                            KisMimeData::NodeInsertionInterface *,
                            bool,
                            QPointF,
                            KisProcessingApplicator *);

    ASSERT_MIME_MEMBER(mimeForLayers, MimeFactory);
    ASSERT_MIME_MEMBER(mimeForLayersDeepCopy, MimeFactory);
    ASSERT_MIME_MEMBER(insertMimeLayers, Insert);
    static_assert(std::is_same_v<decltype(MimeData::mimeForLayers(std::declval<const KisNodeList &>(),
                                                                  std::declval<KisImageSP>())),
                                 QMimeData *>);
    static_assert(
        std::is_same_v<decltype(MimeData::insertMimeLayers(std::declval<const QMimeData *>(),
                                                           std::declval<KisImageSP>(),
                                                           std::declval<KisShapeController *>(),
                                                           std::declval<KisNodeDummy *>(),
                                                           std::declval<KisNodeDummy *>(),
                                                           std::declval<bool>(),
                                                           std::declval<KisMimeData::NodeInsertionInterface *>())),
                       bool>);

    QVERIFY(true);
}

#undef ASSERT_CLIPBOARD_MEMBER
#undef ASSERT_MIME_MEMBER

QTEST_APPLESS_MAIN(KisClipboardSchemaContractTest)

#include "KisClipboardSchemaContractTest.moc"
