/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisLocalStrokeResources.h>

#include <QTest>

#include <memory>

namespace
{
int safeAssertCount = 0;

class TestResource final : public KoResource
{
public:
    explicit TestResource(const QString &type)
        : m_type(type)
    {
    }

    TestResource(const TestResource &rhs)
        : KoResource(rhs)
        , m_type(rhs.m_type)
    {
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
        return {m_type, QStringLiteral("test")};
    }

private:
    QString m_type;
};

KoResourceSP makeResource(const QString &type,
                          const QString &md5,
                          const QString &filename,
                          const QString &name)
{
    KoResourceSP resource(new TestResource(type));
    resource->setMD5Sum(md5);
    resource->setFilename(filename);
    resource->setName(name);
    return resource;
}
}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    ++safeAssertCount;
}

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("unexpected assertion: %s at %s:%d", assertion, file, line);
}

class KisLocalStrokeResourcesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsEmptyAndSanitizesInitialResources();
    void addsAndRemovesResources();
    void clonesAnIndependentContainerWithSharedResources();
    void providesLiveTypeFilteredSources();
};

void KisLocalStrokeResourcesContractTest::startsEmptyAndSanitizesInitialResources()
{
    safeAssertCount = 0;
    KisLocalStrokeResources empty;
    QVERIFY(empty.resources().isEmpty());

    KoResourceSP first = makeResource(QStringLiteral("brushes"),
                                      QStringLiteral("first-md5"),
                                      QStringLiteral("first.res"),
                                      QStringLiteral("First"));
    KoResourceSP second = makeResource(QStringLiteral("presets"),
                                       QStringLiteral("second-md5"),
                                       QStringLiteral("second.res"),
                                       QStringLiteral("Second"));
    KisLocalStrokeResources initialized({first, KoResourceSP(), second});

    QCOMPARE(safeAssertCount, 1);
    QCOMPARE(initialized.resources(), QList<KoResourceSP>({first, second}));
}

void KisLocalStrokeResourcesContractTest::addsAndRemovesResources()
{
    safeAssertCount = 0;
    KoResourceSP first = makeResource(QStringLiteral("brushes"),
                                      QStringLiteral("first-md5"),
                                      QStringLiteral("first.res"),
                                      QStringLiteral("First"));
    KoResourceSP second = makeResource(QStringLiteral("brushes"),
                                       QStringLiteral("second-md5"),
                                       QStringLiteral("second.res"),
                                       QStringLiteral("Second"));
    KisLocalStrokeResources resources;

    resources.addResource(first);
    resources.addResource(second);
    resources.addResource(first);
    QCOMPARE(resources.resources(), QList<KoResourceSP>({first, second, first}));

    resources.removeResource(first);
    QCOMPARE(resources.resources(), QList<KoResourceSP>({second}));
    resources.removeResource(first);
    QCOMPARE(resources.resources(), QList<KoResourceSP>({second}));

    resources.addResource(KoResourceSP());
    QCOMPARE(safeAssertCount, 1);
    QCOMPARE(resources.resources(), QList<KoResourceSP>({second}));
}

void KisLocalStrokeResourcesContractTest::clonesAnIndependentContainerWithSharedResources()
{
    KoResourceSP resource = makeResource(QStringLiteral("brushes"),
                                         QStringLiteral("shared-md5"),
                                         QStringLiteral("shared.res"),
                                         QStringLiteral("Shared"));
    KisLocalStrokeResources original({resource});
    std::unique_ptr<KisLocalStrokeResources> clone(original.clone());

    QVERIFY(clone);
    QCOMPARE(clone->resources(), QList<KoResourceSP>({resource}));
    clone->removeResource(resource);
    QVERIFY(clone->resources().isEmpty());
    QCOMPARE(original.resources(), QList<KoResourceSP>({resource}));

    KoResourceSP cloneOnly = makeResource(QStringLiteral("brushes"),
                                          QStringLiteral("clone-md5"),
                                          QStringLiteral("clone.res"),
                                          QStringLiteral("Clone"));
    clone->addResource(cloneOnly);
    QCOMPARE(clone->resources(), QList<KoResourceSP>({cloneOnly}));
    QCOMPARE(original.resources(), QList<KoResourceSP>({resource}));
}

void KisLocalStrokeResourcesContractTest::providesLiveTypeFilteredSources()
{
    KoResourceSP firstBrush = makeResource(QStringLiteral("brushes"),
                                           QStringLiteral("shared-md5"),
                                           QStringLiteral("shared.res"),
                                           QStringLiteral("Shared"));
    KoResourceSP preset = makeResource(QStringLiteral("presets"),
                                       QStringLiteral("shared-md5"),
                                       QStringLiteral("shared.res"),
                                       QStringLiteral("Shared"));
    KisLocalStrokeResources resources({firstBrush, preset});

    auto &brushes = resources.source(QStringLiteral("brushes"));
    QCOMPARE(brushes.bestMatch(QStringLiteral("shared-md5"),
                               QStringLiteral("shared.res"),
                               QStringLiteral("Shared")),
             firstBrush);
    QCOMPARE(brushes.fallbackResource(), firstBrush);

    auto &presets = resources.source(QStringLiteral("presets"));
    QCOMPARE(presets.exactMatch(QStringLiteral("shared-md5"),
                                QStringLiteral("shared.res"),
                                QStringLiteral("Shared")),
             preset);
    QCOMPARE(presets.fallbackResource(), preset);

    KoResourceSP secondBrush = makeResource(QStringLiteral("brushes"),
                                            QStringLiteral("second-md5"),
                                            QStringLiteral("second.res"),
                                            QStringLiteral("Second"));
    resources.addResource(secondBrush);
    QCOMPARE(brushes.bestMatch(QStringLiteral("second-md5"),
                               QStringLiteral("second.res"),
                               QStringLiteral("Second")),
             secondBrush);

    resources.removeResource(firstBrush);
    QCOMPARE(brushes.fallbackResource(), secondBrush);
    QVERIFY(brushes.bestMatch(QStringLiteral("shared-md5"),
                              QStringLiteral("shared.res"),
                              QStringLiteral("Shared")).isNull());
}

QTEST_GUILESS_MAIN(KisLocalStrokeResourcesContractTest)

#include "KisLocalStrokeResourcesContractTest.moc"
