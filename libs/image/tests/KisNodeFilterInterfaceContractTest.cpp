/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisNodeFilterInterfaceFilterAccess_p.h"
#include "kis_node_filter_interface.h"

#include <QLoggingCategory>
#include <QTest>

namespace
{

struct FilterToken {
    QString name;
    FilterToken *cloneResult = nullptr;
    int references = 0;
    int acquireCalls = 0;
    int releaseCalls = 0;
    int usageReferences = 0;
    int snapshotQueries = 0;
    int cloneCalls = 0;
    bool hasLocalResourcesSnapshot = true;
};

FilterToken *filterToken(const KisFilterConfiguration *configuration)
{
    return reinterpret_cast<FilterToken *>(const_cast<KisFilterConfiguration *>(configuration));
}

KisFilterConfiguration *filterPointer(FilterToken *token)
{
    return reinterpret_cast<KisFilterConfiguration *>(token);
}

KisFilterConfigurationSP sharedFilter(FilterToken *token)
{
    return KisFilterConfigurationSP(filterPointer(token));
}

void compareFilter(const KisNodeFilterInterface &filterInterface, FilterToken *expected)
{
    QCOMPARE(filterInterface.filter().data(), filterPointer(expected));
}

class TrackedNodeFilterInterface : public KisNodeFilterInterface
{
public:
    TrackedNodeFilterInterface(KisFilterConfigurationSP configuration, int *destructionCount)
        : KisNodeFilterInterface(configuration)
        , m_destructionCount(destructionCount)
    {
    }

    ~TrackedNodeFilterInterface() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

} // namespace

const QLoggingCategory &_41000()
{
    static const QLoggingCategory category("librepaint.contract.nodefilter", QtCriticalMsg);
    return category;
}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    QFAIL("unexpected safe assertion");
}

void kisSharedPtrAddReference(KisFilterConfiguration *configuration)
{
    ++filterToken(configuration)->references;
}

bool kisSharedPtrRelease(KisFilterConfiguration *configuration)
{
    FilterToken *token = filterToken(configuration);
    --token->references;
    return token->references != 0;
}

void kisNodeFilterInterfaceAcquireFilter(KisFilterConfiguration *configuration)
{
    FilterToken *token = filterToken(configuration);
    ++token->acquireCalls;
    ++token->usageReferences;
}

bool kisNodeFilterInterfaceReleaseFilter(KisFilterConfiguration *configuration)
{
    FilterToken *token = filterToken(configuration);
    ++token->releaseCalls;
    --token->usageReferences;
    return token->usageReferences != 0;
}

bool kisNodeFilterInterfaceHasLocalResourcesSnapshot(const KisFilterConfiguration *configuration)
{
    FilterToken *token = filterToken(configuration);
    ++token->snapshotQueries;
    return token->hasLocalResourcesSnapshot;
}

KisFilterConfigurationSP kisNodeFilterInterfaceCloneFilter(const KisFilterConfiguration *configuration)
{
    FilterToken *token = filterToken(configuration);
    ++token->cloneCalls;
    return sharedFilter(token->cloneResult);
}

class KisNodeFilterInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionRetainsConfigurationAndExposesIt();
    void copyClonesConfigurationAndOwnsIndependentResult();
    void setFilterReplacesTheOwnerForBothCompareModes();
    void colorSpaceNotificationClonesAndPreservesVirtualLifetime();
};

void KisNodeFilterInterfaceContractTest::constructionRetainsConfigurationAndExposesIt()
{
    FilterToken configuration{QStringLiteral("source")};

    {
        KisFilterConfigurationSP callerOwner = sharedFilter(&configuration);
        KisNodeFilterInterface filterInterface(callerOwner);
        callerOwner.clear();

        QVERIFY(configuration.references > 0);
        compareFilter(filterInterface, &configuration);
        QCOMPARE(configuration.acquireCalls, 1);
        QCOMPARE(configuration.snapshotQueries, 1);
    }

    QCOMPARE(configuration.references, 0);
    QCOMPARE(configuration.releaseCalls, 1);
    QCOMPARE(configuration.usageReferences, 0);
}

void KisNodeFilterInterfaceContractTest::copyClonesConfigurationAndOwnsIndependentResult()
{
    FilterToken source{QStringLiteral("source")};
    FilterToken clone{QStringLiteral("clone")};
    source.cloneResult = &clone;

    {
        KisNodeFilterInterface original(sharedFilter(&source));
        KisNodeFilterInterface copied(original);

        compareFilter(original, &source);
        compareFilter(copied, &clone);
        QCOMPARE(source.cloneCalls, 1);
        QCOMPARE(source.acquireCalls, 1);
        QCOMPARE(clone.acquireCalls, 1);
    }

    QCOMPARE(source.references, 0);
    QCOMPARE(clone.references, 0);
    QCOMPARE(source.releaseCalls, 1);
    QCOMPARE(clone.releaseCalls, 1);
}

void KisNodeFilterInterfaceContractTest::setFilterReplacesTheOwnerForBothCompareModes()
{
    FilterToken first{QStringLiteral("first")};
    FilterToken second{QStringLiteral("second")};
    FilterToken third{QStringLiteral("third")};

    {
        KisNodeFilterInterface filterInterface(sharedFilter(&first));
        filterInterface.setFilter(sharedFilter(&second));
        compareFilter(filterInterface, &second);

        filterInterface.setFilter(sharedFilter(&third), false);
        compareFilter(filterInterface, &third);
    }

    QCOMPARE(first.references, 0);
    QCOMPARE(second.references, 0);
    QCOMPARE(third.references, 0);
    QCOMPARE(first.acquireCalls, 1);
    QCOMPARE(first.releaseCalls, 1);
    QCOMPARE(second.acquireCalls, 1);
    QCOMPARE(second.releaseCalls, 1);
    QCOMPARE(third.acquireCalls, 1);
    QCOMPARE(third.releaseCalls, 1);
    QCOMPARE(first.snapshotQueries, 1);
    QCOMPARE(second.snapshotQueries, 1);
    QCOMPARE(third.snapshotQueries, 1);
}

void KisNodeFilterInterfaceContractTest::colorSpaceNotificationClonesAndPreservesVirtualLifetime()
{
    FilterToken source{QStringLiteral("source")};
    FilterToken clone{QStringLiteral("clone")};
    source.cloneResult = &clone;
    int destructionCount = 0;

    KisNodeFilterInterface *filterInterface = new TrackedNodeFilterInterface(sharedFilter(&source), &destructionCount);
    filterInterface->notifyColorSpaceChanged();
    compareFilter(*filterInterface, &clone);

    QCOMPARE(source.cloneCalls, 1);
    QCOMPARE(source.acquireCalls, 1);
    QCOMPARE(source.releaseCalls, 0);
    QCOMPARE(clone.acquireCalls, 0);

    delete filterInterface;

    QCOMPARE(destructionCount, 1);
    QCOMPARE(source.references, 0);
    QCOMPARE(clone.references, 0);
    QCOMPARE(clone.releaseCalls, 1);
}

QTEST_GUILESS_MAIN(KisNodeFilterInterfaceContractTest)

#include "KisNodeFilterInterfaceContractTest.moc"
