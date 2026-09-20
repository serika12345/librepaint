/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QStandardPaths>
#include <QTest>

#include <KConfigGroup>
#include <KSharedConfig>

#include <KisSupportedArchitectures.h>
#include <KoMultiArchBuildSupport.h>

namespace
{
struct ArchitectureNameFactory {
    template<typename Architecture>
    static QString create(const QString &suffix)
    {
        return QString::fromLatin1(Architecture::name()) + suffix;
    }
};
} // namespace

class KisMultiArchPublicApiTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void defaultVectorizationConfiguration();
    void reportedArchitecturesAreConsistent();
    void scalarFactoryUsesGenericArchitecture();
    void optimizedFactoryUsesReportedBestArchitecture();
};

void KisMultiArchPublicApiTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.deleteEntry("amdDisableVectorWorkaround");
    config.deleteEntry("disableAVXOptimizations");
    config.sync();
}

void KisMultiArchPublicApiTest::defaultVectorizationConfiguration()
{
    const auto [useVectorization, disableAVXOptimizations] = vectorizationConfiguration();

    QVERIFY(useVectorization);
    QVERIFY(!disableAVXOptimizations);
}

void KisMultiArchPublicApiTest::reportedArchitecturesAreConsistent()
{
    const QString baseArchitecture = KisSupportedArchitectures::baseArchName();
    const QString bestArchitecture = KisSupportedArchitectures::bestArchName();
    const QStringList supportedArchitectures =
        KisSupportedArchitectures::supportedInstructionSets().split(QLatin1Char(' '), Qt::SkipEmptyParts);

    QCOMPARE(baseArchitecture, QString::fromLatin1(xsimd::current_arch::name()));
    QVERIFY(!bestArchitecture.isEmpty());
    if (bestArchitecture != QStringLiteral("generic")) {
        QVERIFY(supportedArchitectures.contains(bestArchitecture));
    }
}

void KisMultiArchPublicApiTest::scalarFactoryUsesGenericArchitecture()
{
    QCOMPARE(createScalarClass<ArchitectureNameFactory>(QStringLiteral("-scalar")),
             QString::fromLatin1(xsimd::generic::name()) + QStringLiteral("-scalar"));
}

void KisMultiArchPublicApiTest::optimizedFactoryUsesReportedBestArchitecture()
{
    QCOMPARE(createOptimizedClass<ArchitectureNameFactory>(QStringLiteral("-optimized")),
             KisSupportedArchitectures::bestArchName() + QStringLiteral("-optimized"));
}

QTEST_GUILESS_MAIN(KisMultiArchPublicApiTest)

#include "KisMultiArchPublicApiTest.moc"
