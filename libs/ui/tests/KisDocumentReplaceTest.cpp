/*
 *  SPDX-FileCopyrightText: 2019 Tusooa Zhu <tusooa@vista.aero>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisDocumentReplaceTest.h"

#include <document/KisDocument.h>
#include <application/ui/orchestration/KisPart.h>
#include <QMap>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QScopeGuard>
#include <kundo2stack.h>
#include <KoProgressProxy.h>
#include <kis_composite_progress_proxy.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>
#include <metadata/KoDocumentInfo.h>
#include <stdexcept>
#include <kis_image_animation_interface.h>
#include <kis_layer_utils.h>
#include <kis_time_span.h>
#include <kis_types.h>
#include <testui.h>

namespace {
class RecordingProgressProxy : public KoProgressProxy
{
public:
    int maximum() const override
    {
        return m_maximum;
    }

    void setValue(int value) override
    {
        m_value = value;
    }

    void setRange(int, int maximum) override
    {
        m_maximum = maximum;
    }

    void setFormat(const QString &) override
    {
    }

    int value() const
    {
        return m_value;
    }

private:
    int m_maximum = 0;
    int m_value = 0;
};
}


void KisDocumentReplaceTest::init()
{
    m_doc = KisPart::instance()->createDocument();
    qDebug() << m_doc->newImage("test", 512, 512, KoColorSpaceRegistry::instance()->colorSpace("RGBA", "U8", 0), KoColor(), KisConfig::RASTER_LAYER, 1, "", 96);
}

void KisDocumentReplaceTest::finalize()
{
    delete m_doc;
    m_doc = 0;
}

void KisDocumentReplaceTest::testCopyFromDocument()
{
    init();
    QScopedPointer<KisDocument> clonedDoc(m_doc->lockAndCreateSnapshot());
    KisDocument *anotherDoc = KisPart::instance()->createDocument();
    anotherDoc->newImage("test", 512, 512, KoColorSpaceRegistry::instance()->colorSpace("RGBA", "U8", 0), KoColor(), KisConfig::RASTER_LAYER, 2, "", 96);
    KisImageSP anotherImage(anotherDoc->image());
    KisNodeSP root(anotherImage->rootLayer());
    anotherDoc->copyFromDocument(*(clonedDoc.data()));
    // image pointer should not change
    QCOMPARE(anotherImage.data(), anotherDoc->image().data());
    // root node should change
    QVERIFY(root.data() != anotherDoc->image()->rootLayer().data());
    // node count should be the same
    QList<KisNodeSP> oldNodes, newNodes;
    KisLayerUtils::recursiveApplyNodes(clonedDoc->image()->root(), [&oldNodes](KisNodeSP node) { oldNodes << node; });
    KisLayerUtils::recursiveApplyNodes(anotherDoc->image()->root(), [&newNodes](KisNodeSP node) { newNodes << node; });
    QCOMPARE(oldNodes.size(), newNodes.size());

    KisPart::instance()->removeDocument(anotherDoc);
    finalize();
}

void KisDocumentReplaceTest::testGpuReplacementKeepsReferencesAndRejectsCapacityBeforeMutation_data()
{
    QTest::addColumn<bool>("capacityFailure");
    QTest::newRow("independent-replacement") << false;
    QTest::newRow("full-document-capacity") << true;
}

void KisDocumentReplaceTest::testGpuReplacementKeepsReferencesAndRejectsCapacityBeforeMutation()
{
    QFETCH(bool, capacityFailure);
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device, 128 * 1024 * 1024, capacityFailure ? 2 : 16);
    const auto sourceOwner = Gpu::PaintImageSP::create(execution, QSize(64, 32));
    const auto oldOwner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
    const auto layer = sourceOwner->operations().activeLayer();
    QCOMPARE(sourceOwner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    QCOMPARE(oldOwner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("aabbccff")), Gpu::Result::Success);
    QScopedPointer<KisDocument> source(KisPart::instance()->createDocument());
    QScopedPointer<KisDocument> target(KisPart::instance()->createDocument());
    source->setCurrentImage(KisImage::fromGpuImage(sourceOwner, QStringLiteral("Source image")));
    target->setCurrentImage(KisImage::fromGpuImage(oldOwner, QStringLiteral("Target image")));
    source->setPath(QStringLiteral("/tmp/source-image.kra"));
    target->setPath(QStringLiteral("/tmp/target-image.kra"));
    source->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("Source title"));
    target->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("Target title"));
    target->setModified(true);
    const auto image = KisImageSP(target->image());
    const auto root = image->root();
    const auto retained = root->firstChild()->paintDevice();
    const auto previous = oldOwner->operations().snapshot();
    if (capacityFailure) {
        QVERIFY_EXCEPTION_THROWN(target->copyFromDocument(*source), std::length_error);
        QCOMPARE(target->image().data(), image.data());
        QCOMPARE(image->root(), root);
        QCOMPARE(target->path(), QStringLiteral("/tmp/target-image.kra"));
        QCOMPARE(target->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Target title"));
        QVERIFY(target->isModified());
        QCOMPARE(image->gpuImage(), oldOwner);
        QCOMPARE(oldOwner->operations().snapshot(), previous);
        QCOMPARE(execution.requested(oldOwner->handle()), previous);
    } else {
        target->copyFromDocument(*source);
        QCOMPARE(target->image().data(), image.data());
        QVERIFY(image->root() != root);
        QCOMPARE(image->bounds().size(), QSize(64, 32));
        QCOMPARE(target->path(), source->path());
        QCOMPARE(target->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Source title"));
        QVERIFY(target->isModified());
        const auto copied = image->gpuImage();
        QVERIFY(copied != sourceOwner);
        QVERIFY(copied != oldOwner);
        QCOMPARE(copied->operations().undoCount(), 0);
        QCOMPARE(copied->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
        target->setModified(false);
        QSignalSpy changes(image.data(), &KisImage::sigImageModified);
        QCOMPARE(oldOwner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("ddeeffff")), Gpu::Result::Success);
        QVERIFY(changes.isEmpty());
        QVERIFY(!target->isModified());
        QCOMPARE(sourceOwner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("445566ff")), Gpu::Result::Success);
        QCOMPARE(copied->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
        QCOMPARE(copied->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("778899ff")), Gpu::Result::Success);
        QVERIFY(target->isModified());
        QCOMPARE(sourceOwner->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("445566ff"));
        QByteArray bytes(4, Qt::Uninitialized);
        retained->readBytes(reinterpret_cast<quint8 *>(bytes.data()), 5, 5, 1, 1);
        QCOMPARE(bytes, QByteArray::fromHex("ffeeddff"));
    }
    KisPart::instance()->removeDocument(source.data(), false);
    KisPart::instance()->removeDocument(target.data(), false);
}

void KisDocumentReplaceTest::testDocumentIdentityDelegation()
{
    init();
    QSignalSpy pathChangedSpy(m_doc, &KisDocument::sigPathChanged);

    const QString displayPath = QStringLiteral("content://provider/document/42");
    const QString localPath = QStringLiteral("/tmp/librepaint-open-42.kra");
    m_doc->setPath(displayPath);
    m_doc->setPath(displayPath);
    m_doc->setLocalFilePath(localPath);
    m_doc->setMimeType("application/x-krita");

    QCOMPARE(pathChangedSpy.count(), 1);
    QCOMPARE(m_doc->path(), displayPath);
    QCOMPARE(m_doc->localFilePath(), localPath);
    QCOMPARE(m_doc->mimeType(), QByteArray("application/x-krita"));

    QScopedPointer<KisDocument> snapshot(m_doc->lockAndCreateSnapshot());
    QVERIFY(snapshot);
    QCOMPARE(snapshot->path(), displayPath);
    QCOMPARE(snapshot->localFilePath(), localPath);
    QCOMPARE(snapshot->mimeType(), QByteArray("application/x-krita"));

    QString localPathDuringResetSignal;
    connect(m_doc, &KisDocument::sigPathChanged, this, [&](const QString &path) {
        if (path.isEmpty()) {
            localPathDuringResetSignal = m_doc->localFilePath();
        }
    });
    m_doc->resetPath();
    QCOMPARE(pathChangedSpy.count(), 2);
    QCOMPARE(localPathDuringResetSignal, localPath);
    QVERIFY(m_doc->path().isEmpty());
    QVERIFY(m_doc->localFilePath().isEmpty());

    finalize();
}

void KisDocumentReplaceTest::testDocumentModificationStateDelegation()
{
    init();
    QSignalSpy modifiedSpy(m_doc, &KisDocument::modified);

    QVERIFY(!m_doc->isModified());

    m_doc->setModified(true);
    m_doc->setModified(true);

    QCOMPARE(modifiedSpy.count(), 1);
    QCOMPARE(modifiedSpy.at(0).at(0).toBool(), true);
    QVERIFY(m_doc->isModified());

    QScopedPointer<KisDocument> snapshot(m_doc->lockAndCreateSnapshot());
    QVERIFY(snapshot);
    QVERIFY(!snapshot->isModified());

    m_doc->setModified(false);

    QCOMPARE(modifiedSpy.count(), 2);
    QCOMPARE(modifiedSpy.at(1).at(0).toBool(), false);
    QVERIFY(!m_doc->isModified());

    finalize();
}

void KisDocumentReplaceTest::testQueuedHistoryNotificationsUseCurrentState_data()
{
    QTest::addColumn<bool>("deliveredClean");
    QTest::addColumn<bool>("untrackedEdit");
    QTest::newRow("dirty-notification-after-clean") << false << false;
    QTest::newRow("clean-notification-after-dirty") << true << false;
    QTest::newRow("clean-notification-keeps-untracked-edit") << true << true;
}

void KisDocumentReplaceTest::testQueuedHistoryNotificationsUseCurrentState()
{
    QFETCH(bool, deliveredClean);
    QFETCH(bool, untrackedEdit);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QVERIFY(document->newImage(QStringLiteral("Deferred history"), 64, 64, cs,
                               KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    auto *stack = document->undoStack();
    stack->push(new KUndo2Command);
    stack->setClean();
    bool queued = false;
    bool scheduled = false;
    const auto connection = connect(stack, &KUndo2Stack::cleanChanged, document.data(), [&](bool clean) {
        if (queued || clean != deliveredClean) return;
        queued = true;
        scheduled = QMetaObject::invokeMethod(document.data(), "slotUndoStackCleanChanged",
                                             Qt::QueuedConnection, Q_ARG(bool, clean));
    });
    const auto disconnectHistory = qScopeGuard([&] { disconnect(connection); });
    stack->push(new KUndo2Command);
    stack->undo();
    if (deliveredClean && !untrackedEdit) stack->redo();
    if (untrackedEdit) {
        QVERIFY(QMetaObject::invokeMethod(document.data(), "setImageModifiedWithoutUndo", Qt::DirectConnection));
    }
    QVERIFY(queued && scheduled);
    const bool expected = !stack->isClean() || untrackedEdit;
    QCOMPARE(document->isModified(), expected);
    QApplication::sendPostedEvents(document.data(), QEvent::MetaCall);
    QCOMPARE(document->isModified(), expected);
}

void KisDocumentReplaceTest::testDocumentRecoveryStatusDelegation()
{
    init();
    QSignalSpy recoveredSpy(m_doc, &KisDocument::sigRecoveredChanged);

    QVERIFY(!m_doc->isRecovered());

    m_doc->setRecovered(true);
    m_doc->setRecovered(true);

    QCOMPARE(recoveredSpy.count(), 1);
    QCOMPARE(recoveredSpy.at(0).at(0).toBool(), true);
    QVERIFY(m_doc->isRecovered());

    QScopedPointer<KisDocument> snapshot(m_doc->lockAndCreateSnapshot());
    QVERIFY(snapshot);
    QVERIFY(!snapshot->isRecovered());

    m_doc->setRecovered(false);
    m_doc->setRecovered(false);

    QCOMPARE(recoveredSpy.count(), 2);
    QCOMPARE(recoveredSpy.at(1).at(0).toBool(), false);
    QVERIFY(!m_doc->isRecovered());

    finalize();
}

void KisDocumentReplaceTest::testTemplateRootLayerNameTranslation()
{
    init();

    KisNodeSP layer = m_doc->image()->rootLayer();
    QVERIFY(layer);
    layer->setName(QStringLiteral("Background"));

    const QMap<QString, QString> dictionary {
        {QStringLiteral("Background"), QStringLiteral("Translated Background")}
    };
    m_doc->translateTemplateRootLayerName(dictionary);

    QCOMPARE(layer->name(), QStringLiteral("Translated Background"));

    finalize();
}

void KisDocumentReplaceTest::testImageStateDelegation()
{
    init();

    QVERIFY(m_doc->hasImage());
    QCOMPARE(m_doc->imageObjectName(), QStringLiteral("test"));
    QVERIFY(m_doc->imageMemorySize() >= 0);
    QCOMPARE(m_doc->imageUndoAdapter(), m_doc->image()->undoAdapter());

    RecordingProgressProxy progressProxy;
    m_doc->addImageProgressProxy(&progressProxy);
    m_doc->image()->compositeProgressProxy()->setValue(37);
    QCOMPARE(progressProxy.value(), 37);
    m_doc->removeImageProgressProxy(&progressProxy);
    m_doc->image()->compositeProgressProxy()->setValue(82);
    QCOMPARE(progressProxy.value(), 37);

    QScopedPointer<KisDocument> lockedClone(m_doc->cloneWithImageReadLock());
    QVERIFY(lockedClone);
    QVERIFY(lockedClone->hasImage());
    QCOMPARE(lockedClone->imageObjectName(), m_doc->imageObjectName());

    m_doc->setAnimationTiming(12, 2, 8);
    QCOMPARE(m_doc->image()->animationInterface()->framerate(), 12);
    QCOMPARE(m_doc->image()->animationInterface()->documentPlaybackRange(), KisTimeSpan::fromTimeToTime(2, 8));

    QCOMPARE(m_doc->animationLength(), 9);
    if (m_doc->animationLength() < 4) {
        m_doc->setAnimationRange(0, 4);
    }
    QCOMPARE(m_doc->image()->animationInterface()->documentPlaybackRange(), KisTimeSpan::fromTimeToTime(2, 8));
    m_doc->setAnimationRange(0, 10);
    QCOMPARE(m_doc->image()->animationInterface()->documentPlaybackRange(), KisTimeSpan::fromTimeToTime(0, 10));

    m_doc->refreshProjectionAndWait();

    finalize();
}

void KisDocumentReplaceTest::testApplicationIdleWatcherIdentity()
{
    KisPart *part = KisPart::instance();
    KisIdleWatcher *idleWatcher = part->idleWatcher();

    QVERIFY(idleWatcher);
    QCOMPARE(part->idleWatcher(), idleWatcher);
}

KISTEST_MAIN(KisDocumentReplaceTest)
