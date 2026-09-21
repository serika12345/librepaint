/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisDynamicSensorFactory.h>
#include <KisDynamicSensorFactoryRegistry.h>

#include <QObject>
#include <QString>
#include <QtCore/qtmetamacros.h>
#include <QtTest/qtest.h>
#include <QtTest/qtestcase.h>

namespace
{

struct StoredSensorExpectation {
    const char *id;
    int minimumValue;
    int maximumValue;
};

} // namespace

class KisDynamicSensorRegistryCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void storedInputIdentifiersResolveToExpectedRanges();
};

void KisDynamicSensorRegistryCompatibilityTest::storedInputIdentifiersResolveToExpectedRanges()
{
    // Compatibility requirement: Existing brush presets store these sensor IDs
    // and the curve editor resolves them to the same input ranges.
    const StoredSensorExpectation expectations[] = {
        {"pressure", 0, 100},
        {"fade", 0, 257},
        {"distance", 0, 257},
        {"time", 0, 257},
        {"drawingangle", 0, 360},
    };

    KisDynamicSensorFactoryRegistry *registry = KisDynamicSensorFactoryRegistry::instance();

    for (const StoredSensorExpectation &expectation : expectations) {
        KisDynamicSensorFactory *factory = registry->get(QString::fromLatin1(expectation.id));

        QVERIFY2(factory, expectation.id);
        QCOMPARE(factory->id(), QString::fromLatin1(expectation.id));
        QCOMPARE(factory->minimumValue(), expectation.minimumValue);
        QCOMPARE(factory->maximumValue(257), expectation.maximumValue);
    }
}

QTEST_GUILESS_MAIN(KisDynamicSensorRegistryCompatibilityTest)

#include "KisDynamicSensorRegistryCompatibilityTest.moc"
