/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <simpletest.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <KoColor.h>
#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoCompositeOpRegistry.h>

#include "kis_paint_device.h"
#include "kis_painter.h"
#include "kis_pixel_selection.h"
#include "kis_selection.h"
#include "kis_surrogate_undo_adapter.h"
#include "kis_transaction.h"

namespace {
QByteArray bgra(const QJsonArray &rgba)
{
    QByteArray result(4, Qt::Uninitialized);
    result[0] = char(rgba[2].toInt());
    result[1] = char(rgba[1].toInt());
    result[2] = char(rgba[0].toInt());
    result[3] = char(rgba[3].toInt());
    return result;
}

QByteArray pixels(KisPaintDeviceSP device, const QRect &rect)
{
    QByteArray result(rect.width() * rect.height() * device->pixelSize(), Qt::Uninitialized);
    device->readBytes(reinterpret_cast<quint8 *>(result.data()), rect);
    return result;
}

void paint(KisPaintDeviceSP destination, const QRect &rect, const QByteArray &color,
           const QString &operation, int opacity = 255, KisSelectionSP selection = {})
{
    KisPaintDeviceSP source = new KisPaintDevice(destination->colorSpace());
    // Source and destination deliberately have different tile alignment.
    const QRect sourceRect(3, 5, rect.width(), rect.height());
    source->fill(sourceRect, KoColor(reinterpret_cast<const quint8 *>(color.constData()), destination->colorSpace()));
    KisPainter painter(destination, selection);
    painter.setCompositeOpId(operation);
    painter.setOpacityU8(quint8(opacity));
    painter.bitBlt(rect.topLeft(), source, sourceRect);
}

QString pixelDescription(const QByteArray &pixel)
{
    return QStringLiteral("RGBA(%1,%2,%3,%4)")
        .arg(quint8(pixel[2])).arg(quint8(pixel[1])).arg(quint8(pixel[0])).arg(quint8(pixel[3]));
}

QString pixelDifference(KisPaintDeviceSP device, const QRect &bounds, const QByteArray &expected)
{
    const QByteArray actual = pixels(device, bounds);
    if (actual == expected) return {};
    if (actual.size() != expected.size()) return QStringLiteral("Image sizes differ");
    for (int i = 0; i < actual.size(); i += 4) {
        if (actual.mid(i, 4) == expected.mid(i, 4)) continue;
        return QStringLiteral("(%1,%2): expected %3, actual %4")
            .arg(bounds.x() + i / 4 % bounds.width()).arg(bounds.y() + i / 4 / bounds.width())
            .arg(pixelDescription(expected.mid(i, 4)), pixelDescription(actual.mid(i, 4)));
    }
    return {};
}
}

/**
 * Consumers are the raster editor and the GPU document replacement. Fixed
 * channel values expose changed compositing, mask coverage and edit history
 * without making the old tile representation part of the replacement API.
 */
class KisRasterEditContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void compositing_data();
    void compositing();
    void historyRestoresPixels();
    void replacementEditRestoresOldFootprint();
    void layerOrder();

private:
    const KoColorSpace *m_colorSpace = nullptr;
    QJsonArray m_cases;
};

void KisRasterEditContractTest::initTestCase()
{
    QFile file(QStringLiteral(FILES_DATA_DIR "raster_edit_contract.json"));
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    const QJsonObject fixture = document.object();
    QCOMPARE(fixture["schema"].toInt(), 1);
    m_cases = fixture["cases"].toArray();
    QVERIFY(!m_cases.isEmpty());

    const QString profile = fixture["profile"].toString();
    m_colorSpace = KoColorSpaceRegistry::instance()->rgb8(profile);
    QVERIFY(m_colorSpace);
    // A missing color engine can silently fall back to an unmanaged space.
    QCOMPARE(m_colorSpace->profile()->name(), profile);
    QCOMPARE(m_colorSpace->pixelSize(), quint32(4));
    const KoColor red(Qt::red, m_colorSpace);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(red.data()), 4), bgra({255, 0, 0, 255}));
}

