/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <ui/orchestration/KisPart.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_PART_SIGNATURE(method, signature)                                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPart::method)), signature>)
} // namespace

class KisPartSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void partTypeLifetimeAndSingletonSchemaRemainStable();
    void documentAndViewSignaturesRemainStable();
    void windowSignaturesRemainStable();
    void sessionAndFileSignaturesRemainStable();
    void cacheAndPlaybackSignaturesRemainStable();
};

void KisPartSchemaContractTest::partTypeLifetimeAndSingletonSchemaRemainStable()
{
    using Part = KisPart;

    static_assert(std::is_same_v<KisImageSP, KisSharedPtr<KisImage>>);
    static_assert(std::is_class_v<Part>);
    static_assert(std::is_default_constructible_v<Part>);
    static_assert(std::has_virtual_destructor_v<Part>);
    ASSERT_PART_SIGNATURE(instance, Part * (*)());
    ASSERT_PART_SIGNATURE(exists, bool (*)());
}

void KisPartSchemaContractTest::documentAndViewSignaturesRemainStable()
{
    using Part = KisPart;
    using DocumentList = QList<QPointer<KisDocument>>;
    using ViewList = QList<QPointer<KisView>>;

    ASSERT_PART_SIGNATURE(addDocument, void (Part::*)(KisDocument *, bool));
    ASSERT_PART_SIGNATURE(createDocument, KisDocument * (Part::*)() const);
    ASSERT_PART_SIGNATURE(createTemporaryDocument, KisDocument * (Part::*)() const);
    ASSERT_PART_SIGNATURE(documentCount, int (Part::*)() const);
    ASSERT_PART_SIGNATURE(documents, DocumentList (Part::*)() const);
    ASSERT_PART_SIGNATURE(removeDocument, void (Part::*)(KisDocument *, bool));
    ASSERT_PART_SIGNATURE(documentClosed, void (Part::*)(const QString &));
    ASSERT_PART_SIGNATURE(documentOpened, void (Part::*)(const QString &));
    ASSERT_PART_SIGNATURE(sigDocumentAdded, void (Part::*)(KisDocument *));
    ASSERT_PART_SIGNATURE(sigDocumentRemoved, void (Part::*)(const QString &));
    ASSERT_PART_SIGNATURE(sigDocumentSaved, void (Part::*)(const QString &));
    ASSERT_PART_SIGNATURE(startCustomDocument, void (Part::*)(KisDocument *));
    ASSERT_PART_SIGNATURE(addView, void (Part::*)(KisView *));
    ASSERT_PART_SIGNATURE(createView, KisView * (Part::*)(KisDocument *, KisViewManager *, QWidget *));
    ASSERT_PART_SIGNATURE(currentInputManager, KisInputManager * (Part::*)());
    ASSERT_PART_SIGNATURE(removeView, void (Part::*)(KisView *));
    ASSERT_PART_SIGNATURE(sigViewAdded, void (Part::*)(KisView *));
    ASSERT_PART_SIGNATURE(sigViewRemoved, void (Part::*)(KisView *));
    ASSERT_PART_SIGNATURE(viewCount, int (Part::*)(KisDocument *) const);
    ASSERT_PART_SIGNATURE(views, ViewList (Part::*)() const);
    static_assert(std::is_same_v<decltype(std::declval<Part &>().addDocument(nullptr)), void>);
    static_assert(std::is_same_v<decltype(std::declval<Part &>().removeDocument(nullptr)), void>);
}

void KisPartSchemaContractTest::windowSignaturesRemainStable()
{
    using Part = KisPart;
    using WindowList = QList<QPointer<KisMainWindow>>;

    ASSERT_PART_SIGNATURE(addRecentURLToAllMainWindows, void (Part::*)(QUrl, QUrl));
    ASSERT_PART_SIGNATURE(queueAddRecentURLToAllMainWindowsOnFileSaved, void (Part::*)(QUrl, QUrl));
    ASSERT_PART_SIGNATURE(createMainWindow, KisMainWindow * (Part::*)(QUuid));
    ASSERT_PART_SIGNATURE(notifyMainWindowIsBeingCreated, void (Part::*)(KisMainWindow *));
    ASSERT_PART_SIGNATURE(removeMainWindow, void (Part::*)(KisMainWindow *));
    ASSERT_PART_SIGNATURE(mainWindows, const WindowList &(Part::*)() const);
    ASSERT_PART_SIGNATURE(mainwindowCount, int (Part::*)() const);
    ASSERT_PART_SIGNATURE(currentMainwindow, KisMainWindow * (Part::*)() const);
    ASSERT_PART_SIGNATURE(currentMainwindowAsQWidget, QWidget * (Part::*)() const);
    ASSERT_PART_SIGNATURE(windowById, KisMainWindow * (Part::*)(QUuid) const);
    ASSERT_PART_SIGNATURE(sigMainWindowCreated, void (Part::*)());
    ASSERT_PART_SIGNATURE(sigMainWindowIsBeingCreated, void (Part::*)(KisMainWindow *));
    static_assert(std::is_same_v<decltype(std::declval<Part &>().addRecentURLToAllMainWindows(QUrl())), void>);
    static_assert(
        std::is_same_v<decltype(std::declval<Part &>().queueAddRecentURLToAllMainWindowsOnFileSaved(QUrl())), void>);
    static_assert(std::is_same_v<decltype(std::declval<Part &>().createMainWindow()), KisMainWindow *>);
}

void KisPartSchemaContractTest::sessionAndFileSignaturesRemainStable()
{
    using Part = KisPart;

    ASSERT_PART_SIGNATURE(closeSession, bool (Part::*)(bool));
    ASSERT_PART_SIGNATURE(closingSession, bool (Part::*)() const);
    ASSERT_PART_SIGNATURE(restoreSession, bool (Part::*)(const QString &));
    ASSERT_PART_SIGNATURE(restoreSession, bool (Part::*)(KisSessionResourceSP));
    ASSERT_PART_SIGNATURE(setCurrentSession, void (Part::*)(KisSessionResourceSP));
    ASSERT_PART_SIGNATURE(showSessionManager, void (Part::*)());
    ASSERT_PART_SIGNATURE(startBlankSession, void (Part::*)());
    ASSERT_PART_SIGNATURE(openExistingFile, void (Part::*)(const QString &));
    ASSERT_PART_SIGNATURE(openTemplate, void (Part::*)(const QUrl &));
    static_assert(std::is_same_v<decltype(std::declval<Part &>().closeSession()), bool>);
}

void KisPartSchemaContractTest::cacheAndPlaybackSignaturesRemainStable()
{
    using Part = KisPart;

    ASSERT_PART_SIGNATURE(cachePopulator, KisAnimationCachePopulator * (Part::*)() const);
    ASSERT_PART_SIGNATURE(idleWatcher, KisIdleWatcher * (Part::*)() const);
    ASSERT_PART_SIGNATURE(playbackEngine, KisPlaybackEngine * (Part::*)() const);
    ASSERT_PART_SIGNATURE(playbackEngineChanged, void (Part::*)(KisPlaybackEngine *));
    ASSERT_PART_SIGNATURE(prioritizeFrameForCache, void (Part::*)(KisImageSP, int));
    ASSERT_PART_SIGNATURE(unloadPlaybackEngine, void (Part::*)());
    ASSERT_PART_SIGNATURE(upgradeToPlaybackEngineMLT, void (Part::*)(KoCanvasBase *));
}

#undef ASSERT_PART_SIGNATURE

QTEST_APPLESS_MAIN(KisPartSchemaContractTest)

#include "KisPartSchemaContractTest.moc"
