/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>
#include <kis_qimage_pyramid.h>
#include <kis_random_source.h>
#include <KoColorSpaceTrait.h>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QTest>
#include <QtEndian>
#include <cmath>

using namespace LibrePaint::Image::Gpu;

class KisGpuImageBrushTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void importedTipKeepsNativeMask_data();
    void importedTipKeepsNativeMask();
    void seededPipeKeepsNativeTip();
private:
    std::shared_ptr<Execution> m_execution;
};

void KisGpuImageBrushTest::initTestCase()
{
    QString diagnostic;
    auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    m_execution = std::make_shared<Execution>(device);
}

void KisGpuImageBrushTest::seededPipeKeepsNativeTip()
{
    auto tips = QSharedPointer<ImageBrushTips>::create();
    tips->originalSize = {4, 4}; tips->tipCount = 7; tips->randomSelection = true;
    tips->levels.append({{4, 4}, 1, 0, 18});
    tips->pixels = QByteArray(7 * 72, '\0');
    for (int tip = 0; tip < 7; ++tip) for (int y = 1; y <= 4; ++y) for (int x = 1; x <= 4; ++x) {
        const int index = tip * 72 + (y * 6 + x) * 2;
        tips->pixels[index] = char(tip * 32); tips->pixels[index + 1] = char(255);
    }
    Brush brush;
    brush.rgba = {255, 255, 255, 255}; brush.diameter = 4;
    brush.autoBrush = AutoBrush{};
    brush.autoBrush->mask = AutoBrush::Mask::Image; brush.autoBrush->imageTips = tips;
    PaintImage image(m_execution, QSize(64, 64));
    const auto layer = image.operations().activeLayer();
    const auto stroke = image.beginStroke(layer, brush, 17);
    QVERIFY(!stroke.isNull());
    QCOMPARE(image.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(image.commitStroke(stroke), Result::Success);
    const auto actual = image.readRgbaPixels(layer, QRect(32, 32, 1, 1), 4);
    QCOMPARE(actual.code, ExecutionCode::Success);
    KisRandomSource native(17);
    QCOMPARE(quint8(actual.bytes[3]), quint8(255 - native.generate(0, 6) * 32));
}

void KisGpuImageBrushTest::importedTipKeepsNativeMask_data()
{
    QTest::addColumn<QSize>("sourceSize");
    QTest::addColumn<qreal>("scale");
    QTest::addColumn<qreal>("angle");
    QTest::addColumn<QPointF>("subpixel");
    QTest::addColumn<bool>("partialAlpha");
    QTest::addColumn<bool>("sixteenBits");
    struct Case { QSize size; qreal scale; qreal angle; QPointF subpixel; };
    const QList<Case> cases{{{8, 8}, 1, 0, {}}, {{8, 8}, .75, 0, {.25, .75}},
        {{8, 8}, 1, .37, {.125, .25}}, {{8, 8}, .75, 1.5708, {.75, .125}},
        {{128, 128}, 13.75 / 128, 1.5708, {.25, .75}}, {{128, 128}, .125, 1.97, {.125, .25}},
        {{4, 8}, .75, .37, {.125, .25}}, {{8, 4}, .75, 0, {.25, .75}}};
    for (int i = 0; i < cases.size(); ++i) for (bool partial : {false, true})
        for (bool word : {false, true}) {
            const auto &c = cases[i];
            QTest::newRow(qPrintable(QStringLiteral("geometry%1-%2-%3").arg(i)
                .arg(partial ? "alpha" : "opaque").arg(word ? "u16" : "u8")))
                << c.size << c.scale << c.angle << c.subpixel << partial << word;
        }
}

void KisGpuImageBrushTest::importedTipKeepsNativeMask()
{
    QFETCH(QSize, sourceSize);
    QFETCH(qreal, scale);
    QFETCH(qreal, angle);
    QFETCH(QPointF, subpixel);
    QFETCH(bool, partialAlpha);
    QFETCH(bool, sixteenBits);
    // Generated gray grain fixes the source independently of installed presets.
    // The native pyramid and pigment consumer define the expected mask pixels.
    QImage source(sourceSize, QImage::Format_ARGB32);
    for (int y = 0; y < source.height(); ++y) for (int x = 0; x < source.width(); ++x) {
        const int gray = (x * 37 + y * 19 + x * y * 7) % 256;
        source.setPixel(x, y, qRgba(gray, gray, gray, partialAlpha ? (x * 13 + y * 29) % 256 : 255));
    }
    KisQImagePyramid native(source);
    qreal levelScale;
    const auto level = native.getClosest(QTransform::fromScale(scale, scale), &levelScale);
    auto tips = QSharedPointer<ImageBrushTips>::create();
    tips->originalSize = source.size();
    tips->tipCount = 1;
    const quint32 stride = quint32((level.width() * level.height() * 2 + 3) / 4);
    tips->levels.append({level.size() - QSize(2, 2), levelScale, 0, stride});
    tips->pixels.resize(stride * 4);
    tips->pixels.fill('\0');
    for (int y = 0; y < level.height(); ++y) for (int x = 0; x < level.width(); ++x) {
        const auto pixel = level.pixel(x, y);
        const int index = (y * level.width() + x) * 2;
        tips->pixels[index] = char(qRed(pixel));
        tips->pixels[index + 1] = char(qAlpha(pixel));
    }
    const auto metric = KisQImagePyramid::characteristicSize(source.size(), KisDabShape(scale, 1, angle));
    const QPointF hotSpot(std::max(qreal(1), metric.width()) / 2, std::max(qreal(1), metric.height()) / 2);
    const QPoint origin(16, 16);
    const QPointF position = origin + hotSpot + subpixel;
    const QPointF actualSubpixel = position - hotSpot - origin;
    const auto mask = native.createImage(KisDabShape(scale, 1, -angle), actualSubpixel.x(), actualSubpixel.y());
    QVERIFY(!mask.isNull());
    QVERIFY(mask.width() + origin.x() < 64 && mask.height() + origin.y() < 64);
    const auto format = sixteenBits ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    RasterImage raster;
    raster.size = {64, 64};
    raster.format = format;
    RasterLayer layer;
    layer.name = QStringLiteral("Imported tip");
    layer.uuid = QUuid::createUuid();
    raster.layers.append(layer);
    PaintImage image(m_execution, raster);
    Brush brush;
    const quint16 maximum = sixteenBits ? 65535 : 255;
    brush.rgba = {maximum, maximum, maximum, maximum};
    brush.diameter = std::max(source.width(), source.height()) * scale;
    brush.autoBrush = AutoBrush{};
    brush.autoBrush->mask = AutoBrush::Mask::Image;
    brush.autoBrush->imageTips = tips;
    brush.autoBrush->angle = angle;
    // The imported tip shares the GPU resource binding with repeating paper.
    // Opaque paper keeps native mask pixels unchanged while exercising its offset.
    brush.texture = BrushTexture{};
    brush.texture->size = {2, 2};
    brush.texture->alpha = QByteArray(4, char(255));
    const auto id = image.beginStroke(image.operations().activeLayer(), brush, 17);
    QVERIFY(!id.isNull());
    InputSample sample;
    sample.position = position;
    QCOMPARE(image.appendSamples(id, {sample}), Result::Success);
    QCOMPARE(image.commitStroke(id), Result::Success);
    const auto pixels = image.readRgbaPixels(image.operations().activeLayer(), QRect(0, 0, 64, 64), 64 * 64 * 8);
    QCOMPARE(pixels.code, ExecutionCode::Success);
    QByteArray expected(64 * 64 * 2, '\0'), actual(expected.size(), '\0');
    quint8 color[2]{255, 255};
    QByteArray gray(mask.width() * 2, '\0');
    for (int y = 0; y < mask.height(); ++y) {
        KoColorSpaceTrait<quint8, 2, 1>::fillGrayBrushWithColor(reinterpret_cast<quint8 *>(gray.data()),
            reinterpret_cast<const QRgb *>(mask.constScanLine(y)), color, mask.width());
        for (int x = 0; x < mask.width(); ++x) qToLittleEndian<quint16>(quint8(gray[x * 2 + 1]) * (sixteenBits ? 257 : 1),
            expected.data() + ((y + origin.y()) * 64 + x + origin.x()) * 2);
    }
    for (int p = 0; p < 64 * 64; ++p) qToLittleEndian<quint16>(sixteenBits ?
        qFromLittleEndian<quint16>(pixels.bytes.constData() + p * 8 + 6) : quint8(pixels.bytes[p * 4 + 3]), actual.data() + p * 2);
    if (actual != expected) {
        const auto root = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!root.isEmpty()) {
            QDir().mkpath(root);
            const QString prefix = root + '/' + QString::fromLatin1(QTest::currentDataTag());
            source.save(prefix + "-source.png"); mask.save(prefix + "-native.png");
            for (const auto &entry : {qMakePair(QStringLiteral("-expected.alpha16"), expected), qMakePair(QStringLiteral("-actual.alpha16"), actual)}) {
                QFile file(prefix + entry.first);
                QVERIFY(file.open(QIODevice::WriteOnly));
                QCOMPARE(file.write(entry.second), entry.second.size());
            }
        }
        int differences = 0;
        for (int p = 0; p < 64 * 64; ++p) if (expected.mid(p * 2, 2) != actual.mid(p * 2, 2)) {
            if (differences++ < 12) qDebug() << QPoint(p % 64, p / 64)
                << "native" << qFromLittleEndian<quint16>(expected.constData() + p * 2)
                << "GPU" << qFromLittleEndian<quint16>(actual.constData() + p * 2);
        }
        qDebug() << "different pixels" << differences;
    }
    QCOMPARE(actual, expected);
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuImageBrushTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuImageBrushTest.moc"
