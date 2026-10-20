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
#include <commands/kis_node_property_list_command.h>
#include <commands/kis_node_commands_adapter.h>
#include <GroupLayer.h>
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
    QTest::addColumn<bool>("passThrough");
    QTest::addColumn<QString>("blendMode");
    for (const bool gpu : {false, true}) {
        for (const bool group : {false, true}) {
            for (const bool multiple : {false, true}) {
                for (const bool accept : {false, true}) {
                    const QByteArray tag = QByteArray(gpu ? "gpu-" : "ordinary-")
                        + (group ? "group-" : "paint-") + (multiple ? "multiple-" : "single-") + (accept ? "accept" : "cancel");
                    QTest::newRow(tag.constData()) << gpu << multiple << accept << true << false << group << QString() << 153 << false << false << QStringLiteral("normal");
                }
            }
            const QByteArray prefix = QByteArray(gpu ? "gpu-" : "ordinary-") + (group ? "group-" : "paint-");
            const QByteArray unchanged = prefix + "unchanged-accept";
            const QByteArray mixed = prefix + "mixed-channels-accept";
            QTest::newRow(unchanged.constData()) << gpu << false << true << false << false << group << QString() << 153 << false << false << QStringLiteral("normal");
            QTest::newRow(mixed.constData()) << gpu << true << true << true << true << group << QString() << 153 << false << false << QStringLiteral("normal");
        }
    }
    QTest::newRow("gpu-busy") << true << false << false << true << false << false << QStringLiteral("busy") << 153 << false << false << QStringLiteral("normal");
    QTest::newRow("gpu-unsupported-blend") << true << false << false << true << false << false << QStringLiteral("blend") << 153 << false << false << QStringLiteral("normal");
    for (const bool edit : {false, true}) for (const bool accept : {false, true}) {
        const QByteArray tag = QByteArray("gpu-exact-opacity-") + (edit ? "edited-" : "unchanged-") + (accept ? "accept" : "cancel");
        QTest::newRow(tag.constData()) << true << false << accept << edit << false << false << QString() << 137 << false << false << QStringLiteral("normal");
    }
    QTest::newRow("gpu-completed-painting") << true << false << true << true << false << false << QString() << 153 << true << false << QStringLiteral("normal");
    for (bool gpu : {false, true}) for (bool accept : {false, true}) {
        const QByteArray tag = QByteArray(gpu ? "gpu-" : "ordinary-") + "pass-through-" + (accept ? "accept" : "cancel");
        QTest::newRow(tag.constData()) << gpu << false << accept << true << false << true << QString() << 153 << false << true << QStringLiteral("normal");
    }
    for (const auto &mode : {QStringLiteral("multiply"), QStringLiteral("screen")})
    for (bool gpu : {false, true}) for (bool group : {false, true}) for (bool accept : {false, true}) {
        const QByteArray tag = QByteArray(gpu ? "gpu-" : "ordinary-") + mode.toLatin1() + '-' +
            (group ? "group-" : "paint-") + (accept ? "accept" : "cancel");
        QTest::newRow(tag.constData()) << gpu << true << accept << true << false << group << QString() << 153 << false << false << mode;
    }

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
    QFETCH(bool, passThrough);
    QFETCH(QString, blendMode);
    const bool blended = blendMode != QLatin1String("normal");
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
    if (blended) {
        QScopedPointer<Node> upper(Node::createNode(image, second));
        QVERIFY(upper->setPixelData(QByteArray::fromHex("5d29c989"), 1, 1, 1, 1));
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
    if (passThrough) {
        auto *properties = dialog->findChild<QGroupBox *>(QStringLiteral("grpProperties"));
        QVERIFY(properties);
        bool found = false;
        for (auto *box : properties->findChildren<QCheckBox *>()) {
            if (box->text() != KisLayerPropertiesIcons::passThrough.name()) continue;
            box->setChecked(true);
            found = true;
        }
        QVERIFY(found);
        QVERIFY(dynamic_cast<KisGroupLayer *>(first.data())->passThroughMode());
    }
    if (blended) {
        publicDocument.refreshProjection();
        const auto normal = publicDocument.pixelData(1, 1, 1, 1);
        auto *composite = dialog->findChild<KisCompositeOpComboBox *>(QStringLiteral("cmbComposite"));
        QVERIFY(composite);
        composite->selectCompositeOp(KoID(blendMode));
        QCOMPARE(first->compositeOpId(), blendMode);
        QCOMPARE(second->compositeOpId(), blendMode);
        publicDocument.refreshProjection();
        QVERIFY(publicDocument.pixelData(1, 1, 1, 1) != normal);
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
            composite->selectCompositeOp(KoID(QStringLiteral("overlay")));
            QCOMPARE(first->compositeOpId(), QStringLiteral("overlay"));
        } else {
            QFAIL("Unknown layer property refusal scenario");
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
        QCOMPARE(first->compositeOpId(), blended ? blendMode : QStringLiteral("normal"));
        if (group) QCOMPARE(dynamic_cast<KisGroupLayer *>(first.data())->passThroughMode(), passThrough);
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
            QCOMPARE(node->compositeOpId(), first->compositeOpId());
            if (group) QCOMPARE(dynamic_cast<KisGroupLayer *>(node.data())->passThroughMode(), passThrough);
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
        if (blended) {
            const auto gpuImage = gpuLoaded.document->image();
            Gpu::LayerId sourceId = 0;
            for (const auto &layer : gpuImage->operations().snapshot()->layers) {
                if (layer.uuid == firstUuid || layer.uuid == secondUuid)
                    QCOMPARE(layer.blendMode, (blendMode == QLatin1String("screen") ? Gpu::LayerBlendMode::Screen : Gpu::LayerBlendMode::Multiply));
                if (layer.uuid == source->uuid()) sourceId = layer.id;
            }
            QVERIFY(sourceId);
            QVERIFY(loadedSource->setPixelData(QByteArray::fromHex("713ce2a5"), 1, 1, 1, 1));
            loaded.refreshProjection();
            QCOMPARE(gpuImage->writeRgbaPixels(sourceId, {1, 1, 1, 1}, QByteArray::fromHex("e23c71a5")), Gpu::Result::Success);
            const auto editedPixels = loaded.pixelData(0, 0, 32, 16);
            const QImage editedImage(reinterpret_cast<const uchar *>(editedPixels.constData()), 32, 16, QImage::Format_ARGB32);
            const auto editedRgba = editedImage.convertToFormat(QImage::Format_RGBA8888);
            const QByteArray expectedEdited(reinterpret_cast<const char *>(editedRgba.constBits()), editedRgba.sizeInBytes());
            const auto actualEdited = gpuImage->readRgbaPixels(0, {0, 0, 32, 16}, editedPixels.size());
            QCOMPARE(actualEdited.code, Gpu::ExecutionCode::Success);
            QCOMPARE(actualEdited.bytes, expectedEdited);
            QVERIFY(editedPixels != pixels);
            QFile artifact(QDir(destination).filePath(stem + QStringLiteral("-edited.rgba")));
            QVERIFY(artifact.open(QIODevice::WriteOnly));
            QCOMPARE(artifact.write(actualEdited.bytes), actualEdited.bytes.size());
        }
    } else {
        QCOMPARE(first->opacity(), quint8(initialOpacity));
        QCOMPARE(first->colorLabelIndex(), 0);
        QCOMPARE(publicNode->pixelData(1, 1, 1, 1), QByteArray::fromHex("ab6543ff"));
    }
}

