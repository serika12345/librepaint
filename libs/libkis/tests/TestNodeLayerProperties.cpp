/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "TestNode.h"

#include <simpletest.h>
#include <Document.h>
#include <Node.h>
#include <TransparencyMask.h>
#include <Selection.h>
#include <InfoObject.h>
#include <KoStore.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include <KoColor.h>
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
#include <kis_transparency_mask.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
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
    for (const auto &mode : {QStringLiteral("multiply"), QStringLiteral("screen"), QStringLiteral("darken"), QStringLiteral("lighten"), QStringLiteral("add"), QStringLiteral("linear_dodge")})
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
                    QCOMPARE(layer.blendMode, (blendMode == QLatin1String("screen") ? Gpu::LayerBlendMode::Screen :
                        blendMode == QLatin1String("darken") ? Gpu::LayerBlendMode::Darken :
                        blendMode == QLatin1String("lighten") ? Gpu::LayerBlendMode::Lighten :
                        blendMode == QLatin1String("add") ? Gpu::LayerBlendMode::Addition :
                        blendMode == QLatin1String("linear_dodge") ? Gpu::LayerBlendMode::LinearDodge : Gpu::LayerBlendMode::Multiply));
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
    for (const auto &mode : {QStringLiteral("multiply"), QStringLiteral("screen"), QStringLiteral("darken"), QStringLiteral("lighten"), QStringLiteral("add"), QStringLiteral("linear_dodge")})
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
    const bool addition = blendMode == QLatin1String("add") || blendMode == QLatin1String("linear_dodge");
    const auto sourcePixels = QByteArray::fromHex(addition ? "f0e0d089" : "5d29c989");
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
    const auto blended = QByteArray::fromHex(addition ? "bc946ec5" : blendMode == QLatin1String("screen") ? "9a5b65c5" :
        blendMode == QLatin1String("darken") ? "814739c5" : blendMode == QLatin1String("lighten") ? "915464c5" : "784138c5");
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
        QCOMPARE(owner->operations().snapshot()->layers[1].blendMode, (blendMode == QLatin1String("screen") ? Gpu::LayerBlendMode::Screen :
                        blendMode == QLatin1String("darken") ? Gpu::LayerBlendMode::Darken :
                        blendMode == QLatin1String("lighten") ? Gpu::LayerBlendMode::Lighten :
                        blendMode == QLatin1String("add") ? Gpu::LayerBlendMode::Addition :
                        blendMode == QLatin1String("linear_dodge") ? Gpu::LayerBlendMode::LinearDodge : Gpu::LayerBlendMode::Multiply));
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

void TestNode::testMaskedProjectionFollowsEditsAndReopen_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<int>("maskDefault");
    QTest::addColumn<bool>("sourceDefault");
    for (bool words : {false, true}) for (int maskDefault : {0, 17, 255}) for (bool sourceDefault : {false, true}) {
        const auto name = QByteArray(words ? "u16" : "u8") + "-mask" + QByteArray::number(maskDefault) + (sourceDefault ? "-default" : "-transparent");
        QTest::newRow(name.constData()) << words << maskDefault << sourceDefault;
    }
}

