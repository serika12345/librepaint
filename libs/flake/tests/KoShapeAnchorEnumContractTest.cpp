/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoShapeAnchor.h"

#include <QTest>

class KoShapeAnchorEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void horizontalPlacementValuesRemainStable();
    void verticalPlacementValuesRemainStable();
    void anchorTypeValuesRemainStable();
};

void KoShapeAnchorEnumContractTest::horizontalPlacementValuesRemainStable()
{
    QCOMPARE(int(KoShapeAnchor::HCenter), 0);
    QCOMPARE(int(KoShapeAnchor::HFromInside), 1);
    QCOMPARE(int(KoShapeAnchor::HFromLeft), 2);
    QCOMPARE(int(KoShapeAnchor::HInside), 3);
    QCOMPARE(int(KoShapeAnchor::HLeft), 4);
    QCOMPARE(int(KoShapeAnchor::HOutside), 5);
    QCOMPARE(int(KoShapeAnchor::HRight), 6);

    QCOMPARE(int(KoShapeAnchor::HChar), 0);
    QCOMPARE(int(KoShapeAnchor::HPage), 1);
    QCOMPARE(int(KoShapeAnchor::HPageContent), 2);
    QCOMPARE(int(KoShapeAnchor::HPageStartMargin), 3);
    QCOMPARE(int(KoShapeAnchor::HPageEndMargin), 4);
    QCOMPARE(int(KoShapeAnchor::HFrame), 5);
    QCOMPARE(int(KoShapeAnchor::HFrameContent), 6);
    QCOMPARE(int(KoShapeAnchor::HFrameEndMargin), 7);
    QCOMPARE(int(KoShapeAnchor::HFrameStartMargin), 8);
    QCOMPARE(int(KoShapeAnchor::HParagraph), 9);
    QCOMPARE(int(KoShapeAnchor::HParagraphContent), 10);
    QCOMPARE(int(KoShapeAnchor::HParagraphEndMargin), 11);
    QCOMPARE(int(KoShapeAnchor::HParagraphStartMargin), 12);
}

void KoShapeAnchorEnumContractTest::verticalPlacementValuesRemainStable()
{
    QCOMPARE(int(KoShapeAnchor::VBelow), 0);
    QCOMPARE(int(KoShapeAnchor::VBottom), 1);
    QCOMPARE(int(KoShapeAnchor::VFromTop), 2);
    QCOMPARE(int(KoShapeAnchor::VMiddle), 3);
    QCOMPARE(int(KoShapeAnchor::VTop), 4);

    QCOMPARE(int(KoShapeAnchor::VBaseline), 0);
    QCOMPARE(int(KoShapeAnchor::VChar), 1);
    QCOMPARE(int(KoShapeAnchor::VFrame), 2);
    QCOMPARE(int(KoShapeAnchor::VFrameContent), 3);
    QCOMPARE(int(KoShapeAnchor::VLine), 4);
    QCOMPARE(int(KoShapeAnchor::VPage), 5);
    QCOMPARE(int(KoShapeAnchor::VPageContent), 6);
    QCOMPARE(int(KoShapeAnchor::VParagraph), 7);
    QCOMPARE(int(KoShapeAnchor::VParagraphContent), 8);
    QCOMPARE(int(KoShapeAnchor::VText), 9);
}

void KoShapeAnchorEnumContractTest::anchorTypeValuesRemainStable()
{
    QCOMPARE(int(KoShapeAnchor::AnchorAsCharacter), 0);
    QCOMPARE(int(KoShapeAnchor::AnchorToCharacter), 1);
    QCOMPARE(int(KoShapeAnchor::AnchorParagraph), 2);
    QCOMPARE(int(KoShapeAnchor::AnchorPage), 3);
}

QTEST_GUILESS_MAIN(KoShapeAnchorEnumContractTest)

#include "KoShapeAnchorEnumContractTest.moc"
