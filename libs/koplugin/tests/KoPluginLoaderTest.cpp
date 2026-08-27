/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoPluginLoaderTest.h"

#include "KoJsonTrader.h"
#include "KoPluginLoader.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include <QJsonArray>
#include <QJsonObject>
#include <QMutex>
#include <QPluginLoader>
#include <QSharedPointer>
#include <QTest>

#include <DummyTrivialInterface.h>
#include <KPluginFactory>
#include <KPluginMetaData>

#include <kconfiggroup.h>
#include <ksharedconfig.h>

void KoPluginLoaderTest::initTestCase()
{
    const QByteArray pluginPath = qgetenv("KRITA_PLUGIN_PATH");
    QVERIFY2(!pluginPath.isEmpty(), "CTest must provide the test plugin directory");
    qDebug() << pluginPath;
}

void KoPluginLoaderTest::testPluginConfigValidity()
{
    KoPluginLoader::PluginsConfig config;

    QCOMPARE(config.group, nullptr);
    QCOMPARE(config.blacklist, nullptr);
    QVERIFY(!config.isValid());

    config.group = "dummy";
    QVERIFY(!config.isValid());

    config.blacklist = "dummyblacklist";
    QVERIFY(config.isValid());
}

void KoPluginLoaderTest::testTraderQueries()
{
    KoJsonTrader trader;

    QCOMPARE(KoJsonTrader::instance(), KoJsonTrader::instance());
    QCOMPARE(trader.query(QStringLiteral("Krita/DummyPlugin"), QString()).size(), 4);
    QVERIFY(trader.query(QStringLiteral("Krita/MissingPlugin"), QString()).isEmpty());
    QVERIFY(trader.query(QStringLiteral("Krita/DummyPlugin"), QStringLiteral("image/png")).isEmpty());
}

void KoPluginLoaderTest::testEmptyPluginWrapper()
{
    KoJsonTrader::Plugin plugin;

    QCOMPARE(plugin.instance(), nullptr);
    QVERIFY(plugin.metaData().isEmpty());
    QVERIFY(plugin.fileName().isEmpty());
    QVERIFY(plugin.errorString().isEmpty());
}

void KoPluginLoaderTest::testDynamicPluginWrapper()
{
    const QList<KoJsonTrader::Plugin> offers =
        KoJsonTrader::instance()->query(QStringLiteral("Krita/DummyPlugin"), QString());
    auto offer = std::find_if(offers.cbegin(), offers.cend(), [](const KoJsonTrader::Plugin &plugin) {
        return plugin.metaData().value(QStringLiteral("MetaData")).toObject().value(QStringLiteral("Id")).toString()
            == QStringLiteral("dummyplugin1");
    });
    QVERIFY(offer != offers.cend());

    QMutex mutex;
    const QSharedPointer<QPluginLoader> loader = QSharedPointer<QPluginLoader>::create(offer->fileName());
    KoJsonTrader::Plugin plugin(loader, &mutex);

    QCOMPARE(plugin.fileName(), loader->fileName());
    QCOMPARE(plugin.metaData(), loader->metaData());
    QVERIFY(qobject_cast<KPluginFactory *>(plugin.instance()));
    QCOMPARE(plugin.errorString(), loader->errorString());
}

void KoPluginLoaderTest::testStaticPluginWrapper()
{
    const QJsonObject rawData{
        {QStringLiteral("Id"), QStringLiteral("static-dummy")},
        {QStringLiteral("X-KDE-ServiceTypes"), QJsonArray{QStringLiteral("Krita/DummyPlugin")}},
    };
    const KPluginMetaData metaData(rawData, QStringLiteral("static-dummy"));
    QMutex mutex;
    KoJsonTrader::Plugin plugin(metaData, &mutex);

    QCOMPARE(plugin.fileName(), metaData.fileName());
    QCOMPARE(plugin.metaData().value(QStringLiteral("MetaData")).toObject(), metaData.rawData());
    QCOMPARE(plugin.instance(), nullptr);
    QVERIFY(!plugin.errorString().isEmpty());
}

void KoPluginLoaderTest::testLoadSinglePlugin_data()
{
    QTest::addColumn<QString>("requestedPluginId");
    QTest::addColumn<QString>("expectedPluginId");
    QTest::addColumn<int>("expectedVersion");

    QTest::addRow("load_normal") << "dummyplugin1" << "dummyplugin1" << 1;
    QTest::addRow("load_latest_version") << "dummyplugin2" << "dummyplugin2" << 2;
    QTest::addRow("load_version_as_int") << "dummyplugin3" << "dummyplugin3" << 3;
}

