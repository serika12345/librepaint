/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <utility>

#include <KisColord.h>
#include <kis_color_manager.h>

class KisColordPublicApiTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void missingRecordsAreEmpty();
    void configuredRecordsReturnNamesAndProfiles();
    void changedSignalCarriesDeviceId();

private:
    QTemporaryDir m_temporaryDirectory;
    QProcess m_busDaemon;
    QProcess m_colordService;
    const QByteArray m_firstProfile = QByteArrayLiteral("first-profile");
    const QByteArray m_secondProfile = QByteArrayLiteral("second-profile");
};

void KisColordPublicApiTest::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    const QString firstProfilePath = m_temporaryDirectory.filePath(QStringLiteral("first.icc"));
    const QString secondProfilePath = m_temporaryDirectory.filePath(QStringLiteral("second.icc"));
    for (const auto &[path, contents] : {
             std::pair{firstProfilePath, m_firstProfile},
             std::pair{secondProfilePath, m_secondProfile},
         }) {
        QFile profile(path);
        QVERIFY(profile.open(QFile::WriteOnly));
        QCOMPARE(profile.write(contents), contents.size());
    }

    const QString dbusDaemon = QStandardPaths::findExecutable(QStringLiteral("dbus-daemon"));
    QVERIFY(!dbusDaemon.isEmpty());
    m_busDaemon.start(dbusDaemon,
                      {QStringLiteral("--session"),
                       QStringLiteral("--nofork"),
                       QStringLiteral("--nopidfile"),
                       QStringLiteral("--print-address=1")});
    QVERIFY(m_busDaemon.waitForStarted());
    QVERIFY(m_busDaemon.waitForReadyRead());
    const QByteArray busAddress = m_busDaemon.readLine().trimmed();
    QVERIFY(!busAddress.isEmpty());

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("DBUS_SYSTEM_BUS_ADDRESS"), QString::fromUtf8(busAddress));
    m_colordService.setProcessEnvironment(environment);
    m_colordService.start(QStringLiteral(FAKE_COLORD_SERVICE_PATH), {firstProfilePath, secondProfilePath});
    QVERIFY(m_colordService.waitForStarted());
    QVERIFY(m_colordService.waitForReadyRead());
    QCOMPARE(m_colordService.readLine().trimmed(), QByteArrayLiteral("READY"));

    QVERIFY(qputenv("DBUS_SYSTEM_BUS_ADDRESS", busAddress));
}

void KisColordPublicApiTest::cleanupTestCase()
{
    m_colordService.terminate();
    QVERIFY(m_colordService.waitForFinished());
    m_busDaemon.terminate();
    QVERIFY(m_busDaemon.waitForFinished());
    qunsetenv("DBUS_SYSTEM_BUS_ADDRESS");
}

void KisColordPublicApiTest::missingRecordsAreEmpty()
{
    QObject parent;
    KisColord colord(&parent);
    const QString missingDevice = QStringLiteral("librepaint-missing-device");

    QCOMPARE(colord.parent(), &parent);
    QVERIFY(colord.devices(QStringLiteral("librepaint-missing-type")).isEmpty());
    QVERIFY(colord.deviceName(missingDevice).isEmpty());
    QVERIFY(colord.deviceProfile(missingDevice, 0).isEmpty());
}

void KisColordPublicApiTest::configuredRecordsReturnNamesAndProfiles()
{
    KisColord colord;

    QTRY_COMPARE(colord.devices(QStringLiteral("display")), QStringList({QStringLiteral("librepaint-display")}));
    QCOMPARE(colord.devices(QStringLiteral("printer")), QStringList());
    QCOMPARE(colord.deviceName(QStringLiteral("librepaint-display")), QStringLiteral("LibrePaint Display, LibrePaint"));
    QCOMPARE(colord.deviceProfile(QStringLiteral("librepaint-display"), 0), m_firstProfile);
    QCOMPARE(colord.deviceProfile(QStringLiteral("librepaint-display"), 1), m_secondProfile);
    QCOMPARE(colord.deviceProfile(QStringLiteral("librepaint-display"), -1), m_firstProfile);
    QCOMPARE(colord.deviceProfile(QStringLiteral("librepaint-display"), 2), m_firstProfile);

    KisColorManager manager;
    QTRY_COMPARE(manager.devices(), QStringList({QStringLiteral("librepaint-display")}));
    QCOMPARE(manager.devices(KisColorManager::screen), QStringList({QStringLiteral("librepaint-display")}));
    QCOMPARE(manager.devices(KisColorManager::printer), QStringList());
    QCOMPARE(manager.devices(KisColorManager::camera), QStringList());
    QCOMPARE(manager.devices(KisColorManager::scanner), QStringList());
    QCOMPARE(manager.deviceName(QStringLiteral("librepaint-display")),
             QStringLiteral("LibrePaint Display, LibrePaint"));
    QCOMPARE(manager.displayProfile(QStringLiteral("librepaint-display"), 0), m_firstProfile);
    QCOMPARE(manager.displayProfile(QStringLiteral("librepaint-display"), 1), m_secondProfile);
    QCOMPARE(manager.displayProfile(QStringLiteral("librepaint-display"), 2), m_firstProfile);
}

void KisColordPublicApiTest::changedSignalCarriesDeviceId()
{
    KisColord colord;
    QSignalSpy spy(&colord, qOverload<const QString &>(&KisColord::changed));
    const QString device = QStringLiteral("display-device");

    QVERIFY(spy.isValid());
    QVERIFY(QMetaObject::invokeMethod(&colord, "changed", Qt::DirectConnection, Q_ARG(QString, device)));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toString(), device);
}

QTEST_GUILESS_MAIN(KisColordPublicApiTest)

#include "KisColordPublicApiTest.moc"
