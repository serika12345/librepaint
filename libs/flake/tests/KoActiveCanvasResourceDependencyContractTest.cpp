/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoActiveCanvasResourceDependency.h"

#include <QTest>
#include <QVariant>

namespace
{
class DependencyProbe : public KoActiveCanvasResourceDependency
{
public:
    DependencyProbe(int sourceKey, int targetKey, int *destructionCount = nullptr)
        : KoActiveCanvasResourceDependency(sourceKey, targetKey)
        , m_destructionCount(destructionCount)
    {
    }

    ~DependencyProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    bool shouldUpdateSource(QVariant &source, const QVariant &target) override
    {
        ++updateCallCount;
        observedSource = &source;
        observedTarget = &target;
        sourceBeforeUpdate = source;
        targetDuringUpdate = target;

        if (source == target) {
            return false;
        }

        source = target;
        return true;
    }

    int updateCallCount = 0;
    QVariant *observedSource = nullptr;
    const QVariant *observedTarget = nullptr;
    QVariant sourceBeforeUpdate;
    QVariant targetDuringUpdate;

private:
    int *m_destructionCount;
};
} // namespace

class KoActiveCanvasResourceDependencyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesSourceAndTargetKeys();
    void dispatchesSourceReferenceAndTargetValue();
    void sharedAliasOwnsThroughVirtualBaseLifetime();
};

void KoActiveCanvasResourceDependencyContractTest::preservesSourceAndTargetKeys()
{
    DependencyProbe dependency(-17, 2048);

    QCOMPARE(dependency.sourceKey(), -17);
    QCOMPARE(dependency.targetKey(), 2048);
}

void KoActiveCanvasResourceDependencyContractTest::dispatchesSourceReferenceAndTargetValue()
{
    DependencyProbe dependency(1, 2);
    KoActiveCanvasResourceDependency *base = &dependency;
    QVariant source = QStringLiteral("source");
    const QVariant target = QByteArray("target");

    QVERIFY(base->shouldUpdateSource(source, target));
    QCOMPARE(dependency.updateCallCount, 1);
    QCOMPARE(dependency.observedSource, &source);
    QCOMPARE(dependency.observedTarget, &target);
    QCOMPARE(dependency.sourceBeforeUpdate, QVariant(QStringLiteral("source")));
    QCOMPARE(dependency.targetDuringUpdate, target);
    QCOMPARE(source, target);

    QVERIFY(!base->shouldUpdateSource(source, target));
    QCOMPARE(dependency.updateCallCount, 2);
    QCOMPARE(source, target);
}

void KoActiveCanvasResourceDependencyContractTest::sharedAliasOwnsThroughVirtualBaseLifetime()
{
    int destructionCount = 0;
    KoActiveCanvasResourceDependencySP owner(new DependencyProbe(3, 5, &destructionCount));
    KoActiveCanvasResourceDependencySP copy = owner;

    QCOMPARE(copy->sourceKey(), 3);
    QCOMPARE(copy->targetKey(), 5);
    owner.clear();
    QCOMPARE(destructionCount, 0);

    copy.clear();
    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KoActiveCanvasResourceDependencyContractTest)

#include "KoActiveCanvasResourceDependencyContractTest.moc"
