/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCanvasWorkspace.h"
#include "KisGpuDocumentWindow.h"
#include <testui.h>
#include <QElapsedTimer>
#include <QDomDocument>
#include <KoStore.h>
#include <QFileInfo>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QWindow>

namespace {
int saveCycles(const QString &path)
{
    std::unique_ptr<KoStore> store(KoStore::createStore(path, KoStore::Read));
    if (!store || store->bad() || !store->open(QStringLiteral("documentinfo.xml"))) return -1;
    QDomDocument xml;
    if (!xml.setContent(store->read(store->size()))) return -1;
    return xml.documentElement().firstChildElement(QStringLiteral("about"))
        .firstChildElement(QStringLiteral("editing-cycles")).text().toInt();
}
bool draw(KisGpuDocumentWindow &editor, KisGpuApplicationState &application)
{
    QWindow *window = nullptr;
    for (auto *candidate : QGuiApplication::allWindows())
        if (candidate->objectName() == QStringLiteral("gpuDocumentCanvas-%1").arg(editor.documentId())) window = candidate;
    if (!window || !QTest::qWaitForWindowExposed(window)) return false;
    application.applicationStateChanged(Qt::ApplicationActive);
    const QPointF point(window->width() / 2.0, window->height() / 2.0);
    QMouseEvent press(QEvent::MouseButtonPress, point, point, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease, point, point, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &press);
    QCoreApplication::sendEvent(window, &release);
    auto *canvas = editor.findChild<KisGpuCanvasSession *>();
    QElapsedTimer timeout; timeout.start();
    while (canvas->hasPendingInput() && timeout.elapsed() < 5000) QTest::qWait(1);
    return canvas->inputState() == KisGpuStrokeInput::State::Finished;
}
QByteArray pixels(KisGpuDocumentDevice &documents, quint64 id)
{
    const auto model = documents.document(id);
    auto store = documents.store(id);
    const auto read = store->readback(model->layers().first().paint.pixels, QRect(0, 0, 32, 32));
    QElapsedTimer timeout; timeout.start();
    while (read.completion.status() == KisGpuTileStore::Status::Pending && timeout.elapsed() < 5000) {
        store->poll(); QTest::qWait(1);
    }
    return read.bytes();
}
}

/** Product creation, fixed-version save and reopen share the registered GPU lifetime. */
class KisGpuCanvasWorkspaceTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void closingTheWorkspaceDuringOpenReleasesTheUnpublishedDocument() {
        QTemporaryDir directory;
        KisGpuDocumentDevice documents;
        KisGpuApplicationState application(documents);
        auto workspace = std::make_unique<KisGpuCanvasWorkspace>(documents, application);
        auto *opening = workspace.get();
        QTimer::singleShot(0, [&] { workspace.reset(); });
        QVERIFY(!opening->openCanvas(directory.filePath(QStringLiteral("cancelled.kra"))));
        QVERIFY(!documents.hasDocuments());
        QVERIFY(!documents.device());
    }
    void savesFrozenPixelsAndKeepsNewerEdits() {
        QTemporaryDir directory;
        KisGpuDocumentDevice documents;
        KisGpuApplicationState application(documents);
        KisGpuCanvasWorkspace workspace(documents, application);
        auto *editor = workspace.createCanvas({32, 32});
        QVERIFY2(editor, qPrintable(workspace.lastError()));
        QVERIFY(draw(*editor, application));
        const auto id = editor->documentId();
        QCOMPARE(documents.store(id)->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(documents.store(id)->statistics().pixelReadbackBytes, quint64(0));
        const auto expected = pixels(documents, id);
        const auto path = directory.filePath(QStringLiteral("one.kra"));
        QVERIFY(workspace.saveCanvas(*editor, path));
        QCOMPARE(documents.document(id)->addLayer(QStringLiteral("Newer")).result, KisGpuLayerDocument::Result::Accepted);
        QTRY_VERIFY(editor->windowTitle().contains(QStringLiteral("one.kra")));
        QCOMPARE(saveCycles(path), 1);
        QVERIFY(editor->isWindowModified());
        auto *opened = workspace.openCanvas(path);
        QVERIFY2(opened, qPrintable(workspace.lastError()));
        QCOMPARE(documents.document(opened->documentId())->layers().size(), qsizetype(1));
        QCOMPARE(pixels(documents, opened->documentId()), expected);
        QVERIFY(!opened->isWindowModified());
        const auto second = directory.filePath(QStringLiteral("two.kra"));
        QVERIFY(workspace.saveCanvas(*editor, second));
        QTRY_VERIFY(!editor->isWindowModified());
        QCOMPARE(saveCycles(second), 2);
        auto *again = workspace.openCanvas(second);
        QVERIFY(again);
        QCOMPARE(documents.document(again->documentId())->layers().size(), qsizetype(2));
        QCOMPARE(pixels(documents, again->documentId()), expected);
        QCOMPARE(documents.device()->errorCount(), 0);
    }
    void gpuReleaseCancelsReadbackAndAllowsResave() {
        QTemporaryDir directory;
        KisGpuDocumentDevice documents;
        KisGpuApplicationState application(documents);
        KisGpuCanvasWorkspace workspace(documents, application);
        auto *editor = workspace.createCanvas({32, 32});
        QVERIFY(editor && draw(*editor, application));
        const auto id = editor->documentId();
        editor->revision();
        const auto path = directory.filePath(QStringLiteral("retry.kra"));
        QVERIFY(workspace.saveCanvas(*editor, path));
        QVERIFY(documents.suspend());
        QVERIFY(editor->isWindowModified());
        QVERIFY(!QFileInfo::exists(path));
        QVERIFY(documents.resume());
        QTRY_COMPARE((documents.poll(), documents.document(id)->state()), KisGpuLayerDocument::State::Idle);
        QVERIFY(workspace.saveCanvas(*editor, path));
        QTRY_VERIFY(!editor->isWindowModified());
        QVERIFY(QFileInfo::exists(path));
        auto *opened = workspace.openCanvas(path);
        QVERIFY2(opened, qPrintable(workspace.lastError()));
        QCOMPARE(pixels(documents, id), pixels(documents, opened->documentId()));
        QCOMPARE(documents.device()->errorCount(), 0);
    }
    void failedOpenClosesTheUnpublishedDocument() {
        QTemporaryDir directory;
        KisGpuDocumentDevice documents;
        KisGpuApplicationState application(documents);
        KisGpuCanvasWorkspace workspace(documents, application);
        QVERIFY(!workspace.openCanvas(directory.filePath(QStringLiteral("missing.kra"))));
        QVERIFY(!workspace.lastError().isEmpty());
        QVERIFY(!documents.hasDocuments());
        QVERIFY(!documents.device());
    }
};
KISTEST_MAIN(KisGpuCanvasWorkspaceTest)
#include "KisGpuCanvasWorkspaceTest.moc"
