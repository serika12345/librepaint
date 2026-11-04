/*
 *  SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_doc2_test.h"

#include <application/ui/workspace/KisMainWindow.h>

#include <simpletest.h>

#include "document/KisDocument.h"
#include "application/ui/orchestration/KisPart.h"
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <application/kis_config.h>
#include <testui.h>
#include <KisImportExportManager.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_group_layer.h>
#include <KoColorProfile.h>
#include <metadata/KoDocumentInfo.h>
#include <KisPaintingEngineTestScope.h>
#include <KisGpuPixelTolerance.h>
#include <QTemporaryDir>
#include <cstring>
#include "KritaTransformMaskStubs.h"

namespace {
QByteArray documentInformation(KisDocument *document)
{
    auto xml = KisDocument::createDomDocument(QStringLiteral("document-info"), QStringLiteral("document-info"), QStringLiteral("1.1"));
    return document->documentInfo()->save(xml, true, false).toByteArray();
}

void comparePixels(QByteArray pixels, const QByteArray &reference)
{
    QCOMPARE(pixels.size(), reference.size());
    for (qsizetype i = 0; i < pixels.size(); i += 4)
        if (!pixels[i + 3] && !reference[i + 3])
            std::memcpy(pixels.data() + i, reference.constData() + i, 3);
    QCOMPARE_PIXELS_WITHIN_STEP(pixels, reference);
}

void compareImages(KisImageSP actual, KisImageSP expected, bool compareIdentities)
{
    QCOMPARE(actual->size(), expected->size());
    QCOMPARE(actual->xRes(), expected->xRes());
    QCOMPARE(actual->yRes(), expected->yRes());
    QCOMPARE(actual->colorSpace()->id(), expected->colorSpace()->id());
    QCOMPARE(actual->profile()->rawData(), expected->profile()->rawData());
    QList<QPair<KisNodeSP, KisNodeSP>> pending{{actual->root(), expected->root()}};
    while (!pending.isEmpty()) {
        const auto [node, reference] = pending.takeFirst();
        QCOMPARE(node->metaObject()->className(), reference->metaObject()->className());
        if (reference != expected->root()) {
            QCOMPARE(node->name(), reference->name());
            if (compareIdentities) QCOMPARE(node->uuid(), reference->uuid());
        }
        QCOMPARE(node->childCount(), reference->childCount());
        QCOMPARE(node->visible(), reference->visible());
        QCOMPARE(node->userLocked(), reference->userLocked());
        QCOMPARE(node->opacity(), reference->opacity());
        QCOMPARE(node->compositeOpId(), reference->compositeOpId());
        QCOMPARE(node->x(), reference->x());
        QCOMPARE(node->y(), reference->y());
        if (node->inherits("KisPaintLayer")) {
            const auto device = node->paintDevice();
            const auto referenceDevice = reference->paintDevice();
            const auto area = expected->bounds().united(referenceDevice->exactBounds());
            QByteArray pixels(area.width() * area.height() * 4, Qt::Uninitialized);
            QByteArray referencePixels(pixels.size(), Qt::Uninitialized);
            device->readBytes(reinterpret_cast<quint8 *>(pixels.data()), area);
            referenceDevice->readBytes(reinterpret_cast<quint8 *>(referencePixels.data()), area);
            comparePixels(pixels, referencePixels);
        }
        auto child = node->firstChild();
        for (auto referenceChild = reference->firstChild(); referenceChild; referenceChild = referenceChild->nextSibling()) {
            pending.append({child, referenceChild});
            child = child->nextSibling();
        }
    }
    const auto area = expected->bounds();
    QByteArray pixels(area.width() * area.height() * 4, Qt::Uninitialized);
    QByteArray referencePixels(pixels.size(), Qt::Uninitialized);
    actual->projection()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), area);
    expected->projection()->readBytes(reinterpret_cast<quint8 *>(referencePixels.data()), area);
    comparePixels(pixels, referencePixels);
}
}


void silenceReignsSupreme(QtMsgType /*type*/, const QMessageLogContext &/*context*/, const QString &/*msg*/)
{
}

void KisDocumentTest::initTestCase()
{
    TestUtil::registerTransformMaskStubs();
    qInstallMessageHandler(silenceReignsSupreme);
}

void KisDocumentTest::testOpenImageTwiceInSameDoc()
{
    const TestUtil::PaintingEngineScope gpuEngine(true);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QScopedPointer<KisDocument> doc(KisPart::instance()->createDocument());
    doc->setFileBatchMode(true);
    for (const auto &file : {QStringLiteral("load_test2.kra"), QStringLiteral("load_test.kra")}) {
        const auto path = QString(FILES_DATA_DIR) + '/' + file;
        QVERIFY2(doc->importExportManager()->importDocument(path, KisDocument::nativeFormatMimeType()).isOk(),
                 qPrintable(doc->errorMessage()));
        QVERIFY(doc->image()->gpuImage());
        QVERIFY(!doc->isModified());
        const auto importedInfo = documentInformation(doc.data());
        const auto savedPath = directory.filePath(file);
        QVERIFY2(doc->exportDocumentSync(savedPath, KisDocument::nativeFormatMimeType()), qPrintable(doc->errorMessage()));
        QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
        reopened->setFileBatchMode(true);
        QVERIFY2(reopened->loadNativeFormat(savedPath), qPrintable(reopened->errorMessage()));
        QVERIFY(reopened->image()->gpuImage());
        QCOMPARE(documentInformation(reopened.data()), documentInformation(doc.data()));
        compareImages(reopened->image(), doc->image(), true);
        const TestUtil::PaintingEngineScope cpuEngine(false);
        QScopedPointer<KisDocument> reference(KisPart::instance()->createDocument());
        reference->setFileBatchMode(true);
        QVERIFY(reference->loadNativeFormat(path));
        QCOMPARE(importedInfo, documentInformation(reference.data()));
        reference->image()->waitForDone();
        compareImages(reopened->image(), reference->image(), false);
        QVERIFY(reference->loadNativeFormat(savedPath));
        QCOMPARE(documentInformation(reference.data()), documentInformation(reopened.data()));
        reference->image()->waitForDone();
        compareImages(reference->image(), doc->image(), true);
    }
}

void KisDocumentTest::testLegacyDefaultPixels()
{
    const TestUtil::PaintingEngineScope gpuEngine(true);
    QScopedPointer<KisDocument> doc(KisPart::instance()->createDocument());
    doc->setFileBatchMode(true);
    for (const auto &file : {QStringLiteral("single_layer_no_channel_flags_transp_def_pixel.kra"),
                             QStringLiteral("single_layer_no_channel_flags_nontransp_def_pixel.kra")}) {
        const auto path = QStringLiteral(KRITA_SOURCE_DIR "/plugins/impex/libkra/tests/data/") + file;
        QVERIFY2(doc->loadNativeFormat(path), qPrintable(doc->errorMessage()));
        QVERIFY(doc->image());
        QVERIFY(doc->image()->gpuImage());
        QVERIFY(!doc->isModified());
        const TestUtil::PaintingEngineScope cpuEngine(false);
        QScopedPointer<KisDocument> reference(KisPart::instance()->createDocument());
        reference->setFileBatchMode(true);
        QVERIFY(reference->loadNativeFormat(path));
        reference->image()->waitForDone();
        compareImages(doc->image(), reference->image(), true);
    }
}


KISTEST_MAIN(KisDocumentTest)

