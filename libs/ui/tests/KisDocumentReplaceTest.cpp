/*
 *  SPDX-FileCopyrightText: 2019 Tusooa Zhu <tusooa@vista.aero>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisDocumentReplaceTest.h"

#include <document/KisDocument.h>
#include <application/KisPart.h>
#include <QMap>
#include <QScopedPointer>
#include <QSignalSpy>
#include <KoProgressProxy.h>
#include <kis_composite_progress_proxy.h>
#include <kis_group_layer.h>
#include <kis_image.h>
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
