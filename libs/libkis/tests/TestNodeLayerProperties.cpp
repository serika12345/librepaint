/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "TestNode.h"

#include <simpletest.h>
#include <Document.h>
#include <Node.h>
#include <InfoObject.h>
#include <KoStore.h>
#include <KoColorSpaceRegistry.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuPaintDocument.h>
#include <gpu/KisGpuExecution.h>
#include <kis_color_label_selector_widget.h>
#include <kis_cmb_composite.h>
#include <kis_layer_properties_icons.h>
#include <kis_config.h>
#include <kis_image.h>
#include <kis_group_layer.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_slider_spin_box.h>
#include <kis_transaction.h>
#include <kis_undo_adapter.h>
#include <kis_properties_configuration.h>
#include <metadata/KoDocumentInfo.h>
#include <kundo2stack.h>
#include <nodes/kis_node_manager.h>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QGroupBox>
#include <QLineEdit>
#include <QPointer>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QImage>
#include <memory>

void TestNode::testLayerPropertiesDialogPreservesPreviewCancellationAndHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("multiple");
    QTest::addColumn<bool>("accept");
    QTest::addColumn<bool>("edit");
    QTest::addColumn<bool>("mixedChannels");
    QTest::addColumn<bool>("group");
    QTest::addColumn<QString>("refusal");
    QTest::addColumn<int>("initialOpacity");
    QTest::addColumn<bool>("completedEdit");
    for (const bool gpu : {false, true}) {
        for (const bool group : {false, true}) {
            for (const bool multiple : {false, true}) {
                for (const bool accept : {false, true}) {
                    const QByteArray tag = QByteArray(gpu ? "gpu-" : "ordinary-")
                        + (group ? "group-" : "paint-") + (multiple ? "multiple-" : "single-") + (accept ? "accept" : "cancel");
                    QTest::newRow(tag.constData()) << gpu << multiple << accept << true << false << group << QString() << 153 << false;
                }
            }
            const QByteArray prefix = QByteArray(gpu ? "gpu-" : "ordinary-") + (group ? "group-" : "paint-");
            const QByteArray unchanged = prefix + "unchanged-accept";
            const QByteArray mixed = prefix + "mixed-channels-accept";
            QTest::newRow(unchanged.constData()) << gpu << false << true << false << false << group << QString() << 153 << false;
            QTest::newRow(mixed.constData()) << gpu << true << true << true << true << group << QString() << 153 << false;
        }
    }
    QTest::newRow("gpu-busy") << true << false << false << true << false << false << QStringLiteral("busy") << 153 << false;
    QTest::newRow("gpu-unsupported-blend") << true << false << false << true << false << false << QStringLiteral("blend") << 153 << false;
    QTest::newRow("gpu-unsupported-pass-through") << true << false << false << true << false << true << QStringLiteral("pass-through") << 153 << false;
    for (const bool edit : {false, true}) for (const bool accept : {false, true}) {
        const QByteArray tag = QByteArray("gpu-exact-opacity-") + (edit ? "edited-" : "unchanged-") + (accept ? "accept" : "cancel");
        QTest::newRow(tag.constData()) << true << false << accept << edit << false << false << QString() << 137 << false;
    }
    QTest::newRow("gpu-completed-painting") << true << false << true << true << false << false << QString() << 153 << true;
}

