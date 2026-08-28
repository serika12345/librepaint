/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoLocalStrokeCanvasResources.h>

#include <QTest>

namespace
{
class DestructionTrackedCanvasResources final : public KoLocalStrokeCanvasResources
{
public:
    explicit DestructionTrackedCanvasResources(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~DestructionTrackedCanvasResources() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};
}

class KoLocalStrokeCanvasResourcesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsEmptyAndSupportsSharedPolymorphicLifetime();
    void storesAndReplacesValues();
    void copiesIndependentState();
    void assignmentReplacesWithIndependentStateAndReturnsSelf();
};

void KoLocalStrokeCanvasResourcesContractTest::startsEmptyAndSupportsSharedPolymorphicLifetime()
{
    bool destroyed = false;
    KoLocalStrokeCanvasResourcesSP concrete(new DestructionTrackedCanvasResources(&destroyed));
    KoCanvasResourcesInterfaceSP resources = concrete;
    concrete.clear();

    QVERIFY(!destroyed);
    QVERIFY(!resources->resource(173).isValid());
    resources.clear();
    QVERIFY(destroyed);
}

void KoLocalStrokeCanvasResourcesContractTest::storesAndReplacesValues()
{
    KoLocalStrokeCanvasResources resources;

    resources.storeResource(17, QStringLiteral("first"));
    resources.storeResource(23, 42);
    QCOMPARE(resources.resource(17), QVariant(QStringLiteral("first")));
    QCOMPARE(resources.resource(23), QVariant(42));

    resources.storeResource(17, QStringLiteral("replacement"));
    QCOMPARE(resources.resource(17), QVariant(QStringLiteral("replacement")));
    QVERIFY(!resources.resource(99).isValid());
}

void KoLocalStrokeCanvasResourcesContractTest::copiesIndependentState()
{
    KoLocalStrokeCanvasResources original;
    original.storeResource(17, QStringLiteral("shared-value"));

    KoLocalStrokeCanvasResources copy(original);
    QCOMPARE(copy.resource(17), QVariant(QStringLiteral("shared-value")));

    original.storeResource(17, QStringLiteral("original-only"));
    copy.storeResource(23, QStringLiteral("copy-only"));
    QCOMPARE(copy.resource(17), QVariant(QStringLiteral("shared-value")));
    QVERIFY(!original.resource(23).isValid());
}

void KoLocalStrokeCanvasResourcesContractTest::assignmentReplacesWithIndependentStateAndReturnsSelf()
{
    KoLocalStrokeCanvasResources source;
    source.storeResource(17, QStringLiteral("source"));
    KoLocalStrokeCanvasResources destination;
    destination.storeResource(23, QStringLiteral("discarded"));

    KoLocalStrokeCanvasResources *result = &(destination = source);
    QCOMPARE(result, &destination);
    QCOMPARE(destination.resource(17), QVariant(QStringLiteral("source")));
    QVERIFY(!destination.resource(23).isValid());

    destination.storeResource(17, QStringLiteral("destination-only"));
    QCOMPARE(source.resource(17), QVariant(QStringLiteral("source")));
    QCOMPARE(&(destination = destination), &destination);
    QCOMPARE(destination.resource(17), QVariant(QStringLiteral("destination-only")));
}

QTEST_GUILESS_MAIN(KoLocalStrokeCanvasResourcesContractTest)

#include "KoLocalStrokeCanvasResourcesContractTest.moc"
