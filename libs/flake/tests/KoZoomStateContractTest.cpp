/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoZoomState.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QDebug>
#include <QStandardPaths>
#include <QTest>

#include <cmath>

namespace
{
int safeAssertCount = 0;

class ZoomStepsOverride
{
public:
    explicit ZoomStepsOverride(int steps)
        : m_group(KSharedConfig::openConfig()->group(QString()))
        , m_hadPreviousValue(m_group.hasKey("zoomSteps"))
        , m_previousValue(m_group.readEntry("zoomSteps", 2))
    {
        m_group.writeEntry("zoomSteps", steps);
    }

    ~ZoomStepsOverride()
    {
        if (m_hadPreviousValue) {
            m_group.writeEntry("zoomSteps", m_previousValue);
        } else {
            m_group.deleteEntry("zoomSteps");
        }
    }

private:
    KConfigGroup m_group;
    bool m_hadPreviousValue;
    int m_previousValue;
};

QString debugString(KoZoomMode::Mode mode)
{
    QString output;
    {
        QDebug debug(&output);
        debug.nospace() << mode;
    }
    return output;
}
} // namespace

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    ++safeAssertCount;
}

class KoZoomStateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void defaultStateKeepsCanonicalRange();
    void constructionPreservesAllStateValues();
    void fuzzyEqualityAcceptsEquivalentPositiveValues();
    void comparisonRejectsEachDifferentStateValue();
    void modeValuesAndFlagsRemainStable();
    void supportedModeNamesUseCurrentTranslationKeys();
    void constantModeReportsUnsupportedPlaceholder();
    void standardLevelsKeepHalfStepsAndThirdSnapping();
    void adjacentZoomSearchMovesStrictlyAndKeepsOuterValues();
    void debugOutputNamesKnownAndUnknownModes();
};

void KoZoomStateContractTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

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

void KoZoomStateContractTest::modeValuesAndFlagsRemainStable()
{
    QCOMPARE(int(KoZoomMode::ZOOM_CONSTANT), 0);
    QCOMPARE(int(KoZoomMode::ZOOM_PAGE), 1);
    QCOMPARE(int(KoZoomMode::ZOOM_WIDTH), 2);
    QCOMPARE(int(KoZoomMode::ZOOM_HEIGHT), 16);

    const KoZoomMode::Modes modes = KoZoomMode::ZOOM_PAGE | KoZoomMode::ZOOM_WIDTH | KoZoomMode::ZOOM_HEIGHT;
    QCOMPARE(int(modes), 19);
    QVERIFY(modes.testFlag(KoZoomMode::ZOOM_PAGE));
    QVERIFY(modes.testFlag(KoZoomMode::ZOOM_WIDTH));
    QVERIFY(modes.testFlag(KoZoomMode::ZOOM_HEIGHT));
}

void KoZoomStateContractTest::supportedModeNamesUseCurrentTranslationKeys()
{
    QCOMPARE(KoZoomMode::toString(KoZoomMode::ZOOM_PAGE), i18n("Fit View"));
    QCOMPARE(KoZoomMode::toString(KoZoomMode::ZOOM_WIDTH), i18n("Fit Width"));
    QCOMPARE(KoZoomMode::toString(KoZoomMode::ZOOM_HEIGHT), i18n("Fit Height"));
}

void KoZoomStateContractTest::constantModeReportsUnsupportedPlaceholder()
{
    safeAssertCount = 0;

    QCOMPARE(KoZoomMode::toString(KoZoomMode::ZOOM_CONSTANT), QStringLiteral("XXX"));
    QCOMPARE(safeAssertCount, 1);
}

void KoZoomStateContractTest::standardLevelsKeepHalfStepsAndThirdSnapping()
{
    const ZoomStepsOverride zoomSteps(2);
    const QVector<qreal> levels = KoZoomMode::generateStandardZoomLevels(0.25, 4.0);
    const QVector<qreal> expected{
        0.25,
        1.0 / 3.0,
        0.5,
        2.0 / 3.0,
        1.0,
        4.0 / 3.0,
        2.0,
        8.0 / 3.0,
        4.0,
    };

    QCOMPARE(levels.size(), expected.size());
    for (qsizetype i = 0; i < levels.size(); ++i) {
        QVERIFY(qAbs(levels[i] - expected[i]) < 1e-12);
    }
}

void KoZoomStateContractTest::adjacentZoomSearchMovesStrictlyAndKeepsOuterValues()
{
    const QVector<qreal> levels{0.5, 1.0, 2.0};

    QCOMPARE(KoZoomMode::findNextZoom(0.25, levels), 0.5);
    QCOMPARE(KoZoomMode::findNextZoom(1.0, levels), 2.0);
    QCOMPARE(KoZoomMode::findNextZoom(2.0, levels), 2.0);
    QCOMPARE(KoZoomMode::findNextZoom(3.0, levels), 3.0);

    QCOMPARE(KoZoomMode::findPrevZoom(0.25, levels), 0.25);
    QCOMPARE(KoZoomMode::findPrevZoom(0.5, levels), 0.5);
    QCOMPARE(KoZoomMode::findPrevZoom(1.0, levels), 0.5);
    QCOMPARE(KoZoomMode::findPrevZoom(3.0, levels), 2.0);
}

void KoZoomStateContractTest::debugOutputNamesKnownAndUnknownModes()
{
    QCOMPARE(debugString(KoZoomMode::ZOOM_CONSTANT), QStringLiteral("KoZoomMode::Mode(ZOOM_CONSTANT) "));
    QCOMPARE(debugString(KoZoomMode::ZOOM_PAGE), QStringLiteral("KoZoomMode::Mode(ZOOM_PAGE) "));
    QCOMPARE(debugString(KoZoomMode::ZOOM_WIDTH), QStringLiteral("KoZoomMode::Mode(ZOOM_WIDTH) "));
    QCOMPARE(debugString(KoZoomMode::ZOOM_HEIGHT), QStringLiteral("KoZoomMode::Mode(ZOOM_HEIGHT) "));
    QCOMPARE(debugString(static_cast<KoZoomMode::Mode>(99)), QStringLiteral("KoZoomMode::Mode(UNKNOWN) "));
}

QTEST_GUILESS_MAIN(KoZoomStateContractTest)

#include "KoZoomStateContractTest.moc"
