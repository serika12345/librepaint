/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KoCanvasSupervisor.h>
#include <KoLoadingShapeUpdater.h>
#include <KoShapeBulkActionInterface.h>
#include <KoShapeUserData.h>
#include <KoToolSelection.h>

#include <QPointer>
#include <QTest>

namespace
{
class BulkAction final : public KoShapeBulkActionInterface
{
public:
    ~BulkAction() override
    {
        *destroyed = true;
    }

    void startBulkAction() override
    {
        started = true;
    }

    QRectF endBulkAction() override
    {
        started = false;
        return updateRect;
    }

    bool *destroyed {nullptr};
    bool started {false};
    QRectF updateRect {11.0, 13.0, 17.0, 19.0};
};

class ShapeUserData final : public KoShapeUserData
{
public:
    using KoShapeUserData::KoShapeUserData;

    ShapeUserData(const ShapeUserData &rhs)
        : KoShapeUserData(rhs)
        , value(rhs.value)
    {
    }

    ~ShapeUserData() override
    {
        if (destroyed) {
            *destroyed = true;
        }
    }

    KoShapeUserData *clone() const override
    {
        return new ShapeUserData(*this);
    }

    bool *destroyed {nullptr};
    int value {0};
};

class CanvasSupervisor final : public KoCanvasSupervisor
{
public:
    ~CanvasSupervisor() override
    {
        *destroyed = true;
    }

    QList<KoCanvasObserverBase *> canvasObservers() const override
    {
        return observers;
    }

    bool *destroyed {nullptr};
    QList<KoCanvasObserverBase *> observers;
};

class LoadingShapeUpdater final : public KoLoadingShapeUpdater
{
public:
    ~LoadingShapeUpdater() override
    {
        *destroyed = true;
    }

    void update(KoShape *shape) override
    {
        updatedShape = shape;
    }

    bool *destroyed {nullptr};
    KoShape *updatedShape {nullptr};
};
}

class KoBehaviorInterfacesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void bracketsBulkActionAndReturnsUpdateRect();
    void clonesQObjectUserDataWithoutParent();
    void preservesDefaultToolSelectionStateAndQObjectLifetime();
    void returnsCanvasObserverListThroughInterface();
    void dispatchesLoadedShapeThroughInterface();
};

void KoBehaviorInterfacesContractTest::bracketsBulkActionAndReturnsUpdateRect()
{
    bool destroyed = false;
    auto *implementation = new BulkAction;
    implementation->destroyed = &destroyed;
    KoShapeBulkActionInterface *interface = implementation;

    interface->startBulkAction();
    QVERIFY(implementation->started);
    QCOMPARE(interface->endBulkAction(), implementation->updateRect);
    QVERIFY(!implementation->started);

    delete interface;
    QVERIFY(destroyed);
}

void KoBehaviorInterfacesContractTest::clonesQObjectUserDataWithoutParent()
{
    QObject parent;
    bool destroyed = false;
    auto *implementation = new ShapeUserData(&parent);
    implementation->destroyed = &destroyed;
    implementation->value = 37;
    QPointer<ShapeUserData> guard(implementation);
    KoShapeUserData *interface = implementation;

    QCOMPARE(interface->parent(), &parent);
    QScopedPointer<KoShapeUserData> clone(interface->clone());
    QCOMPARE(static_cast<ShapeUserData *>(clone.data())->value, 37);
    QVERIFY(!clone->parent());

    delete interface;
    QVERIFY(destroyed);
    QVERIFY(guard.isNull());
}

void KoBehaviorInterfacesContractTest::preservesDefaultToolSelectionStateAndQObjectLifetime()
{
    QObject parent;
    auto *selection = new KoToolSelection(&parent);
    QPointer<KoToolSelection> guard(selection);

    QCOMPARE(selection->parent(), &parent);
    QVERIFY(!selection->hasSelection());

    delete selection;
    QVERIFY(guard.isNull());
}

void KoBehaviorInterfacesContractTest::returnsCanvasObserverListThroughInterface()
{
    bool destroyed = false;
    auto *implementation = new CanvasSupervisor;
    implementation->destroyed = &destroyed;
    implementation->observers = {
        reinterpret_cast<KoCanvasObserverBase *>(quintptr(1)),
        reinterpret_cast<KoCanvasObserverBase *>(quintptr(2))};
    KoCanvasSupervisor *interface = implementation;

    QCOMPARE(interface->canvasObservers(), implementation->observers);

    delete interface;
    QVERIFY(destroyed);
}

void KoBehaviorInterfacesContractTest::dispatchesLoadedShapeThroughInterface()
{
    bool destroyed = false;
    auto *implementation = new LoadingShapeUpdater;
    implementation->destroyed = &destroyed;
    KoLoadingShapeUpdater *interface = implementation;
    KoShape *shape = reinterpret_cast<KoShape *>(quintptr(3));

    interface->update(shape);

    QCOMPARE(implementation->updatedShape, shape);
    delete interface;
    QVERIFY(destroyed);
}

QTEST_GUILESS_MAIN(KoBehaviorInterfacesContractTest)

#include "KoBehaviorInterfacesContractTest.moc"
