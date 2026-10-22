/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "TestNode.h"
#include <simpletest.h>
#include <Document.h>
#include <Node.h>
#include <KoColor.h>
#include <KoColorProfile.h>
#include <KoColorSpaceRegistry.h>
#include <KoPointerEvent.h>
#include <KoToolManager.h>
#include <KisGlobalResourcesInterface.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <brushengine/kis_paintop_registry.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <canvas/kis_coordinates_converter.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_node.h>
#include <kis_tool.h>
#include <kis_canvas2.h>
#include <kis_color_button.h>
#include <widgets/kis_color_space_selector.h>
#include <QApplication>
#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPointer>
#include <QPushButton>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <KLocalizedString>
#include <memory>

namespace {
void configureNewDocument(QWidget *widget)
{
    auto *sections = widget->window()->findChild<QTreeWidget *>();
    for (QTreeWidgetItemIterator item(sections); *item; ++item)
        if ((*item)->text(0) == i18n("Custom Document")) sections->setCurrentItem(*item);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    auto *selector = widget->findChild<KisColorSpaceSelector *>(QStringLiteral("colorSpaceSelector"));
    selector->setCurrentColorModel(cs->colorModelId());
    selector->setCurrentColorDepth(cs->colorDepthId());
    selector->setCurrentProfile(cs->profile()->name());
    widget->findChild<QLineEdit *>(QStringLiteral("txtName"))->setText(QStringLiteral("Ordinary GPU session"));
    widget->findChild<QWidget *>(QStringLiteral("doubleWidth"))->setProperty("value", 64.0);
    widget->findChild<QWidget *>(QStringLiteral("doubleHeight"))->setProperty("value", 64.0);
    widget->findChild<QWidget *>(QStringLiteral("doubleResolution"))->setProperty("value", 72.0);
    widget->findChild<QWidget *>(QStringLiteral("intNumLayers"))->setProperty("value", 2);
    widget->findChild<QWidget *>(QStringLiteral("sliderOpacity"))->setProperty("value", 0.0);
    widget->findChild<KisColorButton *>(QStringLiteral("cmbColor"))->setColor(KoColor(Qt::white, cs));
    widget->findChild<QAbstractButton *>(QStringLiteral("radioBackgroundAsProjection"))->setChecked(true);
}
}

