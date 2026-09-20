/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "svg/parsers/SvgTransformParser.h"

#include <QTest>

#include <cmath>

namespace
{
QTransform rotated(qreal angle)
{
    QTransform transform;
    transform.rotate(angle);
    return transform;
}

QTransform rotatedAround(qreal angle, qreal centerX, qreal centerY)
{
    return QTransform::fromTranslate(-centerX, -centerY) * rotated(angle) * QTransform::fromTranslate(centerX, centerY);
}

QTransform skewed(qreal xAngle, qreal yAngle)
{
    constexpr qreal degreesToRadians = qreal(0.017453292519943295769);
    QTransform transform;
    transform.shear(std::tan(degreesToRadians * xAngle), std::tan(degreesToRadians * yAngle));
    return transform;
}
} // namespace

class SvgTransformParserContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesEverySupportedTransformType_data();
    void parsesEverySupportedTransformType();
    void composesTransformsInSourceOrder();
    void rejectsMalformedTransforms_data();
    void rejectsMalformedTransforms();
};

void SvgTransformParserContractTest::parsesEverySupportedTransformType_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QTransform>("expected");

    QTest::newRow("matrix") << QStringLiteral("matrix(1 2 3 4 5 6)") << QTransform(1, 2, 3, 4, 5, 6);
    QTest::newRow("translate-pair") << QStringLiteral("translate(10,-20)") << QTransform::fromTranslate(10, -20);
    QTest::newRow("translate-single") << QStringLiteral("translate(10)") << QTransform::fromTranslate(10, 0);
    QTest::newRow("scale-pair") << QStringLiteral("scale(2 3)") << QTransform::fromScale(2, 3);
    QTest::newRow("scale-single") << QStringLiteral("scale(2)") << QTransform::fromScale(2, 2);
    QTest::newRow("rotate-origin") << QStringLiteral("rotate(90)") << rotated(90);
    QTest::newRow("rotate-center") << QStringLiteral("rotate(30,10,20)") << rotatedAround(30, 10, 20);
    QTest::newRow("skew-x") << QStringLiteral("skewX(45)") << skewed(45, 0);
    QTest::newRow("skew-y") << QStringLiteral("skewY(-30)") << skewed(0, -30);
}

void SvgTransformParserContractTest::parsesEverySupportedTransformType()
{
    QFETCH(QString, source);
    QFETCH(QTransform, expected);

    const SvgTransformParser parser(source);

    QVERIFY(parser.isValid());
    QCOMPARE(parser.transform(), expected);
}

void SvgTransformParserContractTest::composesTransformsInSourceOrder()
{
    const QTransform translation = QTransform::fromTranslate(10, 20);
    const QTransform scale = QTransform::fromScale(2, 3);
    const QTransform rotation = rotated(90);
    const SvgTransformParser parser(QStringLiteral("translate(10,20) scale(2,3), rotate(90)"));

    QVERIFY(parser.isValid());
    QCOMPARE(parser.transform(), rotation * scale * translation);
}

void SvgTransformParserContractTest::rejectsMalformedTransforms_data()
{
    QTest::addColumn<QString>("source");

    QTest::newRow("empty") << QString();
    QTest::newRow("missing-parenthesis") << QStringLiteral("translate(10,20");
    QTest::newRow("unknown-transform") << QStringLiteral("flip(10)");
    QTest::newRow("trailing-input") << QStringLiteral("scale(2) trailing");
    QTest::newRow("incomplete-matrix") << QStringLiteral("matrix(1 2 3 4 5)");
    QTest::newRow("incomplete-rotation-center") << QStringLiteral("rotate(30,10)");
}

void SvgTransformParserContractTest::rejectsMalformedTransforms()
{
    QFETCH(QString, source);

    const SvgTransformParser parser(source);

    QVERIFY(!parser.isValid());
    QCOMPARE(parser.transform(), QTransform());
}

QTEST_GUILESS_MAIN(SvgTransformParserContractTest)

#include "SvgTransformParserContractTest.moc"
