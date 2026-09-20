/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorProfileQuery.h>

#include <QDebug>
#include <QTest>

#include <utility>

class KoColorProfileQueryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionAndPublicValuesClassifyProfiles();
    void copyMoveAndEqualityPreserveEveryCriterion();
    void debugOutputReportsClassification();
};

void KoColorProfileQueryContractTest::constructionAndPublicValuesClassifyProfiles()
{
    const KoColorProfileQuery emptyQuery;

    QCOMPARE(emptyQuery.whitePoint, (KoColorimetryUtils::xy{0.0, 0.0}));
    QVERIFY(emptyQuery.rgbColorants.isEmpty());
    QCOMPARE(emptyQuery.primaries, PRIMARIES_UNSPECIFIED);
    QCOMPARE(emptyQuery.transfer, TRC_UNSPECIFIED);
    QVERIFY(!emptyQuery.hdrReferenceWhite.has_value());
    QVERIFY(!emptyQuery.isGrayscale());
    QVERIFY(!emptyQuery.isRgb());
    QVERIFY(!emptyQuery.isValid());

    const KoColorProfileQuery namedRgb(PRIMARIES_ITU_R_BT_2020_2_AND_2100_0, TRC_ITU_R_BT_2100_0_PQ, 203.0);

    QCOMPARE(namedRgb.primaries, PRIMARIES_ITU_R_BT_2020_2_AND_2100_0);
    QCOMPARE(namedRgb.transfer, TRC_ITU_R_BT_2100_0_PQ);
    QCOMPARE(namedRgb.hdrReferenceWhite, std::optional<double>(203.0));
    QVERIFY(!namedRgb.isGrayscale());
    QVERIFY(namedRgb.isRgb());
    QVERIFY(namedRgb.isValid());

    KoColorProfileQuery grayscale;
    grayscale.whitePoint = {0.3127, 0.3290};
    grayscale.transfer = TRC_LINEAR;

    QVERIFY(grayscale.isGrayscale());
    QVERIFY(!grayscale.isRgb());
    QVERIFY(grayscale.isValid());

    KoColorProfileQuery customRgb;
    customRgb.whitePoint = {0.3127, 0.3290};
    customRgb.rgbColorants = {{0.64, 0.33}, {0.30, 0.60}, {0.15, 0.06}};
    customRgb.transfer = TRC_IEC_61966_2_1;

    QVERIFY(!customRgb.isGrayscale());
    QVERIFY(customRgb.isRgb());
    QVERIFY(customRgb.isValid());
}

void KoColorProfileQueryContractTest::copyMoveAndEqualityPreserveEveryCriterion()
{
    KoColorProfileQuery original(PRIMARIES_ADOBE_RGB_1998, TRC_A98, 160.0);
    original.whitePoint = {0.3127, 0.3290};
    original.rgbColorants = {{0.64, 0.33}, {0.21, 0.71}, {0.15, 0.06}};

    KoColorProfileQuery copied(original);
    QCOMPARE(copied, original);

    KoColorProfileQuery moved(std::move(copied));
    QCOMPARE(moved, original);

    KoColorProfileQuery copyAssigned;
    copyAssigned = original;
    QCOMPARE(copyAssigned, original);

    KoColorProfileQuery moveAssigned;
    moveAssigned = std::move(copyAssigned);
    QCOMPARE(moveAssigned, original);

    KoColorProfileQuery different = original;
    different.whitePoint = {0.3457, 0.3585};
    QVERIFY(different != original);

    different = original;
    different.rgbColorants[0] = {0.68, 0.32};
    QVERIFY(different != original);

    different = original;
    different.primaries = PRIMARIES_ITU_R_BT_2020_2_AND_2100_0;
    QVERIFY(different != original);

    different = original;
    different.transfer = TRC_LINEAR;
    QVERIFY(different != original);

    different = original;
    different.hdrReferenceWhite = 203.0;
    QVERIFY(different != original);
}

void KoColorProfileQueryContractTest::debugOutputReportsClassification()
{
    KoColorProfileQuery grayscale;
    grayscale.whitePoint = {0.3127, 0.3290};
    grayscale.transfer = TRC_LINEAR;

    QString output;
    QDebug(&output) << grayscale;

    QVERIFY(output.contains(QStringLiteral("KoColorProfileQuery(")));
    QVERIFY(output.contains(QStringLiteral("WhitePoint:")));
    QVERIFY(output.contains(QStringLiteral("rgbColorants:QList()")));
    QVERIFY(output.contains(QStringLiteral("isGreyscale:true")));
    QVERIFY(output.contains(QStringLiteral("isRgb:false")));
    QVERIFY(output.contains(QStringLiteral("isValid:true")));
}

QTEST_GUILESS_MAIN(KoColorProfileQueryContractTest)

#include "KoColorProfileQueryContractTest.moc"
