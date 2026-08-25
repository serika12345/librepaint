/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QBuffer>
#include <QColor>
#include <QDomDocument>
#include <QImage>
#include <QTest>
#include <QUuid>

#include <workspace/KisSessionResource.h>
#include <workspace/kis_workspace_resource.h>

class KisWorkspacePersistenceTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void workspaceXmlRoundTripRemainsStable();
    void sessionViewParametersRemainCompatible();
};

void KisWorkspacePersistenceTest::workspaceXmlRoundTripRemainsStable()
{
    KisWorkspaceResource original("workspace.kws");
    original.setName("Painting workspace");
    original.setDockerState(QByteArray::fromHex("00017fff8042"));
    original.setProperty("dockerVisibility", QString("layers:on"));
    original.setProperty("toolbarCount", 3);

    QImage thumbnail(2, 2, QImage::Format_ARGB32_Premultiplied);
    thumbnail.fill(QColor(12, 34, 56, 255));
    original.setImage(thumbnail);

    QByteArray serialized;
    QBuffer output(&serialized);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QVERIFY(original.saveToDevice(&output));
    output.close();

    QDomDocument document;
    QVERIFY(document.setContent(serialized));
    const QDomElement root = document.documentElement();
    QCOMPARE(root.tagName(), QString("Workspace"));
    QCOMPARE(root.attribute("name"), QString("Painting workspace"));
    QCOMPARE(root.attribute("version"), QString("1"));
    QCOMPARE(
        QByteArray::fromBase64(
            root.firstChildElement("state").text().toLatin1()),
        original.dockerState());
    QCOMPARE(
        root.firstChildElement("settings")
            .firstChildElement("param")
            .tagName(),
        QString("param"));
    QCOMPARE(root.firstChildElement("image").isNull(), false);

    KisWorkspaceResource restored("restored.kws");
    QBuffer input(&serialized);
    QVERIFY(input.open(QIODevice::ReadOnly));
    QVERIFY(restored.loadFromDevice(&input, KisResourcesInterfaceSP()));

    QCOMPARE(restored.name(), original.name());
    QCOMPARE(restored.dockerState(), original.dockerState());
    QCOMPARE(restored.getString("dockerVisibility"), QString("layers:on"));
    QCOMPARE(restored.getInt("toolbarCount"), 3);
    QCOMPARE(restored.image().size(), thumbnail.size());
    for (int y = 0; y < thumbnail.height(); ++y) {
        for (int x = 0; x < thumbnail.width(); ++x) {
            QCOMPARE(restored.image().pixelColor(x, y), thumbnail.pixelColor(x, y));
        }
    }
}

void KisWorkspacePersistenceTest::sessionViewParametersRemainCompatible()
{
    const QByteArray storedGeometry("saved-geometry");
    const QUuid windowId("{12345678-1234-5678-9abc-123456789abc}");
    const QByteArray serialized = QString(
        "<WindowLayout name=\"Session\" version=\"1\">"
        "<view window=\"%1\" src=\"file:///tmp/session-document.kra\">"
        "<param name=\"zoom\" type=\"internal\">1.25</param>"
        "<param name=\"window\" type=\"string\"><![CDATA[main]]></param>"
        "<param name=\"geometry\" type=\"bytearray\">%2</param>"
        "</view><session profile=\"artist\"/>"
        "</WindowLayout>")
        .arg(windowId.toString(), QString::fromLatin1(storedGeometry.toBase64()))
        .toUtf8();

    KisSessionResource session("session.ksn");
    QBuffer input;
    input.setData(serialized);
    QVERIFY(input.open(QIODevice::ReadOnly));
    QVERIFY(session.loadFromDevice(&input, KisResourcesInterfaceSP()));

    QByteArray roundTripped;
    QBuffer output(&roundTripped);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QVERIFY(session.saveToDevice(&output));
    output.close();

    QDomDocument document;
    QVERIFY(document.setContent(roundTripped));
    const QDomElement view = document.documentElement().firstChildElement("view");
    QCOMPARE(view.attribute("window"), windowId.toString());
    QCOMPARE(view.attribute("src"), QString("file:///tmp/session-document.kra"));

    const QDomElement geometry = view.firstChildElement("param");
    QCOMPARE(geometry.attribute("name"), QString("geometry"));
    QCOMPARE(geometry.attribute("type"), QString("bytearray"));
    QCOMPARE(
        QByteArray::fromBase64(geometry.text().toLatin1()),
        storedGeometry);

    const QDomElement window = geometry.nextSiblingElement("param");
    QCOMPARE(window.attribute("name"), QString("window"));
    QCOMPARE(window.attribute("type"), QString("string"));
    QCOMPARE(window.text(), QString("main"));

    const QDomElement zoom = window.nextSiblingElement("param");
    QCOMPARE(zoom.attribute("name"), QString("zoom"));
    QCOMPARE(zoom.attribute("type"), QString("string"));
    QCOMPARE(zoom.text(), QString("1.25"));
}

QTEST_MAIN(KisWorkspacePersistenceTest)

#include "KisWorkspacePersistenceTest.moc"
