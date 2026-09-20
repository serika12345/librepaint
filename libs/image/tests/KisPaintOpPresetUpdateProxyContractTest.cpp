/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/KisPaintOpPresetUpdateProxy.h"

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

class DestructionProbe final : public KisPaintOpPresetUpdateProxy
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

class KisPaintOpPresetUpdateProxyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void normalAndRepeatedNotificationsPreserveOrder();
    void uniformNotificationIsImmediate();
    void nestedPostponeSuppressesAndAggregatesNotifications();
    void balancedPostponeWithoutNotificationIsSilent();
    void unmatchedUnpostponeLeavesSubsequentNotificationsBlocked();
    void qObjectOwnershipDestroysDerivedProxy();
};

void KisPaintOpPresetUpdateProxyContractTest::normalAndRepeatedNotificationsPreserveOrder()
{
    KisPaintOpPresetUpdateProxy proxy;
    QStringList events;

    connect(&proxy, &KisPaintOpPresetUpdateProxy::sigSettingsChangedUncompressedEarlyWarning, [&events] {
        events.append(QStringLiteral("early"));
    });
    connect(&proxy, &KisPaintOpPresetUpdateProxy::sigSettingsChangedUncompressed, [&events] {
        events.append(QStringLiteral("uncompressed"));
    });
    connect(&proxy, &KisPaintOpPresetUpdateProxy::sigSettingsChanged, [&events] {
        events.append(QStringLiteral("compressed"));
    });

    proxy.notifySettingsChanged();
    QCOMPARE(events,
             QStringList({QStringLiteral("early"), QStringLiteral("uncompressed"), QStringLiteral("compressed")}));

    proxy.notifySettingsChanged();
    QCOMPARE(events,
             QStringList({QStringLiteral("early"),
                          QStringLiteral("uncompressed"),
                          QStringLiteral("compressed"),
                          QStringLiteral("early"),
                          QStringLiteral("uncompressed")}));

    QTRY_COMPARE_WITH_TIMEOUT(events.size(), 6, 500);
    QCOMPARE(events.last(), QStringLiteral("compressed"));
}

void KisPaintOpPresetUpdateProxyContractTest::uniformNotificationIsImmediate()
{
    KisPaintOpPresetUpdateProxy proxy;
    QSignalSpy spy(&proxy, &KisPaintOpPresetUpdateProxy::sigUniformPropertiesChanged);

    proxy.notifyUniformPropertiesChanged();

    QCOMPARE(spy.count(), 1);
}

void KisPaintOpPresetUpdateProxyContractTest::nestedPostponeSuppressesAndAggregatesNotifications()
{
    KisPaintOpPresetUpdateProxy proxy;
    QStringList events;

    connect(&proxy, &KisPaintOpPresetUpdateProxy::sigSettingsChangedUncompressedEarlyWarning, [&events] {
        events.append(QStringLiteral("early"));
    });
    connect(&proxy, &KisPaintOpPresetUpdateProxy::sigSettingsChangedUncompressed, [&events] {
        events.append(QStringLiteral("uncompressed"));
    });
    connect(&proxy, &KisPaintOpPresetUpdateProxy::sigSettingsChanged, [&events] {
        events.append(QStringLiteral("compressed"));
    });

    proxy.postponeSettingsChanges();
    proxy.postponeSettingsChanges();
    proxy.notifySettingsChanged();
    proxy.notifySettingsChanged();
    QVERIFY(events.isEmpty());

    proxy.unpostponeSettingsChanges();
    QVERIFY(events.isEmpty());

    proxy.unpostponeSettingsChanges();
    QCOMPARE(events,
             QStringList({QStringLiteral("early"), QStringLiteral("uncompressed"), QStringLiteral("compressed")}));
}

void KisPaintOpPresetUpdateProxyContractTest::balancedPostponeWithoutNotificationIsSilent()
{
    KisPaintOpPresetUpdateProxy proxy;
    QSignalSpy compressedSpy(&proxy, &KisPaintOpPresetUpdateProxy::sigSettingsChanged);
    QSignalSpy uncompressedSpy(&proxy, &KisPaintOpPresetUpdateProxy::sigSettingsChangedUncompressed);

    proxy.postponeSettingsChanges();
    proxy.unpostponeSettingsChanges();

    QCOMPARE(compressedSpy.count(), 0);
    QCOMPARE(uncompressedSpy.count(), 0);
}

void KisPaintOpPresetUpdateProxyContractTest::unmatchedUnpostponeLeavesSubsequentNotificationsBlocked()
{
    KisPaintOpPresetUpdateProxy proxy;
    QSignalSpy compressedSpy(&proxy, &KisPaintOpPresetUpdateProxy::sigSettingsChanged);
    QSignalSpy uncompressedSpy(&proxy, &KisPaintOpPresetUpdateProxy::sigSettingsChangedUncompressed);
    QSignalSpy earlyWarningSpy(&proxy, &KisPaintOpPresetUpdateProxy::sigSettingsChangedUncompressedEarlyWarning);

    proxy.unpostponeSettingsChanges();
    proxy.notifySettingsChanged();
    QTest::qWait(150);

    QCOMPARE(compressedSpy.count(), 0);
    QCOMPARE(uncompressedSpy.count(), 0);
    QCOMPARE(earlyWarningSpy.count(), 0);
}

void KisPaintOpPresetUpdateProxyContractTest::qObjectOwnershipDestroysDerivedProxy()
{
    int destructions = 0;
    std::unique_ptr<QObject> proxy = std::make_unique<DestructionProbe>(&destructions);

    proxy.reset();

    QCOMPARE(destructions, 1);
}

QTEST_GUILESS_MAIN(KisPaintOpPresetUpdateProxyContractTest)

#include "KisPaintOpPresetUpdateProxyContractTest.moc"
