/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoMD5Generator.h>
#include <KoResource.h>
#include <KoResourceLoadResult.h>

#include <QBuffer>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <type_traits>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

namespace
{
class ObservedResource : public KoResource
{
public:
    explicit ObservedResource(bool *destroyed = nullptr)
        : m_destroyed(destroyed)
    {
    }

    explicit ObservedResource(const QString &filename, bool *destroyed = nullptr)
        : KoResource(filename)
        , m_destroyed(destroyed)
    {
    }

    ObservedResource(const ObservedResource &rhs)
        : KoResource(rhs)
        , payload(rhs.payload)
        , m_destroyed(nullptr)
    {
    }

    ~ObservedResource() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

    KoResourceSP clone() const override
    {
        return KoResourceSP(new ObservedResource(*this));
    }

    bool loadFromDevice(QIODevice *device, KisResourcesInterfaceSP resourcesInterface) override
    {
        loadedData = device->readAll();
        receivedResourcesInterface = resourcesInterface.data();
        return loadResult;
    }

    bool saveToDevice(QIODevice *device) const override
    {
        savedDevice = device;
        return device->write(payload) == payload.size() && saveResult;
    }

    QPair<QString, QString> resourceType() const override
    {
        return {QStringLiteral("test-resources"), QStringLiteral("observed")};
    }

    QByteArray payload {QByteArrayLiteral("saved-payload")};
    QByteArray loadedData;
    KisResourcesInterface *receivedResourcesInterface {nullptr};
    mutable QIODevice *savedDevice {nullptr};
    bool loadResult {true};
    bool saveResult {true};

private:
    bool *m_destroyed;
};

class AggregateResource final : public ObservedResource
{
public:
    QList<KoResourceLoadResult> linkedResources(KisResourcesInterfaceSP resourcesInterface) const override
    {
        receivedInterface = resourcesInterface.data();
        return linked;
    }

    QList<KoResourceLoadResult> embeddedResources(KisResourcesInterfaceSP resourcesInterface) const override
    {
        receivedInterface = resourcesInterface.data();
        return embedded;
    }

    QList<KoResourceLoadResult> sideLoadedResources(KisResourcesInterfaceSP resourcesInterface) const override
    {
        receivedInterface = resourcesInterface.data();
        return cleared ? QList<KoResourceLoadResult>() : sideLoaded;
    }

    void clearSideLoadedResources() override
    {
        cleared = true;
    }

    QList<int> requiredCanvasResources() const override
    {
        return {17, 23};
    }

    QList<KoResourceLoadResult> linked;
    QList<KoResourceLoadResult> embedded;
    QList<KoResourceLoadResult> sideLoaded;
    mutable KisResourcesInterface *receivedInterface {nullptr};
    bool cleared {false};
};

KoResourceLoadResult failedResult(const QString &name)
{
    return KoResourceLoadResult(KoResourceSignature(
        QStringLiteral("test-resources"),
        QStringLiteral("digest-") + name,
        name + QStringLiteral(".test"),
        name));
}

QString resourceDebugText(const KoResourceSP &resource)
{
    QString text;
    QDebug(&text) << resource;
    return text;
}
}

class KoResourceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesDefaultAndMutableState();
    void constructsFromFilenameAndCopiesState();
    void loadsExistingFileThroughVirtualBoundary();
    void rejectsMissingAndEmptyFiles();
    void savesThroughVirtualBoundary();
    void generatesAndOverridesMd5();
    void preservesDefaultVirtualBehavior();
    void aggregatesAndClearsRelatedResources();
    void comparesHashesAndFormatsSharedPointers();
    void clonesAndDestroysPolymorphically();
};

void KoResourceContractTest::preservesDefaultAndMutableState()
{
    ObservedResource resource;

    QCOMPARE(resource.version(), -1);
    QCOMPARE(resource.resourceId(), -1);
    QVERIFY(!resource.valid());
    QVERIFY(resource.active());
    QVERIFY(!resource.permanent());
    QVERIFY(!resource.isDirty());
    QVERIFY(resource.filename().isEmpty());
    QVERIFY(resource.name().isEmpty());
    QVERIFY(resource.storageLocation().isEmpty());
    QVERIFY(resource.metadata().isEmpty());
    QVERIFY(resource.image().isNull());

    resource.setVersion(7);
    resource.setResourceId(11);
    resource.setValid(true);
    resource.setActive(false);
    resource.setPermanent(true);
    resource.setDirty(true);
    resource.setFilename(QStringLiteral("folder/resource.test"));
    resource.setName(QStringLiteral("Named Resource"));
    resource.setStorageLocation(QStringLiteral("bundle://storage"));
    resource.addMetaData(QStringLiteral("author"), QStringLiteral("LibrePaint"));
    QImage image(2, 3, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::green);
    resource.setImage(image);

    QCOMPARE(resource.version(), 7);
    QCOMPARE(resource.resourceId(), 11);
    QVERIFY(resource.valid());
    QVERIFY(!resource.active());
    QVERIFY(resource.permanent());
    QVERIFY(resource.isDirty());
    QCOMPARE(resource.filename(), QStringLiteral("resource.test"));
    QCOMPARE(resource.name(), QStringLiteral("Named Resource"));
    QCOMPARE(resource.storageLocation(), QStringLiteral("bundle://storage"));
    QCOMPARE(resource.metadata().value(QStringLiteral("author")).toString(), QStringLiteral("LibrePaint"));
    QCOMPARE(resource.image(), image);
}

