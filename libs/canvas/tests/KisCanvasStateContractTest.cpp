/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisCanvasState.h>
#include <KoZoomState.h>

#include <QTest>

#include <array>
#include <cmath>
#include <limits>

namespace
{
KisCanvasState makeCanvasState()
{
    KisCanvasState state;
    state.zoom = 2.0;
    state.effectiveZoom = 1.75;
    state.zoomMode = KoZoomMode::ZOOM_WIDTH;
    state.rotation = 37.5;
    state.mirrorHorizontally = true;
    state.mirrorVertically = false;
    state.documentOffset = QPoint(12, -34);
    state.documentOffsetF = QPointF(12.25, -33.75);
    state.viewportOffsetF = QPointF(-4.5, 8.25);
    state.minimumOffset = QPoint(-50, -60);
    state.maximumOffset = QPoint(700, 800);
    state.canvasSize = QSizeF(1280.5, 720.25);
    state.minimumZoom = 0.125;
    state.maximumZoom = 64.0;
    state.imageRectInWidgetPixels = QRectF(3.5, 4.5, 640.25, 480.75);
    return state;
}
} // namespace

class KisCanvasStateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void canvasStateZoomValuesRemainStable();
    void canvasStateOrientationAndPositionValuesRemainStable();
    void canvasStateBoundsAndDimensionsRemainStable();
    void canvasStateEqualityRemainsStable();
    void canvasStateConversionSignatureAndZoomStateRemainStable();
};

void KisCanvasStateContractTest::canvasStateZoomValuesRemainStable()
{
    using State = KisCanvasState;


    State state = makeCanvasState();
    QCOMPARE(state.zoom, 2.0);
    QCOMPARE(state.effectiveZoom, 1.75);
    QCOMPARE(state.zoomMode, KoZoomMode::ZOOM_WIDTH);
    QCOMPARE(state.minimumZoom, 0.125);
    QCOMPARE(state.maximumZoom, 64.0);

    state.zoom = 3.0;
    state.effectiveZoom = 2.5;
    state.zoomMode = KoZoomMode::ZOOM_PAGE;
    state.minimumZoom = 0.25;
    state.maximumZoom = 96.0;
    QCOMPARE(state.zoom, 3.0);
    QCOMPARE(state.effectiveZoom, 2.5);
    QCOMPARE(state.zoomMode, KoZoomMode::ZOOM_PAGE);
    QCOMPARE(state.minimumZoom, 0.25);
    QCOMPARE(state.maximumZoom, 96.0);
}

void KisCanvasStateContractTest::canvasStateOrientationAndPositionValuesRemainStable()
{
    using State = KisCanvasState;


    State state = makeCanvasState();
    QCOMPARE(state.rotation, 37.5);
    QCOMPARE(state.mirrorHorizontally, true);
    QCOMPARE(state.mirrorVertically, false);
    QCOMPARE(state.documentOffset, QPoint(12, -34));
    QCOMPARE(state.documentOffsetF, QPointF(12.25, -33.75));
    QCOMPARE(state.viewportOffsetF, QPointF(-4.5, 8.25));

    state.rotation = 91.25;
    state.mirrorHorizontally = false;
    state.mirrorVertically = true;
    state.documentOffset = QPoint(-8, 16);
    state.documentOffsetF = QPointF(-8.5, 16.75);
    state.viewportOffsetF = QPointF(2.25, -6.5);
    QCOMPARE(state.rotation, 91.25);
    QCOMPARE(state.mirrorHorizontally, false);
    QCOMPARE(state.mirrorVertically, true);
    QCOMPARE(state.documentOffset, QPoint(-8, 16));
    QCOMPARE(state.documentOffsetF, QPointF(-8.5, 16.75));
    QCOMPARE(state.viewportOffsetF, QPointF(2.25, -6.5));
}