void TestNode::testMaskedProjectionFollowsEditsAndReopen()
{
    // Scripts distinguish raw paint pixels from a masked node projection,
    // retain the projection through mask edits, and continue after KRA/Open.
    QFETCH(bool, words);
    QFETCH(int, maskDefault);
    QFETCH(bool, sourceDefault);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    QByteArray source;
    for (int i = 0; i < 8; ++i) {
        const quint16 channels[]{quint16(37 * (words ? 257 : 1)), quint16(89 * (words ? 257 : 1)),
            quint16(151 * (words ? 257 : 1)), quint16((i < 4 ? 219 : 0) * (words ? 257 : 1))};
        source.append(reinterpret_cast<const char *>(channels), words ? 8 : 0);
        if (!words) for (const auto channel : channels) source.append(char(channel));
    }
    QList<QByteArray> reference;
    QList<QRect> referenceBounds;
    for (bool gpu : {false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
        document->setFileBatchMode(true); document->setAutoSaveActive(false);
        QVERIFY(document->newImage(QStringLiteral("Masked projection"), 32, 16, cs,
            KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 2, {}, 1));
        auto image = KisImageSP(document->image());
        const auto layer = image->root()->lastChild();
        QScopedPointer<Node> node(Node::createNode(image, layer));
        node->setLocked(false); node->setVisible(true); node->setOpacity(85);
        QVERIFY(node->setPixelData(source, -2, 3, 8, 1));
        QVERIFY(node->setPixelData(source.first(cs->pixelSize()), 180, 95, 1, 1));
        if (sourceDefault) layer->paintDevice()->setDefaultPixel(KoColor(QColor(11, 63, 129, 127), cs));
        Document api(document.data(), false);
        QScopedPointer<TransparencyMask> mask(api.createTransparencyMask(QStringLiteral("Coverage")));
        Selection coverage;
        const quint8 defaultCoverage = maskDefault;
        coverage.selection()->pixelSelection()->setDefaultPixel(KoColor(&defaultCoverage, KoColorSpaceRegistry::instance()->alpha8()));
        coverage.setPixelData(QByteArray::fromHex("000180ff000180ff"), -2, 3, 8, 1);
        mask->setSelection(&coverage);
        QVERIFY(node->addChildNode(mask.data(), nullptr));
        const auto settle = [&] { layer->setDirty(image->bounds() | QRect(-64, -64, 192, 192)); image->waitForDone(); };
        settle();
        auto projection = layer->projection();
        QVERIFY(projection);
        int stage = 0;
        const auto check = [&] {
            const auto pixels = node->projectionPixelData(-3, 2, 10, 3);
            QByteArray retained(10 * 3 * cs->pixelSize(), '\0');
            projection->readBytes(reinterpret_cast<quint8 *>(retained.data()), {-3, 2, 10, 3});
            QCOMPARE(retained, pixels);
            QCOMPARE(node->pixelData(-2, 3, 8, 1), source);
            const auto implicit = projection->defaultPixel();
            const auto state = pixels + QByteArray(reinterpret_cast<const char *>(implicit.data()), cs->pixelSize()) +
                node->projectionPixelData(180, 95, 1, 1) + node->projectionPixelData(200, 123, 1, 1);
            const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
            if (!results.isEmpty()) {
                QVERIFY(QDir().mkpath(results));
                QFile output(QDir(results).filePath(QStringLiteral("masked-projection-%1-%2-%3.bin")
                    .arg(QString::fromLatin1(QTest::currentDataTag()), gpu ? QStringLiteral("gpu") : QStringLiteral("native")).arg(stage)));
                QVERIFY(output.open(QIODevice::WriteOnly)); QCOMPARE(output.write(state), state.size());
            }
            if (!gpu) reference.append(state);
            else QCOMPARE(state, reference.value(stage));
            const auto bounds = projection->exactBounds();
            if (!gpu) referenceBounds.append(bounds);
            else QCOMPARE(bounds, referenceBounds.value(stage));
            ++stage;
        };
        check();
        if (const auto owner = image->gpuImage()) {
            const auto fixed = owner->operations().snapshot();
            const auto rejected = owner->execution().readLayerProjectionPixels(owner->handle(), fixed,
                fixed->layers.last().id, {-3, 2, 10, 3}, 0);
            QCOMPARE(rejected.code, LibrePaint::Image::Gpu::ExecutionCode::CapacityExceeded);
            QVERIFY(rejected.bytes.isEmpty());
            QCOMPARE(owner->operations().snapshot(), fixed);
            QCOMPARE(owner->execution().requested(owner->handle()), fixed);
        }
        QVERIFY(node->projectionPixelData(-2, 3, 8, 1) != source);
        Selection replacement;
        replacement.selection()->pixelSelection()->setDefaultPixel(KoColor(&defaultCoverage, KoColorSpaceRegistry::instance()->alpha8()));
        replacement.setPixelData(QByteArray::fromHex("ff800100ff800100"), -2, 3, 8, 1);
        mask->setSelection(&replacement); settle(); check();
        QVERIFY(mask->remove()); image->waitForDone();
        QCOMPARE(node->projectionPixelData(-2, 3, 8, 1), source);
        document->undoStack()->undo(); settle(); check();
        document->undoStack()->redo(); image->waitForDone();
        QCOMPARE(node->projectionPixelData(-2, 3, 8, 1), source);
        document->undoStack()->undo(); settle(); check();
        const auto expected = node->projectionPixelData(-3, 2, 10, 3);
        const auto identity = node->uniqueId();
        QTemporaryDir directory; QVERIFY(directory.isValid());
        const auto path = directory.filePath(QStringLiteral("masked-projection.kra"));
        QVERIFY(document->saveAs(path, KisDocument::nativeFormatMimeType(), false));
        document->waitForSavingToComplete();
        QVERIFY(document->openPath(path, KisDocument::DontAddToRecent));
        image = document->image(); image->waitForDone();
        QScopedPointer<Node> loaded(api.nodeByUniqueID(identity));
        QVERIFY(loaded);
        QCOMPARE(loaded->projectionPixelData(-3, 2, 10, 3), expected);
        QVERIFY(loaded->setPixelData(source, -2, 4, 8, 1));
        QCOMPARE(loaded->pixelData(-2, 4, 8, 1), source);
        QCOMPARE(image->root()->lastChild()->uuid(), identity);
        image->root()->lastChild()->setDirty(image->bounds() | QRect(-64, -64, 192, 192)); image->waitForDone();
        const auto edited = loaded->projectionPixelData(-2, 4, 8, 1);
        if (!gpu) reference.append(edited);
        else QCOMPARE(edited, reference.value(stage));
    }
}

void TestNode::testGpuMaskInsertionPropertiesHistoryAndReopen()
{
    // Scripts attach a prepared mask, keep its wrapper across history, and find
    // the same identity after KRA/Open before making further property edits.
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    const auto restoreEngine = qScopeGuard([&] { if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine); });
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage input;
    input.size = {32, 32};
    input.layers = {{QStringLiteral("Paint")}, {QStringLiteral("Other")}};
    input.layers.first().defaultPixel = {211, 67, 23, 255};
    const auto owner = Gpu::PaintImageSP::create(execution, input);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    const auto image = KisImage::fromGpuImage(owner, QStringLiteral("Public mask properties"));
    document->setCurrentImage(image);
    Document publicDocument(document.data(), false);
    QScopedPointer<Node> parent(Node::createNode(image, image->root()->firstChild()));
    QScopedPointer<Node> other(Node::createNode(image, image->root()->lastChild()));
    QScopedPointer<TransparencyMask> mask(publicDocument.createTransparencyMask(QStringLiteral("Prepared cutouts")));
    QVERIFY(mask);
    const auto identity = mask->uniqueId();
    Selection selection;
    selection.setPixelData(QByteArray::fromHex("11e9"), 4, 7, 2, 1);
    mask->setSelection(&selection);
    mask->setColorLabel(4);
    mask->setPinnedToTimeline(true);
    mask->setVisible(false);
    mask->setLocked(true);
    QCOMPARE(owner->setSelectionRect(QRect(1, 2, 8, 9)), Gpu::Result::Success);
    QVERIFY(parent->addChildNode(mask.data(), nullptr));
    QCOMPARE(owner->selectionRect(), QRect(1, 2, 8, 9));
    QScopedPointer<Node> attached(publicDocument.nodeByUniqueID(identity));
    QVERIFY(attached && *attached == *mask);
    const auto fixed = owner->operations().snapshot();
    QCOMPARE(fixed->layers.first().transparencyMask->coverage(4, 7), quint8(17));
    QCOMPARE(fixed->layers.first().transparencyMask->coverage(5, 7), quint8(233));
    QCOMPARE(fixed->layers.first().transparencyMask->defaultCoverage(), quint8(0));
    const int count = document->undoStack()->count();
    Selection replacement;
    replacement.setPixelData(QByteArray::fromHex("25b7"), 4, 7, 2, 1);
    mask->setSelection(&replacement);
    QCOMPARE(document->undoStack()->count(), count);
    QCOMPARE(owner->operations().snapshot()->layers.first().transparencyMask->coverage(4, 7), quint8(37));
    QCOMPARE(fixed->layers.first().transparencyMask->coverage(4, 7), quint8(17));
    mask->setName(QStringLiteral("Renamed cutouts"));
    mask->setColorLabel(6);
    mask->setPinnedToTimeline(false);
    QCOMPARE(document->undoStack()->count(), count);
    QCOMPARE(fixed->layers.first().transparencyMaskName, QStringLiteral("Prepared cutouts"));
    QCOMPARE(fixed->layers.first().transparencyMaskColorLabelIndex, 4);
    QVERIFY(fixed->layers.first().transparencyMaskPinnedToTimeline);
    QCOMPARE(owner->operations().snapshot()->layers.first().transparencyMaskName, mask->name());
    QCOMPARE(owner->operations().snapshot()->layers.first().transparencyMaskColorLabelIndex, 6);
    QVERIFY(!owner->operations().snapshot()->layers.first().transparencyMaskPinnedToTimeline);
    QVERIFY(mask->remove());
    QVERIFY(!image->root()->firstChild()->firstChild());
    document->undoStack()->undo();
    QScopedPointer<Node> restored(publicDocument.nodeByUniqueID(identity));
    QVERIFY(restored && *restored == *mask);
    QCOMPARE(mask->name(), QStringLiteral("Renamed cutouts"));
    document->undoStack()->redo();
    document->undoStack()->undo();
    QScopedPointer<Node> restoredParent(mask->parentNode());
    QVERIFY(restoredParent);
    QScopedPointer<Node> copy(mask->duplicate());
    QVERIFY(copy && copy->uniqueId() != identity);
    QVERIFY(other->addChildNode(copy.data(), nullptr));
    QCOMPARE(owner->operations().snapshot()->layers.last().transparencyMaskName, mask->name());
    QCOMPARE(owner->operations().snapshot()->layers.last().transparencyMask->coverage(4, 7), quint8(37));
    QCOMPARE(owner->operations().snapshot()->layers.last().transparencyMaskColorLabelIndex, 6);
    QScopedPointer<Node> parentCopy(parent->duplicate());
    QVERIFY(parentCopy);
    const auto copiedChildren = parentCopy->childNodes();
    QVERIFY(copiedChildren.size() == 1);
    QScopedPointer<Node> copiedChild(copiedChildren.first());
    QCOMPARE(copiedChild->name(), mask->name());
    QCOMPARE(copiedChild->colorLabel(), 6);
    const auto copiedChildId = copiedChild->uniqueId();
    copiedChild->setName(QStringLiteral("Prepared layer copy mask"));
    QScopedPointer<Node> root(publicDocument.rootNode());
    QVERIFY(root->addChildNode(parentCopy.data(), nullptr));
    const auto copiedLayer = owner->operations().snapshot()->layers.last();
    QCOMPARE(copiedLayer.transparencyMaskName, QStringLiteral("Prepared layer copy mask"));
    QCOMPARE(copiedLayer.transparencyMaskUuid, copiedChildId);
    QScopedPointer<Node> adoptedChild(publicDocument.nodeByUniqueID(copiedChildId));
    QVERIFY(adoptedChild && *adoptedChild == *copiedChild);
    QCOMPARE(copiedLayer.transparencyMaskColorLabelIndex, 6);
    QCOMPARE(copiedLayer.transparencyMask->coverage(4, 7), quint8(37));
    QVERIFY(copiedLayer.transparencyMaskUuid != identity);
    QVERIFY(copiedLayer.transparencyMaskUuid != copy->uniqueId());
    QTemporaryDir output;
    QVERIFY(output.isValid());
    const auto path = output.filePath(QStringLiteral("mask-properties.kra"));
    document->setFileBatchMode(true);
    QVERIFY2(document->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    QVERIFY(reopened->image()->gpuImage());
    Document loaded(reopened.data(), false);
    QScopedPointer<Node> loadedMask(loaded.nodeByUniqueID(identity));
    QVERIFY(loadedMask);
    QCOMPARE(loadedMask->name(), mask->name());
    QCOMPARE(loadedMask->colorLabel(), 6);
    QVERIFY(!loadedMask->isPinnedToTimeline());
    QVERIFY(!loadedMask->visible());
    QVERIFY(loadedMask->locked());
    auto *loadedTransparencyMask = qobject_cast<TransparencyMask *>(loadedMask.data());
    QVERIFY(loadedTransparencyMask);
    QScopedPointer<Selection> loadedSelection(loadedTransparencyMask->selection());
    QCOMPARE(loadedSelection->pixelData(4, 7, 2, 1), QByteArray::fromHex("25b7"));
    loadedMask->setName(QStringLiteral("Edited after Open"));
    loadedMask->setColorLabel(2);
    QVERIFY(loadedMask->remove());
    reopened->undoStack()->undo();
    QCOMPARE(loadedMask->name(), QStringLiteral("Edited after Open"));
    QCOMPARE(loadedMask->colorLabel(), 2);
    QCOMPARE(reopened->image()->gpuImage()->operations().snapshot()->layers.first().transparencyMaskName, loadedMask->name());
    QVERIFY2(reopened->saveAs(output.filePath(QStringLiteral("re-edited.kra")), KisDocument::nativeFormatMimeType(), false), qPrintable(reopened->errorMessage()));
    reopened->waitForSavingToComplete();
}

void TestNode::testMaskSelectionWritesPreserveNativeHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::newRow("native") << false;
    QTest::newRow("gpu") << true;
}

