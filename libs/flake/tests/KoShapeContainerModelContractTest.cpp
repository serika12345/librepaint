/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoShapeContainer.h"
#include "KoShapeContainerModel.h"

#include <QStringList>
#include <QTest>

void KoShape::setParent(KoShapeContainer *)
{
    qFatal("KoShape::setParent must not be used by this contract");
}

KoShapeContainer *KoShape::parent() const
{
    qFatal("KoShape::parent must not be used by this contract");
    return nullptr;
}

KoShapeContainerModel *KoShapeContainer::model() const
{
    qFatal("KoShapeContainer::model must not be used by this contract");
    return nullptr;
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion '%s' at %s:%d", assertion, file, line);
}

namespace
{
class ModelProbe final : public KoShapeContainerModel
{
public:
    explicit ModelProbe(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~ModelProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void add(KoShape *shape) override
    {
        events.append(QStringLiteral("add"));
        observedShape = shape;
    }

    void remove(KoShape *shape) override
    {
        events.append(QStringLiteral("remove"));
        observedShape = shape;
    }

    void setClipped(const KoShape *shape, bool clipping) override
    {
        events.append(QStringLiteral("setClipped"));
        observedConstShape = shape;
        observedBool = clipping;
    }

    bool isClipped(const KoShape *shape) const override
    {
        events.append(QStringLiteral("isClipped"));
        observedConstShape = shape;
        return clippedResult;
    }

    void setInheritsTransform(const KoShape *shape, bool inherit) override
    {
        events.append(QStringLiteral("setInheritsTransform"));
        observedConstShape = shape;
        observedBool = inherit;
    }

    bool inheritsTransform(const KoShape *shape) const override
    {
        events.append(QStringLiteral("inheritsTransform"));
        observedConstShape = shape;
        return inheritsTransformResult;
    }

    int count() const override
    {
        events.append(QStringLiteral("count"));
        return countResult;
    }

    QList<KoShape *> shapes() const override
    {
        events.append(QStringLiteral("shapes"));
        return shapesResult;
    }

    void containerChanged(KoShapeContainer *container, KoShape::ChangeType type) override
    {
        events.append(QStringLiteral("containerChanged"));
        observedContainer = container;
        observedType = type;
    }

    void childChanged(KoShape *shape, KoShape::ChangeType type) override
    {
        events.append(QStringLiteral("childChanged"));
        observedShape = shape;
        observedType = type;
    }

    void shapeHasBeenAddedToHierarchy(KoShape *shape, KoShapeContainer *addedToSubtree) override
    {
        events.append(QStringLiteral("shapeHasBeenAddedToHierarchy"));
        observedShape = shape;
        observedContainer = addedToSubtree;
    }

    void shapeToBeRemovedFromHierarchy(KoShape *shape, KoShapeContainer *removedFromSubtree) override
    {
        events.append(QStringLiteral("shapeToBeRemovedFromHierarchy"));
        observedShape = shape;
        observedContainer = removedFromSubtree;
    }

    mutable QStringList events;
    mutable const KoShape *observedConstShape = nullptr;
    KoShape *observedShape = nullptr;
    KoShapeContainer *observedContainer = nullptr;
    KoShape::ChangeType observedType = KoShape::GenericMatrixChange;
    bool observedBool = false;
    bool clippedResult = false;
    bool inheritsTransformResult = false;
    int countResult = 0;
    QList<KoShape *> shapesResult;

private:
    int *m_destructionCount;
};
} // namespace

class KoShapeContainerModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyModelPreservesMoveAndDeletesNothing();
    void dispatchesStorageAndFlagsThroughAbstractSurface();
    void dispatchesContainerAndHierarchyNotifications();
    void destroysDerivedModelThroughBase();
};