void KoResourceContractTest::constructsFromFilenameAndCopiesState()
{
    const QString filename = QStringLiteral("folder/original.test");
    ObservedResource original(filename);
    original.setVersion(5);
    original.setName(QStringLiteral("Original"));
    original.setMD5Sum(QStringLiteral("copy-digest"));
    original.payload = QByteArrayLiteral("copy-payload");

    QCOMPARE(original.filename(), filename);
    QCOMPARE(ObservedResource(filename).name(), QStringLiteral("original.test"));

    ObservedResource copied(original);
    QCOMPARE(copied.filename(), filename);
    QCOMPARE(copied.name(), QStringLiteral("Original"));
    QCOMPARE(copied.version(), 5);
    QCOMPARE(copied.md5Sum(false), QStringLiteral("copy-digest"));
    QCOMPARE(copied.payload, QByteArrayLiteral("copy-payload"));
    QVERIFY(!std::is_copy_assignable_v<KoResource>);
}

void KoResourceContractTest::loadsExistingFileThroughVirtualBoundary()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filename = directory.filePath(QStringLiteral("input.test"));
    QFile file(filename);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(QByteArrayLiteral("loaded-payload")), qint64(14));
    file.close();

    auto *interfacePointer = reinterpret_cast<KisResourcesInterface *>(quintptr(1));
    KisResourcesInterfaceSP resourcesInterface(interfacePointer, [](KisResourcesInterface *) {});
    ObservedResource resource(filename);
    QVERIFY(resource.load(resourcesInterface));
    QCOMPARE(resource.loadedData, QByteArrayLiteral("loaded-payload"));
    QCOMPARE(resource.receivedResourcesInterface, interfacePointer);

    QBuffer directBuffer;
    directBuffer.setData(QByteArrayLiteral("direct-payload"));
    QVERIFY(directBuffer.open(QIODevice::ReadOnly));
    QVERIFY(resource.loadFromDevice(&directBuffer, resourcesInterface));
    QCOMPARE(resource.loadedData, QByteArrayLiteral("direct-payload"));

    ObservedResource failed(filename);
    failed.loadResult = false;
    QVERIFY(!failed.load(resourcesInterface));
}

void KoResourceContractTest::rejectsMissingAndEmptyFiles()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    ObservedResource missing(directory.filePath(QStringLiteral("missing.test")));
    QVERIFY(!missing.load(KisResourcesInterfaceSP()));

    const QString emptyFilename = directory.filePath(QStringLiteral("empty.test"));
    QFile emptyFile(emptyFilename);
    QVERIFY(emptyFile.open(QIODevice::WriteOnly));
    emptyFile.close();
    ObservedResource empty(emptyFilename);
    QVERIFY(!empty.load(KisResourcesInterfaceSP()));
}

void KoResourceContractTest::savesThroughVirtualBoundary()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filename = directory.filePath(QStringLiteral("output.test"));
    QFile previousFile(filename);
    QVERIFY(previousFile.open(QIODevice::WriteOnly));
    QCOMPARE(previousFile.write(QByteArrayLiteral("previous-long-payload")), qint64(21));
    previousFile.close();

    ObservedResource resource(filename);
    resource.payload = QByteArrayLiteral("output-payload");
    QVERIFY(resource.save());

    QFile file(filename);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArrayLiteral("output-payload"));
    QVERIFY(resource.savedDevice != nullptr);

    ObservedResource withoutFilename;
    QVERIFY(!withoutFilename.save());

    ObservedResource failed(directory.filePath(QStringLiteral("failed.test")));
    failed.saveResult = false;
    QVERIFY(failed.save());

    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QVERIFY(resource.KoResource::saveToDevice(&buffer));
    QVERIFY(buffer.data().isEmpty());
}

void KoResourceContractTest::generatesAndOverridesMd5()
{
    ObservedResource resource;
    resource.payload = QByteArrayLiteral("abc");

    QVERIFY(resource.md5Sum(false).isEmpty());
    QCOMPARE(resource.md5Sum(), QStringLiteral("900150983cd24fb0d6963f7d28e17f72"));
    resource.setMD5Sum(QStringLiteral("explicit-digest"));
    QCOMPARE(resource.md5Sum(false), QStringLiteral("explicit-digest"));

    resource.setFilename(QStringLiteral("signature.test"));
    resource.setName(QStringLiteral("Signature"));
    const KoResourceSignature signature = resource.signature();
    QCOMPARE(signature.type, QStringLiteral("test-resources"));
    QCOMPARE(signature.md5sum, QStringLiteral("explicit-digest"));
    QCOMPARE(signature.filename, QStringLiteral("signature.test"));
    QCOMPARE(signature.name, QStringLiteral("Signature"));
}

