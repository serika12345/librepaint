/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisQPainterStateSaver.h>

#include <QBrush>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QTest>
#include <QTransform>

class KisQPainterStateSaverTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void restoresPainterState();
};

void KisQPainterStateSaverTest::restoresPainterState()
{
    QImage image(32, 32, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&image);

    const QTransform initialTransform = QTransform::fromTranslate(3.0, 5.0).scale(2.0, 2.0);
    const QPen initialPen(Qt::red, 3.0);
    const QBrush initialBrush(Qt::blue);
    QPainterPath initialClip;
    initialClip.addRect(QRectF(1.0, 2.0, 10.0, 12.0));

    painter.setTransform(initialTransform);
    painter.setOpacity(0.75);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setPen(initialPen);
    painter.setBrush(initialBrush);
    painter.setClipPath(initialClip);

    {
        KisQPainterStateSaver saver(&painter);
        painter.resetTransform();
        painter.setOpacity(0.25);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.setPen(QPen(Qt::green, 1.0));
        painter.setBrush(QBrush(Qt::yellow));
        painter.setClipping(false);

        QVERIFY(painter.transform() != initialTransform);
        QCOMPARE(painter.opacity(), 0.25);
        QCOMPARE(painter.compositionMode(), QPainter::CompositionMode_Source);
        QVERIFY(!painter.hasClipping());
    }

    QCOMPARE(painter.transform(), initialTransform);
    QCOMPARE(painter.opacity(), 0.75);
    QCOMPARE(painter.compositionMode(), QPainter::CompositionMode_SourceOver);
    QCOMPARE(painter.pen(), initialPen);
    QCOMPARE(painter.brush(), initialBrush);
    QVERIFY(painter.hasClipping());
    QCOMPARE(painter.clipPath(), initialClip);
}

QTEST_GUILESS_MAIN(KisQPainterStateSaverTest)

#include "KisQPainterStateSaverTest.moc"
