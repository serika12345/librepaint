/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_painting_tweaks.h"

#include <QImage>
#include <QPainter>
#include <QRegion>
#include <QTest>

#include <array>

class KisPaintingTweaksContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void clipHelpersFollowPainterClip();
    void antsPensUseRequestedPatternAndColors();
    void penBrushSaverRestoresEveryConstructionPath();
    void colorCalculationsBlendMeasureAndSeparateColors();
    void coarseLuminosityPreservesCurrentQuantization();
    void rectangleConversionsProduceTwoTriangles();
};

void KisPaintingTweaksContractTest::clipHelpersFollowPainterClip()
{
    QImage image(64, 64, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&image);
    QRegion clip(QRect(2, 3, 10, 12));
    clip |= QRect(20, 10, 8, 9);
    painter.setClipRegion(clip);

    QCOMPARE(KisPaintingTweaks::safeClipRegion(painter), clip);
    QCOMPARE(KisPaintingTweaks::safeClipBoundingRect(painter), clip.boundingRect());
}

void KisPaintingTweaksContractTest::antsPensUseRequestedPatternAndColors()
{
    QPen antsPen;
    QPen outlinePen;
    KisPaintingTweaks::initAntsPen(
        &antsPen, &outlinePen, 3, 5, QColor(Qt::red), QColor(Qt::blue));

    QCOMPARE(antsPen.style(), Qt::CustomDashLine);
    QCOMPARE(antsPen.dashPattern(), QVector<qreal>({3.0, 5.0}));
    QVERIFY(antsPen.isCosmetic());
    QCOMPARE(antsPen.color(), QColor(Qt::red));
    QCOMPARE(outlinePen.style(), Qt::SolidLine);
    QVERIFY(outlinePen.isCosmetic());
    QCOMPARE(outlinePen.color(), QColor(Qt::blue));
}

void KisPaintingTweaksContractTest::penBrushSaverRestoresEveryConstructionPath()
{
    QImage image(16, 16, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&image);
    const QPen originalPen(Qt::red);
    const QBrush originalBrush(Qt::green);
    painter.setPen(originalPen);
    painter.setBrush(originalBrush);

    {
        KisPaintingTweaks::PenBrushSaver saver(&painter);
        painter.setPen(QPen(Qt::blue));
        painter.setBrush(QBrush(Qt::yellow));
    }
    QCOMPARE(painter.pen(), originalPen);
    QCOMPARE(painter.brush(), originalBrush);

    const QPen replacementPen(Qt::cyan);
    const QBrush replacementBrush(Qt::magenta);
    {
        KisPaintingTweaks::PenBrushSaver saver(
            &painter, replacementPen, replacementBrush);
        QCOMPARE(painter.pen(), replacementPen);
        QCOMPARE(painter.brush(), replacementBrush);
    }
    QCOMPARE(painter.pen(), originalPen);
    QCOMPARE(painter.brush(), originalBrush);

    const QPair<QPen, QBrush> replacementPair {replacementPen, replacementBrush};
    {
        KisPaintingTweaks::PenBrushSaver saver(&painter, replacementPair);
        QCOMPARE(painter.pen(), replacementPen);
        QCOMPARE(painter.brush(), replacementBrush);
    }

    const KisPaintingTweaks::PenBrushSaver::allow_noop_t explicitToken;
    KisPaintingTweaks::PenBrushSaver explicitNoop(nullptr, replacementPair, explicitToken);
    KisPaintingTweaks::PenBrushSaver staticNoop(
        nullptr, replacementPair, KisPaintingTweaks::PenBrushSaver::allow_noop);
    QCOMPARE(painter.pen(), originalPen);
    QCOMPARE(painter.brush(), originalBrush);
}

void KisPaintingTweaksContractTest::colorCalculationsBlendMeasureAndSeparateColors()
{
    const QColor black(Qt::black);
    const QColor white(Qt::white);
    const QColor blended = KisPaintingTweaks::blendColors(black, white, 0.25);
    QVERIFY(qAbs(blended.redF() - 0.75) < 0.0001);
    QVERIFY(qAbs(blended.greenF() - 0.75) < 0.0001);
    QVERIFY(qAbs(blended.blueF() - 0.75) < 0.0001);

    QCOMPARE(KisPaintingTweaks::colorDifference(black, black), 0.0);
    QCOMPARE(KisPaintingTweaks::colorDifference(black, white), 3.0);

    const QColor base = QColor::fromRgbF(0.5, 0.5, 0.5);
    QColor dragged = base;
    KisPaintingTweaks::dragColor(&dragged, base, 0.2);
    QVERIFY(KisPaintingTweaks::colorDifference(dragged, base) >= 0.2);
}

void KisPaintingTweaksContractTest::coarseLuminosityPreservesCurrentQuantization()
{
    QCOMPARE(KisPaintingTweaks::luminosityCoarse(QColor(Qt::black)), 0.1);
    QCOMPARE(KisPaintingTweaks::luminosityCoarse(QColor(Qt::white)), 1.0);
    QVERIFY(qAbs(KisPaintingTweaks::luminosityCoarse(QColor(Qt::red), false) - 0.2126)
            < 0.0001);
}

void KisPaintingTweaksContractTest::rectangleConversionsProduceTwoTriangles()
{
    const QRectF rect(1.0, 2.0, 3.0, 4.0);
    std::array<QVector3D, 6> vertices;
    KisPaintingTweaks::rectToVertices(vertices.data(), rect);
    const std::array<QVector3D, 6> expectedVertices {
        QVector3D(1.0f, 6.0f, 0.0f),
        QVector3D(1.0f, 2.0f, 0.0f),
        QVector3D(4.0f, 6.0f, 0.0f),
        QVector3D(1.0f, 2.0f, 0.0f),
        QVector3D(4.0f, 2.0f, 0.0f),
        QVector3D(4.0f, 6.0f, 0.0f),
    };
    QCOMPARE(vertices, expectedVertices);

    std::array<QVector2D, 6> texCoords;
    KisPaintingTweaks::rectToTexCoords(texCoords.data(), rect);
    const std::array<QVector2D, 6> expectedTexCoords {
        QVector2D(1.0f, 6.0f),
        QVector2D(1.0f, 2.0f),
        QVector2D(4.0f, 6.0f),
        QVector2D(1.0f, 2.0f),
        QVector2D(4.0f, 2.0f),
        QVector2D(4.0f, 6.0f),
    };
    QCOMPARE(texCoords, expectedTexCoords);
}

QTEST_GUILESS_MAIN(KisPaintingTweaksContractTest)

#include "KisPaintingTweaksContractTest.moc"