void KoResourceContractTest::preservesDefaultVirtualBehavior()
{
    ObservedResource resource;
    QImage image(3, 2, QImage::Format_RGB32);
    image.fill(Qt::blue);
    resource.setImage(image);

    resource.updateThumbnail();
    QCOMPARE(resource.thumbnail(), image);
    QVERIFY(resource.thumbnailPath().isEmpty());
    QVERIFY(resource.defaultFileExtension().isEmpty());
    QVERIFY(!resource.isEphemeral());
    QVERIFY(resource.isSerializable());
    QVERIFY(resource.linkedResources(KisResourcesInterfaceSP()).isEmpty());
    QVERIFY(resource.embeddedResources(KisResourcesInterfaceSP()).isEmpty());
    QVERIFY(resource.sideLoadedResources(KisResourcesInterfaceSP()).isEmpty());
    resource.clearSideLoadedResources();
    QVERIFY(resource.requiredCanvasResources().isEmpty());
    QCOMPARE(resource.resourceType(), qMakePair(QStringLiteral("test-resources"), QStringLiteral("observed")));
}

void KoResourceContractTest::aggregatesAndClearsRelatedResources()
{
    AggregateResource resource;
    resource.linked = {failedResult(QStringLiteral("linked"))};
    resource.embedded = {failedResult(QStringLiteral("embedded"))};
    resource.sideLoaded = {failedResult(QStringLiteral("side"))};
    auto *interfacePointer = reinterpret_cast<KisResourcesInterface *>(quintptr(2));
    KisResourcesInterfaceSP resourcesInterface(interfacePointer, [](KisResourcesInterface *) {});

    const QList<KoResourceLoadResult> required = resource.requiredResources(resourcesInterface);
    QCOMPARE(required.size(), 2);
    QCOMPARE(required[0].signature().name, QStringLiteral("linked"));
    QCOMPARE(required[1].signature().name, QStringLiteral("embedded"));
    QCOMPARE(resource.receivedInterface, interfacePointer);
    QCOMPARE(resource.requiredCanvasResources(), QList<int>({17, 23}));

    const QList<KoResourceLoadResult> sideLoaded = resource.takeSideLoadedResources(resourcesInterface);
    QCOMPARE(sideLoaded.size(), 1);
    QCOMPARE(sideLoaded[0].signature().name, QStringLiteral("side"));
    QVERIFY(resource.cleared);
    QVERIFY(resource.sideLoadedResources(resourcesInterface).isEmpty());
}

void KoResourceContractTest::comparesHashesAndFormatsSharedPointers()
{
    KoResourceSP first(new ObservedResource);
    KoResourceSP second(new ObservedResource);
    first->setMD5Sum(QStringLiteral("same-digest"));
    second->setMD5Sum(QStringLiteral("same-digest"));
    first->setName(QStringLiteral("First"));
    first->setFilename(QStringLiteral("first.test"));
    first->setVersion(3);
    first->setValid(true);
    first->setStorageLocation(QStringLiteral("memory"));

    QVERIFY(*first == *second);
    QCOMPARE(qHash(*first), qHash(*second));
    second->setMD5Sum(QStringLiteral("different-digest"));
    QVERIFY(*first != *second);

    const QString text = resourceDebugText(first);
    QVERIFY(text.contains(QStringLiteral("[RESOURCE] Name: \"First\"")));
    QVERIFY(text.contains(QStringLiteral("Version: 3")));
    QVERIFY(text.contains(QStringLiteral("Filename: \"first.test\"")));
    QVERIFY(text.contains(QStringLiteral("MD5: \"same-digest\"")));
    QVERIFY(text.contains(QStringLiteral("Valid: true")));
    QVERIFY(text.contains(QStringLiteral("Storage: \"memory\"")));
    QCOMPARE(resourceDebugText(KoResourceSP()), QStringLiteral("NULL Resource  "));
}

void KoResourceContractTest::clonesAndDestroysPolymorphically()
{
    bool destroyed = false;
    KoResourceSP clone;
    {
        KoResourceSP resource(new ObservedResource(&destroyed));
        resource->setName(QStringLiteral("Clone Source"));
        resource->setMD5Sum(QStringLiteral("clone-digest"));
        clone = resource->clone();
        QCOMPARE(clone->name(), QStringLiteral("Clone Source"));
        QCOMPARE(clone->md5Sum(false), QStringLiteral("clone-digest"));
        QVERIFY(!destroyed);
    }
    QVERIFY(destroyed);
}

QTEST_GUILESS_MAIN(KoResourceContractTest)

#include "KoResourceContractTest.moc"
