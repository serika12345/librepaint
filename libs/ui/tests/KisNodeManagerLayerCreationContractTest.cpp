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

namespace
{

template<typename T>
T *token(quintptr id)
{
    return reinterpret_cast<T *>(id * 32 + 1);
}

KisImage *imageValue = nullptr;
KisNode *lastChildValue = nullptr;
KisLayerSP createdPaintLayerValue;
int imageRequests = 0;
QList<KisImage *> lastChildImageRequests;
QList<KisImage *> visibleImageRequests;
QList<KisNode *> visiblePutAfterRequests;
QStringList paintLayerTypeRequests;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisImage *KisNodeManager::LayerCreationAccess::image(KisNodeManager *)
{
    ++imageRequests;
    return imageValue;
}

KisNode *KisNodeManager::LayerCreationAccess::rootLastChild(KisImage *image)
{
    lastChildImageRequests.append(image);
    return lastChildValue;
}

void KisNodeManager::LayerCreationAccess::createFromVisible(KisImage *image, KisNode *putAfter)
{
    visibleImageRequests.append(image);
    visiblePutAfterRequests.append(putAfter);
}

KisLayerSP KisNodeManager::LayerCreationAccess::createPaintLayer(KisNodeManager *, const QString &nodeType)
{
    paintLayerTypeRequests.append(nodeType);
    return createdPaintLayerValue;
}

class KisNodeManagerLayerCreationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void visibleLayerUsesCurrentImageAndLastRootChild();
    void paintLayerUsesConcreteNodeTypeAndReturnsLayer();
};

void KisNodeManagerLayerCreationContractTest::init()
{
    imageValue = nullptr;
    lastChildValue = nullptr;
    createdPaintLayerValue.clear();
    imageRequests = 0;
    lastChildImageRequests.clear();
    visibleImageRequests.clear();
    visiblePutAfterRequests.clear();
    paintLayerTypeRequests.clear();
}

void KisNodeManagerLayerCreationContractTest::visibleLayerUsesCurrentImageAndLastRootChild()
{
    KisNodeManager manager(nullptr);
    imageValue = token<KisImage>(1);
    lastChildValue = token<KisNode>(2);

    manager.createFromVisible();

    QCOMPARE(imageRequests, 1);
    QCOMPARE(lastChildImageRequests, (QList<KisImage *>{imageValue}));
    QCOMPARE(visibleImageRequests, (QList<KisImage *>{imageValue}));
    QCOMPARE(visiblePutAfterRequests, (QList<KisNode *>{lastChildValue}));
}

void KisNodeManagerLayerCreationContractTest::paintLayerUsesConcreteNodeTypeAndReturnsLayer()
{
    KisNodeManager manager(nullptr);
    createdPaintLayerValue = KisLayerSP(token<KisLayer>(1));

    const KisLayerSP result = manager.createPaintLayer();

    QCOMPARE(paintLayerTypeRequests, (QStringList{QStringLiteral("KisPaintLayer")}));
    QCOMPARE(result.data(), createdPaintLayerValue.data());
}

QTEST_GUILESS_MAIN(KisNodeManagerLayerCreationContractTest)

#include "KisNodeManagerLayerCreationContractTest.moc"
