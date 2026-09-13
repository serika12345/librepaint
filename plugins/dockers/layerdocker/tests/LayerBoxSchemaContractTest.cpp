/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "LayerBox.h"
#include "SyncButtonAndAction.h"

#include <QTest>

#include <type_traits>

class LayerBoxSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void indexListAndLayerBoxLifetimeSchemaRemainStable();
    void observerAndCanvasSignaturesRemainStable();
    void imageNotificationAndFactoryLifetimeSchemaRemainStable();
    void factoryIdentityCreationAndPlacementSchemaRemainStable();
    void actionButtonSynchronizerConstructionRemainsStable();
};

void LayerBoxSchemaContractTest::indexListAndLayerBoxLifetimeSchemaRemainStable()
{
    static_assert(std::is_same_v<QModelIndexList, QList<QModelIndex>>);
    static_assert(std::is_class_v<LayerBox>);
    static_assert(std::is_base_of_v<QDockWidget, LayerBox>);
    static_assert(std::is_base_of_v<KisMainwindowObserver, LayerBox>);
    static_assert(std::is_default_constructible_v<LayerBox>);
    static_assert(std::has_virtual_destructor_v<LayerBox>);
}

void LayerBoxSchemaContractTest::observerAndCanvasSignaturesRemainStable()
{
    using Box = LayerBox;

    static_assert(std::is_same_v<decltype(&Box::observerName), QString (Box::*)()>);
    static_assert(std::is_same_v<decltype(&Box::setViewManager), void (Box::*)(KisViewManager *)>);
    static_assert(std::is_same_v<decltype(&Box::setCanvas), void (Box::*)(KoCanvasBase *)>);
    static_assert(std::is_same_v<decltype(&Box::unsetCanvas), void (Box::*)()>);
}

void LayerBoxSchemaContractTest::imageNotificationAndFactoryLifetimeSchemaRemainStable()
{
    static_assert(std::is_same_v<decltype(&LayerBox::imageChanged), void (LayerBox::*)()>);
    static_assert(std::is_class_v<LayerBoxFactory>);
    static_assert(std::is_base_of_v<KoDockFactoryBase, LayerBoxFactory>);
    static_assert(std::is_default_constructible_v<LayerBoxFactory>);
}

void LayerBoxSchemaContractTest::factoryIdentityCreationAndPlacementSchemaRemainStable()
{
    using Factory = LayerBoxFactory;

    static_assert(std::is_same_v<decltype(&Factory::id), QString (Factory::*)() const>);
    static_assert(std::is_same_v<decltype(&Factory::createDockWidget), QDockWidget *(Factory::*)()>);
    static_assert(
        std::is_same_v<decltype(&Factory::defaultDockPosition), KoDockFactoryBase::DockPosition (Factory::*)() const>);
}

void LayerBoxSchemaContractTest::actionButtonSynchronizerConstructionRemainsStable()
{
    using Synchronizer = SyncButtonAndAction;

    static_assert(std::is_class_v<Synchronizer>);
    static_assert(std::is_base_of_v<QObject, Synchronizer>);
    static_assert(std::is_constructible_v<Synchronizer, KisAction *, QAbstractButton *, QObject *>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(LayerBoxSchemaContractTest)

#include "LayerBoxSchemaContractTest.moc"
