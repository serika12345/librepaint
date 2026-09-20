/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoResource.h>
#include <KoResourceLoadResult.h>

#include <QIODevice>
#include <QTest>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

namespace
{
class TestResource final : public KoResource
{
public:
    TestResource()
    {
        setMD5Sum(QStringLiteral("resource-digest"));
        setFilename(QStringLiteral("resource.test"));
        setName(QStringLiteral("Resource"));
    }

    TestResource(const TestResource &rhs)
        : KoResource(rhs)
    {}

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

KoResourceSignature testSignature()
{
    return KoResourceSignature(
        QStringLiteral("patterns"),
        QStringLiteral("embedded-digest"),
        QStringLiteral("pattern.pat"),
        QStringLiteral("Pattern"));
}

}

class KoResourceLoadResultContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void representsExistingResourceAndTypedAccess();
    void representsEmbeddedResource();
    void representsFailedLink();
};

void KoResourceLoadResultContractTest::representsExistingResourceAndTypedAccess()
{
    // Consumer: paint preset and filter resource resolvers.
    // Operation: Resolve a dependency that is already available in the resource storage.
    // Observable result: The result identifies an existing resource and exposes the typed resource and its signature.
    // Failure impact: A saved preset or filter cannot use an installed dependency during painting or document loading.
    QSharedPointer<TestResource> resource(new TestResource);
    KoResourceLoadResult typedResult(resource);

    QCOMPARE(typedResult.type(), KoResourceLoadResult::ExistingResource);
    QCOMPARE(typedResult.resource(), KoResourceSP(resource));
    QCOMPARE(typedResult.resource<TestResource>(), resource);
    QVERIFY(!typedResult.embeddedResource().isValid());
    QCOMPARE(typedResult.signature(), resource->signature());

    const KoResourceSP baseResource = resource;
    const KoResourceLoadResult baseResult(baseResource);
    QCOMPARE(baseResult.resource(), baseResource);
}

void KoResourceLoadResultContractTest::representsEmbeddedResource()
{
    // Consumer: local stroke snapshot creation and KRA resource import.
    // Operation: Receive a dependency embedded in a document or preset.
    // Observable result: The result identifies embedded content and exposes its bytes and resource signature for import.
    // Failure impact: Embedded brushes, patterns, and filter resources cannot be restored for the document.
    const QByteArray data = QByteArrayLiteral("embedded-data");
    const KoEmbeddedResource embedded(testSignature(), data);
    const KoResourceLoadResult result(embedded);

    QCOMPARE(result.type(), KoResourceLoadResult::EmbeddedResource);
    QVERIFY(result.resource().isNull());
    QCOMPARE(result.embeddedResource().signature(), embedded.signature());
    QCOMPARE(result.embeddedResource().data(), data);
    QCOMPARE(result.signature(), embedded.signature());
}

void KoResourceLoadResultContractTest::representsFailedLink()
{
    // Consumer: preset loading, document saving, and resource-loading diagnostics.
    // Operation: Resolve a linked dependency that is unavailable from the resource storage.
    // Observable result: The result identifies the failed link and preserves its signature for recovery or warning output.
    // Failure impact: Missing dependencies are mistaken for valid resources or cannot be identified to the user.
    const KoResourceSignature signature = testSignature();
    const KoResourceLoadResult result(signature);

    QCOMPARE(result.type(), KoResourceLoadResult::FailedLink);
    QVERIFY(result.resource().isNull());
    QVERIFY(!result.embeddedResource().isValid());
    QCOMPARE(result.signature(), signature);
}

QTEST_GUILESS_MAIN(KoResourceLoadResultContractTest)

#include "KoResourceLoadResultContractTest.moc"
