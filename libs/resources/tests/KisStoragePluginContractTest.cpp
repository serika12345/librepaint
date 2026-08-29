/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisMimeDatabase.h>
#include <KisResourceLoaderRegistry.h>
#include <KisStoragePlugin.h>
#include <KoResource.h>

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

namespace
{
struct LoaderTrace {
    int mimeCalls{0};
    int registryCalls{0};
    int loaderCalls{0};
    QString suffix;
    QString resourceType;
    QString mimeType;
    QString mimeResult{QStringLiteral("application/x-storage-contract")};
    KisResourceLoaderBase *loader{nullptr};
};

LoaderTrace *s_loaderTrace{nullptr};

class ScopedLoaderTrace
{
public:
    explicit ScopedLoaderTrace(LoaderTrace *trace)
    {
        Q_ASSERT(!s_loaderTrace);
        s_loaderTrace = trace;
    }

    ~ScopedLoaderTrace()
    {
        s_loaderTrace = nullptr;
    }
};

class ObservedResource final : public KoResource
{
public:
    explicit ObservedResource(const QString &filename)
        : KoResource(filename)
    {
    }

    ObservedResource(const ObservedResource &rhs)
        : KoResource(rhs)
    {
    }

    KoResourceSP clone() const override
    {
        return KoResourceSP(new ObservedResource(*this));
    }

    bool loadFromDevice(QIODevice *, KisResourcesInterfaceSP) override
    {
        return true;
    }

    QPair<QString, QString> resourceType() const override
    {
        return {QStringLiteral("contract"), QStringLiteral("observed")};
    }
};

class RecordingLoader final : public KisResourceLoaderBase
{
public:
    RecordingLoader()
        : KisResourceLoaderBase(QStringLiteral("observed"),
                                QStringLiteral("contract"),
                                QStringLiteral("Observed loader"),
                                {QStringLiteral("application/x-storage-contract")})
    {
    }

    KoResourceSP create(const QString &name) override
    {
        ++createCalls;
        createdName = name;
        return result;
    }

    int createCalls{0};
    QString createdName;
    KoResourceSP result;
};

class StorageProbe : public KisStoragePlugin
{
public:
    explicit StorageProbe(const QString &location, bool *destroyed = nullptr)
        : KisStoragePlugin(location)
        , m_destroyed(destroyed)
    {
    }

    ~StorageProbe() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

    KisResourceStorage::ResourceItem resourceItem(const QString &url) override
    {
        ++resourceItemCalls;
        resourceItemUrl = url;
        return resourceItemResult;
    }

    bool loadVersionedResource(KoResourceSP resource) override
    {
        ++loadVersionedResourceCalls;
        loadedResource = resource;
        return loadVersionedResourceResult;
    }

    QSharedPointer<KisResourceStorage::ResourceIterator> resources(const QString &resourceType) override
    {
        ++resourcesCalls;
        resourcesType = resourceType;
        return {};
    }

    QSharedPointer<KisResourceStorage::TagIterator> tags(const QString &resourceType) override
    {
        ++tagsCalls;
        tagsType = resourceType;
        return {};
    }

    QString observedLocation() const
    {
        return location();
    }

    int resourceItemCalls{0};
    int loadVersionedResourceCalls{0};
    int resourcesCalls{0};
    int tagsCalls{0};
    QString resourceItemUrl;
    QString resourcesType;
    QString tagsType;
    KisResourceStorage::ResourceItem resourceItemResult;
    KoResourceSP loadedResource;
    bool loadVersionedResourceResult{true};

private:
    bool *m_destroyed;
};

class DispatchProbe final : public StorageProbe
{
public:
    explicit DispatchProbe(const QString &location)
        : StorageProbe(location)
    {
    }

    KoResourceSP resource(const QString &url) override
    {
        resourceUrl = url;
        return resourceResult;
    }

    QString resourceMd5(const QString &url) override
    {
        resourceMd5Url = url;
        return resourceMd5Result;
    }

    QString resourceFilePath(const QString &url) override
    {
        resourceFilePathUrl = url;
        return resourceFilePathResult;
    }

    bool supportsVersioning() const override
    {
        ++supportsVersioningCalls;
        return supportsVersioningResult;
    }

    bool saveAsNewVersion(const QString &resourceType, KoResourceSP resource) override
    {
        saveType = resourceType;
        savedResource = resource;
        return true;
    }

    bool importResource(const QString &url, QIODevice *device) override
    {
        importUrl = url;
        importDevice = device;
        return true;
    }

    bool exportResource(const QString &url, QIODevice *device) override
    {
        exportUrl = url;
        exportDevice = device;
        return true;
    }

