/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisTagLabel.h>

#include <QTest>

class KisTagLabelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void returnsTagNameForRemovingSelectedTag();
};

void KisTagLabelContractTest::returnsTagNameForRemovingSelectedTag()
{
    // Consumer: Bundle tag preview users selecting an already displayed tag.
    // Operation: The preview reads a tag label to remove the matching displayed tag.
    // Observable result: The label returns the tag name it presents.
    // Failure impact: Selecting a tag leaves a duplicate tag label in the bundle preview.
    KisTagLabel label(QStringLiteral("Favorites"));

    QCOMPARE(label.getText(), QStringLiteral("Favorites"));
}

QTEST_MAIN(KisTagLabelContractTest)

#include "KisTagLabelContractTest.moc"
