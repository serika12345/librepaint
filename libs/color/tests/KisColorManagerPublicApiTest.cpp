/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <kis_color_manager.h>

class KisColorManagerPublicApiTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void deviceTypeValuesAreStable();
    void singletonAndDirectConstruction();
    void missingDeviceHasNoNameOrProfile();
    void deviceEnumerationUsesRequestedType();
};

void KisColorManagerPublicApiTest::deviceTypeValuesAreStable()
{
    QCOMPARE(static_cast<int>(KisColorManager::screen), 0);
    QCOMPARE(static_cast<int>(KisColorManager::printer), 1);
    QCOMPARE(static_cast<int>(KisColorManager::camera), 2);
    QCOMPARE(static_cast<int>(KisColorManager::scanner), 3);
}

void KisColorManagerPublicApiTest::singletonAndDirectConstruction()
{
    KisColorManager manager;

    QCOMPARE(manager.parent(), nullptr);
    QVERIFY(KisColorManager::instance());
    QCOMPARE(KisColorManager::instance(), KisColorManager::instance());
}

void KisColorManagerPublicApiTest::missingDeviceHasNoNameOrProfile()
{
    KisColorManager manager;
    const QString missingDevice = QStringLiteral("librepaint-missing-device");

    QVERIFY(manager.deviceName(missingDevice).isEmpty());
    QVERIFY(manager.displayProfile(missingDevice).isEmpty());
    QVERIFY(manager.displayProfile(missingDevice, 1).isEmpty());
}

void KisColorManagerPublicApiTest::deviceEnumerationUsesRequestedType()
{
    KisColorManager manager;
    const QList<KisColorManager::DeviceType> deviceTypes = {
        KisColorManager::screen,
        KisColorManager::printer,
        KisColorManager::camera,
        KisColorManager::scanner,
    };
    QStringList allDevices;

    QCOMPARE(manager.devices(), manager.devices(KisColorManager::screen));
    for (KisColorManager::DeviceType type : deviceTypes) {
        const QStringList devices = manager.devices(type);
        QVERIFY(!devices.contains(QString()));
        for (const QString &device : devices) {
            QVERIFY(!allDevices.contains(device));
            allDevices.append(device);
        }
#ifdef KRITA_COLOR_MANAGER_USES_DUMMY_BACKEND
        QVERIFY(devices.isEmpty());
#endif
    }
}

QTEST_GUILESS_MAIN(KisColorManagerPublicApiTest)

#include "KisColorManagerPublicApiTest.moc"
