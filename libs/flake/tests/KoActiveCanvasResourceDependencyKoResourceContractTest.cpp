/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoActiveCanvasResourceDependencyKoResource.h"

#include <QSet>
#include <QSharedPointer>
#include <QTest>
#include <QVariant>

class ResourceProbe
{
public:
    const QSet<int> &requiredCanvasResources() const
    {
        ++queryCount;
        return requiredKeys;
    }

    QSet<int> requiredKeys;
    mutable int queryCount = 0;
};

using ResourceProbeSP = QSharedPointer<ResourceProbe>;
Q_DECLARE_METATYPE(ResourceProbeSP)

class KoActiveCanvasResourceDependencyKoResourceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesSignedKeysThroughBase();
    void updatesOnlyWhenSourceResourceRequiresTargetKey();
    void nullAndInvalidVariantsDoNotRequestUpdate();
};

void KoActiveCanvasResourceDependencyKoResourceContractTest::preservesSignedKeysThroughBase()
{
    KoActiveCanvasResourceDependencyKoResource<ResourceProbe> dependency(-41, 73);
    const KoActiveCanvasResourceDependency *base = &dependency;

    QCOMPARE(base->sourceKey(), -41);
    QCOMPARE(base->targetKey(), 73);
}

void KoActiveCanvasResourceDependencyKoResourceContractTest::updatesOnlyWhenSourceResourceRequiresTargetKey()
{
    KoActiveCanvasResourceDependencyKoResource<ResourceProbe> dependency(5, 73);
    KoActiveCanvasResourceDependency *base = &dependency;
    const ResourceProbeSP matchingResource(new ResourceProbe);
    matchingResource->requiredKeys = QSet<int>({-9, 73, 2048});
    QVariant matchingSource = QVariant::fromValue(matchingResource);
    const QVariant matchingSourceBefore = matchingSource;
    const QSet<int> matchingKeysBefore = matchingResource->requiredKeys;
    const QVariant firstTarget = QStringLiteral("first target value");
    const QVariant secondTarget = QByteArray("different target value");

    QVERIFY(base->shouldUpdateSource(matchingSource, firstTarget));
    QVERIFY(base->shouldUpdateSource(matchingSource, secondTarget));
    QCOMPARE(matchingSource, matchingSourceBefore);
    QCOMPARE(matchingSource.value<ResourceProbeSP>(), matchingResource);
    QCOMPARE(matchingResource->requiredKeys, matchingKeysBefore);
    QCOMPARE(matchingResource->queryCount, 2);

    const ResourceProbeSP unrelatedResource(new ResourceProbe);
    unrelatedResource->requiredKeys = QSet<int>({-9, 2048});
    QVariant unrelatedSource = QVariant::fromValue(unrelatedResource);
    const QVariant unrelatedSourceBefore = unrelatedSource;

    QVERIFY(!base->shouldUpdateSource(unrelatedSource, firstTarget));
    QCOMPARE(unrelatedSource, unrelatedSourceBefore);
    QCOMPARE(unrelatedResource->requiredKeys, QSet<int>({-9, 2048}));
    QCOMPARE(unrelatedResource->queryCount, 1);
}

void KoActiveCanvasResourceDependencyKoResourceContractTest::nullAndInvalidVariantsDoNotRequestUpdate()
{
    KoActiveCanvasResourceDependencyKoResource<ResourceProbe> dependency(5, 73);
    KoActiveCanvasResourceDependency *base = &dependency;
    const QVariant target = QStringLiteral("ignored target value");
    QVariant invalidSource;
    QVariant wrongTypeSource = 73;
    QVariant nullSource = QVariant::fromValue(ResourceProbeSP());

    QVERIFY(!base->shouldUpdateSource(invalidSource, target));
    QVERIFY(!base->shouldUpdateSource(wrongTypeSource, target));
    QVERIFY(!base->shouldUpdateSource(nullSource, target));
    QVERIFY(!invalidSource.isValid());
    QCOMPARE(wrongTypeSource, QVariant(73));
    QVERIFY(nullSource.value<ResourceProbeSP>().isNull());
}

QTEST_GUILESS_MAIN(KoActiveCanvasResourceDependencyKoResourceContractTest)

#include "KoActiveCanvasResourceDependencyKoResourceContractTest.moc"
