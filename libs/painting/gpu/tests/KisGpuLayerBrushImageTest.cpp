/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuBrushInputWorkloads.h"
#include "KisGpuLayerDocument.h"
#include "KisGpuLayerProjection.h"
#include "KisGpuTestDevice.h"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

/** Brush consumers require the same document pixels after append and replacement on Metal and Vulkan. */
class KisGpuLayerBrushImageTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void generatedInputProducesStableImage_data() {
        QTest::addColumn<bool>("high");
        QTest::addColumn<bool>("replace");
        for (bool high : {false, true}) for (bool replace : {false, true})
            QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(high ? "layer-projection" : "small", replace ? "replace" : "append")))
                << high << replace;
    }
    void generatedInputProducesStableImage() {
        QFETCH(bool, high);
        QFETCH(bool, replace);
        QFile file(QStringLiteral(BRUSH_INPUT_FIXTURE));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto fixture = QJsonDocument::fromJson(file.readAll()).object();
        QCOMPARE(fixture["samples"].toInt(), 15);
        QCOMPARE(fixture["warmups"].toInt(), 2);
        KisGpuBrushInputWorkload workload(high);
        KisGpuTestDevice gpu;
        auto store = std::make_shared<KisGpuTileStore>(gpu.owner, 384 * 1024 * 1024);
        KisGpuLayerDocument document(store, 8);
        KisGpuLayerDocument::LayerId target = 0;
        for (int i = 0; i < workload.layerCount; ++i) {
            const auto added = document.addLayer(QStringLiteral("Layer %1").arg(i));
            QCOMPARE(added.result, KisGpuLayerDocument::Result::Accepted);
            target = added.id;
            const auto edit = document.begin(target);
            QCOMPARE(edit.result, KisGpuLayerDocument::Result::Accepted);
            QCOMPARE(edit.session->append(edit.token, {workload.seed(i)}, workload.occupied), KisGpuEditSession::Result::Accepted);
            QCOMPARE(document.commit(edit.token), KisGpuLayerDocument::Result::Accepted);
            QTRY_COMPARE_WITH_TIMEOUT((document.poll(), document.state()), KisGpuLayerDocument::State::Idle, 5000);
        }
        KisGpuLayerProjection projection(*store, workload.bounds);
        const auto edit = document.begin(target);
        QCOMPARE(edit.result, KisGpuLayerDocument::Result::Accepted);
        KisGpuBrushStroke brush(*edit.session, edit.token, workload.settings(), workload.bounds);
        const auto before = store->statistics();
        for (int i = 0; i < 17; ++i) {
            const auto samples = workload.samples(i, replace);
            const auto result = replace ? brush.replace(samples) : brush.append(samples);
            QCOMPARE(result.result, KisGpuBrushStroke::Result::Accepted);
            QCOMPARE(projection.setLayers(document.projectionLayers()), KisGpuTileStore::Error::None);
            QTRY_COMPARE_WITH_TIMEOUT((document.poll(), projection.poll(), projection.status()), KisGpuTileStore::Status::Succeeded, 5000);
        }
        QCOMPARE(store->statistics().pixelUploadBytes, before.pixelUploadBytes);
        QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
        const auto pixels = gpu.read(projection.snapshot().pixels, workload.bounds);
        const auto key = QStringLiteral("%1-%2").arg(high ? "layer-projection" : "small", replace ? "replace" : "append");
        const auto actual = QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex();
        QCOMPARE(actual, fixture["images"].toObject()[key].toString().toLatin1());
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
};
QTEST_GUILESS_MAIN(KisGpuLayerBrushImageTest)
#include "KisGpuLayerBrushImageTest.moc"
