/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_async_merger.h>

#include <QTest>

#include <type_traits>

class KisAsyncMergerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mergerTypeAndStartSchemaRemainStable();
};

void KisAsyncMergerSchemaContractTest::mergerTypeAndStartSchemaRemainStable()
{
    using Merger = KisAsyncMerger;

    static_assert(std::is_class_v<Merger>);
    static_assert(std::is_default_constructible_v<Merger>);
    static_assert(std::is_same_v<decltype(&Merger::startMerge), void (Merger::*)(KisBaseRectsWalker &, bool)>);
}

QTEST_GUILESS_MAIN(KisAsyncMergerSchemaContractTest)

#include "KisAsyncMergerSchemaContractTest.moc"
