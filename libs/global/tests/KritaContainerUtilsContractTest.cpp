/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "krita_container_utils.h"

#include <QTest>

#include <vector>

class KritaContainerUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void unorderedComparisonPreservesMembershipRule();
    void algorithmsPreserveUniqueAndFilteredValues();
    void traitsClassifySupportedContainers();
};

void KritaContainerUtilsContractTest::unorderedComparisonPreservesMembershipRule()
{
    QVERIFY(KritaUtils::compareListsUnordered(QList<int>{1, 2, 3}, QList<int>{3, 1, 2}));
    QVERIFY(!KritaUtils::compareListsUnordered(QList<int>{1, 2}, QList<int>{1, 2, 3}));
    QVERIFY(!KritaUtils::compareListsUnordered(QList<int>{1, 2, 3}, QList<int>{1, 2, 4}));

    QVERIFY(KritaUtils::compareListsUnordered(QList<int>{1, 1}, QList<int>{1, 2}));
}

void KritaContainerUtilsContractTest::algorithmsPreserveUniqueAndFilteredValues()
{
    QList<int> values{3, 1, 3, 2, 1, 4};
    KritaUtils::makeContainerUnique(values);
    QCOMPARE(values, QList<int>({1, 2, 3, 4}));

    KritaUtils::filterContainer(values, [](int value) {
        return value % 2 == 0;
    });
    QCOMPARE(values, QList<int>({2, 4}));
}

void KritaContainerUtilsContractTest::traitsClassifySupportedContainers()
{
    using VectorTraits = KritaUtils::is_container<std::vector<int>>;
    using AppendableVectorTraits = KritaUtils::is_appendable_container<std::vector<int>>;

    QVERIFY(VectorTraits::value);
    QVERIFY(VectorTraits::test<std::vector<int>>(nullptr));
    QVERIFY(AppendableVectorTraits::value);
    QVERIFY(AppendableVectorTraits::test<std::vector<int>>(nullptr));

    QVERIFY(!KritaUtils::is_container<int>::value);
    QVERIFY(!KritaUtils::is_container<int>::test<int>(nullptr));
    QVERIFY(!KritaUtils::is_appendable_container<int>::value);
    QVERIFY(!KritaUtils::is_appendable_container<int>::test<int>(nullptr));
}

QTEST_GUILESS_MAIN(KritaContainerUtilsContractTest)

#include "KritaContainerUtilsContractTest.moc"
