/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisTagModel.h>

#include <QTest>

class KisTagPseudoUrlCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pseudoTagUrlsRemainStable();
};

// Compatibility requirement: Existing SelectedTags settings depend on the All and All untagged pseudo-tag URLs.
void KisTagPseudoUrlCompatibilityTest::pseudoTagUrlsRemainStable()
{
    // Consumer: Users reopening LibrePaint with a saved tag selection.
    // Operation: The tag chooser resolves the URL saved in the SelectedTags configuration group.
    // Observable result: The saved pseudo-tag URL selects the all-resources or all-untagged-resources view.
    // Failure impact: The chooser cannot restore the user's saved resource filter after an update.
    QCOMPARE(KisAllTagsModel::urlAll(), QStringLiteral("All"));
    QCOMPARE(KisAllTagsModel::urlAllUntagged(), QStringLiteral("All untagged"));
    QVERIFY(KisAllTagsModel::urlAll() != KisAllTagsModel::urlAllUntagged());
}

QTEST_GUILESS_MAIN(KisTagPseudoUrlCompatibilityTest)

#include "KisTagPseudoUrlCompatibilityTest.moc"
