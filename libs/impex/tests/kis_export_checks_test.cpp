/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <ImageSizeCheck.h>
#include <IntegralFrameDuration.h>
#include <KisExportCheckRegistry.h>
#include <KisPreExportChecker.h>

#include <KoColorSpaceRegistry.h>
#include <kis_image.h>
#include <kis_paint_layer.h>

#include <memory>
#include <vector>

namespace
{
KisImageSP createImage(int width, int height)
{
    return new KisImage(nullptr,
                        width,
                        height,
                        KoColorSpaceRegistry::instance()->rgb8(),
                        QStringLiteral("export check contract"));
}

KisImageSP createAnimatedImage(int framerate)
{
    KisImageSP image = createImage(16, 16);
    KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("animated layer"), 255);
    image->addNode(layer);
    layer->enableAnimation();
    image->animationInterface()->setFramerate(framerate);
    return image;
}

class AlwaysNeededCheck final : public KisExportCheckBase
{
public:
    AlwaysNeededCheck(const QString &id, Level level, const QString &warning)
        : KisExportCheckBase(id, level, warning)
    {
    }

    bool checkNeeded(KisImageSP) const override
    {
        return true;
    }

    Level check(KisImageSP) const override
    {
        return m_level;
    }
};

class AlwaysNeededCheckFactory final : public KisExportCheckFactory
{
public:
    KisExportCheckBase *create(KisExportCheckBase::Level level, const QString &customWarning = QString()) override
    {
        const QString warning =
            customWarning.isEmpty() ? QStringLiteral("Unspecified capability warning") : customWarning;
        return new AlwaysNeededCheck(id(), level, warning);
    }

    QString id() const override
    {
        return QStringLiteral("LibrePaintContractCheck");
    }
};

class ScopedFactoryRegistration
{
public:
    explicit ScopedFactoryRegistration(KisExportCheckRegistry *registry)
        : m_registry(registry)
        , m_factory(new AlwaysNeededCheckFactory)
    {
        m_registry->add(m_factory->id(), m_factory);
    }

    ~ScopedFactoryRegistration()
    {
        m_registry->remove(m_factory->id());
        delete m_factory;
    }

    QString id() const
    {
        return m_factory->id();
    }

private:
    KisExportCheckRegistry *m_registry;
    AlwaysNeededCheckFactory *m_factory;
};
} // namespace

class KisExportChecksTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageSizeUsesBothInclusiveLimits();
    void imageSizeFactoryCreatesConfiguredChecks();
    void integralFrameDurationDetection();
    void integralFrameDurationFactory();
    void registryProvidesBuiltInFactories();
    void preExportCheckerClassifiesCapabilityLevels();
};

void KisExportChecksTest::imageSizeUsesBothInclusiveLimits()
{
    ImageSizeCheck check(64, 32, QStringLiteral("PixelDimensions"), KisExportCheckBase::PARTIALLY);

    QCOMPARE(check.id(), QStringLiteral("PixelDimensions"));
    QCOMPARE(check.m_maxW, 64);
    QCOMPARE(check.m_maxH, 32);
    QVERIFY(check.perLayerCheck());
    QVERIFY(check.warning().contains(QStringLiteral("64 x 32")));
    QCOMPARE(check.check(createImage(1, 1)), KisExportCheckBase::PARTIALLY);
    QVERIFY(check.checkNeeded(createImage(64, 32)));
    QVERIFY(check.checkNeeded(createImage(80, 40)));
    QVERIFY(!check.checkNeeded(createImage(63, 32)));
    QVERIFY(!check.checkNeeded(createImage(64, 31)));
    QVERIFY(!check.checkNeeded(createImage(128, 16)));
}

void KisExportChecksTest::imageSizeFactoryCreatesConfiguredChecks()
{
    std::unique_ptr<KisExportCheckFactory> factory = std::make_unique<ImageSizeCheckFactory>();
    QCOMPARE(factory->id(), QStringLiteral("ImageSizeCheck"));

    std::unique_ptr<KisExportCheckBase> defaultBase(
        factory->create(KisExportCheckBase::SUPPORTED, QStringLiteral("Custom dimension warning")));
    auto *defaultCheck = dynamic_cast<ImageSizeCheck *>(defaultBase.get());
    QVERIFY(defaultCheck);
    QCOMPARE(defaultCheck->id(), QStringLiteral("ImageSizeCheck"));
    QCOMPARE(defaultCheck->m_maxW, 100000000);
    QCOMPARE(defaultCheck->m_maxH, 100000000);
    QCOMPARE(defaultCheck->warning(), QStringLiteral("Custom dimension warning"));
    QCOMPARE(defaultCheck->check(createImage(1, 1)), KisExportCheckBase::SUPPORTED);

    ImageSizeCheckFactory configuredFactory;
    std::unique_ptr<KisExportCheckBase> configuredBase(
        configuredFactory.create(512, 256, KisExportCheckBase::UNSUPPORTED));
    auto *configuredCheck = dynamic_cast<ImageSizeCheck *>(configuredBase.get());
    QVERIFY(configuredCheck);
    QCOMPARE(configuredCheck->m_maxW, 512);
    QCOMPARE(configuredCheck->m_maxH, 256);
    QVERIFY(configuredCheck->warning().contains(QStringLiteral("512 x 256")));
    QCOMPARE(configuredCheck->check(createImage(1, 1)), KisExportCheckBase::UNSUPPORTED);
}

