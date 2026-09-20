/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "layerstyles/kis_layer_style_filter.h"

#include <KoID.h>

#include <QTest>

class KisPaintDevice
{
public:
    bool deref();
};

namespace
{

struct CallRecord {
    int processCount = 0;
    int neededCount = 0;
    int changedCount = 0;
    int destroyedCount = 0;
    KisMultipleProjection *projection = nullptr;
    KisLayerStyleKnockoutBlower *blower = nullptr;
    KisLayerStyleFilterEnvironment *environment = nullptr;
    QRect processRect;
    QRect neededRect;
    QRect changedRect;
    bool sourceWasNull = false;
    bool styleWasNull = false;
};

class LayerStyleFilterProbe final : public KisLayerStyleFilter
{
public:
    LayerStyleFilterProbe(const KoID &id, CallRecord *record)
        : KisLayerStyleFilter(id)
        , m_record(record)
    {
    }

    LayerStyleFilterProbe(const LayerStyleFilterProbe &rhs)
        : KisLayerStyleFilter(rhs)
        , m_record(rhs.m_record)
    {
    }

    ~LayerStyleFilterProbe() override
    {
        ++m_record->destroyedCount;
    }

    KisLayerStyleFilter *clone() const override
    {
        return new LayerStyleFilterProbe(*this);
    }

    void processDirectly(KisPaintDeviceSP src,
                         KisMultipleProjection *dst,
                         KisLayerStyleKnockoutBlower *blower,
                         const QRect &applyRect,
                         KisPSDLayerStyleSP style,
                         KisLayerStyleFilterEnvironment *env) const override
    {
        m_record->sourceWasNull = !src;
        m_record->styleWasNull = style.isNull();
        ++m_record->processCount;
        m_record->projection = dst;
        m_record->blower = blower;
        m_record->environment = env;
        m_record->processRect = applyRect;
    }

    QRect neededRect(const QRect &rect, KisPSDLayerStyleSP style, KisLayerStyleFilterEnvironment *env) const override
    {
        m_record->styleWasNull = style.isNull();
        ++m_record->neededCount;
        m_record->environment = env;
        m_record->neededRect = rect;
        return QRect(-7, 11, 13, 17);
    }

    QRect changedRect(const QRect &rect, KisPSDLayerStyleSP style, KisLayerStyleFilterEnvironment *env) const override
    {
        m_record->styleWasNull = style.isNull();
        ++m_record->changedCount;
        m_record->environment = env;
        m_record->changedRect = rect;
        return QRect(19, -23, 29, 31);
    }

private:
    CallRecord *m_record;
};

} // namespace

class KisLayerStyleFilterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void identifierIsCopiedAndOwned();
    void clonePreservesIdentifierAndVirtualLifetime();
    void virtualOperationsForwardArgumentsAndReturnRects();
};

void KisLayerStyleFilterContractTest::identifierIsCopiedAndOwned()
{
    CallRecord record;
    KoID source(QStringLiteral("stable-layer-style"), QStringLiteral("Visible style"));
    LayerStyleFilterProbe filter(source, &record);

    QCOMPARE(filter.id(), QStringLiteral("stable-layer-style"));

    source = KoID(QStringLiteral("changed-source"), QStringLiteral("Changed source"));
    QCOMPARE(filter.id(), QStringLiteral("stable-layer-style"));
}

void KisLayerStyleFilterContractTest::clonePreservesIdentifierAndVirtualLifetime()
{
    CallRecord record;
    KisLayerStyleFilter *clone = nullptr;

    {
        LayerStyleFilterProbe original(KoID(QStringLiteral("clone-id")), &record);
        clone = original.clone();
        QVERIFY(clone != &original);
        QCOMPARE(clone->id(), QStringLiteral("clone-id"));
        QCOMPARE(record.destroyedCount, 0);
    }

    QCOMPARE(record.destroyedCount, 1);
    QCOMPARE(clone->id(), QStringLiteral("clone-id"));
    delete clone;
    QCOMPARE(record.destroyedCount, 2);
}

void KisLayerStyleFilterContractTest::virtualOperationsForwardArgumentsAndReturnRects()
{
    CallRecord record;
    LayerStyleFilterProbe filter(KoID(QStringLiteral("dispatch-id")), &record);
    int projectionToken = 0;
    int blowerToken = 0;
    int environmentToken = 0;
    auto *projection = reinterpret_cast<KisMultipleProjection *>(&projectionToken);
    auto *blower = reinterpret_cast<KisLayerStyleKnockoutBlower *>(&blowerToken);
    auto *environment = reinterpret_cast<KisLayerStyleFilterEnvironment *>(&environmentToken);
    const QRect processRect(3, 5, 7, 11);
    const QRect neededInput(13, 17, 19, 23);
    const QRect changedInput(-29, 31, 37, 41);

    filter.processDirectly(KisPaintDeviceSP(), projection, blower, processRect, KisPSDLayerStyleSP(), environment);
    QCOMPARE(record.processCount, 1);
    QCOMPARE(record.projection, projection);
    QCOMPARE(record.blower, blower);
    QCOMPARE(record.environment, environment);
    QCOMPARE(record.processRect, processRect);
    QVERIFY(record.sourceWasNull);
    QVERIFY(record.styleWasNull);

    QCOMPARE(filter.neededRect(neededInput, KisPSDLayerStyleSP(), environment), QRect(-7, 11, 13, 17));
    QCOMPARE(record.neededCount, 1);
    QCOMPARE(record.neededRect, neededInput);
    QCOMPARE(record.environment, environment);

    QCOMPARE(filter.changedRect(changedInput, KisPSDLayerStyleSP(), environment), QRect(19, -23, 29, 31));
    QCOMPARE(record.changedCount, 1);
    QCOMPARE(record.changedRect, changedInput);
    QCOMPARE(record.environment, environment);
}

QTEST_GUILESS_MAIN(KisLayerStyleFilterContractTest)

#include "KisLayerStyleFilterContractTest.moc"