    bool addResource(const QString &resourceType, KoResourceSP resource) override
    {
        addType = resourceType;
        addedResource = resource;
        return true;
    }

    QImage thumbnail() const override
    {
        return thumbnailResult;
    }

    void setMetaData(const QString &key, const QVariant &value) override
    {
        metaDataKey = key;
        metaDataValue = value;
    }

    QStringList metaDataKeys() const override
    {
        return metaDataKeysResult;
    }

    QVariant metaData(const QString &key) const override
    {
        requestedMetaDataKey = key;
        return metaDataResult;
    }

    bool isValid() const override
    {
        ++isValidCalls;
        return isValidResult;
    }

    QString resourceUrl;
    QString resourceMd5Url;
    QString resourceFilePathUrl;
    QString saveType;
    QString importUrl;
    QString exportUrl;
    QString addType;
    QString metaDataKey;
    mutable QString requestedMetaDataKey;
    KoResourceSP resourceResult;
    KoResourceSP savedResource;
    KoResourceSP addedResource;
    QIODevice *importDevice{nullptr};
    QIODevice *exportDevice{nullptr};
    QString resourceMd5Result{QStringLiteral("override-md5")};
    QString resourceFilePathResult{QStringLiteral("/contract/resource")};
    QImage thumbnailResult{2, 3, QImage::Format_ARGB32};
    QVariant metaDataValue;
    QStringList metaDataKeysResult{QStringLiteral("author"), QStringLiteral("title")};
    QVariant metaDataResult{17};
    mutable int supportsVersioningCalls{0};
    mutable int isValidCalls{0};
    bool supportsVersioningResult{false};
    bool isValidResult{false};
};
} // namespace

QString KisMimeDatabase::mimeTypeForSuffix(const QString &suffix)
{
    if (!s_loaderTrace) {
        qFatal("unexpected MIME database access");
    }

    ++s_loaderTrace->mimeCalls;
    s_loaderTrace->suffix = suffix;
    return s_loaderTrace->mimeResult;
}

struct KisResourceLoaderRegistry::Private {
};

KisResourceLoaderRegistry::KisResourceLoaderRegistry(QObject *parent)
    : QObject(parent)
    , m_d(new Private)
{
}

KisResourceLoaderRegistry::~KisResourceLoaderRegistry() = default;

KisResourceLoaderRegistry *KisResourceLoaderRegistry::instance()
{
    if (!s_loaderTrace) {
        qFatal("unexpected resource loader registry access");
    }

    ++s_loaderTrace->registryCalls;
    static KisResourceLoaderRegistry registry(nullptr);
    return &registry;
}

KisResourceLoaderBase *KisResourceLoaderRegistry::loader(const QString &resourceType, const QString &mimetype) const
{
    if (!s_loaderTrace) {
        qFatal("unexpected resource loader lookup");
    }

    ++s_loaderTrace->loaderCalls;
    s_loaderTrace->resourceType = resourceType;
    s_loaderTrace->mimeType = mimetype;
    return s_loaderTrace->loader;
}

class KisStoragePluginContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesLocationTimestampAndVirtualLifetime()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString missingPath = directory.filePath(QStringLiteral("missing.storage"));
        const QDateTime beforeConstruction = QDateTime::currentDateTime();
        bool destroyed = false;
        std::unique_ptr<KisStoragePlugin> storage(new StorageProbe(missingPath, &destroyed));
        auto *probe = static_cast<StorageProbe *>(storage.get());
        const QDateTime afterConstruction = QDateTime::currentDateTime();

        QCOMPARE(probe->observedLocation(), missingPath);
        const QDateTime missingTimestamp = storage->timestamp();
        QVERIFY(missingTimestamp >= beforeConstruction);
        QVERIFY(missingTimestamp <= afterConstruction);
        QTest::qWait(5);
        QCOMPARE(storage->timestamp(), missingTimestamp);
        storage.reset();
        QVERIFY(destroyed);

        const QString existingPath = directory.filePath(QStringLiteral("existing.storage"));
        QFile file(existingPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("storage"), qint64(7));
        const QDateTime firstModification(QDate(2020, 2, 3), QTime(4, 5, 6), QTimeZone::UTC);
        QVERIFY(file.setFileTime(firstModification, QFileDevice::FileModificationTime));
        file.close();

        StorageProbe existingStorage(existingPath);
        QCOMPARE(existingStorage.timestamp(), QFileInfo(existingPath).lastModified());

        QVERIFY(file.open(QIODevice::ReadWrite));
        const QDateTime secondModification(QDate(2021, 3, 4), QTime(5, 6, 7), QTimeZone::UTC);
        QVERIFY(file.setFileTime(secondModification, QFileDevice::FileModificationTime));
        file.close();
        QCOMPARE(existingStorage.timestamp(), QFileInfo(existingPath).lastModified());
    }

    void parsesResourceUrlAndLoadsVersionedResource()
    {
        QTemporaryDir directory;
        StorageProbe storage(directory.filePath(QStringLiteral("storage")));
        RecordingLoader loader;
        KoResourceSP resource(new ObservedResource(QStringLiteral("created.test")));
        loader.result = resource;

        LoaderTrace trace;
        trace.loader = &loader;
        ScopedLoaderTrace traceScope(&trace);

        QCOMPARE(storage.resource(QStringLiteral("patterns/subfolder/brush.test")), resource);
        QCOMPARE(trace.mimeCalls, 1);
        QCOMPARE(trace.registryCalls, 1);
        QCOMPARE(trace.loaderCalls, 1);
        QCOMPARE(trace.suffix, QStringLiteral("subfolder/brush.test"));
        QCOMPARE(trace.resourceType, QStringLiteral("patterns"));
        QCOMPARE(trace.mimeType, trace.mimeResult);
        QCOMPARE(loader.createCalls, 1);
        QCOMPARE(loader.createdName, QStringLiteral("subfolder/brush.test"));
        QCOMPARE(storage.loadVersionedResourceCalls, 1);
        QCOMPARE(storage.loadedResource, resource);

        storage.loadVersionedResourceResult = false;
        QVERIFY(!storage.resource(QStringLiteral("patterns/rejected.test")));
        QCOMPARE(storage.loadVersionedResourceCalls, 2);
        QCOMPARE(storage.loadedResource, resource);

        const int mimeCalls = trace.mimeCalls;
        const int registryCalls = trace.registryCalls;
        QVERIFY(!storage.resource(QStringLiteral("missing-separator")));
        QVERIFY(!storage.resource(QStringLiteral("///")));
        QCOMPARE(trace.mimeCalls, mimeCalls);
        QCOMPARE(trace.registryCalls, registryCalls);

        trace.loader = nullptr;
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("Could not create loader.*")));
        QVERIFY(!storage.resource(QStringLiteral("patterns/unregistered.test")));
    }

    void dispatchesPureVirtualStorageOperations()
    {
        StorageProbe probe(QStringLiteral("contract-storage"));
        KisStoragePlugin *storage = &probe;
        KoResourceSP resource(new ObservedResource(QStringLiteral("resource.test")));

        probe.resourceItemResult.url = QStringLiteral("result-url");
        probe.resourceItemResult.folder = QStringLiteral("result-folder");
        const KisResourceStorage::ResourceItem item = storage->resourceItem(QStringLiteral("patterns/item.test"));
        QCOMPARE(probe.resourceItemCalls, 1);
        QCOMPARE(probe.resourceItemUrl, QStringLiteral("patterns/item.test"));
        QCOMPARE(item.url, QStringLiteral("result-url"));
        QCOMPARE(item.folder, QStringLiteral("result-folder"));

        probe.loadVersionedResourceResult = false;
        QVERIFY(!storage->loadVersionedResource(resource));
        QCOMPARE(probe.loadVersionedResourceCalls, 1);
        QCOMPARE(probe.loadedResource, resource);

        QVERIFY(storage->resources(QStringLiteral("brushes")).isNull());
        QCOMPARE(probe.resourcesCalls, 1);
        QCOMPARE(probe.resourcesType, QStringLiteral("brushes"));
        QVERIFY(storage->tags(QStringLiteral("patterns")).isNull());
        QCOMPARE(probe.tagsCalls, 1);
        QCOMPARE(probe.tagsType, QStringLiteral("patterns"));
    }

    void providesStableBaseDefaults()
    {
        StorageProbe storage(QStringLiteral("contract-storage"));
        KoResourceSP resource(new ObservedResource(QStringLiteral("resource.test")));
        QBuffer device;
        device.setData(QByteArrayLiteral("unchanged"));
        QVERIFY(device.open(QIODevice::ReadWrite));
        device.seek(3);
        const qint64 originalPosition = device.pos();
        const QByteArray originalData = device.data();

        QVERIFY(!storage.saveAsNewVersion(QStringLiteral("brushes"), resource));
        QVERIFY(!storage.importResource(QStringLiteral("brushes/import.test"), &device));
        QVERIFY(!storage.exportResource(QStringLiteral("brushes/export.test"), &device));
        QVERIFY(!storage.addResource(QStringLiteral("brushes"), resource));
        QCOMPARE(device.pos(), originalPosition);
        QCOMPARE(device.data(), originalData);
        QVERIFY(storage.thumbnail().isNull());

        storage.setMetaData(QStringLiteral("author"), QStringLiteral("LibrePaint"));
        QVERIFY(storage.metaDataKeys().isEmpty());
        const QVariant missingMetaData = storage.metaData(QStringLiteral("author"));
        QVERIFY(missingMetaData.isValid());
        QCOMPARE(missingMetaData.metaType(), QMetaType::fromType<QString>());
        QCOMPARE(missingMetaData.toString(), QString());

        QVERIFY(storage.supportsVersioning());
        QCOMPARE(storage.resourceFilePath(QStringLiteral("brushes/resource.test")), QString());
        QTest::ignoreMessage(QtWarningMsg, "Storage plugins should implement their own checks!");
        QVERIFY(storage.isValid());
    }

    void resourceMd5UsesVirtualResourceLookup()
    {
        DispatchProbe probe(QStringLiteral("contract-storage"));
        KisStoragePlugin *storage = &probe;
        KoResourceSP resource(new ObservedResource(QStringLiteral("resource.test")));
        resource->setMD5Sum(QStringLiteral("0123456789abcdef"));
        probe.resourceResult = resource;

        QCOMPARE(probe.KisStoragePlugin::resourceMd5(QStringLiteral("brushes/resource.test")),
                 QStringLiteral("0123456789abcdef"));
        QCOMPARE(probe.resourceUrl, QStringLiteral("brushes/resource.test"));

        probe.resourceResult.clear();
        QCOMPARE(probe.KisStoragePlugin::resourceMd5(QStringLiteral("brushes/missing.test")), QString());
        QCOMPARE(probe.resourceUrl, QStringLiteral("brushes/missing.test"));

        QCOMPARE(storage->resourceMd5(QStringLiteral("override-url")), QStringLiteral("override-md5"));
        QCOMPARE(probe.resourceMd5Url, QStringLiteral("override-url"));
    }

    void dispatchesAllOptionalVirtualOperations()
    {
        DispatchProbe probe(QStringLiteral("contract-storage"));
        KisStoragePlugin *storage = &probe;
        KoResourceSP resource(new ObservedResource(QStringLiteral("resource.test")));
        QBuffer device;

        probe.resourceResult = resource;
        QCOMPARE(storage->resource(QStringLiteral("patterns/resource.test")), resource);
        QCOMPARE(probe.resourceUrl, QStringLiteral("patterns/resource.test"));
        QCOMPARE(storage->resourceFilePath(QStringLiteral("patterns/resource.test")),
                 QStringLiteral("/contract/resource"));
        QCOMPARE(probe.resourceFilePathUrl, QStringLiteral("patterns/resource.test"));
        QVERIFY(!storage->supportsVersioning());
        QCOMPARE(probe.supportsVersioningCalls, 1);

        QVERIFY(storage->saveAsNewVersion(QStringLiteral("patterns"), resource));
        QCOMPARE(probe.saveType, QStringLiteral("patterns"));
        QCOMPARE(probe.savedResource, resource);
        QVERIFY(storage->importResource(QStringLiteral("patterns/import.test"), &device));
        QCOMPARE(probe.importUrl, QStringLiteral("patterns/import.test"));
        QCOMPARE(probe.importDevice, &device);
        QVERIFY(storage->exportResource(QStringLiteral("patterns/export.test"), &device));
        QCOMPARE(probe.exportUrl, QStringLiteral("patterns/export.test"));
        QCOMPARE(probe.exportDevice, &device);
        QVERIFY(storage->addResource(QStringLiteral("patterns"), resource));
        QCOMPARE(probe.addType, QStringLiteral("patterns"));
        QCOMPARE(probe.addedResource, resource);

        QCOMPARE(storage->thumbnail().size(), QSize(2, 3));
        storage->setMetaData(QStringLiteral("author"), QStringLiteral("LibrePaint"));
        QCOMPARE(probe.metaDataKey, QStringLiteral("author"));
        QCOMPARE(probe.metaDataValue, QVariant(QStringLiteral("LibrePaint")));
        QCOMPARE(storage->metaDataKeys(), probe.metaDataKeysResult);
        QCOMPARE(storage->metaData(QStringLiteral("revision")), QVariant(17));
        QCOMPARE(probe.requestedMetaDataKey, QStringLiteral("revision"));
        QVERIFY(!storage->isValid());
        QCOMPARE(probe.isValidCalls, 1);
    }
};

QTEST_GUILESS_MAIN(KisStoragePluginContractTest)

#include "KisStoragePluginContractTest.moc"
