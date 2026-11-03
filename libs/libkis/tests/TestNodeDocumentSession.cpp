/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "TestNode.h"
#include <simpletest.h>
#include <Document.h>
#include <Node.h>
#include <InfoObject.h>
#include <QDomDocument>
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
#include <metadata/KoDocumentInfo.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_config.h>
#include <kis_image_animation_interface.h>
#include <kis_time_span.h>
#include <kis_properties_configuration.h>
#include <kis_node.h>
#include <kis_tool.h>
#include <kis_canvas2.h>
#include <kis_color_button.h>
#include <widgets/kis_color_space_selector.h>
#include <QApplication>
#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QLineEdit>
#include <QImage>
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
#include <cstring>

namespace {
void configureNewDocument(QWidget *widget, bool sixteenBits = false)
{
    auto *sections = widget->window()->findChild<QTreeWidget *>();
    for (QTreeWidgetItemIterator item(sections); *item; ++item)
        if ((*item)->text(0) == i18n("Custom Document")) sections->setCurrentItem(*item);
    const auto cs = sixteenBits ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
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

void TestNode::testGpuSessionUsesOrdinaryCreationPaintingAndReopen_data()
{
    QTest::addColumn<int>("sourceFormat");
    QTest::addColumn<bool>("sixteenBits");
    QTest::newRow("new-document") << 0 << false;
    QTest::newRow("existing-static-native") << 1 << false;
    QTest::newRow("existing-png") << 2 << false;
    QTest::newRow("new-u16-document") << 0 << true;
    QTest::newRow("existing-u16-native") << 1 << true;
}

void TestNode::testGpuSessionUsesOrdinaryCreationPaintingAndReopen()
{
    QFETCH(int, sourceFormat);
    QFETCH(bool, sixteenBits);
    const auto cs = sixteenBits ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const quint16 foregroundComponents[]{1, 257, 65534, 65535};
    const QByteArray paintedPixel = sixteenBits
        ? QByteArray(reinterpret_cast<const char *>(foregroundComponents), sizeof(foregroundComponents))
        : QByteArray::fromHex("0000ffff");
    const KoColor foreground(reinterpret_cast<const quint8 *>(paintedPixel.constData()), cs);
    QByteArray hiddenPixel = paintedPixel;
    hiddenPixel[hiddenPixel.size() - 1] = 0;
    if (sixteenBits) hiddenPixel[hiddenPixel.size() - 2] = 0;
    const bool existingStaticDocument = sourceFormat == 1;
    const bool existingPng = sourceFormat == 2;
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto destination = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", directory.path()) +
        (sixteenBits ? QLatin1Char('/') + QString::fromLatin1(QTest::currentDataTag()) :
         existingStaticDocument ? QStringLiteral("/existing-static") : existingPng ? QStringLiteral("/existing-png") : QString());
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
    if (existingPng) {
        // Fixed 64 x 64 RGBA8 PNG, white RGB under zero alpha, no ancillary chunks.
        const auto input = QByteArray::fromHex("89504e470d0a1a0a0000000d4948445200000040000000400806000000aa6971de000000a849444154789ce5ce210100000c04a1ef5ffa1603318167d53ee3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed00e2e94d2c2bbd932340000000049454e44ae426082");
        const auto path = QDir(destination).filePath(QStringLiteral("existing-png-source.png"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(input), input.size());
        file.close();
        QVERIFY(window->openDocument(path, KisMainWindow::None));
    } else if (existingStaticDocument) {
        qputenv("LIBREPAINT_GPU_ENGINE", "0");
        std::unique_ptr<KisDocument> source(KisPart::instance()->createDocument());
        source->setFileBatchMode(true);
        QVERIFY(source->newImage(QStringLiteral("Ordinary GPU session"), 64, 64, cs, KoColor(QColor(255, 255, 255, 0), cs),
                                 KisConfig::CANVAS_COLOR, 2, {}, 1));
        int identity = 1;
        for (KisNodeSP layer = KisImageSP(source->image())->root()->firstChild(); layer; layer = layer->nextSibling()) {
            layer->setUuid(QUuid(QStringLiteral("{5a08000%1-1122-3344-5566-778899aabbcc}").arg(identity++)));
            layer->setPinnedToTimeline(false);
        }
        source->setColorHistoryColors({});
        source->setAssistantsGlobalColor(QColor(17, 63, 129, 201));
        source->documentInfo()->setAboutInfo("initial-creator", QStringLiteral("LibrePaint fixture"));
        source->documentInfo()->setAboutInfo("language", QStringLiteral("en"));
        source->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
        auto *animation = source->image()->animationInterface();
        animation->setFramerate(30);
        animation->setDocumentRange(KisTimeSpan::fromTimeToTime(5, 125));
        animation->switchCurrentTimeAsync(17);
        source->image()->waitForDone();
        const auto native = QDir(destination).filePath(QStringLiteral("existing-static-source.kra"));
        KisPropertiesConfigurationSP settings(new KisPropertiesConfiguration);
        settings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
        QVERIFY2(source->saveAs(native, KisDocument::nativeFormatMimeType(), false, settings), qPrintable(source->errorMessage()));
        source->waitForSavingToComplete();
        qputenv("LIBREPAINT_GPU_ENGINE", "1");
        QTimer dismissError;
        connect(&dismissError, &QTimer::timeout, window.get(), [] {
            if (auto *message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) message->accept();
        });
        dismissError.start(10);
        QVERIFY(window->openDocument(native, KisMainWindow::None));
    } else {
        bool created = false;
        QTimer::singleShot(0, window.get(), [&] {
            auto *pane = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            QVERIFY(pane);
            const auto dismiss = qScopeGuard([&] { pane->reject(); });
            auto *widget = pane->findChild<QWidget *>(QStringLiteral("KisCustomImageWidget"));
            QVERIFY(widget);
            configureNewDocument(widget, sixteenBits);
            created = QMetaObject::invokeMethod(widget, "createImage", Qt::DirectConnection);
        });
        window->slotFileNew();
        QVERIFY(created);
    }
    auto *document = window->viewManager()->document();
    QVERIFY(document);
    auto image = KisImageSP(document->image());
    QVERIFY(image->gpuImage());
    QCOMPARE(image->size(), QSize(64, 64));
    QCOMPARE(image->colorSpace(), cs);
    QCOMPARE(image->root()->childCount(), existingPng ? 1U : 2U);
    if (existingStaticDocument) {
        QCOMPARE(image->animationInterface()->framerate(), 30);
        QCOMPARE(image->animationInterface()->currentUITime(), 17);
    }
    QTRY_VERIFY(window->viewManager()->activeNode());
    QVERIFY(!window->viewManager()->activeNode()->userLocked());
    Document api(document, false);
    if (sixteenBits) {
        QScopedPointer<Node> target(Node::createNode(image, image->root()->lastChild()));
        QVERIFY(target->setPixelData(hiddenPixel, 2, 2, 1, 1));
        QCOMPARE(target->pixelData(2, 2, 1, 1), hiddenPixel);
    }
    const int initialHistory = document->undoStack()->count();
    const auto initialPixels = api.pixelData(0, 0, 64, 64);
    QVERIFY(savePixels(QStringLiteral("initial"), initialPixels));
    const quint16 backgroundComponents[]{65535, 65535, 65535, 0};
    const auto backgroundPixel = sixteenBits
        ? QByteArray(reinterpret_cast<const char *>(backgroundComponents), sizeof(backgroundComponents))
        : QByteArray::fromHex("ffffff00");
    QCOMPARE(initialPixels, backgroundPixel.repeated(64 * 64));
    const auto paint = [&](KisDocument *current, QPointF from, QPointF to, bool erase = false) {
        auto preset = KisPaintOpRegistry::instance()->defaultPreset(KoID("roundmarker"), KisGlobalResourcesInterface::instance());
        auto *resources = window->viewManager()->canvasResourceProvider();
        resources->setPaintOpPreset(preset);
        resources->setSize(8);
        resources->setOpacity(1);
        resources->setFGColor(foreground);
        resources->setEraserMode(erase);
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
        QCOMPARE(target->pixelData(int(midpoint.x()), int(midpoint.y()), 1, 1), erase ? hiddenPixel : paintedPixel);
        if (!existingPng) {
            QScopedPointer<Node> background(Node::createNode(current->image(), current->image()->root()->firstChild()));
            QCOMPARE(background->pixelData(int(midpoint.x()), int(midpoint.y()), 1, 1), QByteArray(cs->pixelSize(), '\0'));
        }
    };
    paint(document, {12, 16}, {52, 16});
    QCOMPARE(api.pixelData(32, 16, 1, 1), paintedPixel);
    QCOMPARE(document->undoStack()->count(), initialHistory + 1);
    const auto pixels = api.pixelData(0, 0, 64, 64);
    QVERIFY(savePixels(QStringLiteral("painted"), pixels));
    document->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(api.pixelData(0, 0, 64, 64), initialPixels);
    document->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(api.pixelData(0, 0, 64, 64), pixels);
    if (sixteenBits) {
        paint(document, {12, 16}, {52, 16}, true);
        QCOMPARE(document->undoStack()->count(), initialHistory + 2);
        const auto erased = api.pixelData(0, 0, 64, 64);
        QVERIFY(erased != pixels);
        document->undoStack()->undo();
        image->waitForDone();
        QCOMPARE(api.pixelData(0, 0, 64, 64), pixels);
        document->undoStack()->redo();
        image->waitForDone();
        QCOMPARE(api.pixelData(0, 0, 64, 64), erased);
        document->undoStack()->undo();
        image->waitForDone();
        QCOMPARE(api.pixelData(0, 0, 64, 64), pixels);
    }
    KisPropertiesConfigurationSP nativeSettings;
    if (existingPng || sixteenBits) {
        QDomDocument fixedInfo;
        QVERIFY(fixedInfo.setContent(QByteArrayLiteral("<document-info><about><title>Ordinary PNG session</title>"
            "<creation-date>2000-01-02T03:04:05Z</creation-date><date>2001-02-03T04:05:06Z</date>"
            "<editing-cycles>7</editing-cycles><editing-time>10</editing-time><language>en</language></about>"
            "<author><full-name>LibrePaint fixture</full-name></author></document-info>")));
        QVERIFY(document->documentInfo()->load(fixedInfo));
        document->setColorHistoryColors({});
        document->setAssistantsGlobalColor(QColor(17, 63, 129, 201));
        nativeSettings = new KisPropertiesConfiguration;
        nativeSettings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    }
    const auto path = (existingPng || sixteenBits) ? QDir(destination).filePath(QStringLiteral("ordinary-session.kra")) :
                                    directory.filePath(QStringLiteral("ordinary-session.kra"));
    QVERIFY2(document->saveAs(path, KisDocument::nativeFormatMimeType(), false, nativeSettings), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(window->openDocument(path, KisMainWindow::None));
    document = window->viewManager()->document();
    QVERIFY(document);
    QVERIFY(document->image()->gpuImage());
    QCOMPARE(document->image()->colorSpace(), cs);
    if (existingStaticDocument) {
        QCOMPARE(document->image()->animationInterface()->framerate(), 30);
        QCOMPARE(document->image()->animationInterface()->currentUITime(), 17);
    }
    Document reopened(document, false);
    QCOMPARE(reopened.pixelData(0, 0, 64, 64), pixels);
    if (sixteenBits) {
        QScopedPointer<Node> target(Node::createNode(document->image(), document->image()->root()->lastChild()));
        QCOMPARE(target->pixelData(2, 2, 1, 1), hiddenPixel);
    }
    paint(document, {12, 40}, {52, 40});
    QVERIFY(savePixels(QStringLiteral("reopened-painted"), reopened.pixelData(0, 0, 64, 64)));
    QCOMPARE(reopened.pixelData(32, 40, 1, 1), paintedPixel);
    QCOMPARE(document->undoStack()->count(), 1);
    QVERIFY(document->isModified());
    if (existingPng || sixteenBits) {
        document->setColorHistoryColors({});
        QVERIFY2(document->saveAs(QDir(destination).filePath(QStringLiteral("reopened-edited.kra")),
                                  KisDocument::nativeFormatMimeType(), false, nativeSettings), qPrintable(document->errorMessage()));
        document->waitForSavingToComplete();
        InfoObject outputSettings;
        outputSettings.setProperty("alpha", true);
        outputSettings.setProperty("compression", 3);
        outputSettings.setProperty("saveSRGBProfile", true);
        outputSettings.setProperty("indexed", false);
        outputSettings.setProperty("interlaced", false);
        outputSettings.setProperty("forceSRGB", false);
        bool exportDialogVisited = false;
        QTimer interaction;
        interaction.setInterval(1);
        connect(&interaction, &QTimer::timeout, this, [&] {
            if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
                interaction.stop();
                exportDialogVisited = true;
                dialog->accept();
            }
        });
        interaction.start();
        QVERIFY2(reopened.exportImage(QDir(destination).filePath(QStringLiteral("reopened-edited.png")), outputSettings),
                 qPrintable(document->errorMessage()));
        QVERIFY(exportDialogVisited);
        if (sixteenBits) {
            QFile png(QDir(destination).filePath(QStringLiteral("reopened-edited.png")));
            QVERIFY(png.open(QIODevice::ReadOnly));
            const auto header = png.read(26);
            QCOMPARE(header.size(), 26);
            QCOMPARE(quint8(header[24]), quint8(16));
            const auto decoded = QImage(png.fileName()).convertToFormat(QImage::Format_RGBA64);
            QCOMPARE(decoded.size(), QSize(64, 64));
            const auto expected = reopened.pixelData(0, 0, 64, 64);
            for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) {
                quint16 bgra[4];
                std::memcpy(bgra, expected.constData() + (y * 64 + x) * 8, sizeof(bgra));
                const auto actual = reinterpret_cast<const QRgba64 *>(decoded.constScanLine(y))[x];
                QCOMPARE(actual, QRgba64::fromRgba64(bgra[2], bgra[1], bgra[0], bgra[3]));
            }
        }
    }

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
