/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisQStringListFwd.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <QTest>

#include <type_traits>

class KisQStringListFwdTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aliasesMatchQtContainersAndPreserveValues();
};

void KisQStringListFwdTest::aliasesMatchQtContainersAndPreserveValues()
{
    QVERIFY((std::is_same_v<QStringList, QList<QString>>));
    QVERIFY((std::is_same_v<QByteArrayList, QList<QByteArray>>));
    QVERIFY((std::is_same_v<QVector<int>, QList<int>>));

    QStringList strings{QStringLiteral("first"), QStringLiteral("second")};
    strings.append(QStringLiteral("third"));
    QCOMPARE(strings.join(QLatin1Char(',')), QStringLiteral("first,second,third"));

    QByteArrayList byteArrays{QByteArray("a"), QByteArray("b")};
    byteArrays.prepend(QByteArray("start"));
    QCOMPARE(byteArrays, QByteArrayList({QByteArray("start"), QByteArray("a"), QByteArray("b")}));

    QVector<int> values{1, 2};
    values.append(3);
    QCOMPARE(values, QList<int>({1, 2, 3}));
}

QTEST_GUILESS_MAIN(KisQStringListFwdTest)

#include "KisQStringListFwdTest.moc"
