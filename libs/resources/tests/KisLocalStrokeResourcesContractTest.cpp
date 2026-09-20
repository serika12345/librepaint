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
    // Consumer: paint presets and filters running with a local stroke resource snapshot.
    // Operation: Resolve saved resource signatures while the snapshot gains and loses typed resources.
    // Observable result: An active exact match wins, legacy filename lookup remains available only to best-match, and missing links retain their signature.
    // Failure impact: A stroke uses the wrong brush or pattern, cannot restore an older document, or cannot report a missing dependency.
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
    QCOMPARE(resources.source<TestResource>(QStringLiteral("brushes"))
                 .bestMatch(QStringLiteral("shared-md5"),
                            QStringLiteral("shared.res"),
                            QStringLiteral("Shared")),
             firstBrush.dynamicCast<TestResource>());
    QCOMPARE(brushes.fallbackResource(), firstBrush);

    auto &presets = resources.source(QStringLiteral("presets"));
    QCOMPARE(presets.exactMatch(QStringLiteral("shared-md5"),
                                QStringLiteral("shared.res"),
                                QStringLiteral("Shared")),
             preset);
    QCOMPARE(presets.fallbackResource(), preset);

    KoResourceSP mismatched = makeResource(QStringLiteral("brushes"),
                                           QStringLiteral("ranked-md5"),
                                           QStringLiteral("other.res"),
                                           QStringLiteral("Other"));
    KoResourceSP inactiveExact = makeResource(QStringLiteral("brushes"),
                                              QStringLiteral("ranked-md5"),
                                              QStringLiteral("wanted.res"),
                                              QStringLiteral("Wanted"));
    inactiveExact->setActive(false);
    KoResourceSP activeExact = makeResource(QStringLiteral("brushes"),
                                            QStringLiteral("ranked-md5"),
                                            QStringLiteral("wanted.res"),
                                            QStringLiteral("Wanted"));
    resources.addResource(mismatched);
    resources.addResource(inactiveExact);
    resources.addResource(activeExact);
    QCOMPARE(brushes.bestMatch(QStringLiteral("ranked-md5"),
                               QStringLiteral("wanted.res"),
                               QStringLiteral("Wanted")),
             activeExact);

    KoResourceSP inactiveLegacy = makeResource(QStringLiteral("brushes"),
                                                QStringLiteral("legacy-inactive-md5"),
                                                QStringLiteral("legacy.res"),
                                                QStringLiteral("Wanted"));
    inactiveLegacy->setActive(false);
    KoResourceSP activeLegacy = makeResource(QStringLiteral("brushes"),
                                              QStringLiteral("legacy-active-md5"),
                                              QStringLiteral("legacy.res"),
                                              QStringLiteral("Other"));
    resources.addResource(inactiveLegacy);
    resources.addResource(activeLegacy);
    QCOMPARE(brushes.bestMatch(QStringLiteral("missing-md5"),
                               QStringLiteral("legacy.res"),
                               QStringLiteral("Wanted")),
             activeLegacy);
    QVERIFY(brushes.exactMatch(QStringLiteral("missing-md5"),
                               QStringLiteral("legacy.res"),
                               QStringLiteral("Wanted")).isNull());

    const KoResourceLoadResult missing = brushes.bestMatchLoadResult(
        QStringLiteral("unavailable-md5"), QStringLiteral("missing.res"), QStringLiteral("Missing"));
    QCOMPARE(missing.type(), KoResourceLoadResult::FailedLink);
    QCOMPARE(missing.signature(), KoResourceSignature(QStringLiteral("brushes"),
                                                       QStringLiteral("unavailable-md5"),
                                                       QStringLiteral("missing.res"),
                                                       QStringLiteral("Missing")));

    resources.removeResource(mismatched);
    resources.removeResource(inactiveExact);
    resources.removeResource(activeExact);
    resources.removeResource(inactiveLegacy);
    resources.removeResource(activeLegacy);

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
