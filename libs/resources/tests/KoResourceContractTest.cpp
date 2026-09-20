/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoMD5Generator.h>
#include <KoResource.h>
#include <KoResourceLoadResult.h>

#include <QFile>
#include <QTemporaryDir>
#include <QTest>


void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

namespace
{
class ObservedResource : public KoResource
{
public:
    ObservedResource() = default;

    explicit ObservedResource(const QString &filename)
        : KoResource(filename)
    {
    }

    KoResourceSP clone() const override
    {
        return KoResourceSP(new ObservedResource);
    }

    bool loadFromDevice(QIODevice *device, KisResourcesInterfaceSP) override
    {
        loadedData = device->readAll();
        return loadResult;
    }

    bool saveToDevice(QIODevice *device) const override
    {
        return device->write(payload) == payload.size() && saveResult;
    }

    QPair<QString, QString> resourceType() const override
    {
        return {QStringLiteral("test-resources"), QStringLiteral("observed")};
    }

    QByteArray payload {QByteArrayLiteral("saved-payload")};
    QByteArray loadedData;
    bool loadResult {true};
    bool saveResult {true};
};

class AggregateResource final : public ObservedResource
{
public:
    QList<KoResourceLoadResult> linkedResources(KisResourcesInterfaceSP resourcesInterface) const override
    {
        Q_UNUSED(resourcesInterface);
        return linked;
    }

    QList<KoResourceLoadResult> embeddedResources(KisResourcesInterfaceSP resourcesInterface) const override
    {
        Q_UNUSED(resourcesInterface);
        return embedded;
    }

    QList<KoResourceLoadResult> sideLoadedResources(KisResourcesInterfaceSP resourcesInterface) const override
    {
        Q_UNUSED(resourcesInterface);
        return cleared ? QList<KoResourceLoadResult>() : sideLoaded;
    }

    void clearSideLoadedResources() override
    {
        cleared = true;
    }

    QList<KoResourceLoadResult> linked;
    QList<KoResourceLoadResult> embedded;
    QList<KoResourceLoadResult> sideLoaded;
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

}

class KoResourceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void derivesDisplayNameFromLoadFilename();
    void loadsResourceContent();
    void rejectsMissingAndEmptyFiles();
    void savesResourceContentAndReportsWriteFailures();
    void generatesContentDigestForResourceIdentity();
    void aggregatesAndClearsRelatedResources();
};

void KoResourceContractTest::derivesDisplayNameFromLoadFilename()
{
    // Consumer: resource import and selection interfaces.
    // Operation: Construct a resource for a file selected from a storage location.
    // Observable result: The resource keeps the load path and derives its initial display name from the file name.
    // Failure impact: Imported resources cannot be loaded from their selected path or appear with an unusable name.
    const QString filename = QStringLiteral("folder/original.test");
    const ObservedResource resource(filename);

    QCOMPARE(resource.filename(), filename);
    QCOMPARE(resource.name(), QStringLiteral("original.test"));
}

void KoResourceContractTest::loadsResourceContent()
{
    // Consumer: resource storage import.
    // Operation: Load a non-empty resource file through its resource decoder.
    // Observable result: The decoded file content is available and a decoder failure is reported to the caller.
    // Failure impact: Resource files appear usable after a failed decode or their content is unavailable to painting tools.
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filename = directory.filePath(QStringLiteral("input.test"));
    QFile file(filename);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(QByteArrayLiteral("loaded-payload")), qint64(14));
    file.close();

    ObservedResource resource(filename);
    QVERIFY(resource.load(KisResourcesInterfaceSP()));
    QCOMPARE(resource.loadedData, QByteArrayLiteral("loaded-payload"));

    ObservedResource failed(filename);
    failed.loadResult = false;
    QVERIFY(!failed.load(KisResourcesInterfaceSP()));
}

void KoResourceContractTest::rejectsMissingAndEmptyFiles()
{
    // Consumer: resource import and opening workflows.
    // Operation: Load a missing or empty resource file.
    // Observable result: Loading fails without presenting a resource as valid content.
    // Failure impact: The resource chooser admits unavailable or empty files that cannot be used for painting.
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

void KoResourceContractTest::savesResourceContentAndReportsWriteFailures()
{
    // Consumer: palette editing and resource export workflows.
    // Operation: Save a modified resource to its selected file.
    // Observable result: Successful writes replace the stored content, and missing paths or decoder write failures return false.
    // Failure impact: The UI reports a resource as saved although its edited content was not written.
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

    ObservedResource withoutFilename;
    QVERIFY(!withoutFilename.save());

    ObservedResource failed(directory.filePath(QStringLiteral("failed.test")));
    failed.saveResult = false;
    QVERIFY(!failed.save());
}

void KoResourceContractTest::generatesContentDigestForResourceIdentity()
{
    // Consumer: resource storage deduplication and linked-resource resolution.
    // Operation: Request the identity signature of a serializable resource without a stored digest.
    // Observable result: The resource generates a content digest and includes an explicit stored digest in its signature.
    // Failure impact: Duplicate resources are not recognized and saved documents cannot resolve their dependencies.
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

void KoResourceContractTest::aggregatesAndClearsRelatedResources()
{
    // Consumer: resource locator and local stroke setup.
    // Operation: Request linked and embedded dependencies, then transfer side-loaded resources to the shared storage.
    // Observable result: Required resources retain linked-then-embedded order, and transferred side-loaded resources are cleared.
    // Failure impact: Dependent resources load in the wrong order or are repeatedly imported and retained in memory.
    AggregateResource resource;
    resource.linked = {failedResult(QStringLiteral("linked"))};
    resource.embedded = {failedResult(QStringLiteral("embedded"))};
    resource.sideLoaded = {failedResult(QStringLiteral("side"))};
    KisResourcesInterfaceSP resourcesInterface;

    const QList<KoResourceLoadResult> required = resource.requiredResources(resourcesInterface);
    QCOMPARE(required.size(), 2);
    QCOMPARE(required[0].signature().name, QStringLiteral("linked"));
    QCOMPARE(required[1].signature().name, QStringLiteral("embedded"));

    const QList<KoResourceLoadResult> sideLoaded = resource.takeSideLoadedResources(resourcesInterface);
    QCOMPARE(sideLoaded.size(), 1);
    QCOMPARE(sideLoaded[0].signature().name, QStringLiteral("side"));
    QVERIFY(resource.cleared);
    QVERIFY(resource.sideLoadedResources(resourcesInterface).isEmpty());
}

QTEST_GUILESS_MAIN(KoResourceContractTest)

#include "KoResourceContractTest.moc"
