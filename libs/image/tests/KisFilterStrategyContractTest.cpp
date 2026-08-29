/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_filter_strategy.h"

#include <QSize>
#include <QStringList>
#include <QTest>

#include <memory>

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

namespace
{

void compareReal(qreal actual, qreal expected, qreal tolerance = 1e-12)
{
    QVERIFY2(qAbs(actual - expected) <= tolerance,
             qPrintable(QStringLiteral("actual=%1 expected=%2 tolerance=%3")
                            .arg(actual, 0, 'g', 16)
                            .arg(expected, 0, 'g', 16)
                            .arg(tolerance, 0, 'g', 16)));
}

class DestructionProbe final : public KisFilterStrategy
{
public:
    explicit DestructionProbe(int &destructionCount)
        : KisFilterStrategy(KoID(QStringLiteral("probe"), QStringLiteral("Probe")))
        , m_destructionCount(destructionCount)
    {
    }

    ~DestructionProbe() override
    {
        ++m_destructionCount;
    }

    qreal valueAt(qreal t, qreal weightsPositionScale) const override
    {
        return t * weightsPositionScale;
    }

private:
    int &m_destructionCount;
};

class RegistryDestructionProbe final : public KisFilterStrategyRegistry
{
public:
    explicit RegistryDestructionProbe(int &destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~RegistryDestructionProbe() override
    {
        ++m_destructionCount;
    }

private:
    int &m_destructionCount;
};

} // namespace

class KisFilterStrategyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseStrategyPreservesIdentityDefaultsAndVirtualLifetime();
    void hermiteUsesSymmetricCubicKernel();
    void bicubicUsesFixedPointCubicKernel();
    void bicubicRealKernelRemainsBaseZero();
    void boxUsesScaledHalfOpenSupport();
    void bilinearUsesLinearFloatAndFixedPointKernels();
    void bellAndBSplineUseSymmetricPiecewiseKernels();
    void lanczosUsesSymmetricThreePixelWindow();
    void mitchellUsesSymmetricPiecewiseKernel();
    void mitchellKeepsNarrowFixedPointSupport();
    void registryOwnsStableStrategiesAndDescriptions();
    void registrySelectsPixelArtAndScaledImages();
};

void KisFilterStrategyContractTest::baseStrategyPreservesIdentityDefaultsAndVirtualLifetime()
{
    KisFilterStrategy base(KoID(QStringLiteral("base"), QStringLiteral("Base Filter")));
    QCOMPARE(base.id(), QStringLiteral("base"));
    QCOMPARE(base.name(), QStringLiteral("Base Filter"));
    QCOMPARE(base.valueAt(0.25, 3.0), 0.0);
    QCOMPARE(base.intValueAt(64, 3.0), 0);
    QCOMPARE(base.support(3.0), 0.0);
    QCOMPARE(base.intSupport(3.0), 0);
    QVERIFY(base.description().isEmpty());

    int destructionCount = 0;
    std::unique_ptr<KisFilterStrategy> probe = std::make_unique<DestructionProbe>(destructionCount);
    QCOMPARE(probe->id(), QStringLiteral("probe"));
    QCOMPARE(probe->intValueAt(64, 2.0), 127);
    probe.reset();
    QCOMPARE(destructionCount, 1);
}

void KisFilterStrategyContractTest::hermiteUsesSymmetricCubicKernel()
{
    KisHermiteFilterStrategy strategy;
    KisFilterStrategy const &filter = strategy;

    QCOMPARE(strategy.id(), QStringLiteral("Hermite"));
    QVERIFY(!strategy.name().isEmpty());
    QCOMPARE(strategy.support(4.0), 1.0);
    QCOMPARE(strategy.intSupport(4.0), 256);
    compareReal(filter.valueAt(0.0, 1.0), 1.0);
    compareReal(filter.valueAt(0.5, 1.0), 0.5);
    compareReal(filter.valueAt(-0.5, 4.0), 0.5);
    QCOMPARE(filter.valueAt(1.0, 1.0), 0.0);
    QCOMPARE(filter.intValueAt(0, 1.0), 255);
    QCOMPARE(filter.intValueAt(128, 1.0), 127);
    QCOMPARE(filter.intValueAt(-128, 3.0), 127);
    QCOMPARE(filter.intValueAt(256, 1.0), 0);
}

