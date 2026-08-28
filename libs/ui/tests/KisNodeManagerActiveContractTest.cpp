/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QTest>

#pragma push_macro("Q_OBJECT")
#undef Q_OBJECT
#define Q_OBJECT
#include "nodes/kis_node_manager.h"
#pragma pop_macro("Q_OBJECT")

void kisSharedPtrAddReference(KisLayer *)
{
}

bool kisSharedPtrRelease(KisLayer *)
{
    return true;
}

void kisSharedPtrAddReference(KisPaintDevice *)
{
}

bool kisSharedPtrRelease(KisPaintDevice *)
{
    return true;
}

namespace
{

template<typename T>
T *token(quintptr id)
{
    return reinterpret_cast<T *>(id * 32 + 1);
}

KisLayerSP activeLayerValue;
bool activeMaskValue;
KisPaintDeviceSP activeMaskDeviceValue;
KisPaintDeviceSP activeLayerDeviceValue;
bool activeMaskDeviceForColor;
bool activeLayerPresent;
bool activeLayerHasParentValue;
const KoColorSpace *activeMaskColorSpaceValue;
const KoColorSpace *activeLayerParentColorSpaceValue;
const KoColorSpace *imageColorSpaceValue;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisLayerSP KisNodeManager::ActiveAccess::activeLayer(KisNodeManager *)
{
    return activeLayerValue;
}

bool KisNodeManager::ActiveAccess::hasActiveMask(KisNodeManager *)
{
    return activeMaskValue;
}

KisPaintDeviceSP KisNodeManager::ActiveAccess::activeMaskDevice(KisNodeManager *)
{
    return activeMaskDeviceValue;
}

KisPaintDeviceSP KisNodeManager::ActiveAccess::activeLayerDevice(KisNodeManager *)
{
    return activeLayerDeviceValue;
}

bool KisNodeManager::ActiveAccess::hasActiveMaskDevice(KisNodeManager *)
{
    return activeMaskDeviceForColor;
}

const KoColorSpace *KisNodeManager::ActiveAccess::activeMaskColorSpace(KisNodeManager *)
{
    return activeMaskColorSpaceValue;
}

bool KisNodeManager::ActiveAccess::hasActiveLayer(KisNodeManager *)
{
    return activeLayerPresent;
}

bool KisNodeManager::ActiveAccess::activeLayerHasParent(KisNodeManager *)
{
    return activeLayerHasParentValue;
}

const KoColorSpace *KisNodeManager::ActiveAccess::activeLayerParentColorSpace(KisNodeManager *)
{
    return activeLayerParentColorSpaceValue;
}

const KoColorSpace *KisNodeManager::ActiveAccess::imageColorSpace(KisNodeManager *)
{
    return imageColorSpaceValue;
}

class KisNodeManagerActiveContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void activeLayerUsesLayerManagerValue();
    void activePaintDevicePrefersTheMaskDevice();
    void activeColorSpacePrefersMaskThenParentThenImage();
};

void KisNodeManagerActiveContractTest::init()
{
    activeLayerValue.clear();
    activeMaskValue = false;
    activeMaskDeviceValue.clear();
    activeLayerDeviceValue.clear();
    activeMaskDeviceForColor = false;
    activeLayerPresent = false;
    activeLayerHasParentValue = false;
    activeMaskColorSpaceValue = nullptr;
    activeLayerParentColorSpaceValue = nullptr;
    imageColorSpaceValue = nullptr;
}

void KisNodeManagerActiveContractTest::activeLayerUsesLayerManagerValue()
{
    KisNodeManager manager(nullptr);
    QVERIFY(manager.activeLayer().isNull());
    activeLayerValue = KisLayerSP(token<KisLayer>(1));
    QCOMPARE(manager.activeLayer(), activeLayerValue);
}

void KisNodeManagerActiveContractTest::activePaintDevicePrefersTheMaskDevice()
{
    KisNodeManager manager(nullptr);
    activeLayerDeviceValue = KisPaintDeviceSP(token<KisPaintDevice>(1));
    QCOMPARE(manager.activePaintDevice(), activeLayerDeviceValue);

    activeMaskValue = true;
    activeMaskDeviceValue = KisPaintDeviceSP(token<KisPaintDevice>(2));
    QCOMPARE(manager.activePaintDevice(), activeMaskDeviceValue);
}

void KisNodeManagerActiveContractTest::activeColorSpacePrefersMaskThenParentThenImage()
{
    KisNodeManager manager(nullptr);
    const KoColorSpace *const maskColor = token<KoColorSpace>(1);
    const KoColorSpace *const parentColor = token<KoColorSpace>(2);
    const KoColorSpace *const imageColor = token<KoColorSpace>(3);
    activeMaskDeviceForColor = true;
    activeMaskColorSpaceValue = maskColor;
    activeLayerParentColorSpaceValue = parentColor;
    imageColorSpaceValue = imageColor;
    QCOMPARE(manager.activeColorSpace(), maskColor);

    activeMaskDeviceForColor = false;
    activeLayerPresent = true;
    activeLayerHasParentValue = true;
    QCOMPARE(manager.activeColorSpace(), parentColor);

    activeLayerHasParentValue = false;
    QCOMPARE(manager.activeColorSpace(), imageColor);
}

QTEST_GUILESS_MAIN(KisNodeManagerActiveContractTest)

#include "KisNodeManagerActiveContractTest.moc"