void TestNode::testBlendModeChangesPreservePublicHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<QString>("entry");
    QTest::addColumn<QString>("blendMode");
    for (const auto &mode : {QStringLiteral("multiply"), QStringLiteral("screen")})
    for (bool gpu : {false, true}) for (const auto &entry : {QStringLiteral("api"), QStringLiteral("panel"), QStringLiteral("direct")}) {
        const QByteArray name = QByteArray(gpu ? "gpu-" : "ordinary-") + entry.toLatin1() + '-' + mode.toLatin1();
        QTest::newRow(name.constData()) << gpu << entry << mode;
    }
}

void TestNode::testBlendModeChangesPreservePublicHistory()
{
    QFETCH(bool, gpu);
    QFETCH(QString, entry);
    QFETCH(QString, blendMode);
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    KisImageSP image;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        Gpu::RasterImage input;
        input.size = {4, 4};
        input.layers = {{QStringLiteral("Destination"), true, {}, {}}, {QStringLiteral("Source"), true, {}, {}}};
        input.layers[1].opacity = 137;
        image = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, input), QStringLiteral("Blending mode"));
    } else {
        image = new KisImage(document->createUndoStore(), 4, 4, KoColorSpaceRegistry::instance()->rgb8(), "Blending mode");
        image->addNode(new KisPaintLayer(image, QStringLiteral("Destination"), 255));
        image->addNode(new KisPaintLayer(image, QStringLiteral("Source"), 137));
    }
    document->setCurrentImage(image);
    const auto lower = image->root()->firstChild();
    auto upper = image->root()->lastChild();
    QScopedPointer<Node> destination(Node::createNode(image, lower));
    QScopedPointer<Node> source(Node::createNode(image, upper));
    const auto sourcePixels = QByteArray::fromHex("5d29c989");
    QVERIFY(destination->setPixelData(QByteArray::fromHex("975925ad"), 1, 1, 1, 1));
    QVERIFY(source->setPixelData(sourcePixels, 1, 1, 1, 1));
    Document api(document.data(), false);
    api.refreshProjection();
    const auto normal = api.pixelData(1, 1, 1, 1);
    if (entry == QLatin1String("api")) source->setBlendingMode(blendMode);
    else if (entry == QLatin1String("panel")) {
        KisNodeCommandsAdapter commands(image);
        commands.setCompositeOp(upper, image->colorSpace()->compositeOp(blendMode));
    } else upper->setCompositeOpId(blendMode);
    image->waitForDone();
    QCOMPARE(source->blendingMode(), blendMode);
    QCOMPARE(document->undoStack()->index(), entry == QLatin1String("direct") ? 0 : 1);
    api.refreshProjection();
    const auto blended = QByteArray::fromHex(blendMode == QLatin1String("screen") ? "9a5b65c5" : "784138c5");
    QCOMPARE(api.pixelData(1, 1, 1, 1), blended);
    QCOMPARE(source->pixelData(1, 1, 1, 1), sourcePixels);
    if (entry != QLatin1String("direct")) {
        document->undoStack()->undo();
        image->waitForDone();
        QCOMPARE(source->blendingMode(), QStringLiteral("normal"));
        api.refreshProjection();
        QCOMPARE(api.pixelData(1, 1, 1, 1), normal);
        document->undoStack()->redo();
        image->waitForDone();
        QCOMPARE(source->blendingMode(), blendMode);
        api.refreshProjection();
        QCOMPARE(api.pixelData(1, 1, 1, 1), blended);
    }
    QScopedPointer<Node> copy(source->clone());
    QVERIFY(copy);
    QCOMPARE(copy->blendingMode(), blendMode);
    QCOMPARE(copy->pixelData(1, 1, 1, 1), sourcePixels);
    QScopedPointer<Node> root(api.rootNode());
    QVERIFY(root->addChildNode(copy.data(), nullptr));
    image->waitForDone();
    QCOMPARE(copy->blendingMode(), blendMode);
    if (gpu) {
        const auto owner = image->gpuImage();
        QCOMPARE(owner->operations().snapshot()->layers[1].blendMode, (blendMode == QLatin1String("screen") ? Gpu::LayerBlendMode::Screen : Gpu::LayerBlendMode::Multiply));
        const auto stroke = owner->beginStroke(2, Gpu::Brush{}, 43);
        QCOMPARE(owner->appendSamples(stroke, {{{3, 3}}}), Gpu::Result::Success);
        const auto pending = owner->operations().preview();
        const auto index = document->undoStack()->index();
        source->setBlendingMode(QStringLiteral("normal"));
        QCOMPARE(owner->operations().preview(), pending);
        QCOMPARE(source->blendingMode(), blendMode);
        QCOMPARE(document->undoStack()->index(), index);
        source->setBlendingMode(QStringLiteral("overlay"));
        QCOMPARE(owner->operations().preview(), pending);
        QCOMPARE(source->blendingMode(), blendMode);
        QCOMPARE(owner->cancelStroke(stroke), Gpu::Result::Success);
    }
}

