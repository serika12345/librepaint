/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QImage>
#include <QObject>
#include <QTest>

#include <klocalizedstring.h>

#pragma push_macro("Q_OBJECT")
#undef Q_OBJECT
#define Q_OBJECT
#include "nodes/kis_node_manager.h"
#pragma pop_macro("Q_OBJECT")

namespace
{

template<typename T>
T *token(quintptr id)
{
    return reinterpret_cast<T *>(id * 32 + 1);
}

KisPaintDevice *deviceToken(quintptr id)
{
    return token<KisPaintDevice>(id);
}

KisPaintDevice *activeLayerProjectionValue = nullptr;
KisPaintDevice *visibleProjectionValue = nullptr;
QImage convertedImageValue;
KisReferenceImage *createdReferenceValue = nullptr;
int referenceImageCountValue = -1;
bool canvasWidgetAvailable = false;
int activeLayerProjectionRequests = 0;
int visibleProjectionRequests = 0;
QList<KisPaintDevice *> conversionRequests;
QList<QImage> referenceCreationRequests;
QList<KisReferenceImage *> zIndexReferences;
QList<int> zIndexValues;
QList<KisReferenceImage *> addedReferences;
QStringList switchedToolIds;
QStringList floatingMessages;
QList<int> floatingMessageTimeouts;
QList<bool> floatingMessageHighPriorities;
QList<bool> floatingMessageSingleLines;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisPaintDevice *KisNodeManager::ReferenceImageAccess::activeLayerProjection(KisNodeManager *)
{
    ++activeLayerProjectionRequests;
    return activeLayerProjectionValue;
}

KisPaintDevice *KisNodeManager::ReferenceImageAccess::visibleProjection(KisNodeManager *)
{
    ++visibleProjectionRequests;
    return visibleProjectionValue;
}

QImage KisNodeManager::ReferenceImageAccess::convertToImage(KisPaintDevice *device)
{
    conversionRequests.append(device);
    return convertedImageValue;
}

KisReferenceImage *KisNodeManager::ReferenceImageAccess::createReferenceImage(KisNodeManager *, const QImage &image)
{
    referenceCreationRequests.append(image);
    return createdReferenceValue;
}

void KisNodeManager::ReferenceImageAccess::deleteReferenceImage(KisReferenceImage *)
{
}

int KisNodeManager::ReferenceImageAccess::referenceImageCount(KisNodeManager *)
{
    return referenceImageCountValue;
}

void KisNodeManager::ReferenceImageAccess::setZIndex(KisReferenceImage *reference, int index)
{
    zIndexReferences.append(reference);
    zIndexValues.append(index);
}

void KisNodeManager::ReferenceImageAccess::addReferenceImage(KisNodeManager *, KisReferenceImage *reference)
{
    addedReferences.append(reference);
}

void KisNodeManager::ReferenceImageAccess::switchTool(const QString &toolId)
{
    switchedToolIds.append(toolId);
}

bool KisNodeManager::ReferenceImageAccess::hasCanvasWidget(KisNodeManager *)
{
    return canvasWidgetAvailable;
}

void KisNodeManager::ReferenceImageAccess::showFloatingMessage(KisNodeManager *,
                                                               const QString &message,
                                                               int timeout,
                                                               bool highPriority,
                                                               bool singleLine)
{
    floatingMessages.append(message);
    floatingMessageTimeouts.append(timeout);
    floatingMessageHighPriorities.append(highPriority);
    floatingMessageSingleLines.append(singleLine);
}

class KisNodeManagerReferenceImageContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void entryPointsChooseLayerOrVisibleProjection();
    void successfulCreationAppendsAndSwitchesTool();
    void firstReferenceKeepsDefaultZIndex();
    void failedCreationReportsInputSpecificMessage();
};