void TestNode::testMaskSelectionWritesPreserveNativeHistory()
{
    // setSelection replaces the selection object directly. Undoing a previous
    // pixel transaction keeps that replacement on the same mask node.
    QFETCH(bool, gpu);
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    Gpu::PaintImageSP owner;
    KisImageSP image;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        Gpu::RasterImage input;
        input.size = {32, 32}; input.layers = {{QStringLiteral("Paint")}};
        owner = Gpu::PaintImageSP::create(execution, input);
        image = KisImage::fromGpuImage(owner, QStringLiteral("Mask selection"));
    } else {
        image = new KisImage(document->createUndoStore(), 32, 32, KoColorSpaceRegistry::instance()->rgb8(), QStringLiteral("Mask selection"));
        image->addNode(new KisPaintLayer(image, QStringLiteral("Paint"), 255));
    }
    document->setCurrentImage(image);
    Document publicDocument(document.data(), false);
    QScopedPointer<Node> parent(Node::createNode(image, image->root()->firstChild()));
    QScopedPointer<TransparencyMask> mask(publicDocument.createTransparencyMask(QStringLiteral("Mask")));
    Selection initial; initial.select(4, 7, 2, 1, 17);
    mask->setSelection(&initial);
    QVERIFY(parent->addChildNode(mask.data(), nullptr));
    const auto maskNode = qobject_cast<KisTransparencyMask *>(image->root()->firstChild()->firstChild().data());
    QVERIFY(maskNode);
    if (gpu) {
        const auto pixels = Gpu::SelectionMask::fromAlphaCoverage(QRect(4, 7, 2, 1), QByteArray(2, char(100)), QRect(0, 0, 32, 32));
        QCOMPARE(owner->replaceTransparencyMask(1, pixels), Gpu::Result::Success);
    } else {
        KisTransaction transaction(maskNode->selection()->pixelSelection());
        maskNode->selection()->pixelSelection()->select(QRect(4, 7, 2, 1), 100);
        transaction.commit(image->undoAdapter());
    }
    image->waitForDone();
    const int count = document->undoStack()->count();
    Selection replacement; replacement.select(4, 7, 2, 1, 37);
    mask->setSelection(&replacement);
    QCOMPARE(document->undoStack()->count(), count);
    const auto check = [&] {
        QScopedPointer<Selection> pixels(mask->selection());
        return pixels->pixelData(4, 7, 2, 1) == QByteArray(2, char(37));
    };
    QVERIFY(check());
    document->undoStack()->undo(); image->waitForDone(); QVERIFY(check());
    document->undoStack()->redo(); image->waitForDone(); QVERIFY(check());
    document->undoStack()->undo(); image->waitForDone();
    document->undoStack()->undo(); image->waitForDone();
    QVERIFY(!maskNode->parent());
    document->undoStack()->redo(); image->waitForDone();
    QCOMPARE(image->root()->firstChild()->firstChild().data(), maskNode);
    QVERIFY(check());
}
