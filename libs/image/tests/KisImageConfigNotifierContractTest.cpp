/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisImageConfigNotifier.h"

#include <QSignalSpy>
#include <QTest>

#include <memory>

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected recoverable assertion %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

namespace
{

class DestructionProbe final : public KisImageConfigNotifier
{
public:
    explicit DestructionProbe(int *destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~DestructionProbe() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

} // namespace

class KisImageConfigNotifierContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void singletonReturnsTheSameNotifier();
    void generalNotificationEmitsAndCompressesConfigAndAutoKeySignals();
    void autoKeyNotificationIsDedicatedAndCompressed();
    void globalProofingNotificationIsSynchronousAndUncompressed();
    void generalNotificationDoesNotEmitGlobalProofingSignal();
    void qObjectOwnershipDestroysDerivedNotifier();
};

void KisImageConfigNotifierContractTest::singletonReturnsTheSameNotifier()
{
    KisImageConfigNotifier *const first = KisImageConfigNotifier::instance();
    KisImageConfigNotifier *const second = KisImageConfigNotifier::instance();

    QVERIFY(first);
    QCOMPARE(second, first);
}

void KisImageConfigNotifierContractTest::generalNotificationEmitsAndCompressesConfigAndAutoKeySignals()
{
    KisImageConfigNotifier notifier;
    QSignalSpy configSpy(&notifier, &KisImageConfigNotifier::configChanged);
    QSignalSpy autoKeySpy(&notifier, &KisImageConfigNotifier::autoKeyFrameConfigurationChanged);

    notifier.notifyConfigChanged();
    QCOMPARE(configSpy.count(), 1);
    QCOMPARE(autoKeySpy.count(), 1);

    notifier.notifyConfigChanged();
    QCOMPARE(configSpy.count(), 1);
    QCOMPARE(autoKeySpy.count(), 1);

    QTRY_COMPARE_WITH_TIMEOUT(configSpy.count(), 2, 800);
    QTRY_COMPARE_WITH_TIMEOUT(autoKeySpy.count(), 2, 800);
}

void KisImageConfigNotifierContractTest::autoKeyNotificationIsDedicatedAndCompressed()
{
    KisImageConfigNotifier notifier;
    QSignalSpy configSpy(&notifier, &KisImageConfigNotifier::configChanged);
    QSignalSpy autoKeySpy(&notifier, &KisImageConfigNotifier::autoKeyFrameConfigurationChanged);
    QSignalSpy globalProofingSpy(&notifier, &KisImageConfigNotifier::globalProofingConfigChanged);

    notifier.notifyAutoKeyFrameConfigurationChanged();
    QCOMPARE(autoKeySpy.count(), 1);
    QCOMPARE(configSpy.count(), 0);
    QCOMPARE(globalProofingSpy.count(), 0);

    notifier.notifyAutoKeyFrameConfigurationChanged();
    QCOMPARE(autoKeySpy.count(), 1);
    QTRY_COMPARE_WITH_TIMEOUT(autoKeySpy.count(), 2, 800);
    QCOMPARE(configSpy.count(), 0);
    QCOMPARE(globalProofingSpy.count(), 0);
}

void KisImageConfigNotifierContractTest::globalProofingNotificationIsSynchronousAndUncompressed()
{
    KisImageConfigNotifier notifier;
    QSignalSpy configSpy(&notifier, &KisImageConfigNotifier::configChanged);
    QSignalSpy autoKeySpy(&notifier, &KisImageConfigNotifier::autoKeyFrameConfigurationChanged);
    QSignalSpy globalProofingSpy(&notifier, &KisImageConfigNotifier::globalProofingConfigChanged);

    notifier.notifyGlobalProofingConfigChanged();
    QCOMPARE(globalProofingSpy.count(), 1);
    notifier.notifyGlobalProofingConfigChanged();
    QCOMPARE(globalProofingSpy.count(), 2);
    QCOMPARE(configSpy.count(), 0);
    QCOMPARE(autoKeySpy.count(), 0);
}

void KisImageConfigNotifierContractTest::generalNotificationDoesNotEmitGlobalProofingSignal()
{
    KisImageConfigNotifier notifier;
    QSignalSpy globalProofingSpy(&notifier, &KisImageConfigNotifier::globalProofingConfigChanged);

    notifier.notifyConfigChanged();

    QCOMPARE(globalProofingSpy.count(), 0);
}

void KisImageConfigNotifierContractTest::qObjectOwnershipDestroysDerivedNotifier()
{
    int destructions = 0;
    std::unique_ptr<QObject> notifier = std::make_unique<DestructionProbe>(&destructions);

    notifier.reset();

    QCOMPARE(destructions, 1);
}

QTEST_GUILESS_MAIN(KisImageConfigNotifierContractTest)

#include "KisImageConfigNotifierContractTest.moc"
