/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoFlake.h"
#include "KoFlakeTypes.h"

#include <QSharedPointer>
#include <QTest>

#include <type_traits>

class KoShapeStroke
{
public:
    explicit KoShapeStroke(int *destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~KoShapeStroke()
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

class KoShapeStrokeModel
{
public:
    explicit KoShapeStrokeModel(int *destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~KoShapeStrokeModel()
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

class KoFlakeTypesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sharedStrokeAliasesRetainTheirObjects();
    void fillMarkerAndStyleEnumsRemainOrdered();
    void selectionEnumsRemainOrdered();
    void selectionHandleValuesFollowClockwiseOrder();
    void anchorValuesFollowGridOrder();
    void canvasResourceIdentifierRemainsStable();
};

void KoFlakeTypesContractTest::sharedStrokeAliasesRetainTheirObjects()
{
    static_assert(std::is_same_v<KoShapeStrokeSP, QSharedPointer<KoShapeStroke>>);
    static_assert(std::is_same_v<KoShapeStrokeModelSP, QSharedPointer<KoShapeStrokeModel>>);

    int strokeDestructionCount = 0;
    int modelDestructionCount = 0;
    KoShapeStrokeSP stroke(new KoShapeStroke(&strokeDestructionCount));
    KoShapeStrokeModelSP model(new KoShapeStrokeModel(&modelDestructionCount));

    {
        const KoShapeStrokeSP strokeCopy = stroke;
        const KoShapeStrokeModelSP modelCopy = model;
        QCOMPARE(strokeCopy.data(), stroke.data());
        QCOMPARE(modelCopy.data(), model.data());
    }

    QCOMPARE(strokeDestructionCount, 0);
    QCOMPARE(modelDestructionCount, 0);
    stroke.clear();
    model.clear();
    QCOMPARE(strokeDestructionCount, 1);
    QCOMPARE(modelDestructionCount, 1);
}

void KoFlakeTypesContractTest::fillMarkerAndStyleEnumsRemainOrdered()
{
    QCOMPARE(int(KoFlake::Fill), 0);
    QCOMPARE(int(KoFlake::StrokeFill), 1);

    QCOMPARE(int(KoFlake::None), 0);
    QCOMPARE(int(KoFlake::Solid), 1);
    QCOMPARE(int(KoFlake::Gradient), 2);
    QCOMPARE(int(KoFlake::Pattern), 3);
    QCOMPARE(int(KoFlake::MeshGradient), 4);

    QCOMPARE(int(KoFlake::StartMarker), 0);
    QCOMPARE(int(KoFlake::MidMarker), 1);
    QCOMPARE(int(KoFlake::EndMarker), 2);

    QCOMPARE(int(KoFlake::Background), 0);
    QCOMPARE(int(KoFlake::Foreground), 1);
}

void KoFlakeTypesContractTest::selectionEnumsRemainOrdered()
{
    QCOMPARE(int(KoFlake::FullSelection), 0);
    QCOMPARE(int(KoFlake::StrippedSelection), 1);
    QCOMPARE(int(KoFlake::TopLevelSelection), 2);

    QCOMPARE(int(KoFlake::Selected), 0);
    QCOMPARE(int(KoFlake::Unselected), 1);
    QCOMPARE(int(KoFlake::NextUnselected), 2);
    QCOMPARE(int(KoFlake::ShapeOnTop), 3);
}

void KoFlakeTypesContractTest::selectionHandleValuesFollowClockwiseOrder()
{
    QCOMPARE(int(KoFlake::TopMiddleHandle), 0);
    QCOMPARE(int(KoFlake::TopRightHandle), 1);
    QCOMPARE(int(KoFlake::RightMiddleHandle), 2);
    QCOMPARE(int(KoFlake::BottomRightHandle), 3);
    QCOMPARE(int(KoFlake::BottomMiddleHandle), 4);
    QCOMPARE(int(KoFlake::BottomLeftHandle), 5);
    QCOMPARE(int(KoFlake::LeftMiddleHandle), 6);
    QCOMPARE(int(KoFlake::TopLeftHandle), 7);
    QCOMPARE(int(KoFlake::NoHandle), 8);
}

void KoFlakeTypesContractTest::anchorValuesFollowGridOrder()
{
    QCOMPARE(int(KoFlake::TopLeft), 0);
    QCOMPARE(int(KoFlake::Top), 1);
    QCOMPARE(int(KoFlake::TopRight), 2);
    QCOMPARE(int(KoFlake::Left), 3);
    QCOMPARE(int(KoFlake::Center), 4);
    QCOMPARE(int(KoFlake::Right), 5);
    QCOMPARE(int(KoFlake::BottomLeft), 6);
    QCOMPARE(int(KoFlake::Bottom), 7);
    QCOMPARE(int(KoFlake::BottomRight), 8);
    QCOMPARE(int(KoFlake::NoAnchor), 9);
    QCOMPARE(int(KoFlake::NumAnchorPositions), 10);
}

void KoFlakeTypesContractTest::canvasResourceIdentifierRemainsStable()
{
    QCOMPARE(int(KoFlake::HotPosition), 1410100299);
}

QTEST_GUILESS_MAIN(KoFlakeTypesContractTest)

#include "KoFlakeTypesContractTest.moc"
