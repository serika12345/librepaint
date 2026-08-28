/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_config_notifier.h"

#include <QSignalSpy>
#include <QTest>

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected recoverable assertion %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

class KisConfigNotifierContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void singletonAndLocalLifetimeAreStable();
    void immediateNotificationsEmitOnce();
    void valueNotificationsForwardArguments();
    void dropFramesNotificationUsesFirstActiveCompression();
};

void KisConfigNotifierContractTest::singletonAndLocalLifetimeAreStable()
{
    KisConfigNotifier *first = KisConfigNotifier::instance();
    QVERIFY(first);
    QCOMPARE(KisConfigNotifier::instance(), first);

    KisConfigNotifier localNotifier;
    QVERIFY(&localNotifier != first);
}

void KisConfigNotifierContractTest::immediateNotificationsEmitOnce()
{
    KisConfigNotifier notifier;
    QSignalSpy configSpy(&notifier, &KisConfigNotifier::configChanged);
    QSignalSpy pixelGridSpy(&notifier, &KisConfigNotifier::pixelGridModeChanged);
    QSignalSpy colorHistorySpy(&notifier, &KisConfigNotifier::colorHistoryModeChanged);
    QSignalSpy touchPaintingSpy(&notifier, &KisConfigNotifier::touchPaintingChanged);
    QSignalSpy colorSamplerSpy(&notifier, &KisConfigNotifier::sigColorSamplerPreviewStyleChanged);

    notifier.notifyConfigChanged();
    notifier.notifyPixelGridModeChanged();
    notifier.notifyColorHistoryModeChanged();
    notifier.notifyTouchPaintingChanged();
    notifier.notifyColorSamplerPreviewStyleChanged();

    QCOMPARE(configSpy.count(), 1);
    QCOMPARE(pixelGridSpy.count(), 1);
    QCOMPARE(colorHistorySpy.count(), 1);
    QCOMPARE(touchPaintingSpy.count(), 1);
    QCOMPARE(colorSamplerSpy.count(), 1);
}

void KisConfigNotifierContractTest::valueNotificationsForwardArguments()
{
    KisConfigNotifier notifier;
    QSignalSpy colorThemeSpy(&notifier, &KisConfigNotifier::signalColorThemeChanged);
    QSignalSpy longPressSpy(&notifier, &KisConfigNotifier::sigLongPressChanged);

    notifier.notifyColorThemeChanged(QStringLiteral("dark.theme"));
    notifier.notifyLongPressChanged(true);

    QCOMPARE(colorThemeSpy.count(), 1);
    QCOMPARE(colorThemeSpy.takeFirst().at(0).toString(), QStringLiteral("dark.theme"));
    QCOMPARE(longPressSpy.count(), 1);
    QCOMPARE(longPressSpy.takeFirst().at(0).toBool(), true);
}

void KisConfigNotifierContractTest::dropFramesNotificationUsesFirstActiveCompression()
{
    KisConfigNotifier notifier;
    QSignalSpy spy(&notifier, &KisConfigNotifier::dropFramesModeChanged);

    notifier.notifyDropFramesModeChanged();
    QCOMPARE(spy.count(), 1);

    notifier.notifyDropFramesModeChanged();
    QCOMPARE(spy.count(), 1);
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 600);
}

QTEST_GUILESS_MAIN(KisConfigNotifierContractTest)

#include "KisConfigNotifierContractTest.moc"
