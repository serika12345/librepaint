/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <KoResourceBundleManifest.h>

#include <QBuffer>
#include <QIODevice>
#include <QSet>
#include <QTest>

#include <algorithm>
#include <memory>

namespace
{
using ResourceReference = KoResourceBundleManifest::ResourceReference;

class ManifestProbe final : public KoResourceBundleManifest
{
public:
    explicit ManifestProbe(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~ManifestProbe() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};

class RejectingDevice final : public QIODevice
{
public:
    bool open(QIODeviceBase::OpenMode mode) override
    {
        ++openCallCount;
        requestedMode = mode;
        return false;
    }

    int openCallCount{0};
    QIODeviceBase::OpenMode requestedMode{QIODeviceBase::NotOpen};

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
    void resourceReferencePreservesDefaultsAndConstructorValues();
    void mutationsExposeTypesTagsAndFiles();
    void saveAndLoadPreserveManifestSerializationRules();
    void invalidOrUnavailableDevicesReportCurrentResults();
};

void KoResourceBundleManifestContractTest::resourceReferencePreservesDefaultsAndConstructorValues()
{
    const ResourceReference defaults;
    QVERIFY(defaults.resourcePath.isEmpty());
    QVERIFY(defaults.tagList.isEmpty());
    QVERIFY(defaults.fileTypeName.isEmpty());
    QVERIFY(defaults.md5sum.isEmpty());
    QVERIFY(defaults.filenameInBundle.isEmpty());

    const ResourceReference explicitFilename(QStringLiteral("source/ink.gbr"),
                                             {QStringLiteral("ink"), QStringLiteral("favorite")},
                                             QStringLiteral("kis_brushes"),
                                             QStringLiteral("0123456789abcdef"),
                                             42,
                                             QStringLiteral("versions/ink.0002.gbr"));
    QCOMPARE(explicitFilename.resourcePath, QStringLiteral("source/ink.gbr"));
    QCOMPARE(explicitFilename.tagList, QStringList({QStringLiteral("ink"), QStringLiteral("favorite")}));
    QCOMPARE(explicitFilename.fileTypeName, QStringLiteral("kis_brushes"));
    QCOMPARE(explicitFilename.md5sum, QStringLiteral("0123456789abcdef"));
    QCOMPARE(explicitFilename.resourceId, 42);
    QCOMPARE(explicitFilename.filenameInBundle, QStringLiteral("versions/ink.0002.gbr"));

    const ResourceReference pathAsFilename(QStringLiteral("pattern.pat"),
                                           {},
                                           QStringLiteral("ko_patterns"),
                                           QStringLiteral("fedcba9876543210"));
    QCOMPARE(pathAsFilename.resourceId, -1);
    QCOMPARE(pathAsFilename.filenameInBundle, pathAsFilename.resourcePath);
}

void KoResourceBundleManifestContractTest::mutationsExposeTypesTagsAndFiles()
{
    bool destroyed = false;

    {
        std::unique_ptr<KoResourceBundleManifest> manifest(new ManifestProbe(&destroyed));
        QVERIFY(manifest->types().isEmpty());
        QVERIFY(manifest->tags().isEmpty());
        QVERIFY(manifest->files().isEmpty());

        manifest->addResource(QStringLiteral("kis_brushes"),
                              QStringLiteral("brush-a.gbr"),
                              {QStringLiteral("ink"), QStringLiteral("favorite")},
                              QStringLiteral("md5-a"),
                              11);
        manifest->addResource(QStringLiteral("kis_brushes"),
                              QStringLiteral("brush-b.gbr"),
                              {QStringLiteral("favorite"), QStringLiteral("texture")},
                              QStringLiteral("md5-b"),
                              12,
                              QStringLiteral("versions/brush-b.0002.gbr"));
        manifest->addResource(QStringLiteral("ko_patterns"),
                              QStringLiteral("pattern.pat"),
                              {QStringLiteral("texture")},
                              QStringLiteral("md5-c"),
                              13);
        manifest->addResource(QStringLiteral("kis_brushes"),
                              QStringLiteral("shared.resource"),
                              {QStringLiteral("shared")},
                              QStringLiteral("md5-d"),
                              14);
        manifest->addResource(QStringLiteral("ko_patterns"),
                              QStringLiteral("shared.resource"),
                              {QStringLiteral("shared")},
                              QStringLiteral("md5-e"),
                              15);

        QCOMPARE(manifest->types(), QStringList({QStringLiteral("kis_brushes"), QStringLiteral("ko_patterns")}));
        QCOMPARE(sorted(manifest->tags()),
                 QStringList({QStringLiteral("favorite"),
                              QStringLiteral("ink"),
                              QStringLiteral("shared"),
                              QStringLiteral("texture")}));
        QCOMPARE(manifest->files().size(), 5);
        QCOMPARE(manifest->files(QStringLiteral("kis_brushes")).size(), 3);
        QCOMPARE(manifest->files(QStringLiteral("ko_patterns")).size(), 2);
        QVERIFY(manifest->files(QStringLiteral("missing")).isEmpty());

        QList<ResourceReference> brushes = manifest->files(QStringLiteral("kis_brushes"));
        const ResourceReference *versionedBrush = findResource(brushes, QStringLiteral("brush-b.gbr"));
        QVERIFY(versionedBrush);
        QCOMPARE(versionedBrush->resourceId, 12);
        QCOMPARE(versionedBrush->filenameInBundle, QStringLiteral("versions/brush-b.0002.gbr"));

        const ResourceReference *brushToRemoveReference = findResource(brushes, QStringLiteral("brush-a.gbr"));
        QVERIFY(brushToRemoveReference);
        ResourceReference brushToRemove = *brushToRemoveReference;
        manifest->removeResource(brushToRemove);
        QCOMPARE(manifest->files(QStringLiteral("kis_brushes")).size(), 2);
        QCOMPARE(sorted(manifest->tags()),
                 QStringList({QStringLiteral("favorite"), QStringLiteral("shared"), QStringLiteral("texture")}));

        manifest->removeFile(QStringLiteral("shared.resource"));
        QCOMPARE(manifest->files(QStringLiteral("kis_brushes")).size(), 1);
        QCOMPARE(manifest->files(QStringLiteral("ko_patterns")).size(), 1);
        QCOMPARE(sorted(manifest->tags()), QStringList({QStringLiteral("favorite"), QStringLiteral("texture")}));

        manifest->removeFile(QStringLiteral("pattern.pat"));
        QVERIFY(manifest->files(QStringLiteral("ko_patterns")).isEmpty());
        QCOMPARE(manifest->files().size(), 1);
        QCOMPARE(manifest->types(), QStringList({QStringLiteral("kis_brushes"), QStringLiteral("ko_patterns")}));

        manifest->removeFile(QStringLiteral("not-present"));
        QCOMPARE(manifest->files().size(), 1);
        QVERIFY(!destroyed);
    }

    QVERIFY(destroyed);
}

void KoResourceBundleManifestContractTest::saveAndLoadPreserveManifestSerializationRules()
{
    KoResourceBundleManifest source;
    source.addResource(QStringLiteral("kis_brushes"),
                       QStringLiteral("source/ink.gbr"),
                       {QStringLiteral("ink & wash"), QStringLiteral("favorite")},
                       QStringLiteral("616263"),
                       44,
                       QStringLiteral("versions/ink.0002.gbr"));
    source.addResource(QStringLiteral("ko_patterns"),
                       QStringLiteral("pattern.pat"),
                       {QStringLiteral("texture")},
                       QStringLiteral("646566"),
                       45);

    QBuffer output;
    QVERIFY(source.save(&output));
    QVERIFY(output.isOpen());
    QVERIFY(output.isWritable());
    const QByteArray xml = output.data();
    QVERIFY(xml.startsWith("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
    QVERIFY(xml.contains("xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\""));
    QVERIFY(xml.contains("manifest:version=\"1.2\""));
    QVERIFY(xml.contains("manifest:full-path=\"/\""));
    QVERIFY(xml.contains("manifest:media-type=\"application/x-krita-resourcebundle\""));
    QVERIFY(xml.contains("manifest:media-type=\"brushes\""));
    QVERIFY(xml.contains("manifest:full-path=\"brushes/versions/ink.0002.gbr\""));
    QVERIFY(xml.contains("manifest:md5sum=\"616263\""));
    QVERIFY(xml.contains("ink &amp; wash"));
    QVERIFY(xml.contains("manifest:media-type=\"patterns\""));
    QVERIFY(xml.contains("manifest:full-path=\"patterns/pattern.pat\""));

    QBuffer input;
    input.setData(xml);
    KoResourceBundleManifest loaded;
    QVERIFY(loaded.load(&input));
    QVERIFY(input.isOpen());
    QVERIFY(input.isReadable());
    QCOMPARE(loaded.types(), QStringList({QStringLiteral("brushes"), QStringLiteral("patterns")}));
    QCOMPARE(loaded.files().size(), 2);

    const QList<ResourceReference> brushes = loaded.files(QStringLiteral("brushes"));
    QCOMPARE(brushes.size(), 1);
    const ResourceReference &brush = brushes.first();
    QCOMPARE(brush.resourcePath, QStringLiteral("brushes/versions/ink.0002.gbr"));
    QCOMPARE(brush.fileTypeName, QStringLiteral("brushes"));
    QCOMPARE(brush.tagList, QStringList({QStringLiteral("ink & wash"), QStringLiteral("favorite")}));
    QCOMPARE(brush.md5sum, QStringLiteral("abc"));
    QCOMPARE(brush.resourceId, -1);
    QCOMPARE(brush.filenameInBundle, brush.resourcePath);

    const QList<ResourceReference> patterns = loaded.files(QStringLiteral("patterns"));
    QCOMPARE(patterns.size(), 1);
    QCOMPARE(patterns.first().resourcePath, QStringLiteral("patterns/pattern.pat"));
    QCOMPARE(patterns.first().md5sum, QStringLiteral("def"));
}

void KoResourceBundleManifestContractTest::invalidOrUnavailableDevicesReportCurrentResults()
{
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
    QCOMPARE(unavailableInput.openCallCount, 1);
    QCOMPARE(unavailableInput.requestedMode, QIODeviceBase::OpenMode(QIODeviceBase::ReadOnly));

    RejectingDevice unavailableOutput;
    QVERIFY(!unavailableManifest.save(&unavailableOutput));
    QCOMPARE(unavailableOutput.openCallCount, 1);
    QCOMPARE(unavailableOutput.requestedMode, QIODeviceBase::OpenMode(QIODeviceBase::WriteOnly));
}

QTEST_GUILESS_MAIN(KoResourceBundleManifestContractTest)

#include "KoResourceBundleManifestContractTest.moc"
