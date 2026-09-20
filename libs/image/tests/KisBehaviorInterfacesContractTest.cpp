/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisCroppedOriginalLayerInterface.h>
#include <KisDecoratedNodeInterface.h>
#include <KisDelayedUpdateNodeInterface.h>
#include <KisInterstrokeDataFactory.h>
#include <KisTransactionWrapperFactory.h>
#include <KisTransformMaskTestingInterface.h>
#include <kis_paint_device.h>

#include <QTest>

namespace
{
class CroppedOriginalLayer final : public KisCroppedOriginalLayerInterface
{
public:
    ~CroppedOriginalLayer() override
    {
        *destroyed = true;
    }

    void forceUpdateHiddenAreaOnOriginal() override
    {
        ++updateCount;
    }

    bool *destroyed {nullptr};
    int updateCount {0};
};

class DelayedUpdateNode final : public KisDelayedUpdateNodeInterface
{
public:
    ~DelayedUpdateNode() override
    {
        *destroyed = true;
    }

    void forceUpdateTimedNode() override
    {
        pending = false;
        ++updateCount;
    }

    bool hasPendingTimedUpdates() const override
    {
        return pending;
    }

    bool *destroyed {nullptr};
    bool pending {true};
    int updateCount {0};
};

class TransactionWrapperFactory final : public KisTransactionWrapperFactory
{
public:
    ~TransactionWrapperFactory() override
    {
        *destroyed = true;
    }

    KUndo2Command *createBeginTransactionCommand(KisPaintDeviceSP device) override
    {
        beginReceivedNullDevice = !device;
        return beginCommand;
    }

    KUndo2Command *createEndTransactionCommand() override
    {
        return endCommand;
    }

    bool *destroyed {nullptr};
    bool beginReceivedNullDevice {false};
    KUndo2Command *beginCommand {reinterpret_cast<KUndo2Command *>(quintptr(1))};
    KUndo2Command *endCommand {reinterpret_cast<KUndo2Command *>(quintptr(2))};
};

class DecoratedNode final : public KisDecoratedNodeInterface
{
public:
    ~DecoratedNode() override
    {
        *destroyed = true;
    }

    bool decorationsVisible() const override
    {
        return visible;
    }

    void setDecorationsVisible(bool value, bool update) override
    {
        visible = value;
        updateRequested = update;
    }

    using KisDecoratedNodeInterface::setDecorationsVisible;

    bool *destroyed {nullptr};
    bool visible {true};
    bool updateRequested {false};
};

class InterstrokeDataFactory final : public KisInterstrokeDataFactory
{
public:
    ~InterstrokeDataFactory() override
    {
        *destroyed = true;
    }

    bool isCompatible(KisInterstrokeData *data) override
    {
        lastCheckedData = data;
        return data == compatibleData;
    }

    KisInterstrokeData *create(KisPaintDeviceSP device) override
    {
        createReceivedNullDevice = !device;
        return createdData;
    }

    bool *destroyed {nullptr};
    bool createReceivedNullDevice {false};
    KisInterstrokeData *compatibleData {reinterpret_cast<KisInterstrokeData *>(quintptr(3))};
    KisInterstrokeData *createdData {reinterpret_cast<KisInterstrokeData *>(quintptr(4))};
    KisInterstrokeData *lastCheckedData {nullptr};
};

class TransformMaskNotifications final : public KisTransformMaskTestingInterface
{
public:
    ~TransformMaskNotifications() override
    {
        *destroyed = true;
    }

    void notifyForceUpdateTimedNode() override { calls.append(1); }
    void notifyThreadSafeForceStaticImageUpdate() override { calls.append(2); }
    void notifySlotDelayedStaticUpdate() override { calls.append(3); }
    void notifyDecorateRectTriggeredStaticImageUpdate() override { calls.append(4); }
    void notifyRecalculateStaticImage() override { calls.append(5); }

    bool *destroyed {nullptr};
    QList<int> calls;
};
}

class KisBehaviorInterfacesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void updatesCroppedOriginalThroughInterface();
    void exposesDelayedUpdateStateThroughInterface();
    void createsTransactionBoundaryCommandsThroughInterface();
    void preservesDecoratedNodeVisibilityConvenienceUpdate();
    void checksAndCreatesInterstrokeDataThroughInterface();
    void dispatchesTransformMaskNotificationsThroughInterface();
};

void KisBehaviorInterfacesContractTest::updatesCroppedOriginalThroughInterface()
{
    bool destroyed = false;
    auto *implementation = new CroppedOriginalLayer;
    implementation->destroyed = &destroyed;
    KisCroppedOriginalLayerInterface *interface = implementation;

    interface->forceUpdateHiddenAreaOnOriginal();

    QCOMPARE(implementation->updateCount, 1);
    delete interface;
    QVERIFY(destroyed);
}

void KisBehaviorInterfacesContractTest::exposesDelayedUpdateStateThroughInterface()
{
    bool destroyed = false;
    auto *implementation = new DelayedUpdateNode;
    implementation->destroyed = &destroyed;
    KisDelayedUpdateNodeInterface *interface = implementation;

    QVERIFY(interface->hasPendingTimedUpdates());
    interface->forceUpdateTimedNode();
    QVERIFY(!interface->hasPendingTimedUpdates());
    QCOMPARE(implementation->updateCount, 1);

    delete interface;
    QVERIFY(destroyed);
}

void KisBehaviorInterfacesContractTest::createsTransactionBoundaryCommandsThroughInterface()
{
    bool destroyed = false;
    auto *implementation = new TransactionWrapperFactory;
    implementation->destroyed = &destroyed;
    KisTransactionWrapperFactory *interface = implementation;

    QCOMPARE(interface->createBeginTransactionCommand({}), implementation->beginCommand);
    QVERIFY(implementation->beginReceivedNullDevice);
    QCOMPARE(interface->createEndTransactionCommand(), implementation->endCommand);

    delete interface;
    QVERIFY(destroyed);
}

void KisBehaviorInterfacesContractTest::preservesDecoratedNodeVisibilityConvenienceUpdate()
{
    bool destroyed = false;
    auto *implementation = new DecoratedNode;
    implementation->destroyed = &destroyed;
    KisDecoratedNodeInterface *interface = implementation;

    QVERIFY(interface->decorationsVisible());
    interface->setDecorationsVisible(false);
    QVERIFY(!interface->decorationsVisible());
    QVERIFY(implementation->updateRequested);

    interface->setDecorationsVisible(true, false);
    QVERIFY(interface->decorationsVisible());
    QVERIFY(!implementation->updateRequested);

    delete interface;
    QVERIFY(destroyed);
}

void KisBehaviorInterfacesContractTest::checksAndCreatesInterstrokeDataThroughInterface()
{
    bool destroyed = false;
    auto *implementation = new InterstrokeDataFactory;
    implementation->destroyed = &destroyed;
    KisInterstrokeDataFactory *interface = implementation;

    QVERIFY(interface->isCompatible(implementation->compatibleData));
    QCOMPARE(implementation->lastCheckedData, implementation->compatibleData);
    QVERIFY(!interface->isCompatible(nullptr));
    QCOMPARE(interface->create({}), implementation->createdData);
    QVERIFY(implementation->createReceivedNullDevice);

    delete interface;
    QVERIFY(destroyed);
}

void KisBehaviorInterfacesContractTest::dispatchesTransformMaskNotificationsThroughInterface()
{
    bool destroyed = false;
    auto *implementation = new TransformMaskNotifications;
    implementation->destroyed = &destroyed;
    KisTransformMaskTestingInterface *interface = implementation;

    interface->notifyForceUpdateTimedNode();
    interface->notifyThreadSafeForceStaticImageUpdate();
    interface->notifySlotDelayedStaticUpdate();
    interface->notifyDecorateRectTriggeredStaticImageUpdate();
    interface->notifyRecalculateStaticImage();

    QCOMPARE(implementation->calls, QList<int>({1, 2, 3, 4, 5}));
    delete interface;
    QVERIFY(destroyed);
}

QTEST_GUILESS_MAIN(KisBehaviorInterfacesContractTest)

#include "KisBehaviorInterfacesContractTest.moc"
