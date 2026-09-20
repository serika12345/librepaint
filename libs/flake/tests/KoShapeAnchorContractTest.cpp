/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoShapeAnchor.h"

#include <QTest>

namespace
{
class TextLocationProbe : public KoShapeAnchor::TextLocation
{
public:
    explicit TextLocationProbe(bool *wasDestroyed)
        : m_wasDestroyed(wasDestroyed)
    {
    }

    ~TextLocationProbe() override
    {
        *m_wasDestroyed = true;
    }

    const QTextDocument *document() const override
    {
        return nullptr;
    }

    int position() const override
    {
        return 42;
    }

private:
    bool *m_wasDestroyed;
};

class PlacementStrategyProbe : public KoShapeAnchor::PlacementStrategy
{
public:
    explicit PlacementStrategyProbe(bool *wasDestroyed)
        : m_wasDestroyed(wasDestroyed)
    {
    }

    ~PlacementStrategyProbe() override
    {
        *m_wasDestroyed = true;
    }

    void detachFromModel() override
    {
    }

    void updateContainerModel() override
    {
    }

private:
    bool *m_wasDestroyed;
};
} // namespace

class KoShapeAnchorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void inlineAnchorUsesCharacterRelativePosition();
    void textLocationRemainsOwnedByTheTextDocument();
    void replacingPlacementStrategyDisposesPreviousStrategy();
};

void KoShapeAnchorContractTest::inlineAnchorUsesCharacterRelativePosition()
{
    // Consumer: Text-flow layout that changes a drawing shape to an inline character.
    // Operation: Change a page-relative anchor to an inline anchor.
    // Observable result: The anchor uses the character-relative left position while preserving its vertical placement.
    // Failure impact: An inline drawing shape retains its old page-relative position and is laid out in the wrong place.
    KoShapeAnchor anchor(nullptr);
    anchor.setHorizontalPos(KoShapeAnchor::HRight);
    anchor.setHorizontalRel(KoShapeAnchor::HPageContent);
    anchor.setVerticalPos(KoShapeAnchor::VBottom);
    anchor.setVerticalRel(KoShapeAnchor::VParagraphContent);

    anchor.setAnchorType(KoShapeAnchor::AnchorAsCharacter);

    QCOMPARE(anchor.anchorType(), KoShapeAnchor::AnchorAsCharacter);
    QCOMPARE(anchor.horizontalPos(), KoShapeAnchor::HLeft);
    QCOMPARE(anchor.horizontalRel(), KoShapeAnchor::HChar);
    QCOMPARE(anchor.verticalPos(), KoShapeAnchor::VBottom);
    QCOMPARE(anchor.verticalRel(), KoShapeAnchor::VParagraphContent);
}

void KoShapeAnchorContractTest::textLocationRemainsOwnedByTheTextDocument()
{
    // Consumer: Text-document integration that retains the anchor's text location.
    // Operation: Bind a document-owned text location, then destroy the anchor.
    // Observable result: The text location remains alive for its document owner.
    // Failure impact: Destroying a drawing shape invalidates the document's text location and can corrupt text layout.
    bool locationWasDestroyed = false;

    {
        TextLocationProbe location(&locationWasDestroyed);
        {
            KoShapeAnchor anchor(nullptr);
            anchor.setTextLocation(&location);

            QCOMPARE(anchor.textLocation(), &location);
        }

        QVERIFY(!locationWasDestroyed);
    }

    QVERIFY(locationWasDestroyed);
}

void KoShapeAnchorContractTest::replacingPlacementStrategyDisposesPreviousStrategy()
{
    // Consumer: Text-flow layout that replaces a shape's placement strategy.
    // Operation: Set one placement strategy, replace it, then destroy the anchor.
    // Observable result: The previous strategy is disposed on replacement and the final strategy is disposed with the anchor.
    // Failure impact: Superseded layout strategies leak or outlive the drawing shape that owns them.
    bool firstStrategyWasDestroyed = false;
    bool secondStrategyWasDestroyed = false;

    {
        KoShapeAnchor anchor(nullptr);
        anchor.setPlacementStrategy(new PlacementStrategyProbe(&firstStrategyWasDestroyed));
        anchor.setPlacementStrategy(new PlacementStrategyProbe(&secondStrategyWasDestroyed));

        QVERIFY(firstStrategyWasDestroyed);
        QVERIFY(!secondStrategyWasDestroyed);
    }

    QVERIFY(secondStrategyWasDestroyed);
}

QTEST_GUILESS_MAIN(KoShapeAnchorContractTest)

#include "KoShapeAnchorContractTest.moc"