void TestNode::testLayerPropertiesDialogPreservesPreviewCancellationAndHistory()
{
    // The layer panel's Properties operation previews attributes together. Cancel
    // retains the redo branch; acceptance records one reversible document edit.
    QFETCH(bool, multiple);
    QFETCH(bool, accept);
    QFETCH(bool, gpu);
    QFETCH(bool, edit);
    QFETCH(bool, mixedChannels);
    QFETCH(bool, group);
    QFETCH(QString, refusal);
    QFETCH(int, initialOpacity);
    QFETCH(bool, completedEdit);
    namespace Gpu = LibrePaint::Image::Gpu;
    KisConfig(true).disableOpenGL();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    KisImageSP image;
    Gpu::PaintImageSP owner;
    KisLayerSP first, second, source;
    const QUuid firstUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}"));
    const QUuid secondUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}"));
    const QUuid sourceUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        Gpu::RasterImage input;
        input.size = {32, 16};
        input.layers = {{QStringLiteral("First"), true, {}, {}}, {QStringLiteral("Second"), true, {}, {}}};
        input.layers[0].opacity = quint8(initialOpacity);
        input.layers[1].opacity = 204;
        input.layers[0].uuid = firstUuid;
        input.layers[1].uuid = secondUuid;
        if (group) {
            input.layers[0].kind = Gpu::LayerKind::Group;
            Gpu::RasterLayer child{QStringLiteral("Source"), true, sourceUuid, {}};
            child.parentUuid = firstUuid;
            input.layers.prepend(child);
        }
        owner = Gpu::PaintImageSP::create(execution, input);
        image = KisImage::fromGpuImage(owner, QStringLiteral("Layer properties dialog"));
        for (auto node = image->root()->firstChild(); node; node = node->nextSibling()) {
            if (node->name() == QLatin1String("First")) first = dynamic_cast<KisLayer *>(node.data());
            else if (node->name() == QLatin1String("Second")) second = dynamic_cast<KisLayer *>(node.data());
        }
        if (first) source = group ? dynamic_cast<KisLayer *>(first->firstChild().data()) : first;
    } else {
        const auto *cs = KoColorSpaceRegistry::instance()->rgb8();
        image = new KisImage(document->createUndoStore(), 32, 16, cs, "Layer properties dialog");
        first = group ? KisLayerSP(new KisGroupLayer(image, QStringLiteral("First"), quint8(initialOpacity))) :
                        KisLayerSP(new KisPaintLayer(image, QStringLiteral("First"), quint8(initialOpacity)));
        second = new KisPaintLayer(image, QStringLiteral("Second"), 204);
        image->addNode(first);
        image->addNode(second);
        first->setUuid(firstUuid);
        second->setUuid(secondUuid);
        source = first;
        if (group) {
            source = new KisPaintLayer(image, QStringLiteral("Source"), 255);
            source->setUuid(sourceUuid);
            image->addNode(source, first);
        }
    }
    QVERIFY(first && second && source);
    first->setChannelFlags(QBitArray(4, true));
    second->setChannelFlags(QBitArray(4, true));
    if (mixedChannels) {
        QBitArray channels(4, true);
        channels.setBit(1, false);
        second->setChannelFlags(channels);
    }
    const auto originalFlags = first->channelFlags();
    document->setCurrentImage(image);
    const auto originalPixels = QByteArray::fromHex("432d17ff");
    QScopedPointer<Node> publicNode(Node::createNode(image, source));
    QVERIFY(publicNode->setPixelData(originalPixels, 1, 1, 1, 1));
    if (gpu) {
        Gpu::LayerId firstId = 0;
        for (const auto &layer : owner->operations().snapshot()->layers)
            if (layer.uuid == source->uuid()) firstId = layer.id;
        QVERIFY(firstId);
        QCOMPARE(owner->replaceRgbaPixels(firstId, QRect(1, 1, 1, 1), QByteArray::fromHex("4365abff")), Gpu::Result::Success);
    } else {
        KisTransaction transaction(source->paintDevice());
        const auto edited = QByteArray::fromHex("ab6543ff");
        source->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(edited.constData()), QRect(1, 1, 1, 1));
        transaction.commit(image->undoAdapter());
    }
    image->waitForDone();
    document->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(document->undoStack()->index(), 0);
    QCOMPARE(document->undoStack()->count(), 1);
    document->setModified(false);

    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    QPointer<QDialog> dialog;
    const auto cleanup = qScopeGuard([&] {
        if (dialog) {
            dialog->reject();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.get()));
    auto *manager = window->viewManager()->nodeManager();
    manager->slotImageRequestNodeReselection(first, multiple ? KisNodeList{first, second} : KisNodeList{first});
    QTRY_COMPARE(manager->activeNode(), KisNodeSP(first));
    QTRY_COMPARE(manager->selectedNodes().size(), multiple ? 2 : 1);
    Document publicDocument(lifetime, false);
    publicDocument.refreshProjection();
    const auto originalProjection = publicDocument.pixelData(1, 1, 1, 1);

    manager->nodeProperties(manager->activeNode());
    const auto findDialog = [&] {
        for (auto *widget : QApplication::topLevelWidgets()) {
            auto *candidate = qobject_cast<QDialog *>(widget);
            if (candidate && candidate->findChild<QLineEdit *>(QStringLiteral("editName"))) {
                dialog = candidate;
                return true;
            }
        }
        return false;
    };
    QTRY_VERIFY(findDialog());
    auto *opacity = dialog->findChild<KisSliderSpinBox *>(QStringLiteral("intOpacity"));
    auto *applyOpacity = dialog->findChild<QCheckBox *>(QStringLiteral("chkOpacity"));
    auto *label = dialog->findChild<KisColorLabelSelectorWidget *>(QStringLiteral("colorLabelSelector"));
    auto *channels = dialog->findChild<QGroupBox *>(QStringLiteral("grpActiveChannels"));
    QVERIFY(opacity && applyOpacity && label && channels);
    const auto flags = channels->findChildren<QCheckBox *>();
    QCOMPARE(flags.size(), 4);
    if (edit) {
        applyOpacity->setChecked(true);
        opacity->setValue(35);
        label->setCurrentIndex(5);
        flags.first()->setChecked(false);
        if (!multiple) dialog->findChild<QLineEdit *>(QStringLiteral("editName"))->setText(QStringLiteral("Renamed"));
    }
    QCOMPARE(first->opacity(), quint8(edit ? 89 : initialOpacity));
    QCOMPARE(second->opacity(), quint8(multiple && edit ? 89 : 204));
    QCOMPARE(first->colorLabelIndex(), edit ? 5 : 0);
    QCOMPARE(first->channelFlags().isEmpty() || first->channelFlags().testBit(0), !edit);
    QCOMPARE(lifetime->undoStack()->index(), 0);
    QCOMPARE(lifetime->undoStack()->count(), 1);
    QVERIFY(!lifetime->isModified());
    publicDocument.refreshProjection();
    const auto preview = publicDocument.pixelData(1, 1, 1, 1);
    QCOMPARE(preview != originalProjection, edit);
    if (completedEdit) {
        Gpu::LayerId sourceId = 0;
        for (const auto &layer : owner->operations().snapshot()->layers)
            if (layer.uuid == source->uuid()) sourceId = layer.id;
        QVERIFY(sourceId);
        QCOMPARE(owner->replaceRgbaPixels(sourceId, {2, 2, 1, 1}, QByteArray::fromHex("112233ff")), Gpu::Result::Success);
        QCOMPARE(publicNode->pixelData(2, 2, 1, 1), QByteArray::fromHex("332211ff"));
    }
    const int priorIndex = lifetime->undoStack()->index();
    const int priorCount = lifetime->undoStack()->count();
    QObject notificationContext;
    QList<quint8> acceptanceOpacities;
    const auto acceptanceConnection = QObject::connect(first.data(), &KisNode::sigNodeChangedInternal,
        &notificationContext, [&] { acceptanceOpacities.append(first->opacity()); });

    if (!refusal.isEmpty()) {
        Gpu::OperationId operation;
        if (refusal == QLatin1String("busy")) {
            operation = owner->beginStroke(owner->operations().activeLayer(), Gpu::Brush{}, 43);
            QVERIFY(!operation.isNull());
            QCOMPARE(owner->appendSamples(operation, {{{4, 4}}}), Gpu::Result::Success);
        } else if (refusal == QLatin1String("blend")) {
            auto *composite = dialog->findChild<KisCompositeOpComboBox *>(QStringLiteral("cmbComposite"));
            QVERIFY(composite);
            composite->selectCompositeOp(KoID(QStringLiteral("multiply")));
            QCOMPARE(first->compositeOpId(), QStringLiteral("multiply"));
        } else {
            auto *properties = dialog->findChild<QGroupBox *>(QStringLiteral("grpProperties"));
            QVERIFY(properties);
            bool toggled = false;
            for (auto *box : properties->findChildren<QCheckBox *>()) {
                if (box->text() != KisLayerPropertiesIcons::passThrough.name()) continue;
                box->setChecked(true);
                toggled = true;
            }
            QVERIFY(toggled);
            QVERIFY(dynamic_cast<KisGroupLayer *>(first.data())->passThroughMode());
        }
        const auto pending = owner->operations().preview();
        const auto modified = lifetime->isModified();
        dialog->accept();
        QVERIFY(dialog && dialog->isVisible());
        QCOMPARE(owner->operations().preview(), pending);
        QCOMPARE(lifetime->undoStack()->index(), 0);
        QCOMPARE(lifetime->undoStack()->count(), 1);
        QCOMPARE(lifetime->isModified(), modified);
        if (!operation.isNull()) QCOMPARE(owner->cancelStroke(operation), Gpu::Result::Success);
    }

    if (accept) dialog->accept();
    else dialog->reject();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QTRY_VERIFY(!dialog);
    QObject::disconnect(acceptanceConnection);
    if (accept) {
        for (const auto value : acceptanceOpacities) QCOMPARE(value, quint8(edit ? 89 : initialOpacity));
    }
    image->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->index(), priorIndex + int(accept));
    QCOMPARE(lifetime->undoStack()->count(), accept ? priorIndex + 1 : priorCount);
    QCOMPARE(lifetime->isModified(), accept);
    if (accept) {
        first->setVisible(false);
        if (multiple) first->setName(QStringLiteral("Direct multi name"));
        if (mixedChannels) {
            auto channels = first->channelFlags();
            channels.setBit(1, false);
            first->setChannelFlags(channels);
        }
    }
    if (accept) lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(lifetime->undoStack()->index(), priorIndex);
    if (completedEdit) QCOMPARE(publicNode->pixelData(2, 2, 1, 1), QByteArray::fromHex("332211ff"));
    QCOMPARE(first->opacity(), quint8(initialOpacity));
    QCOMPARE(second->opacity(), quint8(204));
    QCOMPARE(first->name(), accept && multiple ? QStringLiteral("Direct multi name") : QStringLiteral("First"));
    QCOMPARE(second->name(), QStringLiteral("Second"));
    QCOMPARE(first->colorLabelIndex(), 0);
    QCOMPARE(first->compositeOpId(), QStringLiteral("normal"));
    if (group) QVERIFY(!dynamic_cast<KisGroupLayer *>(first.data())->passThroughMode());
    QVERIFY(first->visible());
    if (mixedChannels && accept) {
        QBitArray expected(4, true);
        expected.setBit(1, false);
        QCOMPARE(first->channelFlags(), expected);
    } else QCOMPARE(first->channelFlags(), originalFlags);
    publicDocument.refreshProjection();
    auto undoProjection = originalProjection;
    if (mixedChannels && accept) undoProjection[1] = 0;
    QCOMPARE(publicDocument.pixelData(1, 1, 1, 1), undoProjection);
    QCOMPARE(publicNode->pixelData(1, 1, 1, 1), originalPixels);
    lifetime->undoStack()->redo();
    image->waitForDone();
    if (accept) {
        QCOMPARE(first->opacity(), quint8(edit ? 89 : initialOpacity));
        QCOMPARE(second->opacity(), quint8(multiple && edit ? 89 : 204));
        QCOMPARE(first->name(), multiple ? QStringLiteral("Direct multi name") :
                 edit ? QStringLiteral("Renamed") : QStringLiteral("First"));
        QCOMPARE(first->colorLabelIndex(), edit ? 5 : 0);
        QVERIFY(first->visible());
        publicDocument.refreshProjection();
        auto redoProjection = preview;
        if (mixedChannels) redoProjection[1] = 0;
        QCOMPARE(publicDocument.pixelData(1, 1, 1, 1), redoProjection);
        QCOMPARE(publicNode->pixelData(1, 1, 1, 1), originalPixels);
        const auto pixels = publicDocument.pixelData(0, 0, 32, 16);
        QTemporaryDir output;
        QVERIFY(output.isValid());
        const auto preserveFailure = qScopeGuard([&] {
            if (QTest::currentTestFailed()) {
                output.setAutoRemove(false);
                qInfo() << "Layer property artifacts retained at" << output.path();
            }
        });
        const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        const auto destination = resultDirectory.isEmpty() ? output.path() : resultDirectory;
        QVERIFY(QDir().mkpath(destination));
        const QString stem = QStringLiteral("layer-properties-") + QString::fromLatin1(QTest::currentDataTag());
        const auto kraPath = QDir(destination).filePath(stem + QStringLiteral(".kra"));
        lifetime->documentInfo()->setAboutInfo("title", QStringLiteral("Fixed layer properties document"));
        lifetime->documentInfo()->setAboutInfo("initial-creator", QStringLiteral("LibrePaint fixture"));
        lifetime->documentInfo()->setAboutInfo("language", QStringLiteral("en"));
        lifetime->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
        lifetime->setFileBatchMode(true);
        KisPropertiesConfigurationSP saveConfiguration(new KisPropertiesConfiguration);
        saveConfiguration->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
        QVERIFY2(lifetime->saveAs(kraPath, KisDocument::nativeFormatMimeType(), false, saveConfiguration), qPrintable(lifetime->errorMessage()));
        lifetime->waitForSavingToComplete();
        QCOMPARE(lifetime->undoStack()->index(), priorIndex + 1);
        QCOMPARE(lifetime->undoStack()->count(), priorIndex + 1);
        QScopedPointer<KoStore> store(KoStore::createStore(kraPath, KoStore::Read));
        QVERIFY(store && !store->bad());
        QVERIFY(store->open(QStringLiteral("mergedimage.png")));
        const auto merged = QImage::fromData(store->device()->readAll()).convertToFormat(QImage::Format_ARGB32);
        QVERIFY(store->close());
        QCOMPARE(merged.size(), QSize(32, 16));
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(merged.constBits()), merged.sizeInBytes()), pixels);
        QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
        reopened->setFileBatchMode(true);
        QVERIFY2(reopened->openPath(kraPath, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
        Document loaded(reopened.data(), false);
        QCOMPARE(loaded.pixelData(0, 0, 32, 16), pixels);
        QScopedPointer<Node> loadedSource(loaded.nodeByUniqueID(source->uuid()));
        QVERIFY(loadedSource);
        QCOMPARE(loadedSource->pixelData(1, 1, 1, 1), originalPixels);
        bool foundLayer = false;
        for (auto node = reopened->image()->root()->firstChild(); node; node = node->nextSibling()) {
            if (node->uuid() != firstUuid) continue;
            foundLayer = true;
            QCOMPARE(node->name(), first->name());
            QCOMPARE(node->opacity(), first->opacity());
            QCOMPARE(node->colorLabelIndex(), first->colorLabelIndex());
            const auto *layer = dynamic_cast<const KisLayer *>(node.data());
            QVERIFY(layer);
            for (int channel = 0; channel < 4; ++channel)
                QCOMPARE(layer->channelFlags().isEmpty() || layer->channelFlags().testBit(channel),
                         first->channelFlags().isEmpty() || first->channelFlags().testBit(channel));
        }
        QVERIFY(foundLayer);
        if (group) {
            QScopedPointer<Node> loadedParent(loadedSource->parentNode());
            QVERIFY(loadedParent);
            QCOMPARE(loadedParent->uniqueId(), firstUuid);
        }
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        namespace Files = LibrePaint::Document::Gpu;
        auto gpuLoaded = Files::PaintDocument::openKra(execution, kraPath);
        QVERIFY2(gpuLoaded.code == Files::FileCode::Success, qPrintable(gpuLoaded.diagnostic));
        const QImage pixelImage(reinterpret_cast<const uchar *>(pixels.constData()), 32, 16, QImage::Format_ARGB32);
        const auto rgba = pixelImage.convertToFormat(QImage::Format_RGBA8888);
        const auto restored = gpuLoaded.document->image()->readRgbaPixels(0, {0, 0, 32, 16}, pixels.size());
        QCOMPARE(restored.code, Gpu::ExecutionCode::Success);
        QCOMPARE(restored.bytes, QByteArray(reinterpret_cast<const char *>(rgba.constBits()), rgba.sizeInBytes()));
        InfoObject pngConfiguration;
        pngConfiguration.setProperty("alpha", true);
        pngConfiguration.setProperty("interlaced", false);
        pngConfiguration.setProperty("compression", 3);
        pngConfiguration.setProperty("saveSRGBProfile", true);
        pngConfiguration.setProperty("forceSRGB", false);
        pngConfiguration.setProperty("storeAuthor", false);
        pngConfiguration.setProperty("storeMetaData", false);
        const auto pngPath = QDir(destination).filePath(stem + QStringLiteral(".png"));
        QVERIFY2(publicDocument.exportImage(pngPath, pngConfiguration), qPrintable(lifetime->errorMessage()));
        const auto png = QImage(pngPath).convertToFormat(QImage::Format_ARGB32);
        QCOMPARE(png.size(), QSize(32, 16));
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(png.constBits()), png.sizeInBytes()), pixels);
    } else {
        QCOMPARE(first->opacity(), quint8(initialOpacity));
        QCOMPARE(first->colorLabelIndex(), 0);
        QCOMPARE(publicNode->pixelData(1, 1, 1, 1), QByteArray::fromHex("ab6543ff"));
    }
}
