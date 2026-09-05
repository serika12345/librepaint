/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <Document.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_DOCUMENT_ANIMATION_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Document::method)), signature>)
} // namespace

class DocumentAnimationSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void documentAnimationTimelineQuerySignaturesRemainStable();
    void documentAnimationTimelineMutationSignaturesRemainStable();
    void documentAudioLevelSignaturesRemainStable();
    void documentAudioTrackSignaturesRemainStable();
    void documentAnimationImportSignatureRemainsStable();
};

void DocumentAnimationSchemaContractTest::documentAnimationTimelineQuerySignaturesRemainStable()
{
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(animationLength, int (Document::*)());
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(currentTime, int (Document::*)());
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(framesPerSecond, int (Document::*)());
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(fullClipRangeEndTime, int (Document::*)());
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(fullClipRangeStartTime, int (Document::*)());
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(playBackEndTime, int (Document::*)());
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(playBackStartTime, int (Document::*)());
}

void DocumentAnimationSchemaContractTest::documentAnimationTimelineMutationSignaturesRemainStable()
{
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(setCurrentTime, void (Document::*)(int));
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(setFramesPerSecond, void (Document::*)(int));
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(setFullClipRangeEndTime, void (Document::*)(int));
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(setFullClipRangeStartTime, void (Document::*)(int));
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(setPlayBackRange, void (Document::*)(int, int));
}

void DocumentAnimationSchemaContractTest::documentAudioLevelSignaturesRemainStable()
{
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(audioLevel, qreal (Document::*)() const);
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(setAudioLevel, void (Document::*)(qreal));
}

void DocumentAnimationSchemaContractTest::documentAudioTrackSignaturesRemainStable()
{
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(audioTracks, QList<QString> (Document::*)() const);
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(setAudioTracks, bool (Document::*)(QList<QString>) const);
}

void DocumentAnimationSchemaContractTest::documentAnimationImportSignatureRemainsStable()
{
    ASSERT_DOCUMENT_ANIMATION_SIGNATURE(importAnimation, bool (Document::*)(const QList<QString> &, int, int));
}

#undef ASSERT_DOCUMENT_ANIMATION_SIGNATURE

QTEST_APPLESS_MAIN(DocumentAnimationSchemaContractTest)

#include "DocumentAnimationSchemaContractTest.moc"
