/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoZoomState.h"

#include <QTest>

#include <cmath>

class KoZoomStateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateKeepsCanonicalRange();
    void constructionPreservesAllStateValues();
    void fuzzyEqualityAcceptsEquivalentPositiveValues();
    void comparisonRejectsEachDifferentStateValue();
};

void KoZoomStateContractTest::defaultStateKeepsCanonicalRange()
{
    const KoZoomState state;

    QCOMPARE(state.mode, KoZoomMode::ZOOM_CONSTANT);
    QCOMPARE(state.zoom, 1.0);
    QCOMPARE(state.minZoom, 0.1);
    QCOMPARE(state.maxZoom, 90.0);
}

void KoZoomStateContractTest::constructionPreservesAllStateValues()
{
    const KoZoomState state(KoZoomMode::ZOOM_WIDTH, 2.5, 0.25, 32.0);

    QCOMPARE(state.mode, KoZoomMode::ZOOM_WIDTH);
    QCOMPARE(state.zoom, 2.5);
    QCOMPARE(state.minZoom, 0.25);
    QCOMPARE(state.maxZoom, 32.0);
}

void KoZoomStateContractTest::fuzzyEqualityAcceptsEquivalentPositiveValues()
{
    const KoZoomState state(KoZoomMode::ZOOM_PAGE, 2.0, 0.25, 16.0);
    const KoZoomState adjacent(KoZoomMode::ZOOM_PAGE,
                               std::nextafter(state.zoom, 3.0),
                               std::nextafter(state.minZoom, 1.0),
                               std::nextafter(state.maxZoom, 17.0));

    QVERIFY(state == adjacent);
    QVERIFY(!(state != adjacent));
}

void KoZoomStateContractTest::comparisonRejectsEachDifferentStateValue()
{
    const KoZoomState state(KoZoomMode::ZOOM_HEIGHT, 4.0, 0.5, 64.0);

    const KoZoomState differentMode(KoZoomMode::ZOOM_CONSTANT, state.zoom, state.minZoom, state.maxZoom);
    const KoZoomState differentZoom(state.mode, 4.25, state.minZoom, state.maxZoom);
    const KoZoomState differentMinZoom(state.mode, state.zoom, 0.75, state.maxZoom);
    const KoZoomState differentMaxZoom(state.mode, state.zoom, state.minZoom, 96.0);

    for (const KoZoomState &other : {differentMode, differentZoom, differentMinZoom, differentMaxZoom}) {
        QVERIFY(!(state == other));
        QVERIFY(state != other);
    }
}

QTEST_GUILESS_MAIN(KoZoomStateContractTest)

#include "KoZoomStateContractTest.moc"
