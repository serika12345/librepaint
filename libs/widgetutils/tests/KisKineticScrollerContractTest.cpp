/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisKineticScroller.h>

#include <KConfigGroup>
#include <KSharedConfig>

#include <QListView>
#include <QStandardPaths>
#include <QTest>

namespace
{
const QStringList kineticScrollingKeys {
    QStringLiteral("KineticScrollingSensitivity"),
    QStringLiteral("KineticScrollingEnabled"),
    QStringLiteral("KineticScrollingHideScrollbar"),
    QStringLiteral("KineticScrollingResistanceCoefficient"),
    QStringLiteral("KineticScrollingDragVelocitySmoothingFactor"),
    QStringLiteral("KineticScrollingMinimumVelocity"),
    QStringLiteral("KineticScrollingAxisLockThreshold"),
    QStringLiteral("KineticScrollingMaxClickThroughVelocity"),
    QStringLiteral("KineticScrollingFlickAccelerationFactor"),
    QStringLiteral("KineticScrollingOvershotDragResistanceFactor"),
    QStringLiteral("KineticScrollingOvershootDragDistanceFactor"),
    QStringLiteral("KineticScrollingOvershootScrollDistanceFactor"),
    QStringLiteral("KineticScrollingOvershootScrollTime"),
    QStringLiteral("KineticScrollingMousePressDelay"),
    QStringLiteral("KineticScrollingGesture")
};

KConfigGroup kineticScrollingConfig()
{
    return KSharedConfig::openConfig()->group(QString());
}

void clearKineticScrollingConfig()
{
    KConfigGroup config = kineticScrollingConfig();
    for (const QString &key : kineticScrollingKeys) {
        config.deleteEntry(key);
    }
    config.sync();
}
}

class KisKineticScrollerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanup();
    void cleanupTestCase();
    void configuredGestureFollowsSetting_data();
    void configuredGestureFollowsSetting();
    void disabledConfigurationReturnsNoScroller();
    void enabledConfigurationUsesPixelScrolling();
    void cursorTracksScrollerState();
};

void KisKineticScrollerContractTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    clearKineticScrollingConfig();
}

void KisKineticScrollerContractTest::cleanup()
{
    clearKineticScrollingConfig();
}

void KisKineticScrollerContractTest::cleanupTestCase()
{
    clearKineticScrollingConfig();
}

void KisKineticScrollerContractTest::configuredGestureFollowsSetting_data()
{
    QTest::addColumn<int>("setting");
    QTest::addColumn<QScroller::ScrollerGestureType>("expectedGesture");

    QTest::newRow("touch") << 0 << QScroller::TouchGesture;
    QTest::newRow("left mouse") << 1 << QScroller::LeftMouseButtonGesture;
    QTest::newRow("middle mouse") << 2 << QScroller::MiddleMouseButtonGesture;
    QTest::newRow("right mouse") << 3 << QScroller::RightMouseButtonGesture;
    QTest::newRow("unknown") << 99 << QScroller::MiddleMouseButtonGesture;
}

void KisKineticScrollerContractTest::configuredGestureFollowsSetting()
{
    QFETCH(int, setting);
    QFETCH(QScroller::ScrollerGestureType, expectedGesture);
    KConfigGroup config = kineticScrollingConfig();
    config.writeEntry("KineticScrollingGesture", setting);
    config.sync();

    QCOMPARE(KisKineticScroller::getConfiguredGestureType(), expectedGesture);
}

void KisKineticScrollerContractTest::disabledConfigurationReturnsNoScroller()
{
    KConfigGroup config = kineticScrollingConfig();
    config.writeEntry("KineticScrollingEnabled", false);
    config.sync();
    QListView view;

    QVERIFY(!KisKineticScroller::createPreconfiguredScroller(&view));
}

void KisKineticScrollerContractTest::enabledConfigurationUsesPixelScrolling()
{
    KConfigGroup config = kineticScrollingConfig();
    config.writeEntry("KineticScrollingEnabled", true);
    config.writeEntry("KineticScrollingHideScrollbar", false);
    config.writeEntry("KineticScrollingGesture", 0);
    config.sync();
    QListView view;

    QScroller *scroller = KisKineticScroller::createPreconfiguredScroller(&view);

    QVERIFY(scroller);
    QCOMPARE(scroller, QScroller::scroller(&view));
    QCOMPARE(view.horizontalScrollMode(), QAbstractItemView::ScrollPerPixel);
    QCOMPARE(view.verticalScrollMode(), QAbstractItemView::ScrollPerPixel);
}

void KisKineticScrollerContractTest::cursorTracksScrollerState()
{
    QWidget source;

    KisKineticScroller::updateCursor(&source, QScroller::Pressed);
    QCOMPARE(source.cursor().shape(), Qt::OpenHandCursor);

    KisKineticScroller::updateCursor(&source, QScroller::Dragging);
    QCOMPARE(source.cursor().shape(), Qt::ClosedHandCursor);

    KisKineticScroller::updateCursor(&source, QScroller::Scrolling);
    QCOMPARE(source.cursor().shape(), Qt::ArrowCursor);

    KisKineticScroller::updateCursor(&source, QScroller::Inactive);
    QCOMPARE(source.cursor().shape(), Qt::ArrowCursor);
}

QTEST_MAIN(KisKineticScrollerContractTest)

#include "KisKineticScrollerContractTest.moc"
