/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QTextStream>

#include <utility>

class FakeColorManager : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.ColorManager")

public Q_SLOTS:
    QList<QDBusObjectPath> GetDevices() const
    {
        return {QDBusObjectPath(QStringLiteral("/org/freedesktop/ColorManager/devices/display"))};
    }
};

class FakeColorDevice : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.ColorManager.Device")
    Q_PROPERTY(QString Model READ model CONSTANT)
    Q_PROPERTY(QString Vendor READ vendor CONSTANT)
    Q_PROPERTY(QString Colorspace READ colorspace CONSTANT)
    Q_PROPERTY(QString Kind READ kind CONSTANT)
    Q_PROPERTY(QString DeviceId READ deviceId CONSTANT)
    Q_PROPERTY(QList<QDBusObjectPath> Profiles READ profiles CONSTANT)

public:
    QString model() const
    {
        return QStringLiteral("LibrePaint Display");
    }

    QString vendor() const
    {
        return QStringLiteral("LibrePaint");
    }

    QString colorspace() const
    {
        return QStringLiteral("rgb");
    }

    QString kind() const
    {
        return QStringLiteral("display");
    }

    QString deviceId() const
    {
        return QStringLiteral("librepaint-display");
    }

    QList<QDBusObjectPath> profiles() const
    {
        return {
            QDBusObjectPath(QStringLiteral("/org/freedesktop/ColorManager/profiles/first")),
            QDBusObjectPath(QStringLiteral("/org/freedesktop/ColorManager/profiles/second")),
        };
    }
};

class FakeColorProfile : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.ColorManager.Profile")
    Q_PROPERTY(QString Kind READ kind CONSTANT)
    Q_PROPERTY(QString Filename READ filename CONSTANT)
    Q_PROPERTY(QString Title READ title CONSTANT)
    Q_PROPERTY(qlonglong Created READ created CONSTANT)
    Q_PROPERTY(QString Colorspace READ colorspace CONSTANT)

public:
    FakeColorProfile(QString filename, QString title)
        : m_filename(std::move(filename))
        , m_title(std::move(title))
    {
    }

    QString kind() const
    {
        return QStringLiteral("display-device");
    }

    QString filename() const
    {
        return m_filename;
    }

    QString title() const
    {
        return m_title;
    }

    qlonglong created() const
    {
        return 1;
    }

    QString colorspace() const
    {
        return QStringLiteral("rgb");
    }

private:
    const QString m_filename;
    const QString m_title;
};

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    if (application.arguments().size() != 3) {
        return 2;
    }

    FakeColorManager manager;
    FakeColorDevice device;
    FakeColorProfile firstProfile(application.arguments().at(1), QStringLiteral("First profile"));
    FakeColorProfile secondProfile(application.arguments().at(2), QStringLiteral("Second profile"));
    QDBusConnection bus = QDBusConnection::systemBus();
    const auto exportFlags = QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties;

    if (!bus.isConnected() || !bus.registerService(QStringLiteral("org.freedesktop.ColorManager"))
        || !bus.registerObject(QStringLiteral("/org/freedesktop/ColorManager"), &manager, exportFlags)
        || !bus.registerObject(QStringLiteral("/org/freedesktop/ColorManager/devices/display"), &device, exportFlags)
        || !bus.registerObject(QStringLiteral("/org/freedesktop/ColorManager/profiles/first"),
                               &firstProfile,
                               exportFlags)
        || !bus.registerObject(QStringLiteral("/org/freedesktop/ColorManager/profiles/second"),
                               &secondProfile,
                               exportFlags)) {
        return 3;
    }

    QTextStream(stdout) << "READY\n" << Qt::flush;
    return application.exec();
}

#include "FakeColordService.moc"