void KisNodeManagerReferenceImageContractTest::init()
{
    activeLayerProjectionValue = nullptr;
    visibleProjectionValue = nullptr;
    convertedImageValue = QImage();
    createdReferenceValue = nullptr;
    referenceImageCountValue = -1;
    canvasWidgetAvailable = false;
    activeLayerProjectionRequests = 0;
    visibleProjectionRequests = 0;
    conversionRequests.clear();
    referenceCreationRequests.clear();
    zIndexReferences.clear();
    zIndexValues.clear();
    addedReferences.clear();
    switchedToolIds.clear();
    floatingMessages.clear();
    floatingMessageTimeouts.clear();
    floatingMessageHighPriorities.clear();
    floatingMessageSingleLines.clear();
}

void KisNodeManagerReferenceImageContractTest::entryPointsChooseLayerOrVisibleProjection()
{
    KisNodeManager manager(nullptr);
    activeLayerProjectionValue = deviceToken(1);
    visibleProjectionValue = deviceToken(2);
    convertedImageValue = QImage(2, 1, QImage::Format_ARGB32);

    manager.createReferenceImageFromLayer();
    manager.createReferenceImageFromVisible();

    QCOMPARE(activeLayerProjectionRequests, 1);
    QCOMPARE(visibleProjectionRequests, 1);
    QCOMPARE(conversionRequests, (QList<KisPaintDevice *>{activeLayerProjectionValue, visibleProjectionValue}));
    QCOMPARE(referenceCreationRequests, (QList<QImage>{convertedImageValue, convertedImageValue}));
    QVERIFY(addedReferences.isEmpty());
    QVERIFY(floatingMessages.isEmpty());
}

void KisNodeManagerReferenceImageContractTest::successfulCreationAppendsAndSwitchesTool()
{
    KisNodeManager manager(nullptr);
    activeLayerProjectionValue = deviceToken(1);
    convertedImageValue = QImage(2, 1, QImage::Format_ARGB32);
    createdReferenceValue = token<KisReferenceImage>(1);
    referenceImageCountValue = 4;

    manager.createReferenceImage(true);

    QCOMPARE(conversionRequests, (QList<KisPaintDevice *>{activeLayerProjectionValue}));
    QCOMPARE(zIndexReferences, (QList<KisReferenceImage *>{createdReferenceValue}));
    QCOMPARE(zIndexValues, (QList<int>{4}));
    QCOMPARE(addedReferences, (QList<KisReferenceImage *>{createdReferenceValue}));
    QCOMPARE(switchedToolIds, (QStringList{QStringLiteral("ToolReferenceImages")}));
    QVERIFY(floatingMessages.isEmpty());
}

void KisNodeManagerReferenceImageContractTest::firstReferenceKeepsDefaultZIndex()
{
    KisNodeManager manager(nullptr);
    visibleProjectionValue = deviceToken(2);
    createdReferenceValue = token<KisReferenceImage>(1);

    manager.createReferenceImage(false);

    QCOMPARE(visibleProjectionRequests, 1);
    QVERIFY(zIndexReferences.isEmpty());
    QCOMPARE(addedReferences, (QList<KisReferenceImage *>{createdReferenceValue}));
    QCOMPARE(switchedToolIds, (QStringList{QStringLiteral("ToolReferenceImages")}));
}

void KisNodeManagerReferenceImageContractTest::failedCreationReportsInputSpecificMessage()
{
    KisNodeManager manager(nullptr);
    canvasWidgetAvailable = true;

    manager.createReferenceImage(true);
    manager.createReferenceImage(false);

    QCOMPARE(floatingMessages,
             (QStringList{i18nc("error dialog from the reference tool",
                                "Could not create a reference image from the active layer."),
                          i18nc("error dialog from the reference tool",
                                "Could not create a reference image from the visible canvas.")}));
    QCOMPARE(floatingMessageTimeouts, (QList<int>{5000, 5000}));
    QCOMPARE(floatingMessageHighPriorities, (QList<bool>{true, true}));
    QCOMPARE(floatingMessageSingleLines, (QList<bool>{true, true}));
    QVERIFY(addedReferences.isEmpty());
    QVERIFY(switchedToolIds.isEmpty());
}

QTEST_GUILESS_MAIN(KisNodeManagerReferenceImageContractTest)

#include "KisNodeManagerReferenceImageContractTest.moc"
