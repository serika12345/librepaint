/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentWindow.h"
#include "KisGpuCanvasSession.h"
#include <QAction>
#include <QMouseEvent>
#include <QWindow>
#include <QtTest>

namespace {
void mouse(QWindow &window, QEvent::Type type, QPointF position, Qt::MouseButton button, Qt::MouseButtons buttons)
{
    QMouseEvent event(type, position, position, button, buttons, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &event);
}
QWindow *drawingWindow(quint64 id)
{
    for (auto *window : QGuiApplication::allWindows())
        if (window->objectName() == QStringLiteral("gpuDocumentCanvas-%1").arg(id)) return window;
    return nullptr;
}
}

/** A product window exposes GPU editing and closes only the document it owns. */
class KisGpuDocumentWindowTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void drawingUndoRedoAndWindowOwnership() {
        KisGpuDocumentDevice documents;
        KisGpuApplicationState application(documents);
        const auto id = documents.createDocument({});
        const auto other = documents.createDocument({});
        auto model = documents.document(id);
        QCOMPARE(model->addLayer(QStringLiteral("Paint")).result, KisGpuLayerDocument::Result::Accepted);
        {
            KisGpuDocumentWindow editor(documents, application, id, QRect(0, 0, 64, 64));
            editor.show();
            auto *window = drawingWindow(id);
            QVERIFY(window);
            QVERIFY(QTest::qWaitForWindowExposed(window));
            auto *canvas = editor.findChild<KisGpuCanvasSession *>();
            QVERIFY(canvas);
            application.applicationStateChanged(Qt::ApplicationActive);
            const QPointF center(window->width() / 2.0, window->height() / 2.0);
            mouse(*window, QEvent::MouseButtonPress, center, Qt::LeftButton, Qt::LeftButton);
            mouse(*window, QEvent::MouseButtonRelease, center, Qt::LeftButton, Qt::NoButton);
            QTRY_COMPARE(canvas->inputState(), KisGpuStrokeInput::State::Finished);
            QVERIFY(model->layers().first().paint.pixels.tileCount() > 0);
            const auto painted = model->layers().first().paint.pixels;
            auto *undo = editor.findChild<QAction *>(QStringLiteral("gpuUndo"));
            auto *redo = editor.findChild<QAction *>(QStringLiteral("gpuRedo"));
            QVERIFY(undo && redo);
            undo->trigger();
            QTRY_COMPARE(model->layers().first().paint.pixels.tileCount(), qsizetype(0));
            redo->trigger();
            QTRY_COMPARE(model->layers().first().paint.pixels, painted);
            auto *layer = editor.findChild<QAction *>(QStringLiteral("gpuAddLayer"));
            QVERIFY(layer);
            layer->trigger();
            QCOMPARE(model->layers().size(), qsizetype(2));
            QCOMPARE(documents.store(id)->statistics().pixelUploadBytes, quint64(0));
            QCOMPARE(documents.store(id)->statistics().pixelReadbackBytes, quint64(0));
            QCOMPARE(documents.device()->errorCount(), 0);
        }
        QVERIFY(!documents.document(id));
        QVERIFY(documents.document(other));
        QCOMPARE(model->state(), KisGpuLayerDocument::State::Suspended);
        QVERIFY(documents.closeDocument(other));
        QVERIFY(!documents.device());
    }
    void externalCloseDetachesTheWindow() {
        KisGpuDocumentDevice documents;
        KisGpuApplicationState application(documents);
        const auto id = documents.createDocument({});
        documents.document(id)->addLayer(QStringLiteral("Paint"));
        KisGpuDocumentWindow editor(documents, application, id, QRect(0, 0, 64, 64));
        editor.show();
        QVERIFY(documents.closeDocument(id));
        QVERIFY(!editor.isVisible());
        QVERIFY(!documents.device());
    }
};
QTEST_MAIN(KisGpuDocumentWindowTest)
#include "KisGpuDocumentWindowTest.moc"