void TestNode::testGpuSessionUsesOrdinaryCreationPaintingAndReopen()
{
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto destination = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", directory.path());
    QVERIFY(QDir().mkpath(destination));
    const auto retainFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) { directory.setAutoRemove(false); qInfo() << "Ordinary GPU session artifacts at" << destination; }
    });
    const auto savePixels = [&](const QString &stage, const QByteArray &pixels) {
        QFile file(QDir(destination).filePath(QStringLiteral("ordinary-session-") + stage + QStringLiteral(".bgra")));
        return file.open(QIODevice::WriteOnly) && file.write(pixels) == pixels.size();
    };
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    const auto close = qScopeGuard([&] {
        if (auto *document = window->viewManager()->document()) {
            document->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->show();
    bool created = false;
    QTimer::singleShot(0, window.get(), [&] {
        auto *pane = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        QVERIFY(pane);
        const auto dismiss = qScopeGuard([&] { pane->reject(); });
        auto *widget = pane->findChild<QWidget *>(QStringLiteral("KisCustomImageWidget"));
        QVERIFY(widget);
        configureNewDocument(widget);
        created = QMetaObject::invokeMethod(widget, "createImage", Qt::DirectConnection);
    });
    window->slotFileNew();
    QVERIFY(created);
    auto *document = window->viewManager()->document();
    QVERIFY(document);
    auto image = KisImageSP(document->image());
    QVERIFY(image->gpuImage());
    QCOMPARE(image->size(), QSize(64, 64));
    QCOMPARE(image->root()->childCount(), 2U);
    QTRY_VERIFY(window->viewManager()->activeNode());
    QVERIFY(!window->viewManager()->activeNode()->userLocked());
    Document api(document, false);
    const auto initialPixels = api.pixelData(0, 0, 64, 64);
    QVERIFY(savePixels(QStringLiteral("initial"), initialPixels));
    QCOMPARE(initialPixels, QByteArray::fromHex("ffffff00").repeated(64 * 64));
    const auto paint = [&](KisDocument *current, QPointF from, QPointF to) {
        auto preset = KisPaintOpRegistry::instance()->defaultPreset(KoID("roundmarker"), KisGlobalResourcesInterface::instance());
        auto *resources = window->viewManager()->canvasResourceProvider();
        resources->setPaintOpPreset(preset);
        resources->setSize(8);
        resources->setOpacity(1);
        resources->setFGColor(KoColor(Qt::red, KoColorSpaceRegistry::instance()->rgb8()));
        resources->setEraserMode(false);
        resources->setCurrentCompositeOp(QStringLiteral("normal"));
        resources->setGlobalAlphaLock(false);
        resources->setMirrorHorizontal(false);
        resources->setMirrorVertical(false);
        auto *view = window->activeView();
        QVERIFY(view);
        QTRY_VERIFY(view->currentNode());
        QCOMPARE(view->document(), current);
        QCOMPARE(view->currentNode(), current->image()->root()->lastChild());
        KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
        auto *tool = dynamic_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush")));
        QVERIFY(tool);
        QVERIFY(tool->setProperty("smoothingType", 0));
        const auto *converter = view->canvasBase()->coordinatesConverter();
        QMouseEvent press(QEvent::MouseButtonPress, converter->imageToWidget(from), QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        press.setTimestamp(100);
        KoPointerEvent begin(&press, converter->imageToDocument(from));
        tool->beginPrimaryAction(&begin);
        QVERIFY(begin.isAccepted());
        const auto midpoint = (from + to) / 2;
        QMouseEvent move(QEvent::MouseMove, converter->imageToWidget(midpoint), QPointF(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        move.setTimestamp(110);
        KoPointerEvent update(&move, converter->imageToDocument(midpoint));
        tool->continuePrimaryAction(&update);
        QMouseEvent release(QEvent::MouseButtonRelease, converter->imageToWidget(to), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        release.setTimestamp(120);
        KoPointerEvent end(&release, converter->imageToDocument(to));
        tool->endPrimaryAction(&end);
        current->image()->waitForDone();
        QScopedPointer<Node> target(Node::createNode(current->image(), view->currentNode()));
        QCOMPARE(target->pixelData(int(midpoint.x()), int(midpoint.y()), 1, 1), QByteArray::fromHex("0000ffff"));
        QScopedPointer<Node> background(Node::createNode(current->image(), current->image()->root()->firstChild()));
        QCOMPARE(background->pixelData(int(midpoint.x()), int(midpoint.y()), 1, 1), QByteArray(4, '\0'));
    };
    paint(document, {12, 16}, {52, 16});
    QCOMPARE(api.pixelData(32, 16, 1, 1), QByteArray::fromHex("0000ffff"));
    QCOMPARE(document->undoStack()->count(), 1);
    const auto pixels = api.pixelData(0, 0, 64, 64);
    QVERIFY(savePixels(QStringLiteral("painted"), pixels));
    document->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(api.pixelData(0, 0, 64, 64), initialPixels);
    document->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(api.pixelData(0, 0, 64, 64), pixels);
    const auto path = directory.filePath(QStringLiteral("ordinary-session.kra"));
    QVERIFY2(document->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(window->openDocument(path, KisMainWindow::None));
    document = window->viewManager()->document();
    QVERIFY(document);
    QVERIFY(document->image()->gpuImage());
    Document reopened(document, false);
    QCOMPARE(reopened.pixelData(0, 0, 64, 64), pixels);
    paint(document, {12, 40}, {52, 40});
    QVERIFY(savePixels(QStringLiteral("reopened-painted"), reopened.pixelData(0, 0, 64, 64)));
    QCOMPARE(reopened.pixelData(32, 40, 1, 1), QByteArray::fromHex("0000ffff"));
    QCOMPARE(document->undoStack()->count(), 1);
    QVERIFY(document->isModified());
}

void TestNode::testGpuSessionCreationRefusalKeepsTheDialog()
{
    qRegisterMetaType<KisDocument *>("KisDocument*");
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->show();
    bool errorShown = false, published = false;
    QTimer::singleShot(0, window.get(), [&] {
        auto *pane = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        QVERIFY(pane);
        const auto dismiss = qScopeGuard([&] { pane->reject(); });
        disconnect(pane, SIGNAL(documentSelected(KisDocument*)), nullptr, nullptr);
        QSignalSpy publications(pane, SIGNAL(documentSelected(KisDocument*)));
        auto *widget = pane->findChild<QWidget *>(QStringLiteral("KisCustomImageWidget"));
        QVERIFY(widget);
        configureNewDocument(widget);
        widget->findChild<QAbstractButton *>(QStringLiteral("radioBackgroundAsFill"))->setChecked(true);
        QTimer::singleShot(0, pane, [&] {
            if (auto *message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                errorShown = !message->text().isEmpty();
                message->accept();
            }
        });
        QVERIFY(QMetaObject::invokeMethod(widget, "createImage", Qt::DirectConnection));
        published = !publications.isEmpty();
        for (const auto &arguments : publications) delete qvariant_cast<KisDocument *>(arguments[0]);
        QVERIFY(!published);
        QVERIFY(pane->isVisible());
        QVERIFY(widget->findChild<QDialogButtonBox *>(QStringLiteral("newDialogConfirmationButtonBox"))->button(QDialogButtonBox::Ok)->isEnabled());
    });
    window->slotFileNew();
    QVERIFY(errorShown);
    QVERIFY(!published);
    QVERIFY(!window->viewManager()->document());
}
