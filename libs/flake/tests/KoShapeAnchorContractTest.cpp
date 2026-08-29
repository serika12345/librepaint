/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoShapeAnchor.h"

#include <QObject>
#include <QPointF>
#include <QString>
#include <QTest>

namespace
{
class AnchorDestructionProbe : public KoShapeAnchor
{
public:
    AnchorDestructionProbe(KoShape *shape, int *destructionCount)
        : KoShapeAnchor(shape)
        , m_destructionCount(destructionCount)
    {
    }

    ~AnchorDestructionProbe() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

class TextLocationProbe : public KoShapeAnchor::TextLocation
{
public:
    TextLocationProbe(const QTextDocument *document, int position, int *destructionCount)
        : m_document(document)
        , m_position(position)
        , m_destructionCount(destructionCount)
    {
    }

    ~TextLocationProbe() override
    {
        ++*m_destructionCount;
    }

    const QTextDocument *document() const override
    {
        ++documentCallCount;
        return m_document;
    }

    int position() const override
    {
        ++positionCallCount;
        return m_position;
    }

    mutable int documentCallCount = 0;
    mutable int positionCallCount = 0;

private:
    const QTextDocument *m_document;
    int m_position;
    int *m_destructionCount;
};

struct PlacementCounters {
    int detachCount = 0;
    int updateCount = 0;
    int destructionCount = 0;
};

class PlacementStrategyProbe : public KoShapeAnchor::PlacementStrategy
{
public:
    explicit PlacementStrategyProbe(PlacementCounters *counters)
        : m_counters(counters)
    {
    }

    ~PlacementStrategyProbe() override
    {
        ++m_counters->destructionCount;
    }

    void detachFromModel() override
    {
        ++m_counters->detachCount;
    }

    void updateContainerModel() override
    {
        ++m_counters->updateCount;
    }

private:
    PlacementCounters *m_counters;
};

KoShape *shapePointer(QObject *token)
{
    return reinterpret_cast<KoShape *>(token);
}

const QTextDocument *documentPointer(const QObject *token)
{
    return reinterpret_cast<const QTextDocument *>(token);
}
} // namespace

class KoShapeAnchorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionRetainsBorrowedShapeAndDefaultState();
    void anchorAndPositionStateRoundTrips();
    void offsetKeepsReferenceBackedState();
    void textLocationIsBorrowedAndVirtuallyDispatched();
    void placementStrategyIsOwnedAndVirtuallyDispatched();
    void getterOnlyLayoutMetadataRemainsAtLegacyDefaults();
};

void KoShapeAnchorContractTest::constructionRetainsBorrowedShapeAndDefaultState()
{
    QObject shapeToken;
    KoShape *const shape = shapePointer(&shapeToken);
    int destructionCount = 0;

    KoShapeAnchor *anchor = new AnchorDestructionProbe(shape, &destructionCount);
    QCOMPARE(anchor->shape(), shape);
    QCOMPARE(anchor->anchorType(), KoShapeAnchor::AnchorToCharacter);
    QCOMPARE(anchor->horizontalPos(), KoShapeAnchor::HLeft);
    QCOMPARE(anchor->horizontalRel(), KoShapeAnchor::HChar);
    QCOMPARE(anchor->verticalPos(), KoShapeAnchor::VTop);
    QCOMPARE(anchor->verticalRel(), KoShapeAnchor::VLine);
    QCOMPARE(anchor->offset(), QPointF());
    QCOMPARE(anchor->textLocation(), nullptr);
    QCOMPARE(anchor->placementStrategy(), nullptr);

    delete anchor;
    QCOMPARE(destructionCount, 1);
}

void KoShapeAnchorContractTest::anchorAndPositionStateRoundTrips()
{
    KoShapeAnchor anchor(nullptr);

    anchor.setHorizontalPos(KoShapeAnchor::HRight);
    anchor.setHorizontalRel(KoShapeAnchor::HPageContent);
    anchor.setVerticalPos(KoShapeAnchor::VBottom);
    anchor.setVerticalRel(KoShapeAnchor::VParagraphContent);
    anchor.setAnchorType(KoShapeAnchor::AnchorPage);

    QCOMPARE(anchor.anchorType(), KoShapeAnchor::AnchorPage);
    QCOMPARE(anchor.horizontalPos(), KoShapeAnchor::HRight);
    QCOMPARE(anchor.horizontalRel(), KoShapeAnchor::HPageContent);
    QCOMPARE(anchor.verticalPos(), KoShapeAnchor::VBottom);
    QCOMPARE(anchor.verticalRel(), KoShapeAnchor::VParagraphContent);

    anchor.setAnchorType(KoShapeAnchor::AnchorAsCharacter);
    QCOMPARE(anchor.anchorType(), KoShapeAnchor::AnchorAsCharacter);
    QCOMPARE(anchor.horizontalPos(), KoShapeAnchor::HLeft);
    QCOMPARE(anchor.horizontalRel(), KoShapeAnchor::HChar);
    QCOMPARE(anchor.verticalPos(), KoShapeAnchor::VBottom);
    QCOMPARE(anchor.verticalRel(), KoShapeAnchor::VParagraphContent);
}