void KisExportChecksTest::integralFrameDurationDetection()
{
    IntegralFrameDurationCheck check(QStringLiteral("FrameDuration"), KisExportCheckBase::PARTIALLY);

    QCOMPARE(check.id(), QStringLiteral("FrameDuration"));
    QVERIFY(check.warning().contains(QStringLiteral("fractions of a millisecond")));
    QVERIFY(!check.perLayerCheck());
    QVERIFY(!check.checkNeeded(createImage(16, 16)));
    QVERIFY(!check.checkNeeded(createAnimatedImage(25)));
    QVERIFY(check.checkNeeded(createAnimatedImage(24)));
    QCOMPARE(check.check(createAnimatedImage(24)), KisExportCheckBase::PARTIALLY);
}

void KisExportChecksTest::integralFrameDurationFactory()
{
    std::unique_ptr<KisExportCheckFactory> factory = std::make_unique<IntegralFrameDurationCheckFactory>();

    QCOMPARE(factory->id(), QStringLiteral("IntegralFrameDurationCheck"));
    std::unique_ptr<KisExportCheckBase> base(
        factory->create(KisExportCheckBase::UNSUPPORTED, QStringLiteral("Custom frame warning")));
    auto *check = dynamic_cast<IntegralFrameDurationCheck *>(base.get());
    QVERIFY(check);
    QCOMPARE(check->id(), QStringLiteral("IntegralFrameDurationCheck"));
    QCOMPARE(check->warning(), QStringLiteral("Custom frame warning"));
    QCOMPARE(check->check(createAnimatedImage(24)), KisExportCheckBase::UNSUPPORTED);
}

void KisExportChecksTest::registryProvidesBuiltInFactories()
{
    KisExportCheckRegistry registry;

    QVERIFY(registry.contains(QStringLiteral("ImageSizeCheck")));
    QVERIFY(registry.contains(QStringLiteral("MultiLayerCheck")));
    QVERIFY(registry.contains(QStringLiteral("AnimationCheck")));
    QCOMPARE(registry.get(QStringLiteral("ImageSizeCheck"))->id(), QStringLiteral("ImageSizeCheck"));

    KisExportCheckRegistry *singleton = KisExportCheckRegistry::instance();
    QVERIFY(singleton);
    QCOMPARE(KisExportCheckRegistry::instance(), singleton);
    QVERIFY(singleton->contains(QStringLiteral("ImageSizeCheck")));
}

void KisExportChecksTest::preExportCheckerClassifiesCapabilityLevels()
{
    KisExportCheckRegistry *registry = KisExportCheckRegistry::instance();
    ScopedFactoryRegistration registration(registry);
    const KisImageSP image = createImage(16, 16);

    std::vector<std::unique_ptr<KisExportCheckBase>> ownedChecks;
    QMap<QString, KisExportCheckBase *> supportedChecks;
    for (const QString &id : registry->keys()) {
        if (id == registration.id()) {
            continue;
        }
        ownedChecks.emplace_back(registry->get(id)->create(KisExportCheckBase::SUPPORTED));
        supportedChecks.insert(id, ownedChecks.back().get());
    }

    KisPreExportChecker missingCapability;
    QVERIFY(!missingCapability.check(image, supportedChecks));
    QCOMPARE(missingCapability.warnings(), QStringList({QStringLiteral("Unspecified capability warning")}));
    QVERIFY(missingCapability.errors().isEmpty());

    AlwaysNeededCheck partial(registration.id(),
                              KisExportCheckBase::PARTIALLY,
                              QStringLiteral("Partially supported capability"));
    QMap<QString, KisExportCheckBase *> partialChecks = supportedChecks;
    partialChecks.insert(registration.id(), &partial);
    KisPreExportChecker partialCapability;
    QVERIFY(!partialCapability.check(image, partialChecks));
    QCOMPARE(partialCapability.warnings(), QStringList({QStringLiteral("Partially supported capability")}));
    QVERIFY(partialCapability.errors().isEmpty());

    AlwaysNeededCheck unsupported(registration.id(),
                                  KisExportCheckBase::UNSUPPORTED,
                                  QStringLiteral("Unsupported capability"));
    QMap<QString, KisExportCheckBase *> unsupportedChecks = supportedChecks;
    unsupportedChecks.insert(registration.id(), &unsupported);
    KisPreExportChecker unsupportedCapability;
    QVERIFY(!unsupportedCapability.check(image, unsupportedChecks));
    QVERIFY(unsupportedCapability.warnings().isEmpty());
    QCOMPARE(unsupportedCapability.errors(), QStringList({QStringLiteral("Unsupported capability")}));

    AlwaysNeededCheck supported(registration.id(), KisExportCheckBase::SUPPORTED, QStringLiteral("unused"));
    QMap<QString, KisExportCheckBase *> completeChecks = supportedChecks;
    completeChecks.insert(registration.id(), &supported);
    KisPreExportChecker supportedCapability;
    QVERIFY(supportedCapability.check(image, completeChecks));
    QVERIFY(supportedCapability.warnings().isEmpty());
    QVERIFY(supportedCapability.errors().isEmpty());
}

QTEST_GUILESS_MAIN(KisExportChecksTest)

#include "kis_export_checks_test.moc"
