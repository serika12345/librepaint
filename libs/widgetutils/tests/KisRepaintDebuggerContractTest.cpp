/* This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QImage>
#include <QPaintEvent>
#include <QTest>
#include <QVector>

#include <array>
#include <type_traits>

#include "KisRepaintDebugger.h"

namespace
{

QImage transparentImage(const QSize &size = QSize(12, 10))
{
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

bool isPainted(const QImage &image, const QPoint &point)
{
    return image.pixelColor(point).alpha() != 0;
}

} // namespace

class KisRepaintDebuggerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void enabledStateAndValueLifetime();
    void rectangleOverloadsPaintOnlyRequestedAreas();
    void paintEventUsesItsUpdateRectangle();
    void paintFullCoversTheDevice();
    void colorsRepeatAfterSixPaintOperations();
};

void KisRepaintDebuggerContractTest::initTestCase()
{
    qputenv("KRITA_DEBUG_REPAINT", "1");
}

void KisRepaintDebuggerContractTest::enabledStateAndValueLifetime()
{
    QVERIFY(KisRepaintDebugger::enabled());
    QVERIFY(std::is_default_constructible_v<KisRepaintDebugger>);
    QVERIFY(std::is_destructible_v<KisRepaintDebugger>);

    KisRepaintDebugger debugger;
    Q_UNUSED(debugger)
}

void KisRepaintDebuggerContractTest::rectangleOverloadsPaintOnlyRequestedAreas()
{
    KisRepaintDebugger debugger;
    QImage singleRectangleImage = transparentImage();
    const QRect singleRectangle(2, 3, 4, 2);

    debugger.paint(&singleRectangleImage, singleRectangle);

    QVERIFY(isPainted(singleRectangleImage, singleRectangle.center()));
    QVERIFY(!isPainted(singleRectangleImage, QPoint(0, 0)));
    QVERIFY(!isPainted(singleRectangleImage, QPoint(11, 9)));

    QImage multipleRectangleImage = transparentImage();
    const QVector<QRect> rectangles{QRect(1, 1, 2, 2), QRect(8, 6, 3, 3)};

    debugger.paint(&multipleRectangleImage, rectangles);

    QVERIFY(isPainted(multipleRectangleImage, QPoint(1, 1)));
    QVERIFY(isPainted(multipleRectangleImage, QPoint(9, 7)));
    QCOMPARE(multipleRectangleImage.pixelColor(1, 1), multipleRectangleImage.pixelColor(9, 7));
    QVERIFY(!isPainted(multipleRectangleImage, QPoint(5, 5)));
}

void KisRepaintDebuggerContractTest::paintEventUsesItsUpdateRectangle()
{
    KisRepaintDebugger debugger;
    QImage image = transparentImage();
    const QRect updateRectangle(3, 2, 5, 4);
    QPaintEvent event(updateRectangle);

    debugger.paint(&image, &event);

    QVERIFY(isPainted(image, updateRectangle.center()));
    QVERIFY(!isPainted(image, QPoint(0, 0)));
    QVERIFY(!isPainted(image, QPoint(11, 9)));
}

void KisRepaintDebuggerContractTest::paintFullCoversTheDevice()
{
    KisRepaintDebugger debugger;
    QImage image = transparentImage();

    debugger.paintFull(&image);

    QVERIFY(isPainted(image, QPoint(0, 0)));
    QVERIFY(isPainted(image, QPoint(11, 0)));
    QVERIFY(isPainted(image, QPoint(0, 9)));
    QVERIFY(isPainted(image, QPoint(11, 9)));
}

void KisRepaintDebuggerContractTest::colorsRepeatAfterSixPaintOperations()
{
    KisRepaintDebugger debugger;
    std::array<QColor, 7> colors;

    for (QColor &color : colors) {
        QImage image = transparentImage(QSize(1, 1));
        debugger.paintFull(&image);
        color = image.pixelColor(0, 0);
    }

    QVERIFY(colors[0] != colors[1]);
    QCOMPARE(colors[0], colors[6]);
    for (const QColor &color : colors) {
        QCOMPARE(color.alpha(), 63);
    }
}

QTEST_GUILESS_MAIN(KisRepaintDebuggerContractTest)

#include "KisRepaintDebuggerContractTest.moc"
