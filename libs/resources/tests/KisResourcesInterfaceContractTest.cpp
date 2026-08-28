/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisResourcesInterface.h>
#include <KisResourcesInterface_p.h>

#include <QTest>

#include <atomic>
#include <functional>
#include <thread>
#include <utility>
#include <vector>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("unexpected assertion: %s at %s:%d", assertion, file, line);
}

namespace
{
class TestResource : public KoResource
{
public:
    KoResourceSP clone() const override
    {
        return KoResourceSP(new TestResource(*this));
    }

    bool loadFromDevice(QIODevice *, KisResourcesInterfaceSP) override
    {
        return true;
    }

    bool saveToDevice(QIODevice *) const override
    {
        return true;
    }

    QPair<QString, QString> resourceType() const override
    {
        return {QStringLiteral("test-resources"), QStringLiteral("test")};
    }
};

class OtherResource final : public TestResource
{
};

template<typename T = TestResource>
QSharedPointer<T> makeResource(const QString &md5,
                               const QString &filename,
                               const QString &name,
                               bool active = true)
{
    QSharedPointer<T> resource(new T);
    resource->setMD5Sum(md5);
    resource->setFilename(filename);
    resource->setName(name);
    resource->setActive(active);
    return resource;
}

class RecordingSource final : public KisResourcesInterface::ResourceSourceAdapter
{
public:
    explicit RecordingSource(const QString &type, bool *destroyed = nullptr)
        : ResourceSourceAdapter(type)
        , m_destroyed(destroyed)
    {
    }

    ~RecordingSource() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

    QVector<KoResourceSP> resourcesForFilename(const QString &filename) const override
    {
        requestedFilenames.append(filename);
        return byFilename.value(filename);
    }

    QVector<KoResourceSP> resourcesForName(const QString &name) const override
    {
        requestedNames.append(name);
        return byName.value(name);
    }

    QVector<KoResourceSP> resourcesForMD5(const QString &md5) const override
    {
        requestedMd5s.append(md5);
        return byMd5.value(md5);
    }

    KoResourceSP fallbackResource() const override
    {
        return fallback;
    }

    QHash<QString, QVector<KoResourceSP>> byFilename;
    QHash<QString, QVector<KoResourceSP>> byName;
    QHash<QString, QVector<KoResourceSP>> byMd5;
    KoResourceSP fallback;
    mutable QStringList requestedFilenames;
    mutable QStringList requestedNames;
    mutable QStringList requestedMd5s;

private:
    bool *m_destroyed;
};

class RecordingInterface final : public KisResourcesInterface
{
public:
    using Factory = std::function<ResourceSourceAdapter *(const QString &)>;

    explicit RecordingInterface(Factory factory = Factory(), bool *destroyed = nullptr)
        : m_factory(std::move(factory))
        , m_destroyed(destroyed)
    {
    }

    ~RecordingInterface() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

    mutable std::atomic_int createCount {0};
    mutable QStringList createdTypes;

protected:
    ResourceSourceAdapter *createSourceImpl(const QString &type) const override
    {
        ++createCount;
        createdTypes.append(type);
        return m_factory ? m_factory(type) : new RecordingSource(type);
    }

private:
    Factory m_factory;
    bool *m_destroyed;
};

class ObservedPrivate final : public KisResourcesInterfacePrivate
{
public:
    explicit ObservedPrivate(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~ObservedPrivate() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};

class PrivateStateInterface final : public KisResourcesInterface
{
public:
    PrivateStateInterface(KisResourcesInterfacePrivate *privateData, bool *destroyed)
        : KisResourcesInterface(privateData)
        , m_destroyed(destroyed)
    {
    }

    ~PrivateStateInterface() override
    {
        *m_destroyed = true;
    }

protected:
    ResourceSourceAdapter *createSourceImpl(const QString &type) const override
    {
        return new RecordingSource(type);
    }

private:
    bool *m_destroyed;
};
}

class KisResourcesInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cachesSourcesAndOwnsTheirLifetime();
    void destroysInjectedPrivateStatePolymorphically();
    void exposesResourceQueriesAndFallback();
    void ranksMd5CandidatesAndUsesLegacyFallbacks();
    void distinguishesExactMatchingFromBestMatching();
    void returnsExistingOrFailedLoadResults();
    void preservesTypedAdapterResults();
};