void KoShapeContainerModelContractTest::emptyModelPreservesMoveAndDeletesNothing()
{
    int shapeToken = 0;
    auto *shape = reinterpret_cast<KoShape *>(&shapeToken);
    ModelProbe model;
    QPointF move(7.5, -3.25);

    model.KoShapeContainerModel::proposeMove(shape, move);
    QCOMPARE(move, QPointF(7.5, -3.25));

    model.deleteOwnedShapes();
    const QStringList expectedEvents = {
        QStringLiteral("shapes"),
        QStringLiteral("count"),
    };
    QCOMPARE(model.events, expectedEvents);
}

void KoShapeContainerModelContractTest::dispatchesStorageAndFlagsThroughAbstractSurface()
{
    int firstShapeToken = 0;
    int secondShapeToken = 0;
    auto *firstShape = reinterpret_cast<KoShape *>(&firstShapeToken);
    auto *secondShape = reinterpret_cast<KoShape *>(&secondShapeToken);
    ModelProbe model;
    KoShapeContainerModel *interface = &model;

    model.clippedResult = true;
    model.inheritsTransformResult = false;
    model.countResult = 2;
    model.shapesResult = {firstShape, secondShape};

    interface->add(firstShape);
    QCOMPARE(model.observedShape, firstShape);
    interface->remove(secondShape);
    QCOMPARE(model.observedShape, secondShape);

    interface->setClipped(firstShape, true);
    QCOMPARE(model.observedConstShape, firstShape);
    QVERIFY(model.observedBool);
    QVERIFY(interface->isClipped(secondShape));
    QCOMPARE(model.observedConstShape, secondShape);

    interface->setInheritsTransform(secondShape, false);
    QCOMPARE(model.observedConstShape, secondShape);
    QVERIFY(!model.observedBool);
    QVERIFY(!interface->inheritsTransform(firstShape));
    QCOMPARE(model.observedConstShape, firstShape);

    QCOMPARE(interface->count(), 2);
    QCOMPARE(interface->shapes(), QList<KoShape *>({firstShape, secondShape}));

    const QStringList expectedEvents = {
        QStringLiteral("add"),
        QStringLiteral("remove"),
        QStringLiteral("setClipped"),
        QStringLiteral("isClipped"),
        QStringLiteral("setInheritsTransform"),
        QStringLiteral("inheritsTransform"),
        QStringLiteral("count"),
        QStringLiteral("shapes"),
    };
    QCOMPARE(model.events, expectedEvents);
}

void KoShapeContainerModelContractTest::dispatchesContainerAndHierarchyNotifications()
{
    int shapeToken = 0;
    int containerToken = 0;
    auto *shape = reinterpret_cast<KoShape *>(&shapeToken);
    auto *container = reinterpret_cast<KoShapeContainer *>(&containerToken);
    ModelProbe model;
    KoShapeContainerModel *interface = &model;

    interface->containerChanged(container, KoShape::SizeChanged);
    QCOMPARE(model.observedContainer, container);
    QCOMPARE(model.observedType, KoShape::SizeChanged);

    interface->childChanged(shape, KoShape::PositionChanged);
    QCOMPARE(model.observedShape, shape);
    QCOMPARE(model.observedType, KoShape::PositionChanged);

    interface->shapeHasBeenAddedToHierarchy(shape, container);
    QCOMPARE(model.observedShape, shape);
    QCOMPARE(model.observedContainer, container);

    interface->shapeToBeRemovedFromHierarchy(shape, container);
    QCOMPARE(model.observedShape, shape);
    QCOMPARE(model.observedContainer, container);

    const QStringList expectedEvents = {
        QStringLiteral("containerChanged"),
        QStringLiteral("childChanged"),
        QStringLiteral("shapeHasBeenAddedToHierarchy"),
        QStringLiteral("shapeToBeRemovedFromHierarchy"),
    };
    QCOMPARE(model.events, expectedEvents);
}

void KoShapeContainerModelContractTest::destroysDerivedModelThroughBase()
{
    int destructionCount = 0;
    KoShapeContainerModel *model = new ModelProbe(&destructionCount);

    delete model;

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KoShapeContainerModelContractTest)

#include "KoShapeContainerModelContractTest.moc"
