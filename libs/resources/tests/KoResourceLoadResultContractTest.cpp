/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoResource.h>
#include <KoResourceLoadResult.h>

#include <QDebug>
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
    explicit TestResource(bool *destroyed = nullptr)
        : m_destroyed(destroyed)
    {
        setMD5Sum(QStringLiteral("resource-digest"));
        setFilename(QStringLiteral("resource.test"));
        setName(QStringLiteral("Resource"));
    }

    TestResource(const TestResource &rhs)
        : KoResource(rhs)
        , m_destroyed(nullptr)
    {
    }

    ~TestResource() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

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

private:
    bool *m_destroyed;
};

KoResourceSignature testSignature()
{
    return KoResourceSignature(
        QStringLiteral("patterns"),
        QStringLiteral("embedded-digest"),
        QStringLiteral("pattern.pat"),
        QStringLiteral("Pattern"));
}

QString debugText(const KoResourceLoadResult &result)
{
    QString text;
    QDebug(&text) << result;
    return text;
}
}

class KoResourceLoadResultContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void representsExistingResourceAndTypedAccess();
    void representsEmbeddedResource();
    void representsFailedLink();
    void copiesAssignsAndReleasesResource();
    void formatsAllResultTypes();
};

void KoResourceLoadResultContractTest::representsExistingResourceAndTypedAccess()
{
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
    const KoResourceSignature signature = testSignature();
    const KoResourceLoadResult result(signature);

    QCOMPARE(result.type(), KoResourceLoadResult::FailedLink);
    QVERIFY(result.resource().isNull());
    QVERIFY(!result.embeddedResource().isValid());
    QCOMPARE(result.signature(), signature);
}

void KoResourceLoadResultContractTest::copiesAssignsAndReleasesResource()
{
    bool destroyed = false;
    {
        QSharedPointer<TestResource> resource(new TestResource(&destroyed));
        KoResourceLoadResult original(resource);
        KoResourceLoadResult copied(original);
        KoResourceLoadResult assigned(testSignature());
        assigned = copied;
        resource.clear();

        QVERIFY(!destroyed);
        QCOMPARE(copied.resource<TestResource>(), original.resource<TestResource>());
        QCOMPARE(assigned.resource<TestResource>(), original.resource<TestResource>());
    }
    QVERIFY(destroyed);
}

void KoResourceLoadResultContractTest::formatsAllResultTypes()
{
    QSharedPointer<TestResource> resource(new TestResource);
    const KoResourceLoadResult existing(resource);
    const KoEmbeddedResource embedded(testSignature(), QByteArrayLiteral("embedded-data"));
    const KoResourceLoadResult embeddedResult(embedded);
    const KoResourceLoadResult failed(testSignature());

    QVERIFY(debugText(existing).startsWith(QStringLiteral("KoResourceLoadResult(ExistingResource:")));
    QVERIFY(debugText(existing).contains(QStringLiteral("resource.test")));
    QVERIFY(debugText(embeddedResult).startsWith(QStringLiteral("KoResourceLoadResult(EmbeddedResource:")));
    QVERIFY(debugText(embeddedResult).contains(QStringLiteral("pattern.pat")));
    QVERIFY(debugText(failed).startsWith(QStringLiteral("KoResourceLoadResult(FailedLink:")));
    QVERIFY(debugText(failed).contains(QStringLiteral("pattern.pat")));
}

QTEST_GUILESS_MAIN(KoResourceLoadResultContractTest)

#include "KoResourceLoadResultContractTest.moc"