void TestNode::testPassThroughGroupsKeepPublicReadsAndPropertyHistory()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    KisConfig(true).disableOpenGL();
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage input;
    input.size = {4, 4};
    const QUuid groupUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    input.layers = {{QStringLiteral("Bottom"), true, {}, {}, {37, 89, 151, 173}},
                    {QStringLiteral("First"), true, {}, {}, {201, 41, 93, 137}},
                    {QStringLiteral("Second"), true, {}, {}, {17, 211, 77, 189}},
                    {QStringLiteral("Group"), true, groupUuid, {}}};
    input.layers[1].parentUuid = input.layers[2].parentUuid = groupUuid;
    input.layers[1].opacity = 153;
    input.layers[2].opacity = 211;
    input.layers[3].kind = Gpu::LayerKind::Group;
    input.layers[3].opacity = 137;
    input.layers[3].projectionChannelMask = 9;
    const auto owner = Gpu::PaintImageSP::create(execution, input);
    const auto image = KisImage::fromGpuImage(owner, QStringLiteral("Pass-through group"));
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(image);
    Document publicDocument(document.data(), false);
    QScopedPointer<Node> publicNode(publicDocument.nodeByUniqueID(groupUuid));
    auto *publicGroup = dynamic_cast<GroupLayer *>(publicNode.data());
    QVERIFY(publicGroup);
    QVERIFY(!publicGroup->passThroughMode());
    QCOMPARE(publicGroup->pixelData(0, 0, 1, 1), QByteArray());
    const auto own = QByteArray::fromHex("50b630bc");
    QCOMPARE(publicGroup->projectionPixelData(0, 0, 1, 1), own);
    auto group = image->root()->lastChild();
    QVERIFY(group && group->uuid() == groupUuid);
    auto properties = group->sectionModelProperties();
    for (auto &property : properties)
        if (property.id == KisLayerPropertiesIcons::passThrough.id()) property.state = true;
    try {
        KisNodePropertyListCommand::setNodePropertiesAutoUndo(group, image, properties);
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    QVERIFY(publicGroup->passThroughMode());
    QVERIFY(owner->operations().snapshot()->layers[3].passThrough);
    QCOMPARE(document->undoStack()->index(), 1);
    QCOMPARE(document->undoStack()->count(), 1);
    QCOMPARE(publicGroup->projectionPixelData(0, 0, 1, 1), own);
    QCOMPARE(publicGroup->thumbnail(2, 2).pixelColor(0, 0), QColor(48, 182, 80, 188));
    QCOMPARE(publicDocument.pixelData(0, 0, 1, 1), QByteArray::fromHex("975934d1"));
    document->undoStack()->undo();
    QVERIFY(!publicGroup->passThroughMode());
    QCOMPARE(publicDocument.pixelData(0, 0, 1, 1), QByteArray::fromHex("97592acd"));
    document->undoStack()->redo();
    QVERIFY(publicGroup->passThroughMode());
    QCOMPARE(publicDocument.pixelData(0, 0, 1, 1), QByteArray::fromHex("975934d1"));
    publicGroup->setPassThroughMode(false);
    QCOMPARE(document->undoStack()->index(), 1);
    QVERIFY(!owner->operations().snapshot()->layers[3].passThrough);
    QCOMPARE(publicGroup->projectionPixelData(0, 0, 1, 1), own);
    publicGroup->setPassThroughMode(true);
    QScopedPointer<Node> copied(publicGroup->clone());
    QVERIFY(copied);
    const auto *copiedGroup = dynamic_cast<GroupLayer *>(copied.data());
    QVERIFY(copiedGroup && copiedGroup->passThroughMode());
    QCOMPARE(copied->projectionPixelData(0, 0, 1, 1), own);
}
