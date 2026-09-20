/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <KoResourceBundleManifest.h>

#include <QBuffer>
#include <QIODevice>
#include <QTest>

#include <algorithm>

namespace
{
using ResourceReference = KoResourceBundleManifest::ResourceReference;

class RejectingDevice final : public QIODevice
{
public:
    bool open(QIODeviceBase::OpenMode) override
    {
        return false;
    }

protected:
    qint64 readData(char *, qint64) override
    {
        return -1;
    }

    qint64 writeData(const char *, qint64) override
    {
        return -1;
    }
};

QStringList sorted(QStringList values)
{
    values.sort();
    return values;
}

const ResourceReference *findResource(const QList<ResourceReference> &resources, const QString &resourcePath)
{
    const auto result = std::find_if(resources.cbegin(), resources.cend(), [&](const ResourceReference &resource) {
        return resource.resourcePath == resourcePath;
    });
    return result == resources.cend() ? nullptr : &*result;
}
} // namespace

class KoResourceBundleManifestContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mutationsExposeTypesTagsAndFiles();
    void invalidOrUnavailableDevicesReportCurrentResults();
};

void KoResourceBundleManifestContractTest::mutationsExposeTypesTagsAndFiles()
{
    // Consumer: bundle editing and resource loading.
    // Operation: Add and remove versioned resources, types, and tags in a bundle manifest.
    // Observable result: The resource loader receives the remaining type-specific files and tags.
    // Failure impact: A bundle can expose a deleted asset, hide an available asset, or apply an incorrect tag.
    KoResourceBundleManifest manifest;
    QVERIFY(manifest.types().isEmpty());
    QVERIFY(manifest.tags().isEmpty());
    QVERIFY(manifest.files().isEmpty());

    manifest.addResource(QStringLiteral("kis_brushes"),
                         QStringLiteral("brush-a.gbr"),
                         {QStringLiteral("ink"), QStringLiteral("favorite")},
                         QStringLiteral("md5-a"),
                         11);
    manifest.addResource(QStringLiteral("kis_brushes"),
                         QStringLiteral("brush-b.gbr"),
                         {QStringLiteral("favorite"), QStringLiteral("texture")},
                         QStringLiteral("md5-b"),
                         12,
                         QStringLiteral("versions/brush-b.0002.gbr"));
    manifest.addResource(QStringLiteral("ko_patterns"),
                         QStringLiteral("pattern.pat"),
                         {QStringLiteral("texture")},
                         QStringLiteral("md5-c"),
                         13);
    manifest.addResource(QStringLiteral("kis_brushes"),
                         QStringLiteral("shared.resource"),
                         {QStringLiteral("shared")},
                         QStringLiteral("md5-d"),
                         14);
    manifest.addResource(QStringLiteral("ko_patterns"),
                         QStringLiteral("shared.resource"),
                         {QStringLiteral("shared")},
                         QStringLiteral("md5-e"),
                         15);

    QCOMPARE(sorted(manifest.types()),
             QStringList({QStringLiteral("kis_brushes"), QStringLiteral("ko_patterns")}));
    QCOMPARE(sorted(manifest.tags()),
             QStringList({QStringLiteral("favorite"),
                          QStringLiteral("ink"),
                          QStringLiteral("shared"),
                          QStringLiteral("texture")}));
    QCOMPARE(manifest.files().size(), 5);
    QCOMPARE(manifest.files(QStringLiteral("kis_brushes")).size(), 3);
    QCOMPARE(manifest.files(QStringLiteral("ko_patterns")).size(), 2);
    QVERIFY(manifest.files(QStringLiteral("missing")).isEmpty());

    QList<ResourceReference> brushes = manifest.files(QStringLiteral("kis_brushes"));
    const ResourceReference *versionedBrush = findResource(brushes, QStringLiteral("brush-b.gbr"));
    QVERIFY(versionedBrush);
    QCOMPARE(versionedBrush->resourceId, 12);
    QCOMPARE(versionedBrush->filenameInBundle, QStringLiteral("versions/brush-b.0002.gbr"));

    const ResourceReference *brushToRemoveReference = findResource(brushes, QStringLiteral("brush-a.gbr"));
    QVERIFY(brushToRemoveReference);
    ResourceReference brushToRemove = *brushToRemoveReference;
    manifest.removeResource(brushToRemove);
    QCOMPARE(manifest.files(QStringLiteral("kis_brushes")).size(), 2);
    QCOMPARE(sorted(manifest.tags()),
             QStringList({QStringLiteral("favorite"), QStringLiteral("shared"), QStringLiteral("texture")}));

    manifest.removeFile(QStringLiteral("shared.resource"));
    QCOMPARE(manifest.files(QStringLiteral("kis_brushes")).size(), 1);
    QCOMPARE(manifest.files(QStringLiteral("ko_patterns")).size(), 1);
    QCOMPARE(sorted(manifest.tags()), QStringList({QStringLiteral("favorite"), QStringLiteral("texture")}));

    manifest.removeFile(QStringLiteral("pattern.pat"));
    QVERIFY(manifest.files(QStringLiteral("ko_patterns")).isEmpty());
    QCOMPARE(manifest.files().size(), 1);
    QCOMPARE(sorted(manifest.types()),
             QStringList({QStringLiteral("kis_brushes"), QStringLiteral("ko_patterns")}));

    manifest.removeFile(QStringLiteral("not-present"));
    QCOMPARE(manifest.files().size(), 1);
}

void KoResourceBundleManifestContractTest::invalidOrUnavailableDevicesReportCurrentResults()
{
    // Consumer: bundle import and bundle editing.
    // Operation: Load a malformed manifest or read from and write to an unavailable device.
    // Observable result: The operation fails and a malformed manifest retains no resource entries.
    // Failure impact: A corrupt bundle can appear to contain stale resources or be saved as though it were valid.
    KoResourceBundleManifest malformedManifest;
    malformedManifest.addResource(QStringLiteral("kis_brushes"),
                                  QStringLiteral("existing.gbr"),
                                  {},
                                  QStringLiteral("abc"));
    QBuffer malformedXml;
    malformedXml.setData("<manifest");
    QVERIFY(!malformedManifest.load(&malformedXml));
    QVERIFY(malformedManifest.files().isEmpty());

    KoResourceBundleManifest wrongRootManifest;
    QBuffer wrongRootXml;
    wrongRootXml.setData("<manifest xmlns=\"urn:wrong\"/>");
    QVERIFY(!wrongRootManifest.load(&wrongRootXml));

    const QByteArray invalidEntryXml =
        "<manifest:manifest xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\">"
        "<manifest:file-entry manifest:media-type=\"brushes\" "
        "manifest:full-path=\"brushes/missing-md5.gbr\"/>"
        "</manifest:manifest>";
    KoResourceBundleManifest invalidEntryManifest;
    QBuffer invalidEntryBuffer;
    invalidEntryBuffer.setData(invalidEntryXml);
    QVERIFY(invalidEntryManifest.load(&invalidEntryBuffer));
    QVERIFY(invalidEntryManifest.files().isEmpty());

    KoResourceBundleManifest unavailableManifest;
    RejectingDevice unavailableInput;
    QVERIFY(!unavailableManifest.load(&unavailableInput));

    RejectingDevice unavailableOutput;
    QVERIFY(!unavailableManifest.save(&unavailableOutput));
}

QTEST_GUILESS_MAIN(KoResourceBundleManifestContractTest)

#include "KoResourceBundleManifestContractTest.moc"