void KoPluginLoaderTest::testLoadSinglePlugin()
{
    QFETCH(QString, requestedPluginId);
    QFETCH(QString, expectedPluginId);
    QFETCH(int, expectedVersion);

    KoPluginLoader *loader = KoPluginLoader::instance();

    KPluginFactory *factory = loader->loadSinglePlugin(requestedPluginId, "Krita/DummyPlugin");
    QVERIFY(factory);

    std::unique_ptr<DummyTrivialInterface> plugin1(factory->create<DummyTrivialInterface>());
    QVERIFY(plugin1);

    QCOMPARE(plugin1->version(), expectedVersion);
    QCOMPARE(plugin1->name(), expectedPluginId);
}

void KoPluginLoaderTest::testLoadSinglePluginPredicates()
{
    KoPluginLoader *loader = KoPluginLoader::instance();
    KPluginFactory *singlePredicateFactory =
        loader->loadSinglePlugin(std::make_pair(QStringLiteral("Id"), QStringLiteral("dummyplugin1")),
                                 QStringLiteral("Krita/DummyPlugin"));
    QVERIFY(singlePredicateFactory);
    std::unique_ptr<DummyTrivialInterface> singlePredicatePlugin(
        singlePredicateFactory->create<DummyTrivialInterface>());
    QVERIFY(singlePredicatePlugin);
    QCOMPARE(singlePredicatePlugin->name(), QStringLiteral("dummyplugin1"));

    const std::vector<std::pair<QString, QString>> predicates{
        {QStringLiteral("Id"), QStringLiteral("dummyplugin2")},
        {QStringLiteral("X-Krita-Version"), QStringLiteral("1")},
    };
    KPluginFactory *multiplePredicateFactory =
        loader->loadSinglePlugin(predicates, QStringLiteral("Krita/DummyPlugin"));
    QVERIFY(multiplePredicateFactory);
    std::unique_ptr<DummyTrivialInterface> multiplePredicatePlugin(
        multiplePredicateFactory->create<DummyTrivialInterface>());
    QVERIFY(multiplePredicatePlugin);
    QCOMPARE(multiplePredicatePlugin->name(), QStringLiteral("dummyplugin2"));
    QCOMPARE(multiplePredicatePlugin->version(), 1);

    QVERIFY(!loader->loadSinglePlugin(std::make_pair(QStringLiteral("Id"), QStringLiteral("missing-plugin")),
                                      QStringLiteral("Krita/DummyPlugin")));
}

void KoPluginLoaderTest::testCachedLoad()
{
    KoPluginLoader loader;
    QObject firstOwner;
    QObject secondOwner;

    loader.load(QStringLiteral("Krita/DummyPlugin"), {}, &firstOwner, true);
    loader.load(QStringLiteral("Krita/DummyPlugin"), {}, &secondOwner, true);

    QCOMPARE(firstOwner.children().size(), 3);
    QVERIFY(secondOwner.children().isEmpty());
}

void KoPluginLoaderTest::testLoadAll_data()
{
    QTest::addColumn<QStringList>("blacklistedIds");

    QTest::addRow("load_all") << QStringList{};
    QTest::addRow("load_blacklist") << QStringList{"dummyplugin1"};
    QTest::addRow("load_blacklist_duplicated") << QStringList{"dummyplugin2"};
}

void KoPluginLoaderTest::testLoadAll()
{
    QFETCH(QStringList, blacklistedIds);

    KoPluginLoader::PluginsConfig config;

    if (!blacklistedIds.isEmpty()) {
        config.group = "dummy";
        config.blacklist = "dummyblacklist";

        KConfigGroup configGroup(KSharedConfig::openConfig(), config.group);
        configGroup.writeEntry(config.blacklist, blacklistedIds);
    }

    std::unique_ptr<QObject> parent(new QObject);

    KoPluginLoader *loader = KoPluginLoader::instance();
    loader->load("Krita/DummyPlugin", config, parent.get(), false);

    std::vector<std::pair<QString, int>> childrenIds;
    for (QObject *obj : parent->children()) {
        DummyTrivialInterface *iface = qobject_cast<DummyTrivialInterface *>(obj);
        QVERIFY(iface);
        childrenIds.emplace_back(iface->name(), iface->version());
    }
    std::sort(childrenIds.begin(), childrenIds.end());

    std::vector<std::pair<QString, int>> expectedIds = {{"dummyplugin1", 1}, {"dummyplugin2", 2}, {"dummyplugin3", 3}};

    if (!blacklistedIds.isEmpty()) {
        auto it = expectedIds.begin();
        while (it != expectedIds.end()) {
            if (blacklistedIds.contains(it->first)) {
                it = expectedIds.erase(it);
            } else {
                ++it;
            }
        }

        KSharedConfig::openConfig()->deleteGroup(config.group);
    }

    QCOMPARE(childrenIds, expectedIds);
}

QTEST_GUILESS_MAIN(KoPluginLoaderTest)
