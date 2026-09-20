/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorSpaceEngine.h>

#include <QTest>

struct KoColorProfileQuery {
};

namespace
{

class EngineProbe final : public KoColorSpaceEngine
{
public:
    EngineProbe(const QString &identifier, const QString &displayName, int *destructionCount = nullptr)
        : KoColorSpaceEngine(identifier, displayName)
        , destructionCount(destructionCount)
    {
    }

    ~EngineProbe() override
    {
        if (destructionCount) {
            ++*destructionCount;
        }
    }

    KoColorConversionTransformation *
    createColorTransformation(const KoColorSpace *,
                              const KoColorSpace *,
                              KoColorConversionTransformation::Intent,
                              KoColorConversionTransformation::ConversionFlags) const override
    {
        return nullptr;
    }

    const KoColorProfile *addProfile(const QString &filename) override
    {
        lastAddedFilename = filename;
        ++filenameAddCalls;
        return filenameProfileResult;
    }

    const KoColorProfile *addProfile(const QByteArray &data) override
    {
        lastAddedData = data;
        ++dataAddCalls;
        return dataProfileResult;
    }

    const KoColorProfile *getProfile(const KoColorProfileQuery &query) override
    {
        lastProfileQuery = &query;
        ++getProfileCalls;
        return queriedProfileResult;
    }

    void removeProfile(const QString &filename) override
    {
        lastRemovedFilename = filename;
        ++removeProfileCalls;
    }

    int *destructionCount = nullptr;
    const KoColorProfile *filenameProfileResult = nullptr;
    const KoColorProfile *dataProfileResult = nullptr;
    const KoColorProfile *queriedProfileResult = nullptr;
    QString lastAddedFilename;
    QByteArray lastAddedData;
    const KoColorProfileQuery *lastProfileQuery = nullptr;
    QString lastRemovedFilename;
    int filenameAddCalls = 0;
    int dataAddCalls = 0;
    int getProfileCalls = 0;
    int removeProfileCalls = 0;
};

} // namespace

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

class KoColorSpaceEngineContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void unicodeIdentityAndVirtualLifetimeArePreserved();
    void profileOperationsPreserveArgumentsAndResults();
    void defaultCompatibilityAcceptsEveryToken();
    void localRegistryOwnsEnginesAndSingletonIsStable();
};

void KoColorSpaceEngineContractTest::unicodeIdentityAndVirtualLifetimeArePreserved()
{
    int destructionCount = 0;
    KoColorSpaceEngine *engine =
        new EngineProbe(QStringLiteral("engine-色域-α"), QStringLiteral("表示用 色空間 engine"), &destructionCount);

    QCOMPARE(engine->id(), QStringLiteral("engine-色域-α"));
    QCOMPARE(engine->name(), QStringLiteral("表示用 色空間 engine"));
    QCOMPARE(&engine->id(), &engine->id());
    QCOMPARE(&engine->name(), &engine->name());
    QCOMPARE(destructionCount, 0);

    delete engine;
    QCOMPARE(destructionCount, 1);
}

void KoColorSpaceEngineContractTest::profileOperationsPreserveArgumentsAndResults()
{
    EngineProbe engine(QStringLiteral("profile-engine"), QStringLiteral("Profile Engine"));
    char filenameProfileStorage = 0;
    char dataProfileStorage = 0;
    char queriedProfileStorage = 0;
    const auto *filenameProfile = reinterpret_cast<const KoColorProfile *>(&filenameProfileStorage);
    const auto *dataProfile = reinterpret_cast<const KoColorProfile *>(&dataProfileStorage);
    const auto *queriedProfile = reinterpret_cast<const KoColorProfile *>(&queriedProfileStorage);
    engine.filenameProfileResult = filenameProfile;
    engine.dataProfileResult = dataProfile;
    engine.queriedProfileResult = queriedProfile;
    KoColorSpaceEngine &interface = engine;

    const QString filename = QStringLiteral("プロファイル-色域.icc");
    QCOMPARE(interface.addProfile(filename), filenameProfile);
    QCOMPARE(engine.lastAddedFilename, filename);
    QCOMPARE(engine.filenameAddCalls, 1);

    const QByteArray profileData("profile\0binary\x7f", 15);
    QCOMPARE(interface.addProfile(profileData), dataProfile);
    QCOMPARE(engine.lastAddedData, profileData);
    QCOMPARE(engine.dataAddCalls, 1);

    const KoColorProfileQuery query;
    QCOMPARE(interface.getProfile(query), queriedProfile);
    QCOMPARE(engine.lastProfileQuery, &query);
    QCOMPARE(engine.getProfileCalls, 1);

    const QString removedFilename = QStringLiteral("削除-色域.icc");
    interface.removeProfile(removedFilename);
    QCOMPARE(engine.lastRemovedFilename, removedFilename);
    QCOMPARE(engine.removeProfileCalls, 1);
}

void KoColorSpaceEngineContractTest::defaultCompatibilityAcceptsEveryToken()
{
    EngineProbe engine(QStringLiteral("compatibility-engine"), QStringLiteral("Compatibility Engine"));
    char profileStorage = 0;
    const auto *profile = reinterpret_cast<const KoColorProfile *>(&profileStorage);

    QVERIFY(engine.supportsColorSpace(QStringLiteral("model-広色域"), QStringLiteral("depth-浮動小数"), profile));
    QVERIFY(engine.supportsColorSpace(QString(), QString(), nullptr));
}

void KoColorSpaceEngineContractTest::localRegistryOwnsEnginesAndSingletonIsStable()
{
    KoColorSpaceEngineRegistry *globalRegistry = KoColorSpaceEngineRegistry::instance();
    QVERIFY(globalRegistry);
    QCOMPARE(KoColorSpaceEngineRegistry::instance(), globalRegistry);
    const int globalCount = globalRegistry->count();

    int firstDestructionCount = 0;
    int secondDestructionCount = 0;
    {
        KoColorSpaceEngineRegistry registry;
        auto *first = new EngineProbe(QStringLiteral("局所-engine-一"),
                                      QStringLiteral("局所 Engine One"),
                                      &firstDestructionCount);
        auto *second = new EngineProbe(QStringLiteral("局所-engine-二"),
                                       QStringLiteral("局所 Engine Two"),
                                       &secondDestructionCount);

        registry.add(first);
        registry.add(second);

        QCOMPARE(registry.count(), 2);
        QCOMPARE(registry.get(QStringLiteral("局所-engine-一")), first);
        QCOMPARE(registry.get(QStringLiteral("局所-engine-二")), second);
        QCOMPARE(firstDestructionCount, 0);
        QCOMPARE(secondDestructionCount, 0);
    }

    QCOMPARE(firstDestructionCount, 1);
    QCOMPARE(secondDestructionCount, 1);
    QCOMPARE(globalRegistry->count(), globalCount);
}

QTEST_GUILESS_MAIN(KoColorSpaceEngineContractTest)

#include "KoColorSpaceEngineContractTest.moc"
