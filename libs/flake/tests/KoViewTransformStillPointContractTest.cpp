/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoViewTransformStillPoint.h"

#include <QDebug>
#include <QTest>

#include <utility>

namespace
{
QString pointDebugText(const QPointF &point)
{
    QString output;
    QDebug debug(&output);
    debug.nospace() << point;
    return output;
}
} // namespace

class KoViewTransformStillPointContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultConstructionKeepsBothOrigins();
    void valueAndPairConstructionPreservePointRoles();
    void copyAndMoveConstructionPreservePoints();
    void copyAndMoveAssignmentPreservePoints();
    void debugOutputNamesPointsInOrder();
};

void KoViewTransformStillPointContractTest::defaultConstructionKeepsBothOrigins()
{
    const KoViewTransformStillPoint point;

    QCOMPARE(point.docPoint(), QPointF());
    QCOMPARE(point.viewPoint(), QPointF());
}

void KoViewTransformStillPointContractTest::valueAndPairConstructionPreservePointRoles()
{
    const QPointF docPoint(-12.5, 7.25);
    const QPointF viewPoint(103.75, -41.5);

    const KoViewTransformStillPoint fromValues(docPoint, viewPoint);
    QCOMPARE(fromValues.docPoint(), docPoint);
    QCOMPARE(fromValues.viewPoint(), viewPoint);

    const std::pair<QPointF, QPointF> pair(docPoint, viewPoint);
    const KoViewTransformStillPoint fromPair(pair);
    QCOMPARE(fromPair.docPoint(), docPoint);
    QCOMPARE(fromPair.viewPoint(), viewPoint);
}

void KoViewTransformStillPointContractTest::copyAndMoveConstructionPreservePoints()
{
    const QPointF docPoint(-3.5, 9.25);
    const QPointF viewPoint(81.75, -17.5);
    const KoViewTransformStillPoint source(docPoint, viewPoint);

    const KoViewTransformStillPoint copied(source);
    QCOMPARE(copied.docPoint(), docPoint);
    QCOMPARE(copied.viewPoint(), viewPoint);

    KoViewTransformStillPoint moveSource(docPoint, viewPoint);
    const KoViewTransformStillPoint moved(std::move(moveSource));
    QCOMPARE(moved.docPoint(), docPoint);
    QCOMPARE(moved.viewPoint(), viewPoint);
}

void KoViewTransformStillPointContractTest::copyAndMoveAssignmentPreservePoints()
{
    const QPointF docPoint(1.5, -2.75);
    const QPointF viewPoint(-33.25, 64.5);
    const KoViewTransformStillPoint source(docPoint, viewPoint);
    KoViewTransformStillPoint copyTarget;

    KoViewTransformStillPoint &copyResult = (copyTarget = source);
    QCOMPARE(&copyResult, &copyTarget);
    QCOMPARE(copyTarget.docPoint(), docPoint);
    QCOMPARE(copyTarget.viewPoint(), viewPoint);

    KoViewTransformStillPoint moveSource(docPoint, viewPoint);
    KoViewTransformStillPoint moveTarget;
    KoViewTransformStillPoint &moveResult = (moveTarget = std::move(moveSource));
    QCOMPARE(&moveResult, &moveTarget);
    QCOMPARE(moveTarget.docPoint(), docPoint);
    QCOMPARE(moveTarget.viewPoint(), viewPoint);
}

void KoViewTransformStillPointContractTest::debugOutputNamesPointsInOrder()
{
    const QPointF docPoint(1.25, -2.5);
    const QPointF viewPoint(3.75, 4.5);
    const KoViewTransformStillPoint point(docPoint, viewPoint);
    QString output;

    {
        QDebug debug(&output);
        debug.nospace() << point;
    }

    const QString expected = QStringLiteral("KoViewTransformStillPoint(docPoint: ") + pointDebugText(docPoint)
        + QStringLiteral(", viewPoint: ") + pointDebugText(viewPoint) + QLatin1Char(')');
    QCOMPARE(output.trimmed(), expected);
}

QTEST_GUILESS_MAIN(KoViewTransformStillPointContractTest)

#include "KoViewTransformStillPointContractTest.moc"
