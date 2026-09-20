/*
 *  SPDX-FileCopyrightText: 2020 Agata Cacko <cacko.azh@gmail.com>
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "TestKoZoomAction.h"

#include <KoZoomActionState.h>

#include <KLocalizedString>

#include <QDebug>
#include <QLocale>
#include <QTest>

#include <tuple>
#include <type_traits>

namespace
{
struct ZoomModeCollaboratorState {
    QVector<qreal> levels;
    QVector<QPair<qreal, qreal>> rangeRequests;
};

ZoomModeCollaboratorState zoomModeCollaboratorState;
QLocale previousLocale;
QByteArray previousTranslationDomain;

void compareState(const KoZoomState &actual, const KoZoomState &expected)
{
    QCOMPARE(static_cast<int>(actual.mode), static_cast<int>(expected.mode));
    QCOMPARE(actual.zoom, expected.zoom);
    QCOMPARE(actual.minZoom, expected.minZoom);
    QCOMPARE(actual.maxZoom, expected.maxZoom);
}

void compareItems(const QVector<KoZoomActionState::ZoomItem> &actual,
                  const QVector<KoZoomActionState::ZoomItem> &expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (qsizetype i = 0; i < actual.size(); ++i) {
        QCOMPARE(static_cast<int>(std::get<0>(actual[i])), static_cast<int>(std::get<0>(expected[i])));
        QCOMPARE(std::get<1>(actual[i]), std::get<1>(expected[i]));
        QCOMPARE(std::get<2>(actual[i]), std::get<2>(expected[i]));
    }
}

KoZoomActionState::ZoomItem item(KoZoomMode::Mode mode, qreal zoom, const QString &text)
{
    return {mode, zoom, text};
}
} // namespace

QString KoZoomMode::toString(Mode mode)
{
    switch (mode) {
    case ZOOM_PAGE:
        return QStringLiteral("全体表示🌐");
    case ZOOM_WIDTH:
        return QStringLiteral("幅表示↔");
    case ZOOM_HEIGHT:
        return QStringLiteral("高さ表示↕");
    case ZOOM_CONSTANT:
        return QStringLiteral("固定倍率");
    }
    return {};
}

QVector<qreal> KoZoomMode::generateStandardZoomLevels(qreal minZoom, qreal maxZoom)
{
    zoomModeCollaboratorState.rangeRequests.append(qMakePair(minZoom, maxZoom));
    return zoomModeCollaboratorState.levels;
}

QDebug operator<<(QDebug dbg, const KoZoomMode::Mode &mode)
{
    dbg.nospace() << "KoZoomMode::Mode(";

    switch (mode) {
    case KoZoomMode::ZOOM_CONSTANT:
        dbg << "ZOOM_CONSTANT";
        break;
    case KoZoomMode::ZOOM_PAGE:
        dbg << "ZOOM_PAGE";
        break;
    case KoZoomMode::ZOOM_WIDTH:
        dbg << "ZOOM_WIDTH";
        break;
    case KoZoomMode::ZOOM_HEIGHT:
        dbg << "ZOOM_HEIGHT";
        break;
    }

    dbg << ")";
    return dbg.space();
}

void TestKoZoomAction::initTestCase()
{
    previousLocale = QLocale();
    previousTranslationDomain = KLocalizedString::applicationDomain();
    QLocale::setDefault(QLocale::c());
    KLocalizedString::setApplicationDomain("test-ko-zoom-action-state");
}

void TestKoZoomAction::cleanupTestCase()
{
    KLocalizedString::setApplicationDomain(previousTranslationDomain);
    QLocale::setDefault(previousLocale);
}

void TestKoZoomAction::testTypeAndConstruction()
{
    static_assert(std::is_same_v<KoZoomActionState::ZoomItem, std::tuple<KoZoomMode::Mode, qreal, QString>>);
    static_assert(std::is_same_v<decltype(KoZoomActionState::zoomState), KoZoomState>);
    static_assert(std::is_same_v<decltype(KoZoomActionState::standardLevels), QVector<qreal>>);
    static_assert(std::is_same_v<decltype(KoZoomActionState::guiLevels), QVector<KoZoomActionState::ZoomItem>>);
    static_assert(std::is_same_v<decltype(KoZoomActionState::realGuiLevels), QVector<KoZoomActionState::ZoomItem>>);
    static_assert(std::is_same_v<decltype(KoZoomActionState::currentRealLevelIndex), int>);
    static_assert(std::is_same_v<decltype(KoZoomActionState::currentRealLevelText), QString>);

    zoomModeCollaboratorState.levels = {0.1, 0.25, 0.5, 1.0, 4.0, 12.0};
    zoomModeCollaboratorState.rangeRequests.clear();

    const KoZoomState input(KoZoomMode::ZOOM_CONSTANT, 0.75, 0.1, 12.0);
    const KoZoomActionState state(input);
    const QVector<QPair<qreal, qreal>> expectedRangeRequests{qMakePair(0.1, 12.0)};

    compareState(state.zoomState, input);
    QCOMPARE(zoomModeCollaboratorState.rangeRequests, expectedRangeRequests);
    QCOMPARE(state.standardLevels, zoomModeCollaboratorState.levels);

    const QVector<KoZoomActionState::ZoomItem> expectedGuiLevels{
        item(KoZoomMode::ZOOM_PAGE, -1.0, QStringLiteral("全体表示🌐")),
        item(KoZoomMode::ZOOM_WIDTH, -1.0, QStringLiteral("幅表示↔")),
        item(KoZoomMode::ZOOM_HEIGHT, -1.0, QStringLiteral("高さ表示↕")),
        item(KoZoomMode::ZOOM_CONSTANT, 0.25, QStringLiteral("25.0%")),
        item(KoZoomMode::ZOOM_CONSTANT, 0.5, QStringLiteral("50.0%")),
        item(KoZoomMode::ZOOM_CONSTANT, 1.0, QStringLiteral("100.0%")),
        item(KoZoomMode::ZOOM_CONSTANT, 4.0, QStringLiteral("400.0%")),
    };
    compareItems(state.guiLevels, expectedGuiLevels);

    QVector<KoZoomActionState::ZoomItem> expectedRealLevels = expectedGuiLevels;
    expectedRealLevels.insert(5, item(KoZoomMode::ZOOM_CONSTANT, 0.75, QStringLiteral("75.0%")));
    compareItems(state.realGuiLevels, expectedRealLevels);
    QCOMPARE(state.currentRealLevelIndex, 5);
    QCOMPARE(state.currentRealLevelText, QStringLiteral("75.0%"));
}

void TestKoZoomAction::testStateTransitions()
{
    const QVector<qreal> originalLevels{0.1, 0.25, 0.5, 1.0, 4.0, 12.0};
    zoomModeCollaboratorState.levels = originalLevels;
    zoomModeCollaboratorState.rangeRequests.clear();

    KoZoomActionState state(KoZoomState(KoZoomMode::ZOOM_CONSTANT, 0.75, 0.1, 12.0));
    QCOMPARE(zoomModeCollaboratorState.rangeRequests.size(), 1);

    zoomModeCollaboratorState.levels = {99.0};
    const KoZoomState exactLevel(KoZoomMode::ZOOM_CONSTANT, 0.5, 0.1, 12.0);
    state.setZoomState(exactLevel);
    compareState(state.zoomState, exactLevel);
    QCOMPARE(zoomModeCollaboratorState.rangeRequests.size(), 1);
    QCOMPARE(state.standardLevels, originalLevels);
    compareItems(state.realGuiLevels, state.guiLevels);
    QCOMPARE(state.currentRealLevelIndex, 4);
    QCOMPARE(state.currentRealLevelText, QStringLiteral("50.0%"));

    state.setZoomState(KoZoomState(KoZoomMode::ZOOM_PAGE, 0.5, 0.1, 12.0));
    QCOMPARE(zoomModeCollaboratorState.rangeRequests.size(), 1);
    QCOMPARE(state.currentRealLevelIndex, 0);
    QCOMPARE(state.currentRealLevelText, QStringLiteral("全体表示🌐 (50.0%)"));

    state.setZoomState(KoZoomState(KoZoomMode::ZOOM_WIDTH, 0.5, 0.1, 12.0));
    QCOMPARE(state.currentRealLevelIndex, 1);
    QCOMPARE(state.currentRealLevelText, QStringLiteral("幅表示↔ (50.0%)"));

    state.setZoomState(KoZoomState(KoZoomMode::ZOOM_HEIGHT, 0.5, 0.1, 12.0));
    QCOMPARE(state.currentRealLevelIndex, 2);
    QCOMPARE(state.currentRealLevelText, QStringLiteral("高さ表示↕ (50.0%)"));

    const QVector<qreal> replacementLevels{0.2, 0.4, 0.8, 1.6};
    zoomModeCollaboratorState.levels = replacementLevels;
    const KoZoomState changedBounds(KoZoomMode::ZOOM_CONSTANT, 0.6, 0.2, 1.6);
    state.setZoomState(changedBounds);
    const QVector<QPair<qreal, qreal>> expectedRangeRequests{qMakePair(0.1, 12.0), qMakePair(0.2, 1.6)};

    compareState(state.zoomState, changedBounds);
    QCOMPARE(zoomModeCollaboratorState.rangeRequests, expectedRangeRequests);
    QCOMPARE(state.standardLevels, replacementLevels);
    QCOMPARE(state.currentRealLevelIndex, 5);
    QCOMPARE(state.currentRealLevelText, QStringLiteral("60.0%"));
    QCOMPARE(std::get<1>(state.realGuiLevels.at(5)), 0.6);
}

void TestKoZoomAction::testNearestStandardLevels()
{
    zoomModeCollaboratorState.levels = {0.1, 0.25, 0.5, 1.0, 4.0, 12.0};
    zoomModeCollaboratorState.rangeRequests.clear();

    const KoZoomActionState state(KoZoomState(KoZoomMode::ZOOM_CONSTANT, 0.75, 0.1, 12.0));

    QCOMPARE(state.calcNearestStandardLevel(0.01), 0);
    QCOMPARE(state.calcNearestStandardLevel(0.1), 0);
    QCOMPARE(state.calcNearestStandardLevel(0.250001), 1);
    QCOMPARE(state.calcNearestStandardLevel(0.3), 2);
    QCOMPARE(state.calcNearestStandardLevel(0.75), 3);
    QCOMPARE(state.calcNearestStandardLevel(99.0), 5);
    QCOMPARE(state.calcNearestStandardLevel(), 3);
}

void TestKoZoomAction::testDebugOutput()
{
    const KoZoomActionState::ZoomItem zoomItem{
        KoZoomMode::ZOOM_WIDTH,
        1.25,
        QStringLiteral("倍率🌐"),
    };

    QString output;
    {
        QDebug debug(&output);
        debug.nospace() << zoomItem;
    }

    QCOMPARE(output.simplified(),
             QStringLiteral("KoZoomActionState::ZoomItem(KoZoomMode::Mode(ZOOM_WIDTH) , 1.25 , \"倍率🌐\" )"));
}

QTEST_GUILESS_MAIN(TestKoZoomAction)
