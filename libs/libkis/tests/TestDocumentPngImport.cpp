/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "TestDocument.h"
#include <simpletest.h>
#include <Document.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <kis_image.h>
#include <QFile>
#include <QScopeGuard>
#include <QTemporaryDir>

namespace {
// Fixed 2 x 1 RGBA8 PNGs with the same opaque pixels. The variants have
// absent pHYs, unknown pHYs units, and metre units, respectively.
const QByteArray resolutionPngs[] = {
    QByteArray::fromHex("89504e470d0a1a0a0000000d4948445200000002000000010806000000f4227f8a"
                        "0000001149444154789c63105432feef1090f01f000b5b0355651e6f7f0000000049454e44ae426082"),
    QByteArray::fromHex("89504e470d0a1a0a0000000d4948445200000002000000010806000000f4227f8a"
                        "000000097048597300002e230000171200e245d64d"
                        "0000001149444154789c63105432feef1090f01f000b5b0355651e6f7f0000000049454e44ae426082"),
    QByteArray::fromHex("89504e470d0a1a0a0000000d4948445200000002000000010806000000f4227f8a"
                        "000000097048597300002e2300001712019542e6db"
                        "0000001149444154789c63105432feef1090f01f000b5b0355651e6f7f0000000049454e44ae426082")
};
}

void TestDocument::testPngWithoutPhysicalResolutionKeepsImageDefault_data()
{
    QTest::addColumn<bool>("gpuSession");
    QTest::addColumn<int>("precedingPng");
    for (bool gpu : {false, true}) {
        for (int preceding = 0; preceding < 3; ++preceding) {
            const QByteArray name = QByteArray(gpu ? "gpu-session-" : "cpu-session-") +
                QList<QByteArray>{"first-open", "after-unknown-units", "after-metre-units"}[preceding];
            QTest::newRow(name.constData()) << gpu << preceding;
        }
    }
}

void TestDocument::testPngWithoutPhysicalResolutionKeepsImageDefault()
{
    QFETCH(bool, gpuSession);
    QFETCH(int, precedingPng);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", gpuSession ? "1" : "0");
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    const auto open = [&](int index) {
        const auto path = directory.filePath(QStringLiteral("resolution-%1.png").arg(index));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(resolutionPngs[index]) != resolutionPngs[index].size()) return false;
        file.close();
        return document->openPath(path, KisDocument::DontAddToRecent);
    };
    if (precedingPng) QVERIFY2(open(precedingPng), qPrintable(document->errorMessage()));
    QVERIFY2(open(0), qPrintable(document->errorMessage()));
    KisImageSP image = document->image();
    QVERIFY(image);
    image->waitForDone();
    QCOMPARE(image->width(), 2);
    QCOMPARE(image->height(), 1);
    Document api(document.data(), false);
    QCOMPARE(api.pixelData(0, 0, 2, 1), QByteArray::fromHex("332211ff605040ff"));
    QCOMPARE(image->xRes(), 1.0);
    QCOMPARE(image->yRes(), 1.0);
}