void KisFilterStrategyContractTest::bicubicUsesFixedPointCubicKernel()
{
    KisBicubicFilterStrategy strategy;
    KisFilterStrategy const &filter = strategy;

    QCOMPARE(strategy.id(), QStringLiteral("Bicubic"));
    QVERIFY(!strategy.name().isEmpty());
    QVERIFY(!strategy.description().isEmpty());
    QCOMPARE(strategy.support(1.0), 2.0);
    QCOMPARE(strategy.intSupport(1.0), 512);
    QCOMPARE(filter.intValueAt(0, 1.0), 255);
    QCOMPARE(filter.intValueAt(128, 7.0), 143);
    QCOMPARE(filter.intValueAt(-128, 2.0), 143);
    QCOMPARE(filter.intValueAt(256, 1.0), 0);
    QCOMPARE(filter.intValueAt(512, 1.0), 0);
}

void KisFilterStrategyContractTest::bicubicRealKernelRemainsBaseZero()
{
    KisBicubicFilterStrategy strategy;
    KisFilterStrategy const &filter = strategy;

    QCOMPARE(filter.valueAt(0.0, 1.0), 0.0);
}

void KisFilterStrategyContractTest::boxUsesScaledHalfOpenSupport()
{
    KisBoxFilterStrategy strategy;
    KisFilterStrategy &filter = strategy;

    QCOMPARE(strategy.id(), QStringLiteral("NearestNeighbor"));
    QVERIFY(!strategy.name().isEmpty());
    QVERIFY(!strategy.description().isEmpty());
    compareReal(filter.support(2.0), 1.02);
    QCOMPARE(filter.intSupport(1.5), 194);
    QCOMPARE(filter.valueAt(-1.0, 2.0), 1.0);
    QCOMPARE(filter.valueAt(0.999, 2.0), 1.0);
    QCOMPARE(filter.valueAt(1.0, 2.0), 0.0);
    QCOMPARE(filter.intValueAt(-256, 2.0), 255);
    QCOMPARE(filter.intValueAt(255, 2.0), 255);
    QCOMPARE(filter.intValueAt(256, 2.0), 0);
}

void KisFilterStrategyContractTest::bilinearUsesLinearFloatAndFixedPointKernels()
{
    KisBilinearFilterStrategy strategy;
    KisFilterStrategy const &filter = strategy;

    QCOMPARE(strategy.id(), QStringLiteral("Bilinear"));
    QVERIFY(!strategy.name().isEmpty());
    QVERIFY(!strategy.description().isEmpty());
    QCOMPARE(strategy.support(5.0), 1.0);
    QCOMPARE(strategy.intSupport(5.0), 256);
    QCOMPARE(filter.valueAt(0.0, 1.0), 1.0);
    QCOMPARE(filter.valueAt(0.5, 1.0), 0.5);
    QCOMPARE(filter.valueAt(-0.5, 9.0), 0.5);
    QCOMPARE(filter.valueAt(1.0, 1.0), 0.0);
    QCOMPARE(filter.intValueAt(0, 1.0), 255);
    QCOMPARE(filter.intValueAt(128, 1.0), 128);
    QCOMPARE(filter.intValueAt(255, 1.0), 1);
    QCOMPARE(filter.intValueAt(256, 1.0), 0);
}

void KisFilterStrategyContractTest::bellAndBSplineUseSymmetricPiecewiseKernels()
{
    KisBellFilterStrategy bell;
    KisFilterStrategy const &bellFilter = bell;
    QCOMPARE(bell.id(), QStringLiteral("Bell"));
    QCOMPARE(bell.support(1.0), 1.5);
    QCOMPARE(bell.intSupport(1.0), 384);
    compareReal(bellFilter.valueAt(0.0, 1.0), 0.75);
    compareReal(bellFilter.valueAt(0.5, 1.0), 0.5);
    compareReal(bellFilter.valueAt(-1.0, 3.0), 0.125);
    QCOMPARE(bellFilter.valueAt(1.5, 1.0), 0.0);

    KisBSplineFilterStrategy spline;
    KisFilterStrategy const &splineFilter = spline;
    QCOMPARE(spline.id(), QStringLiteral("BSpline"));
    QCOMPARE(spline.support(1.0), 2.0);
    QCOMPARE(spline.intSupport(1.0), 512);
    compareReal(splineFilter.valueAt(0.0, 1.0), 2.0 / 3.0);
    compareReal(splineFilter.valueAt(1.0, 1.0), 1.0 / 6.0);
    compareReal(splineFilter.valueAt(-1.5, 8.0), 1.0 / 48.0);
    QCOMPARE(splineFilter.valueAt(2.0, 1.0), 0.0);
}

