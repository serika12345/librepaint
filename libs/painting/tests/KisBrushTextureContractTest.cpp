/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "strokes/KisMaskingBrushCompositeOp.h"

/** Prepared texture masks change only source alpha before the painter composites a dab. */
class KisBrushTextureContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void multiplyMatchesFixedAlphaProducts() {
        QFile file(QStringLiteral(TEXTURE_FIXTURE));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto fixture = QJsonDocument::fromJson(file.readAll()).object();
        QCOMPARE(fixture["schema"].toInt(), 1);
        const auto alpha = fixture["alpha"].toArray();
        const auto products = fixture["products"].toArray();
        KisMaskingBrushCompositeOp<quint8, KIS_MASKING_BRUSH_COMPOSITE_MULT, true, true, false> operation(4, 3, 1);
        for (int row = 0; row < alpha.size(); ++row) {
            const auto expected = QByteArray::fromHex(products[row].toString().toLatin1());
            for (int column = 0; column < alpha.size(); ++column) {
                const quint8 mask = alpha[column].toInt();
                quint8 pixel[] = {17, 83, 199, quint8(alpha[row].toInt())};
                operation.composite(&mask, 1, pixel, 4, 1, 1);
                QCOMPARE(pixel[3], quint8(expected[column]));
                QCOMPARE(pixel[0], quint8(17));
                QCOMPARE(pixel[1], quint8(83));
                QCOMPARE(pixel[2], quint8(199));
            }
        }
    }
};
QTEST_GUILESS_MAIN(KisBrushTextureContractTest)
#include "KisBrushTextureContractTest.moc"
