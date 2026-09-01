/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "svg/SvgUtil.h"

#include <QTest>
#include <QTransform>

#include <type_traits>

class SvgUtilViewGeometryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preserveAspectRatioSchemaAndDefaultsRemainStable();
    void preserveAspectRatioParsingFormattingAndTransformRemainStable();
    void userSpaceIdentityConversionsRemainStable();
    void objectBoundingBoxConversionsRemainStable();
    void numberLexingAndListNormalizationRemainStable();
};

void SvgUtilViewGeometryContractTest::preserveAspectRatioSchemaAndDefaultsRemainStable()
{
    using Parser = SvgUtil::PreserveAspectRatioParser;

    static_assert(std::is_class_v<SvgUtil>);
    static_assert(std::is_class_v<Parser>);
    static_assert(std::is_enum_v<Parser::Alignment>);
    static_assert(std::is_same_v<decltype(Parser::defer), bool>);
    static_assert(std::is_same_v<decltype(Parser::mode), Qt::AspectRatioMode>);
    static_assert(std::is_same_v<decltype(Parser::xAlignment), Parser::Alignment>);
    static_assert(std::is_same_v<decltype(Parser::yAlignment), Parser::Alignment>);

    QCOMPARE(int(Parser::Min), 0);
    QCOMPARE(int(Parser::Middle), 1);
    QCOMPARE(int(Parser::Max), 2);

    const Parser invalid(QStringLiteral("not-an-aspect-ratio"));
    QVERIFY(!invalid.defer);
    QCOMPARE(invalid.mode, Qt::IgnoreAspectRatio);
    QCOMPARE(invalid.xAlignment, Parser::Min);
    QCOMPARE(invalid.yAlignment, Parser::Min);
}

void SvgUtilViewGeometryContractTest::preserveAspectRatioParsingFormattingAndTransformRemainStable()
{
    using Parser = SvgUtil::PreserveAspectRatioParser;

    const Parser middle(QStringLiteral("xMiDyMiD meet"));
    QVERIFY(!middle.defer);
    QCOMPARE(middle.mode, Qt::KeepAspectRatio);
    QCOMPARE(middle.xAlignment, Parser::Middle);
    QCOMPARE(middle.yAlignment, Parser::Middle);
    QCOMPARE(middle.rectAnchorPoint(QRectF(10.0, 20.0, 80.0, 40.0)), QPointF(50.0, 40.0));
    QCOMPARE(middle.toString(), QString());

    const Parser slice(QStringLiteral("defer xMaxYMin slice"));
    QVERIFY(slice.defer);
    QCOMPARE(slice.mode, Qt::KeepAspectRatioByExpanding);
    QCOMPARE(slice.xAlignment, Parser::Max);
    QCOMPARE(slice.yAlignment, Parser::Min);
    QCOMPARE(slice.rectAnchorPoint(QRectF(10.0, 20.0, 80.0, 40.0)), QPointF(90.0, 20.0));
    QCOMPARE(slice.toString(), QStringLiteral("defer xMaxYMin slice"));

    const QRectF elementBounds(10.0, 20.0, 200.0, 100.0);
    const QRectF viewRect(0.0, 0.0, 100.0, 100.0);

    QTransform meetTransform;
    SvgUtil::parseAspectRatio(middle, elementBounds, viewRect, &meetTransform);
    QCOMPARE(meetTransform.mapRect(viewRect), QRectF(60.0, 20.0, 100.0, 100.0));

    QTransform sliceTransform;
    SvgUtil::parseAspectRatio(slice, elementBounds, viewRect, &sliceTransform);
    QCOMPARE(sliceTransform.mapRect(viewRect), QRectF(10.0, 20.0, 200.0, 200.0));

    const Parser none(QStringLiteral("none"));
    const QTransform unchanged = QTransform::fromTranslate(7.0, -9.0);
    QTransform ignoredTransform = unchanged;
    SvgUtil::parseAspectRatio(none, elementBounds, viewRect, &ignoredTransform);
    QCOMPARE(ignoredTransform, unchanged);
    QCOMPARE(none.toString(), QStringLiteral("none"));
}

void SvgUtilViewGeometryContractTest::userSpaceIdentityConversionsRemainStable()
{
    QCOMPARE(SvgUtil::fromUserSpace(-12.5), -12.5);
    QCOMPARE(SvgUtil::toUserSpace(7.25), 7.25);
    QCOMPARE(SvgUtil::toUserSpace(QPointF(-3.0, 4.5)), QPointF(-3.0, 4.5));
    QCOMPARE(SvgUtil::toUserSpace(QRectF(-3.0, 4.5, 8.0, -2.0)), QRectF(-3.0, 4.5, 8.0, -2.0));
    QCOMPARE(SvgUtil::toUserSpace(QSizeF(8.0, -2.0)), QSizeF(8.0, -2.0));
}

void SvgUtilViewGeometryContractTest::objectBoundingBoxConversionsRemainStable()
{
    const QRectF bounds(-10.0, 20.0, 40.0, -30.0);
    const QPointF objectPoint(0.25, 0.5);
    const QSizeF objectSize(0.5, 0.2);

    QCOMPARE(SvgUtil::objectToUserSpace(objectPoint, bounds), QPointF(0.0, 5.0));
    QCOMPARE(SvgUtil::objectToUserSpace(objectSize, bounds), QSizeF(20.0, -6.0));
    QCOMPARE(SvgUtil::userSpaceToObject(QPointF(0.0, 5.0), bounds), objectPoint);
    QCOMPARE(SvgUtil::userSpaceToObject(QSizeF(20.0, -6.0), bounds), objectSize);

    const QRectF emptyBounds(3.0, 4.0, 0.0, 0.0);
    QCOMPARE(SvgUtil::userSpaceToObject(QPointF(99.0, -99.0), emptyBounds), QPointF());
    QCOMPARE(SvgUtil::userSpaceToObject(QSizeF(99.0, -99.0), emptyBounds), QSizeF(0.0, 0.0));
}

void SvgUtilViewGeometryContractTest::numberLexingAndListNormalizationRemainStable()
{
    const QByteArray exponentSource("+12.5e-2tail");
    qreal number = -1.0;
    const char *end = SvgUtil::parseNumber(exponentSource.constData(), number);
    QCOMPARE(number, qreal(0.125));
    QCOMPARE(end - exponentSource.constData(), qsizetype(8));
    QCOMPARE(QByteArray(end), QByteArray("tail"));

    const QByteArray negativeFraction("-.5E+2");
    end = SvgUtil::parseNumber(negativeFraction.constData(), number);
    QCOMPARE(number, qreal(-50.0));
    QCOMPARE(end - negativeFraction.constData(), negativeFraction.size());

    const QByteArray noNumber("tail");
    end = SvgUtil::parseNumber(noNumber.constData(), number);
    QCOMPARE(number, qreal(0.0));
    QCOMPARE(end, noNumber.constData());

    QCOMPARE(SvgUtil::simplifyList(QStringLiteral("  alpha,\r\n beta,, gamma  ")),
             QStringList({QStringLiteral("alpha"), QStringLiteral("beta"), QStringLiteral("gamma")}));
    QVERIFY(SvgUtil::simplifyList(QString()).isEmpty());
}

QTEST_GUILESS_MAIN(SvgUtilViewGeometryContractTest)

#include "SvgUtilViewGeometryContractTest.moc"