void KisRasterEditContractTest::compositing_data()
{
    QTest::addColumn<QJsonObject>("fixture");
    QTest::addColumn<QRect>("rect");
    const QList<QRect> rectangles {
        QRect(-1, -1, 1, 1),
        QRect(-2, -2, 67, 67),
        QRect(62, 62, 131, 3)
    };
    for (const auto &value : m_cases) {
        const QJsonObject fixture = value.toObject();
        for (int i = 0; i < rectangles.size(); ++i) {
            const QByteArray name = fixture["id"].toString().toUtf8() + '-' + QByteArray::number(i);
            QTest::newRow(name.constData()) << fixture << rectangles[i];
        }
    }
}

void KisRasterEditContractTest::compositing()
{
    QFETCH(QJsonObject, fixture);
    QFETCH(QRect, rect);
    KisPaintDeviceSP device = new KisPaintDevice(m_colorSpace);
    const QByteArray destination = bgra(fixture["dst"].toArray());
    const QByteArray expected = bgra(fixture["expected"].toArray());
    const QRect bounds = rect.adjusted(-2, -2, 2, 2);
    device->fill(bounds, KoColor(reinterpret_cast<const quint8 *>(destination.constData()), m_colorSpace));
    const int mask = fixture["mask"].toInt();
    KisSelectionSP selection;
    QRect paintedRect = rect;
    if (mask >= 0) {
        selection = new KisSelection;
        // Leave a border unselected whenever the input is larger than one pixel.
        if (rect.width() > 2 && rect.height() > 2) {
            paintedRect = rect.adjusted(1, 1, -1, -1);
        }
        selection->pixelSelection()->select(paintedRect, quint8(mask));
        selection->updateProjection();
    }
    paint(device, rect, bgra(fixture["src"].toArray()), fixture["op"].toString(),
          fixture["opacity"].toInt(), selection);
    const QByteArray actual = pixels(device, bounds);
    for (int y = bounds.top(); y <= bounds.bottom(); ++y) {
        for (int x = bounds.left(); x <= bounds.right(); ++x) {
            const QByteArray wanted = paintedRect.contains(x, y) ? expected : destination;
            const int offset = ((y - bounds.top()) * bounds.width() + x - bounds.left()) * 4;
            const QByteArray got = actual.mid(offset, 4);
            QVERIFY2(got == wanted,
                     qPrintable(QStringLiteral("(%1,%2): expected %3, actual %4")
                         .arg(x).arg(y).arg(pixelDescription(wanted), pixelDescription(got))));
        }
    }
}

void KisRasterEditContractTest::historyRestoresPixels()
{
    KisPaintDeviceSP device = new KisPaintDevice(m_colorSpace);
    KisSurrogateUndoAdapter history;
    const QRect bounds(-3, -3, 134, 70);
    const QByteArray background = bgra({20, 60, 100, 192});
    device->fill(bounds, KoColor(reinterpret_cast<const quint8 *>(background.constData()), m_colorSpace));
    const QByteArray original = pixels(device, bounds);

    KisTransaction stroke(device);
    paint(device, QRect(-1, -1, 67, 67), bgra({255, 0, 0, 128}), COMPOSITE_OVER);
    paint(device, QRect(62, 1, 67, 65), bgra({0, 255, 0, 128}), COMPOSITE_OVER);
    const QByteArray painted = pixels(device, bounds);
    QVERIFY(painted != original);
    stroke.commit(&history);

    KisTransaction erase(device);
    paint(device, QRect(0, 0, 65, 65), bgra({0, 0, 0, 255}), COMPOSITE_ERASE);
    const QByteArray erased = pixels(device, bounds);
    QVERIFY(erased != painted);
    erase.commit(&history);

    KisTransaction cancelledAtHead(device);
    paint(device, bounds, bgra({255, 255, 255, 255}), COMPOSITE_OVER);
    cancelledAtHead.revert();
    QCOMPARE(pixelDifference(device, bounds, erased), QString());

    history.undo();
    QCOMPARE(pixelDifference(device, bounds, painted), QString());
    history.undo();
    QCOMPARE(pixelDifference(device, bounds, original), QString());
    history.redo();
    QCOMPARE(pixelDifference(device, bounds, painted), QString());

    // A tentative version has its own edit history. The accepted device keeps
    // its pending redo branch while the tentative version is drawn and dropped.
    KisPaintDeviceSP working = new KisPaintDevice(*device);
    KisTransaction cancelled(working);
    paint(working, bounds, bgra({255, 255, 255, 255}), COMPOSITE_OVER);
    QVERIFY(pixels(working, bounds) != painted);
    QCOMPARE(pixelDifference(device, bounds, painted), QString());
    cancelled.revert();
    working.clear();
    QCOMPARE(pixelDifference(device, bounds, painted), QString());
    history.redo();
    QCOMPARE(pixelDifference(device, bounds, erased), QString());
}

