/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../ui/kis_clipboard.h"

#include <QImage>

#include <type_traits>
#include <utility>

#include <QTest>

namespace
{

#define ASSERT_CLIPBOARD_MEMBER(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisClipboard::method)), signature>)

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

#undef ASSERT_CLIPBOARD_MEMBER

QTEST_APPLESS_MAIN(KisClipboardSchemaContractTest)

#include "KisClipboardSchemaContractTest.moc"
