/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QtTest>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <optional>
#include <stdexcept>
#include "../KisJpegCoefficientStore_p.h"
#include "../kis_jpeg_destination.h"

class KisJpegCoefficientStoreTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testBoundedCoefficientsPreserveJPEG_data();
    void testBoundedCoefficientsPreserveJPEG();
    void testCoefficientBackingFailures_data();
    void testCoefficientBackingFailures();
};

namespace {
QByteArray coefficientJPEG(J_COLOR_SPACE color, bool progressive, bool subsampled,
                          QIODevice *backing, qsizetype &bufferBytes)
{
    jpeg_compress_struct encoder{};
    jpeg_error_mgr errors;
    encoder.err = jpeg_std_error(&errors);
    errors.error_exit = [](j_common_ptr context) {
        char message[JMSG_LENGTH_MAX];
        context->err->format_message(context, message);
        throw std::runtime_error(message);
    };
    std::optional<KisJpegCoefficientStore> coefficients;
    QBuffer output;
    const auto destroy = qScopeGuard([&] { jpeg_destroy_compress(&encoder); });
    jpeg_create_compress(&encoder);
    if (backing) coefficients.emplace(&encoder, *backing, 32 * 1024);
    if (!output.open(QIODevice::WriteOnly)) throw std::runtime_error("JPEG fixture output could not open");
    KisJPEGDestination::setDestination(&encoder, &output);
    encoder.image_width = 257;
    encoder.image_height = 129;
    encoder.input_components = color == JCS_GRAYSCALE ? 1 : color == JCS_CMYK ? 4 : 3;
    encoder.in_color_space = color;
    jpeg_set_defaults(&encoder);
    jpeg_set_quality(&encoder, 80, true);
    encoder.optimize_coding = true;
    if (!subsampled) for (int i = 0; i < encoder.num_components; ++i) {
        encoder.comp_info[i].h_samp_factor = 1;
        encoder.comp_info[i].v_samp_factor = 1;
    }
    if (progressive) jpeg_simple_progression(&encoder);
    jpeg_start_compress(&encoder, true);
    QByteArray row(encoder.image_width * encoder.input_components, '\0');
    while (encoder.next_scanline < encoder.image_height) {
        for (unsigned x = 0; x < encoder.image_width; ++x) for (int c = 0; c < encoder.input_components; ++c)
            row[int(x) * encoder.input_components + c] = char((x * 13 + encoder.next_scanline * 7 + c * 71) % 256);
        auto *pixels = reinterpret_cast<JSAMPLE *>(row.data());
        jpeg_write_scanlines(&encoder, &pixels, 1);
    }
    jpeg_finish_compress(&encoder);
    if (coefficients) bufferBytes = coefficients->peakBufferBytes();
    return output.data();
}

class FailedCoefficientBacking final : public QBuffer
{
public:
    explicit FailedCoefficientBacking(int failure) : failure(failure) {}
    bool injected = false;
    bool open(OpenMode mode) override
    {
        if (failure == 0) { injected = true; return false; }
        return QBuffer::open(mode);
    }
    bool seek(qint64 position) override
    {
        if (failure == 1) { injected = true; return false; }
        return QBuffer::seek(position);
    }
protected:
    qint64 writeData(const char *data, qint64 size) override
    {
        if (failure == 2) { injected = true; return -1; }
        return QBuffer::writeData(data, qMin(size, qint64(37)));
    }
    qint64 readData(char *data, qint64 size) override
    {
        if (failure == 3 || failure == 4) { injected = true; return failure == 3 ? 0 : -1; }
        return QBuffer::readData(data, qMin(size, qint64(29)));
    }
private:
    int failure;
};
}

void KisJpegCoefficientStoreTest::testBoundedCoefficientsPreserveJPEG_data()
{
    QTest::addColumn<int>("color");
    QTest::addColumn<bool>("progressive");
    QTest::addColumn<bool>("subsampled");
    for (const auto color : {JCS_RGB, JCS_GRAYSCALE, JCS_CMYK}) for (const bool progressive : {false, true}) {
        for (const bool subsampled : {false, true}) {
            if (subsampled && color != JCS_RGB) continue;
            const QByteArray name = QByteArray::number(int(color)) + (progressive ? "-progressive" : "-sequential") +
                (subsampled ? "-subsampled" : "-full-chroma");
            QTest::newRow(name.constData()) << int(color) << progressive << subsampled;
        }
    }
}

void KisJpegCoefficientStoreTest::testBoundedCoefficientsPreserveJPEG()
{
    QFETCH(int, color);
    QFETCH(bool, progressive);
    QFETCH(bool, subsampled);
    QByteArray expected, actual;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto artifactDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", directory.path());
    const auto preserve = qScopeGuard([&] {
        if (!QTest::currentTestFailed() && !qEnvironmentVariableIsSet("LIBREPAINT_GPU_RESULT_DIR")) return;
        directory.setAutoRemove(!QTest::currentTestFailed());
        QDir().mkpath(artifactDirectory);
        for (const auto &item : {qMakePair(QStringLiteral("expected"), expected), qMakePair(QStringLiteral("actual"), actual)}) {
            QFile file(QDir(artifactDirectory).filePath(QStringLiteral("coefficient-") +
                QString::fromLatin1(QTest::currentDataTag()) + QLatin1Char('-') + item.first + QStringLiteral(".jpg")));
            if (file.open(QIODevice::WriteOnly)) file.write(item.second);
        }
        if (QTest::currentTestFailed()) qInfo() << "Coefficient comparison artifacts retained at" << artifactDirectory;
    });
    qsizetype bufferBytes = 0;
    QBuffer backing;
    try {
        expected = coefficientJPEG(J_COLOR_SPACE(color), progressive, subsampled, nullptr, bufferBytes);
        actual = coefficientJPEG(J_COLOR_SPACE(color), progressive, subsampled, &backing, bufferBytes);
    } catch (const std::exception &error) { QFAIL(error.what()); }
    QCOMPARE(actual, expected);
    QVERIFY(bufferBytes > 0 && bufferBytes <= 32 * 1024);
    QVERIFY(backing.size() > 0);
    FailedCoefficientBacking shortTransfers(5);
    try {
        QCOMPARE(coefficientJPEG(J_COLOR_SPACE(color), progressive, subsampled, &shortTransfers, bufferBytes), expected);
    } catch (const std::exception &error) { QFAIL(error.what()); }
}

void KisJpegCoefficientStoreTest::testCoefficientBackingFailures_data()
{
    QTest::addColumn<int>("failure");
    QTest::newRow("open") << 0;
    QTest::newRow("seek") << 1;
    QTest::newRow("write") << 2;
    QTest::newRow("premature-end") << 3;
    QTest::newRow("read") << 4;
}

void KisJpegCoefficientStoreTest::testCoefficientBackingFailures()
{
    QFETCH(int, failure);
    FailedCoefficientBacking backing(failure);
    qsizetype bufferBytes = 0;
    bool failed = false;
    try { coefficientJPEG(JCS_RGB, true, false, &backing, bufferBytes); }
    catch (const std::runtime_error &) { failed = true; }
    QVERIFY(failed);
    QVERIFY(backing.injected);
    QCOMPARE(coefficientJPEG(JCS_RGB, true, false, nullptr, bufferBytes).isEmpty(), false);
}

QTEST_GUILESS_MAIN(KisJpegCoefficientStoreTest)
#include "KisJpegCoefficientStoreTest.moc"