void KisRasterEditContractTest::replacementEditRestoresOldFootprint()
{
    KisPaintDeviceSP device = new KisPaintDevice(m_colorSpace);
    KisSurrogateUndoAdapter history;
    const QRect bounds(-4, -4, 136, 72);
    const QByteArray original = pixels(device, bounds);
    KisPaintDeviceSP working = new KisPaintDevice(*device);
    paint(working, QRect(-1, -1, 67, 67), bgra({255, 0, 0, 128}), COMPOSITE_OVER);
    QVERIFY(pixels(working, bounds) != original);
    QCOMPARE(pixelDifference(device, bounds, original), QString());

    working = new KisPaintDevice(*device);
    paint(working, QRect(63, 1, 67, 65), bgra({255, 0, 0, 128}), COMPOSITE_OVER);
    const QByteArray accepted = pixels(working, bounds);
    QCOMPARE(pixelDifference(device, bounds, original), QString());

    KisPaintDeviceSP direct = new KisPaintDevice(m_colorSpace);
    paint(direct, QRect(63, 1, 67, 65), bgra({255, 0, 0, 128}), COMPOSITE_OVER);
    QCOMPARE(pixelDifference(direct, bounds, accepted), QString());

    KisTransaction replacement(device);
    {
        KisPainter painter(device);
        painter.setCompositeOpId(COMPOSITE_COPY);
        painter.bitBlt(bounds.topLeft(), working, bounds);
    }
    replacement.commit(&history);
    QCOMPARE(pixelDifference(device, bounds, accepted), QString());
    history.undo();
    QCOMPARE(pixelDifference(device, bounds, original), QString());
    history.redo();
    QCOMPARE(pixelDifference(device, bounds, accepted), QString());
}

void KisRasterEditContractTest::layerOrder()
{
    const QRect rect(-2, -2, 67, 67);
    KisPaintDeviceSP projection = new KisPaintDevice(m_colorSpace);
    paint(projection, rect, bgra({0, 0, 255, 255}), COMPOSITE_OVER);
    paint(projection, rect, bgra({255, 0, 0, 128}), COMPOSITE_OVER);
    paint(projection, rect, bgra({0, 255, 0, 128}), COMPOSITE_OVER);
    QCOMPARE(pixelDifference(projection, rect, bgra({64, 128, 63, 255}).repeated(rect.width() * rect.height())), QString());
    projection->clear();
    paint(projection, rect, bgra({0, 0, 255, 255}), COMPOSITE_OVER);
    paint(projection, rect, bgra({0, 255, 0, 128}), COMPOSITE_OVER);
    paint(projection, rect, bgra({255, 0, 0, 128}), COMPOSITE_OVER);
    QCOMPARE(pixelDifference(projection, rect, bgra({128, 64, 63, 255}).repeated(rect.width() * rect.height())), QString());
}

SIMPLE_TEST_MAIN(KisRasterEditContractTest)

#include "KisRasterEditContractTest.moc"
