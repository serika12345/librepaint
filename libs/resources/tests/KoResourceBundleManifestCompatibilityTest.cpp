/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <KoResourceBundleManifest.h>

#include <QBuffer>
#include <QTest>

class KoResourceBundleManifestCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void savesAndLoadsBundleManifestXml();
};

void KoResourceBundleManifestCompatibilityTest::savesAndLoadsBundleManifestXml()
{
    // Compatibility requirement: resource bundle files and bundle importers depend on the manifest XML entry names and values.
    // Consumer: bundle export and import.
    // Operation: Save a versioned brush and pattern to the bundle manifest, then load that XML.
    // Observable result: The XML preserves its namespace, root entry, resource type, path, MD5, and tags, and importing restores both resources.
    // Failure impact: A saved bundle cannot be imported or resolves a resource to the wrong file, content, or tag.
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
    QCOMPARE(loaded.files().size(), 2);

    const auto brushes = loaded.files(QStringLiteral("brushes"));
    QCOMPARE(brushes.size(), 1);
    const auto &brush = brushes.first();
    QCOMPARE(brush.resourcePath, QStringLiteral("brushes/versions/ink.0002.gbr"));
    QCOMPARE(brush.fileTypeName, QStringLiteral("brushes"));
    QCOMPARE(brush.tagList, QStringList({QStringLiteral("ink & wash"), QStringLiteral("favorite")}));
    QCOMPARE(brush.md5sum, QStringLiteral("abc"));
    QCOMPARE(brush.filenameInBundle, brush.resourcePath);

    const auto patterns = loaded.files(QStringLiteral("patterns"));
    QCOMPARE(patterns.size(), 1);
    QCOMPARE(patterns.first().resourcePath, QStringLiteral("patterns/pattern.pat"));
    QCOMPARE(patterns.first().md5sum, QStringLiteral("def"));
}

QTEST_GUILESS_MAIN(KoResourceBundleManifestCompatibilityTest)

#include "KoResourceBundleManifestCompatibilityTest.moc"