void KoShapeAnchorContractTest::offsetKeepsReferenceBackedState()
{
    KoShapeAnchor anchor(nullptr);

    anchor.setOffset(QPointF(12.5, -7.25));
    const QPointF *const storedOffset = &anchor.offset();
    QCOMPARE(*storedOffset, QPointF(12.5, -7.25));

    anchor.setOffset(QPointF(-3.0, 9.0));
    QCOMPARE(&anchor.offset(), storedOffset);
    QCOMPARE(*storedOffset, QPointF(-3.0, 9.0));
}

void KoShapeAnchorContractTest::textLocationIsBorrowedAndVirtuallyDispatched()
{
    QObject documentToken;
    const QTextDocument *const document = documentPointer(&documentToken);
    int destructionCount = 0;

    {
        TextLocationProbe location(document, 42, &destructionCount);
        {
            KoShapeAnchor anchor(nullptr);
            anchor.setTextLocation(&location);

            KoShapeAnchor::TextLocation *const storedLocation = anchor.textLocation();
            QCOMPARE(storedLocation, &location);
            QCOMPARE(storedLocation->document(), document);
            QCOMPARE(storedLocation->position(), 42);
            QCOMPARE(location.documentCallCount, 1);
            QCOMPARE(location.positionCallCount, 1);
        }

        QCOMPARE(destructionCount, 0);
        QCOMPARE(location.position(), 42);
        QCOMPARE(location.positionCallCount, 2);
    }

    QCOMPARE(destructionCount, 1);
}

void KoShapeAnchorContractTest::placementStrategyIsOwnedAndVirtuallyDispatched()
{
    PlacementCounters firstCounters;
    PlacementCounters secondCounters;
    PlacementCounters finalCounters;

    {
        KoShapeAnchor anchor(nullptr);
        auto *firstStrategy = new PlacementStrategyProbe(&firstCounters);
        anchor.setPlacementStrategy(firstStrategy);

        QCOMPARE(anchor.placementStrategy(), firstStrategy);
        anchor.placementStrategy()->detachFromModel();
        anchor.placementStrategy()->updateContainerModel();
        QCOMPARE(firstCounters.detachCount, 1);
        QCOMPARE(firstCounters.updateCount, 1);

        anchor.setPlacementStrategy(firstStrategy);
        QCOMPARE(firstCounters.destructionCount, 0);

        auto *secondStrategy = new PlacementStrategyProbe(&secondCounters);
        anchor.setPlacementStrategy(secondStrategy);
        QCOMPARE(firstCounters.destructionCount, 1);
        QCOMPARE(anchor.placementStrategy(), secondStrategy);

        anchor.setPlacementStrategy(nullptr);
        QCOMPARE(secondCounters.destructionCount, 1);
        QCOMPARE(anchor.placementStrategy(), nullptr);

        anchor.setPlacementStrategy(new PlacementStrategyProbe(&finalCounters));
    }

    QCOMPARE(finalCounters.destructionCount, 1);
}

void KoShapeAnchorContractTest::getterOnlyLayoutMetadataRemainsAtLegacyDefaults()
{
    KoShapeAnchor anchor(nullptr);

    QVERIFY(anchor.flowWithText());
    QCOMPARE(anchor.pageNumber(), -1);
    QCOMPARE(anchor.wrapInfluenceOnPosition(), QString());

    anchor.setAnchorType(KoShapeAnchor::AnchorPage);
    anchor.setOffset(QPointF(4.0, 5.0));
    QVERIFY(anchor.flowWithText());
    QCOMPARE(anchor.pageNumber(), -1);
    QCOMPARE(anchor.wrapInfluenceOnPosition(), QString());
}

QTEST_GUILESS_MAIN(KoShapeAnchorContractTest)

#include "KoShapeAnchorContractTest.moc"