void KisFilterStrategyContractTest::lanczosUsesSymmetricThreePixelWindow()
{
    KisLanczos3FilterStrategy lanczos;
    KisFilterStrategy const &lanczosFilter = lanczos;
    QCOMPARE(lanczos.id(), QStringLiteral("Lanczos3"));
    QVERIFY(!lanczos.description().isEmpty());
    QCOMPARE(lanczos.support(1.0), 3.0);
    QCOMPARE(lanczos.intSupport(1.0), 768);
    compareReal(lanczosFilter.valueAt(0.0, 1.0), 1.0);
    compareReal(lanczosFilter.valueAt(-0.5, 4.0), lanczosFilter.valueAt(0.5, 1.0));
    compareReal(lanczosFilter.valueAt(1.0, 1.0), 0.0, 1e-15);
    QCOMPARE(lanczosFilter.valueAt(3.0, 1.0), 0.0);
}

void KisFilterStrategyContractTest::mitchellUsesSymmetricPiecewiseKernel()
{
    KisMitchellFilterStrategy mitchell;
    KisFilterStrategy const &mitchellFilter = mitchell;
    QCOMPARE(mitchell.id(), QStringLiteral("Mitchell"));
    QCOMPARE(mitchell.support(1.0), 2.0);
    compareReal(mitchellFilter.valueAt(0.0, 1.0), 8.0 / 9.0);
    compareReal(mitchellFilter.valueAt(1.0, 1.0), 1.0 / 18.0);
    compareReal(mitchellFilter.valueAt(-0.5, 6.0), mitchellFilter.valueAt(0.5, 1.0));
    QCOMPARE(mitchellFilter.valueAt(2.0, 1.0), 0.0);
}

void KisFilterStrategyContractTest::mitchellKeepsNarrowFixedPointSupport()
{
    KisMitchellFilterStrategy mitchell;

    QCOMPARE(mitchell.support(1.0), 2.0);
    QCOMPARE(mitchell.intSupport(1.0), 256);
}

void KisFilterStrategyContractTest::registryOwnsStableStrategiesAndDescriptions()
{
    int destructionCount = 0;
    std::unique_ptr<KisFilterStrategyRegistry> emptyRegistry =
        std::make_unique<RegistryDestructionProbe>(destructionCount);
    QVERIFY(emptyRegistry->listKeys().isEmpty());
    emptyRegistry.reset();
    QCOMPARE(destructionCount, 1);

    KisFilterStrategyRegistry *registry = KisFilterStrategyRegistry::instance();
    QCOMPARE(KisFilterStrategyRegistry::instance(), registry);
    QCOMPARE(registry->count(), 8);
    QCOMPARE(registry->value(QStringLiteral("Box")), registry->value(QStringLiteral("NearestNeighbor")));

    QStringList actualIds;
    const QList<KoID> keys = registry->listKeys();
    for (const KoID &key : keys) {
        actualIds.append(key.id());
        QVERIFY(!key.name().isEmpty());
    }
    actualIds.sort();
    const QStringList expectedIds{
        QStringLiteral("BSpline"),
        QStringLiteral("Bell"),
        QStringLiteral("Bicubic"),
        QStringLiteral("Bilinear"),
        QStringLiteral("Hermite"),
        QStringLiteral("Lanczos3"),
        QStringLiteral("Mitchell"),
        QStringLiteral("NearestNeighbor"),
    };
    QCOMPARE(actualIds, expectedIds);

    const QString descriptions = registry->formattedDescriptions();
    QVERIFY(descriptions.startsWith(QStringLiteral("<html><head/><body>")));
    QVERIFY(descriptions.endsWith(QStringLiteral("</body></html>")));
    int describedStrategies = 0;
    for (const QString &id : actualIds) {
        KisFilterStrategy *strategy = registry->value(id);
        if (!strategy->description().isEmpty()) {
            ++describedStrategies;
            QVERIFY(descriptions.contains(strategy->name()));
            QVERIFY(descriptions.contains(strategy->description()));
        }
    }
    QCOMPARE(describedStrategies, 4);
}

void KisFilterStrategyContractTest::registrySelectsPixelArtAndScaledImages()
{
    KisFilterStrategyRegistry *registry = KisFilterStrategyRegistry::instance();
    KisFilterStrategy *nearestNeighbor = registry->value(QStringLiteral("NearestNeighbor"));
    KisFilterStrategy *bicubic = registry->value(QStringLiteral("Bicubic"));

    QCOMPARE(registry->autoFilterStrategy(QSize(256, 1000), QSize(1000, 1000)), nearestNeighbor);
    QCOMPARE(registry->autoFilterStrategy(QSize(512, 512), QSize(512, 512)), nearestNeighbor);
    QCOMPARE(registry->autoFilterStrategy(QSize(512, 512), QSize(1024, 512)), bicubic);
    QCOMPARE(registry->autoFilterStrategy(QSize(512, 512), QSize(256, 512)), bicubic);
}

QTEST_GUILESS_MAIN(KisFilterStrategyContractTest)

#include "KisFilterStrategyContractTest.moc"
