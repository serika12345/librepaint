/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisTagLabel.h>

#include <QPointer>
#include <QTest>

class KisTagLabelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesTextAndFollowsParentLifetime();
};

void KisTagLabelContractTest::preservesTextAndFollowsParentLifetime()
{
    QPointer<KisTagLabel> label;
    {
        QWidget parent;
        label = new KisTagLabel(QStringLiteral("Favorites"), &parent);

        QCOMPARE(label->getText(), QStringLiteral("Favorites"));
        QCOMPARE(label->parentWidget(), &parent);
    }

    QVERIFY(label.isNull());
}

QTEST_MAIN(KisTagLabelContractTest)

#include "KisTagLabelContractTest.moc"
