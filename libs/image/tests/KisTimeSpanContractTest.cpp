/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "kis_time_span.h"

#include <QDebug>
#include <QDomDocument>
#include <QTest>

#include <limits>

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    qFatal("Unexpected time-span assertion");
}

class KisTimeSpanContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionAndFiniteInfiniteStateRemainStable();
    void containmentExpansionAndOverlapRemainStable();
    void unionAndIntersectionRemainStable();
    void domRoundTripAndDiagnosticRemainStable();
};

void KisTimeSpanContractTest::constructionAndFiniteInfiniteStateRemainStable()
{
    const KisTimeSpan invalid;
    QCOMPARE(invalid.start(), 0);
    QCOMPARE(invalid.end(), -1);
    QCOMPARE(invalid.duration(), 0);
    QVERIFY(!invalid.isValid());
    QVERIFY(!invalid.isInfinite());
    QVERIFY(invalid == KisTimeSpan());

    const KisTimeSpan finite = KisTimeSpan::fromTimeToTime(5, 9);
    QCOMPARE(finite.start(), 5);
    QCOMPARE(finite.end(), 9);
    QCOMPARE(finite.duration(), 5);
    QVERIFY(finite.isValid());
    QVERIFY(!finite.isInfinite());
    QVERIFY(finite == KisTimeSpan::fromTimeWithDuration(5, 5));

    QVERIFY(!KisTimeSpan::fromTimeToTime(9, 5).isValid());
    QVERIFY(!KisTimeSpan::fromTimeWithDuration(5, 0).isValid());

    const KisTimeSpan infinite = KisTimeSpan::infinite(7);
    QCOMPARE(infinite.start(), 7);
    QCOMPARE(infinite.end(), std::numeric_limits<int>::min());
    QCOMPARE(infinite.duration(), 0);
    QVERIFY(infinite.isValid());
    QVERIFY(infinite.isInfinite());
    QVERIFY(infinite == KisTimeSpan::infinite(7));
}

void KisTimeSpanContractTest::containmentExpansionAndOverlapRemainStable()
{
    KisTimeSpan finite = KisTimeSpan::fromTimeToTime(5, 9);
    QVERIFY(finite.contains(5));
    QVERIFY(finite.contains(9));
    QVERIFY(!finite.contains(4));
    QVERIFY(!finite.contains(10));

    finite.include(3);
    finite.include(12);
    QCOMPARE(finite.start(), 3);
    QCOMPARE(finite.end(), 12);

    QVERIFY(finite.overlaps(KisTimeSpan::fromTimeToTime(12, 15)));
    QVERIFY(!finite.overlaps(KisTimeSpan::fromTimeToTime(13, 15)));
    QVERIFY(!finite.overlaps(KisTimeSpan()));

    const KisTimeSpan infinite = KisTimeSpan::infinite(10);
    QVERIFY(!infinite.contains(9));
    QVERIFY(infinite.contains(10));
    QVERIFY(infinite.contains(100));
    QVERIFY(infinite.overlaps(KisTimeSpan::fromTimeToTime(5, 10)));
    QVERIFY(KisTimeSpan::fromTimeToTime(10, 15).overlaps(infinite));
    QVERIFY(!infinite.overlaps(KisTimeSpan::fromTimeToTime(0, 9)));
}

void KisTimeSpanContractTest::unionAndIntersectionRemainStable()
{
    const KisTimeSpan left = KisTimeSpan::fromTimeToTime(5, 10);
    const KisTimeSpan right = KisTimeSpan::fromTimeToTime(8, 14);

    QVERIFY((left | right) == KisTimeSpan::fromTimeToTime(5, 14));
    QVERIFY((left & right) == KisTimeSpan::fromTimeToTime(8, 10));
    QVERIFY(!(left & KisTimeSpan::fromTimeToTime(20, 25)).isValid());
    QVERIFY((KisTimeSpan() | left) == left);
    QVERIFY((left | KisTimeSpan()) == left);

    const KisTimeSpan infinite = KisTimeSpan::infinite(8);
    QVERIFY((infinite | left) == KisTimeSpan::infinite(5));
    QVERIFY((infinite & left) == KisTimeSpan::fromTimeToTime(8, 10));

    KisTimeSpan unionAssignment = left;
    const KisTimeSpan *unionAddress = &(unionAssignment |= right);
    QCOMPARE(unionAddress, &unionAssignment);
    QVERIFY(unionAssignment == KisTimeSpan::fromTimeToTime(5, 14));

    KisTimeSpan intersectionAssignment = left;
    const KisTimeSpan *intersectionAddress = &(intersectionAssignment &= right);
    QCOMPARE(intersectionAddress, &intersectionAssignment);
    QVERIFY(intersectionAssignment == KisTimeSpan::fromTimeToTime(8, 10));
}

void KisTimeSpanContractTest::domRoundTripAndDiagnosticRemainStable()
{
    QDomDocument document("time-span-contract");
    QDomElement root = document.createElement("root");
    document.appendChild(root);

    const KisTimeSpan finite = KisTimeSpan::fromTimeToTime(3, 7);
    KisDomUtils::saveValue(&root, "finite", finite);
    const QDomElement finiteElement = root.firstChildElement("finite");
    QCOMPARE(finiteElement.attribute("type"), QString("timerange"));
    QCOMPARE(finiteElement.attribute("from"), QString("3"));
    QCOMPARE(finiteElement.attribute("to"), QString("7"));

    const KisTimeSpan infinite = KisTimeSpan::infinite(11);
    KisDomUtils::saveValue(&root, "infinite", infinite);
    const QDomElement infiniteElement = root.firstChildElement("infinite");
    QCOMPARE(infiniteElement.attribute("from"), QString("11"));
    QVERIFY(!infiniteElement.hasAttribute("to"));

    KisDomUtils::saveValue(&root, "invalid", KisTimeSpan());
    const QDomElement invalidElement = root.firstChildElement("invalid");
    QVERIFY(!invalidElement.hasAttribute("from"));
    QVERIFY(!invalidElement.hasAttribute("to"));

    KisTimeSpan loaded;
    QVERIFY(KisDomUtils::loadValue(root, "finite", &loaded));
    QVERIFY(loaded == finite);
    QVERIFY(KisDomUtils::loadValue(root, "infinite", &loaded));
    QVERIFY(loaded == infinite);
    QVERIFY(KisDomUtils::loadValue(root, "invalid", &loaded));
    QVERIFY(loaded == KisTimeSpan());

    const KisTimeSpan unchanged = KisTimeSpan::fromTimeToTime(20, 25);
    loaded = unchanged;
    QVERIFY(!KisDomUtils::loadValue(root, "missing", &loaded));
    QVERIFY(loaded == unchanged);

    QDomElement wrongType = document.createElement("wrongType");
    wrongType.setAttribute("type", "value");
    root.appendChild(wrongType);
    QVERIFY(!KisDomUtils::loadValue(root, "wrongType", &loaded));
    QVERIFY(loaded == unchanged);

    QString diagnostic;
    {
        QDebug debug(&diagnostic);
        debug << finite;
    }
    QCOMPARE(diagnostic, QString("KisTimeSpan(3, 7) "));
}

QTEST_GUILESS_MAIN(KisTimeSpanContractTest)

#include "KisTimeSpanContractTest.moc"