void KisCanvasStateContractTest::canvasStateBoundsAndDimensionsRemainStable()
{
    using State = KisCanvasState;


    State state = makeCanvasState();
    QCOMPARE(state.minimumOffset, QPoint(-50, -60));
    QCOMPARE(state.maximumOffset, QPoint(700, 800));
    QCOMPARE(state.canvasSize, QSizeF(1280.5, 720.25));
    QCOMPARE(state.imageRectInWidgetPixels, QRectF(3.5, 4.5, 640.25, 480.75));

    state.minimumOffset = QPoint(-70, -80);
    state.maximumOffset = QPoint(900, 1000);
    state.canvasSize = QSizeF(1920.75, 1080.5);
    state.imageRectInWidgetPixels = QRectF(7.25, 9.5, 800.5, 600.25);
    QCOMPARE(state.minimumOffset, QPoint(-70, -80));
    QCOMPARE(state.maximumOffset, QPoint(900, 1000));
    QCOMPARE(state.canvasSize, QSizeF(1920.75, 1080.5));
    QCOMPARE(state.imageRectInWidgetPixels, QRectF(7.25, 9.5, 800.5, 600.25));
}

void KisCanvasStateContractTest::canvasStateEqualityRemainsStable()
{
    using State = KisCanvasState;
    using Mutation = void (*)(State &);


    const State state = makeCanvasState();
    QVERIFY(state == makeCanvasState());

    const std::array<Mutation, 15> differentValueMutations{{
        [](State &other) {
            other.zoom = 2.25;
        },
        [](State &other) {
            other.effectiveZoom = 2.0;
        },
        [](State &other) {
            other.zoomMode = KoZoomMode::ZOOM_HEIGHT;
        },
        [](State &other) {
            other.rotation = 38.0;
        },
        [](State &other) {
            other.mirrorHorizontally = false;
        },
        [](State &other) {
            other.mirrorVertically = true;
        },
        [](State &other) {
            other.documentOffset = QPoint(13, -34);
        },
        [](State &other) {
            other.documentOffsetF = QPointF(12.5, -33.75);
        },
        [](State &other) {
            other.viewportOffsetF = QPointF(-4.25, 8.25);
        },
        [](State &other) {
            other.minimumOffset = QPoint(-51, -60);
        },
        [](State &other) {
            other.maximumOffset = QPoint(701, 800);
        },
        [](State &other) {
            other.canvasSize = QSizeF(1281.5, 720.25);
        },
        [](State &other) {
            other.minimumZoom = 0.25;
        },
        [](State &other) {
            other.maximumZoom = 65.0;
        },
        [](State &other) {
            other.imageRectInWidgetPixels = QRectF(3.75, 4.5, 640.25, 480.75);
        },
    }};

    for (const Mutation mutate : differentValueMutations) {
        State other = state;
        mutate(other);
        QVERIFY(!(state == other));
    }

    using QrealMember = qreal State::*;
    const std::array<QrealMember, 5> fuzzyMembers{{
        &State::zoom,
        &State::effectiveZoom,
        &State::rotation,
        &State::minimumZoom,
        &State::maximumZoom,
    }};

    for (const QrealMember member : fuzzyMembers) {
        State other = state;
        other.*member = std::nextafter(other.*member, std::numeric_limits<qreal>::infinity());
        QVERIFY(qFuzzyCompare(state.*member, other.*member));
        QVERIFY(state == other);
    }
}

void KisCanvasStateContractTest::canvasStateConversionSignatureAndZoomStateRemainStable()
{
    using State = KisCanvasState;


    const State state = makeCanvasState();
    const KoZoomState zoomState = state.zoomState();
    QCOMPARE(zoomState.mode, state.zoomMode);
    QCOMPARE(zoomState.zoom, state.zoom);
    QCOMPARE(zoomState.minZoom, state.minimumZoom);
    QCOMPARE(zoomState.maxZoom, state.maximumZoom);
}

QTEST_APPLESS_MAIN(KisCanvasStateContractTest)

#include "KisCanvasStateContractTest.moc"