void KisResourcesInterfaceContractTest::cachesSourcesAndOwnsTheirLifetime()
{
    bool interfaceDestroyed = false;
    bool adapterDestroyed = false;
    {
        QSharedPointer<KisResourcesInterface> resourcesInterface(
            new RecordingInterface(
                [&adapterDestroyed](const QString &type) {
                    return new RecordingSource(type, &adapterDestroyed);
                },
                &interfaceDestroyed));
        auto *recordingInterface = static_cast<RecordingInterface *>(resourcesInterface.data());

        auto *first = &resourcesInterface->source(QStringLiteral("cached"));
        auto *second = &resourcesInterface->source(QStringLiteral("cached"));
        QCOMPARE(first, second);
        QCOMPARE(recordingInterface->createCount.load(), 1);

        constexpr int threadCount = 8;
        std::atomic_bool start {false};
        std::vector<KisResourcesInterface::ResourceSourceAdapter *> threadedSources(threadCount, nullptr);
        std::vector<std::thread> threads;
        threads.reserve(threadCount);
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([&, i] {
                while (!start.load(std::memory_order_acquire)) {
                    std::this_thread::yield();
                }
                threadedSources[i] = &resourcesInterface->source(QStringLiteral("threaded"));
            });
        }
        start.store(true, std::memory_order_release);
        for (std::thread &thread : threads) {
            thread.join();
        }

        for (KisResourcesInterface::ResourceSourceAdapter *source : threadedSources) {
            QCOMPARE(source, threadedSources.front());
        }
        QCOMPARE(recordingInterface->createCount.load(), 2);
        QCOMPARE(recordingInterface->createdTypes,
                 QStringList({QStringLiteral("cached"), QStringLiteral("threaded")}));
        QVERIFY(!adapterDestroyed);
        QVERIFY(!interfaceDestroyed);
    }
    QVERIFY(adapterDestroyed);
    QVERIFY(interfaceDestroyed);
}

void KisResourcesInterfaceContractTest::destroysInjectedPrivateStatePolymorphically()
{
    bool privateDestroyed = false;
    bool interfaceDestroyed = false;
    {
        QSharedPointer<KisResourcesInterface> resourcesInterface(
            new PrivateStateInterface(new ObservedPrivate(&privateDestroyed), &interfaceDestroyed));
        QVERIFY(!privateDestroyed);
        QVERIFY(!interfaceDestroyed);
    }
    QVERIFY(privateDestroyed);
    QVERIFY(interfaceDestroyed);
}

void KisResourcesInterfaceContractTest::exposesResourceQueriesAndFallback()
{
    bool destroyed = false;
    {
        RecordingSource source(QStringLiteral("brushes"), &destroyed);
        KoResourceSP filenameResource = makeResource(QStringLiteral("filename-md5"),
                                                     QStringLiteral("file.res"),
                                                     QStringLiteral("Filename"));
        KoResourceSP nameResource = makeResource(QStringLiteral("name-md5"),
                                                 QStringLiteral("name.res"),
                                                 QStringLiteral("Name"));
        KoResourceSP md5Resource = makeResource(QStringLiteral("digest"),
                                                QStringLiteral("md5.res"),
                                                QStringLiteral("MD5"));
        source.byFilename.insert(QStringLiteral("file.res"), {filenameResource});
        source.byName.insert(QStringLiteral("Name"), {nameResource});
        source.byMd5.insert(QStringLiteral("digest"), {md5Resource});
        source.fallback = nameResource;

        QCOMPARE(source.resourcesForFilename(QStringLiteral("file.res")),
                 QVector<KoResourceSP>({filenameResource}));
        QCOMPARE(source.resourcesForName(QStringLiteral("Name")),
                 QVector<KoResourceSP>({nameResource}));
        QCOMPARE(source.resourcesForMD5(QStringLiteral("digest")),
                 QVector<KoResourceSP>({md5Resource}));
        QCOMPARE(source.fallbackResource(), nameResource);
    }
    QVERIFY(destroyed);
}

void KisResourcesInterfaceContractTest::ranksMd5CandidatesAndUsesLegacyFallbacks()
{
    RecordingSource source(QStringLiteral("brushes"));
    KoResourceSP mismatched = makeResource(QStringLiteral("digest"),
                                           QStringLiteral("other.res"),
                                           QStringLiteral("Other"));
    KoResourceSP inactiveExact = makeResource(QStringLiteral("digest"),
                                              QStringLiteral("wanted.res"),
                                              QStringLiteral("Wanted"),
                                              false);
    KoResourceSP exact = makeResource(QStringLiteral("digest"),
                                      QStringLiteral("wanted.res"),
                                      QStringLiteral("Wanted"));
    source.byMd5.insert(QStringLiteral("digest"), {mismatched, inactiveExact, exact});

    QCOMPARE(source.bestMatch(QStringLiteral("digest"),
                              QStringLiteral("wanted.res"),
                              QStringLiteral("Wanted")),
             exact);

    KoResourceSP inactiveFallback = makeResource(QStringLiteral("old"),
                                                 QStringLiteral("legacy.res"),
                                                 QStringLiteral("Wanted"),
                                                 false);
    KoResourceSP activeFallback = makeResource(QStringLiteral("old"),
                                               QStringLiteral("legacy.res"),
                                               QStringLiteral("Other"));
    source.byFilename.insert(QStringLiteral("legacy.res"), {inactiveFallback, activeFallback});
    QCOMPARE(source.bestMatch(QStringLiteral("missing"),
                              QStringLiteral("legacy.res"),
                              QStringLiteral("Wanted")),
             activeFallback);

    KoResourceSP nameFallback = makeResource(QStringLiteral("named"),
                                             QStringLiteral("named.res"),
                                             QStringLiteral("Only Name"));
    source.byName.insert(QStringLiteral("Only Name"), {nameFallback});
    QCOMPARE(source.bestMatch(QString(), QString(), QStringLiteral("Only Name")), nameFallback);
    QVERIFY(source.bestMatch(QString(), QString(), QString()).isNull());
}

