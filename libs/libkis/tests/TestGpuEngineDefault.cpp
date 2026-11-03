/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <testui.h>
#include <Document.h>
#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <application/kis_config.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_node.h>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <kundo2stack.h>

class TestGpuEngineDefault : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void engineSelection_data();
    void engineSelection();
    void ordinaryDocumentsUseSelectedEngine_data();
    void ordinaryDocumentsUseSelectedEngine();
};

void TestGpuEngineDefault::engineSelection_data()
{
    QTest::addColumn<QByteArray>("setting");
    QTest::addColumn<bool>("enabled");
    QTest::newRow("unset") << QByteArray() << false;
    QTest::newRow("empty") << QByteArray("") << false;
    QTest::newRow("disabled") << QByteArray("0") << false;
    QTest::newRow("enabled") << QByteArray("1") << true;
    QTest::newRow("other-value") << QByteArray("false") << false;
    QTest::newRow("two-zeros") << QByteArray("00") << false;
}

void TestGpuEngineDefault::engineSelection()
{
    QFETCH(QByteArray, setting);
    QFETCH(bool, enabled);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    if (setting.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
    else qputenv("LIBREPAINT_GPU_ENGINE", setting);
    QCOMPARE(KisPart::instance()->gpuEngineEnabled(), enabled);
}

void TestGpuEngineDefault::ordinaryDocumentsUseSelectedEngine_data()
{
    QTest::addColumn<QByteArray>("setting");
    QTest::addColumn<QString>("extension");
    QTest::addColumn<QString>("mime");
    struct Format { const char *extension; const char *mime; };
    const Format formats[] = {{"kra", "application/x-krita"}, {"png", "image/png"}, {"jpg", "image/jpeg"}};
    for (const auto &setting : {QByteArray(), QByteArray("0"), QByteArray("1")}) {
        for (const auto &format : formats) {
            const QByteArray name = (setting.isNull() ? QByteArray("default") : setting) + "-" + format.extension;
            QTest::newRow(name.constData()) << setting << QString::fromLatin1(format.extension) << QString::fromLatin1(format.mime);
        }
    }
}

void TestGpuEngineDefault::ordinaryDocumentsUseSelectedEngine()
{
    QFETCH(QByteArray, setting);
    QFETCH(QString, extension);
    QFETCH(QString, mime);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("source.") + extension);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    const KoColor color(QColor(17, 34, 51), cs);
    const auto create = [&](KisDocument &document) {
        return document.newImage(QStringLiteral("Default engine"), 16, 8, cs, color,
                                 KisConfig::RASTER_LAYER, 1, {}, 1);
    };
    // CPU-authored files exercise ordinary import independently of GPU export.
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    {
        QScopedPointer<KisDocument> fixture(KisPart::instance()->createDocument());
        fixture->setFileBatchMode(true);
        QVERIFY2(create(*fixture), qPrintable(fixture->errorMessage()));
        QVERIFY2(fixture->saveAs(path, mime.toLatin1(), false), qPrintable(fixture->errorMessage()));
        fixture->waitForSavingToComplete();
    }

    if (setting.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
    else qputenv("LIBREPAINT_GPU_ENGINE", setting);
    const bool fallback = qEnvironmentVariableIsSet("LIBREPAINT_TEST_GPU_UNAVAILABLE") && setting == "1";
    const bool gpu = setting == "1" && !fallback;
    const auto expectFallback = [&] {
        if (fallback) QTest::ignoreMessage(QtWarningMsg,
            QRegularExpression(QStringLiteral("GPU engine initialization failed\\. Using the CPU painting engine: .+")));
    };
    QScopedPointer<KisDocument> created(KisPart::instance()->createDocument());
    expectFallback();
    QVERIFY2(create(*created), qPrintable(created->errorMessage()));
    QCOMPARE(bool(created->image()->gpuImage()), gpu);
    QCOMPARE(created->image()->bounds(), QRect(0, 0, 16, 8));
    QCOMPARE(created->image()->root()->childCount(), quint32(1));
    QCOMPARE(created->undoStack()->count(), 0);
    QVERIFY(!created->isModified());
    QVERIFY(created->errorMessage().isEmpty());
    Document createdApi(created.data(), false);
    QCOMPARE(createdApi.pixelData(3, 3, 1, 1), QByteArray::fromHex("332211ff"));
    QCOMPARE(created->image()->projection()->pixel(QPoint(3, 3)), color);
    const auto createdPath = directory.filePath(QStringLiteral("created.") + extension);
    created->setFileBatchMode(true);
    QVERIFY2(created->saveAs(createdPath, mime.toLatin1(), false), qPrintable(created->errorMessage()));
    created->waitForSavingToComplete();
    QCOMPARE(bool(created->image()->gpuImage()), gpu);
    QVERIFY(!created->isModified());

    for (const auto &source : {path, createdPath}) {
        QScopedPointer<KisDocument> opened(KisPart::instance()->createDocument());
        opened->setFileBatchMode(true);
        expectFallback();
        QVERIFY2(opened->openPath(source, KisDocument::DontAddToRecent), qPrintable(opened->errorMessage()));
        QCOMPARE(bool(opened->image()->gpuImage()), gpu);
        QCOMPARE(opened->image()->bounds(), QRect(0, 0, 16, 8));
        QVERIFY(!opened->isModified());
        Document openedApi(opened.data(), false);
        const auto pixel = openedApi.pixelData(3, 3, 1, 1);
        const auto expected = QByteArray::fromHex("332211ff");
        QCOMPARE(pixel.size(), expected.size());
        for (int i = 0; i < pixel.size(); ++i) {
            QVERIFY(qAbs(int(quint8(pixel[i])) - int(quint8(expected[i]))) <= (extension == QLatin1String("jpg") ? 2 : 0));
        }
    }
}

KISTEST_MAIN(TestGpuEngineDefault)
#include "TestGpuEngineDefault.moc"
