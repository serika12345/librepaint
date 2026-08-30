/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoDockRegistry.h"
#include "KoPluginLoader.h"

#include <QByteArray>
#include <QTest>

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion '%s' at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion '%s' at %s:%d", assertion, file, line);
}

namespace
{
struct PluginLoadRecord {
    int callCount = 0;
    QString serviceType;
    QByteArray group;
    QByteArray blacklist;
    QObject *owner = nullptr;
    bool cache = false;
};

PluginLoadRecord &pluginLoadRecord()
{
    static PluginLoadRecord record;
    return record;
}

class DockFactoryProbe : public KoDockFactoryBase
{
public:
    explicit DockFactoryProbe(int *destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~DockFactoryProbe() override
    {
        ++*m_destructionCount;
    }

    QString id() const override
    {
        return QStringLiteral("owned-dock");
    }

    DockPosition defaultDockPosition() const override
    {
        return DockLeft;
    }

    QDockWidget *createDockWidget() override
    {
        return nullptr;
    }

private:
    int *m_destructionCount;
};
} // namespace

KoPluginLoader::KoPluginLoader()
    : d(nullptr)
{
}

KoPluginLoader::~KoPluginLoader() = default;

KoPluginLoader *KoPluginLoader::instance()
{
    static KoPluginLoader loader;
    return &loader;
}

void KoPluginLoader::load(const QString &serviceType, const PluginsConfig &config, QObject *owner, bool cache)
{
    PluginLoadRecord &record = pluginLoadRecord();
    ++record.callCount;
    record.serviceType = serviceType;
    record.group = config.group ? QByteArray(config.group) : QByteArray();
    record.blacklist = config.blacklist ? QByteArray(config.blacklist) : QByteArray();
    record.owner = owner;
    record.cache = cache;
}

class KoDockRegistryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ownsRegisteredFactoriesUntilDestruction();
    void initializesDockPluginsOnceForStableSingleton();
};

void KoDockRegistryContractTest::ownsRegisteredFactoriesUntilDestruction()
{
    int destructionCount = 0;

    {
        KoDockRegistry registry;
        QCOMPARE(registry.count(), 0);

        auto *factory = new DockFactoryProbe(&destructionCount);
        registry.add(factory);

        QCOMPARE(registry.count(), 1);
        QCOMPARE(registry.value(QStringLiteral("owned-dock")), factory);
        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

void KoDockRegistryContractTest::initializesDockPluginsOnceForStableSingleton()
{
    PluginLoadRecord &record = pluginLoadRecord();
    record = PluginLoadRecord();

    KoDockRegistry *first = KoDockRegistry::instance();
    KoDockRegistry *second = KoDockRegistry::instance();

    QVERIFY(first);
    QCOMPARE(second, first);
    QCOMPARE(record.callCount, 1);
    QCOMPARE(record.serviceType, QStringLiteral("Krita/Dock"));
    QCOMPARE(record.group, QByteArrayLiteral("krita"));
    QCOMPARE(record.blacklist, QByteArrayLiteral("DockerPluginsDisabled"));
    QCOMPARE(record.owner, nullptr);
    QVERIFY(record.cache);
}

QTEST_GUILESS_MAIN(KoDockRegistryContractTest)

#include "KoDockRegistryContractTest.moc"