void KisResourcesInterfaceContractTest::distinguishesExactMatchingFromBestMatching()
{
    RecordingSource source(QStringLiteral("brushes"));
    KoResourceSP resource = makeResource(QStringLiteral("actual"),
                                         QStringLiteral("legacy.res"),
                                         QStringLiteral("Legacy"));
    source.byFilename.insert(QStringLiteral("legacy.res"), {resource});

    QCOMPARE(source.bestMatch(QStringLiteral("missing"),
                              QStringLiteral("legacy.res"),
                              QStringLiteral("Legacy")),
             resource);
    QVERIFY(source.exactMatch(QStringLiteral("missing"),
                              QStringLiteral("legacy.res"),
                              QStringLiteral("Legacy")).isNull());
    QCOMPARE(source.exactMatch(QString(),
                               QStringLiteral("legacy.res"),
                               QStringLiteral("Legacy")),
             resource);
}

void KisResourcesInterfaceContractTest::returnsExistingOrFailedLoadResults()
{
    RecordingSource source(QStringLiteral("brushes"));
    KoResourceSP resource = makeResource(QStringLiteral("digest"),
                                         QStringLiteral("found.res"),
                                         QStringLiteral("Found"));
    source.byMd5.insert(QStringLiteral("digest"), {resource});

    const KoResourceLoadResult found = source.bestMatchLoadResult(
        QStringLiteral("digest"), QStringLiteral("found.res"), QStringLiteral("Found"));
    QCOMPARE(found.type(), KoResourceLoadResult::ExistingResource);
    QCOMPARE(found.resource(), resource);

    const KoResourceLoadResult missing = source.bestMatchLoadResult(
        QStringLiteral("missing"), QStringLiteral("missing.res"), QStringLiteral("Missing"));
    QCOMPARE(missing.type(), KoResourceLoadResult::FailedLink);
    QCOMPARE(missing.signature().type, QStringLiteral("brushes"));
    QCOMPARE(missing.signature().md5sum, QStringLiteral("missing"));
    QCOMPARE(missing.signature().filename, QStringLiteral("missing.res"));
    QCOMPARE(missing.signature().name, QStringLiteral("Missing"));
}

void KisResourcesInterfaceContractTest::preservesTypedAdapterResults()
{
    QSharedPointer<TestResource> resource = makeResource(
        QStringLiteral("digest"), QStringLiteral("typed.res"), QStringLiteral("Typed"));
    RecordingSource source(QStringLiteral("typed"));
    source.byMd5.insert(QStringLiteral("digest"), {resource});
    source.fallback = resource;

    KisResourcesInterface::TypedResourceSourceAdapter<TestResource> typedSource(&source);
    QCOMPARE(typedSource.bestMatch(QStringLiteral("digest"),
                                   QStringLiteral("typed.res"),
                                   QStringLiteral("Typed")),
             resource);
    QCOMPARE(typedSource.exactMatch(QStringLiteral("digest"),
                                    QStringLiteral("typed.res"),
                                    QStringLiteral("Typed")),
             resource);
    QCOMPARE(typedSource.fallbackResource(), resource);
    QCOMPARE(typedSource.bestMatchLoadResult(QStringLiteral("digest"),
                                             QStringLiteral("typed.res"),
                                             QStringLiteral("Typed")).resource<TestResource>(),
             resource);

    RecordingInterface resourcesInterface([resource](const QString &type) {
        auto *createdSource = new RecordingSource(type);
        createdSource->byMd5.insert(QStringLiteral("digest"), {resource});
        return createdSource;
    });
    QCOMPARE(resourcesInterface.source<TestResource>(QStringLiteral("typed"))
                 .bestMatch(QStringLiteral("digest"),
                            QStringLiteral("typed.res"),
                            QStringLiteral("Typed")),
             resource);

    KisResourcesInterface::TypedResourceSourceAdapter<OtherResource> wrongType(&source);
    QVERIFY(wrongType.fallbackResource().isNull());
}

QTEST_GUILESS_MAIN(KisResourcesInterfaceContractTest)

#include "KisResourcesInterfaceContractTest.moc"
