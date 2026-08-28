/*
 *  SPDX-FileCopyrightText: 2024 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisHistoryListTest.h"

#include <QTest>

#include "KisHistoryList.h"
#include "KisSortedHistoryList.h"

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}


namespace {
auto refState = [](std::initializer_list<int> list) -> std::vector<int> {
    return list;
};

std::vector<int> getState(const KisHistoryList<int> &list) {
    std::vector<int> state;
    state.reserve(list.size());
    std::copy(list.cbegin(), list.cend(), std::back_inserter(state));
    return state;
}

std::vector<int> getState(const KisSortedHistoryList<int> &list) {
    std::vector<int> state;
    state.reserve(list.size());
    std::copy(list.cbegin(), list.cend(), std::back_inserter(state));
    return state;
}

}

void KisHistoryListTest::testRotation()
{
    KisHistoryList<int> list(5);

    QCOMPARE(getState(list), refState({}));

    list.append(1);
    QCOMPARE(getState(list), refState({1}));

    list.append(2);
    QCOMPARE(getState(list), refState({2, 1}));

    list.append(3);
    QCOMPARE(getState(list), refState({3, 2, 1}));

    list.append(4);
    QCOMPARE(getState(list), refState({4, 3, 2, 1}));

    list.append(5);
    QCOMPARE(getState(list), refState({5, 4, 3, 2, 1}));

    list.append(6);
    QCOMPARE(getState(list), refState({6, 5, 4, 3, 2}));

    list.clear();
    QCOMPARE(list.size(), 0);
    QCOMPARE(getState(list), refState({}));
}

void KisHistoryListTest::testBubbleUp()
{
    KisHistoryList<int> list(5);

    QCOMPARE(getState(list), refState({}));

    list.append(1);
    list.append(2);
    list.append(3);
    list.append(4);
    list.append(5);
    QCOMPARE(getState(list), refState({5, 4, 3, 2, 1}));

    list.append(3);

    QCOMPARE(getState(list), refState({3, 5, 4, 2, 1}));
}

void KisHistoryListTest::testSortedList()
{
    KisSortedHistoryList<int> list(5);

    list.append(1);
    list.append(2);
    list.append(3);
    list.append(4);
    list.append(5);
    list.append(3);
    list.append(10);
    list.append(11);
    list.append(3);

    // first, check if unsorted version works correctly
    QCOMPARE(getState(list), refState({3, 11, 10, 5, 4}));

    list.setCompareLess(std::less{});

    // set up sorting, the values should be sorted now
    QCOMPARE(getState(list), refState({3, 4, 5, 10, 11}));

    list.append(12);

    // the oldest values are dropped from the middle of the sorted list
    QCOMPARE(getState(list), refState({3, 5, 10, 11, 12}));

    list.append(13);

    // more oldest values are dropped from the middle
    QCOMPARE(getState(list), refState({3, 10, 11, 12, 13}));

    list.setCompareLess(std::greater{});

    // reverse sorting order
    QCOMPARE(getState(list), refState({13, 12, 11, 10, 3}));

    list.setCompareLess(KisSortedHistoryList<int>::compare_less{});

    // disable sorting, show the original historical order
    QCOMPARE(getState(list), refState({13, 12, 3, 11, 10}));
}

void KisHistoryListTest::testSortedListAccessAndClear()
{
    KisSortedHistoryList<int> list(3);

    QCOMPARE(list.maxSize(), 3);
    QCOMPARE(list.size(), 0);
    KisSortedHistoryList<int>::const_iterator begin = list.cbegin();
    QCOMPARE(begin, list.cend());

    list.append(4);
    list.append(2);
    list.append(3);
    QCOMPARE(list.at(0), 3);
    QCOMPARE(list.at(1), 2);
    QCOMPARE(list.at(2), 4);

    list.setCompareLess(std::less<>());
    QCOMPARE(list.at(0), 2);
    QCOMPARE(list.at(1), 3);
    QCOMPARE(list.at(2), 4);

    list.clear();
    QCOMPARE(list.size(), 0);
    begin = list.cbegin();
    QCOMPARE(begin, list.cend());
}


QTEST_GUILESS_MAIN(KisHistoryListTest)
