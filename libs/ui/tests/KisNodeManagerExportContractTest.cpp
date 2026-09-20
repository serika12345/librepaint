/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <algorithm>

#include <QObject>
#include <QTest>

#include <klocalizedstring.h>

#pragma push_macro("Q_OBJECT")
#undef Q_OBJECT
#define Q_OBJECT
#include "nodes/kis_node_manager.h"
#pragma pop_macro("Q_OBJECT")

void kisSharedPtrAddReference(KisNode *)
{
}

bool kisSharedPtrRelease(KisNode *)
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

KisNodeSP nodeToken(quintptr id)
{
    return KisNodeSP(token<KisNode>(id));
}

KisNodeSP activeNodeValue;
KisPaintDevice *projectionValue = nullptr;
QRect imageBoundsValue;
QRect nodeBoundsValue;
QString nodeNameValue;
qreal imageXResolutionValue = 1.0;
qreal imageYResolutionValue = 1.0;
quint8 nodeOpacityValue = 0;
KisShapeLayer *shapeLayerValue = nullptr;
QString svgFilenameValue;
QSizeF imagePixelSizeValue;
QList<KoShape *> shapeValues;
bool saveSvgResult = true;
int activeNodeRequests = 0;
int noActiveNodeReports = 0;
int svgFilenameRequests = 0;
QStringList floatingMessages;
KisNodeList projectionRequests;
QList<KisPaintDevice *> savedDevices;
QStringList savedDeviceNames;
QList<QRect> savedDeviceBounds;
QList<qreal> savedDeviceXResolutions;
QList<qreal> savedDeviceYResolutions;
QList<quint8> savedDeviceOpacities;
KisNodeList shapeLayerRequests;
QList<QList<KoShape *>> sortRequests;
QStringList savedSvgFilenames;
QList<QSizeF> savedSvgSizes;
QList<QList<KoShape *>> savedSvgShapes;
QStringList svgFailureFilenames;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisNodeSP KisNodeManager::NodeExportAccess::activeNode(KisNodeManager *)
{
    ++activeNodeRequests;
    return activeNodeValue;
}

KisPaintDevice *KisNodeManager::NodeExportAccess::projection(KisNodeSP node)
{
    projectionRequests.append(node);
    return projectionValue;
}

void KisNodeManager::NodeExportAccess::reportNoActiveNode()
{
    ++noActiveNodeReports;
}

void KisNodeManager::NodeExportAccess::showFloatingMessage(KisNodeManager *, const QString &message)
{
    floatingMessages.append(message);
}

QRect KisNodeManager::NodeExportAccess::imageBounds(KisNodeManager *)
{
    return imageBoundsValue;
}

QRect KisNodeManager::NodeExportAccess::nodeBounds(KisNodeSP)
{
    return nodeBoundsValue;
}

QString KisNodeManager::NodeExportAccess::nodeName(KisNodeSP)
{
    return nodeNameValue;
}

qreal KisNodeManager::NodeExportAccess::imageXResolution(KisNodeManager *)
{
    return imageXResolutionValue;
}

qreal KisNodeManager::NodeExportAccess::imageYResolution(KisNodeManager *)
{
    return imageYResolutionValue;
}

quint8 KisNodeManager::NodeExportAccess::nodeOpacity(KisNodeSP)
{
    return nodeOpacityValue;
}

void KisNodeManager::NodeExportAccess::saveDevice(KisNodeManager *,
                                                  KisPaintDevice *device,
                                                  const QString &defaultName,
                                                  const QRect &bounds,
                                                  qreal xResolution,
                                                  qreal yResolution,
                                                  quint8 opacity)
{
    savedDevices.append(device);
    savedDeviceNames.append(defaultName);
    savedDeviceBounds.append(bounds);
    savedDeviceXResolutions.append(xResolution);
    savedDeviceYResolutions.append(yResolution);
    savedDeviceOpacities.append(opacity);
}

KisShapeLayer *KisNodeManager::NodeExportAccess::shapeLayer(KisNodeSP node)
{
    shapeLayerRequests.append(node);
    return shapeLayerValue;
}

QString KisNodeManager::NodeExportAccess::chooseSvgFilename(KisNodeManager *)
{
    ++svgFilenameRequests;
    return svgFilenameValue;
}

QSizeF KisNodeManager::NodeExportAccess::imagePixelSize(KisNodeManager *)
{
    return imagePixelSizeValue;
}

QList<KoShape *> KisNodeManager::NodeExportAccess::shapes(KisShapeLayer *)
{
    return shapeValues;
}

void KisNodeManager::NodeExportAccess::sortShapes(QList<KoShape *> *shapes)
{
    sortRequests.append(*shapes);
    std::reverse(shapes->begin(), shapes->end());
}

bool KisNodeManager::NodeExportAccess::saveSvg(const QString &filename,
                                               const QSizeF &sizeInPoints,
                                               const QList<KoShape *> &shapes)
{
    savedSvgFilenames.append(filename);
    savedSvgSizes.append(sizeInPoints);
    savedSvgShapes.append(shapes);
    return saveSvgResult;
}

void KisNodeManager::NodeExportAccess::showSvgFailure(const QString &filename)
{
    svgFailureFilenames.append(filename);
}

class KisNodeManagerExportContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void pixelExportRequiresActiveNodeAndProjection();
    void pixelExportUsesCombinedBoundsAndImageProperties();
    void svgExportRequiresShapeLayerAndFilename();
    void svgExportSortsShapesAndUsesPointSize();
    void svgFailureReportsFilename();
};

void KisNodeManagerExportContractTest::init()
{
    activeNodeValue.clear();
    projectionValue = nullptr;
    imageBoundsValue = QRect();
    nodeBoundsValue = QRect();
    nodeNameValue.clear();
    imageXResolutionValue = 1.0;
    imageYResolutionValue = 1.0;
    nodeOpacityValue = 0;
    shapeLayerValue = nullptr;
    svgFilenameValue.clear();
    imagePixelSizeValue = QSizeF();
    shapeValues.clear();
    saveSvgResult = true;
    activeNodeRequests = 0;
    noActiveNodeReports = 0;
    svgFilenameRequests = 0;
    floatingMessages.clear();
    projectionRequests.clear();
    savedDevices.clear();
    savedDeviceNames.clear();
    savedDeviceBounds.clear();
    savedDeviceXResolutions.clear();
    savedDeviceYResolutions.clear();
    savedDeviceOpacities.clear();
    shapeLayerRequests.clear();
    sortRequests.clear();
    savedSvgFilenames.clear();
    savedSvgSizes.clear();
    savedSvgShapes.clear();
    svgFailureFilenames.clear();
}

void KisNodeManagerExportContractTest::pixelExportRequiresActiveNodeAndProjection()
{
    KisNodeManager manager(nullptr);

    manager.saveNodeAsImage();

    QCOMPARE(activeNodeRequests, 1);
    QCOMPARE(noActiveNodeReports, 1);
    QVERIFY(projectionRequests.isEmpty());
    QVERIFY(savedDevices.isEmpty());

    activeNodeValue = nodeToken(1);
    manager.saveNodeAsImage();

    QCOMPARE(projectionRequests, (KisNodeList{activeNodeValue}));
    QCOMPARE(floatingMessages,
             (QStringList{i18nc("warning message when trying to export a transform mask", "Layer has no pixel data")}));
    QVERIFY(savedDevices.isEmpty());
}

void KisNodeManagerExportContractTest::pixelExportUsesCombinedBoundsAndImageProperties()
{
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);
    projectionValue = token<KisPaintDevice>(1);
    imageBoundsValue = QRect(0, 0, 100, 80);
    nodeBoundsValue = QRect(-10, 20, 30, 90);
    nodeNameValue = QStringLiteral("Layer 1");
    imageXResolutionValue = 2.0;
    imageYResolutionValue = 3.0;
    nodeOpacityValue = 117;

    manager.saveNodeAsImage();

    QCOMPARE(savedDevices, (QList<KisPaintDevice *>{projectionValue}));
    QCOMPARE(savedDeviceNames, (QStringList{nodeNameValue}));
    QCOMPARE(savedDeviceBounds, (QList<QRect>{imageBoundsValue | nodeBoundsValue}));
    QCOMPARE(savedDeviceXResolutions, (QList<qreal>{imageXResolutionValue}));
    QCOMPARE(savedDeviceYResolutions, (QList<qreal>{imageYResolutionValue}));
    QCOMPARE(savedDeviceOpacities, (QList<quint8>{nodeOpacityValue}));
}

void KisNodeManagerExportContractTest::svgExportRequiresShapeLayerAndFilename()
{
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);

    manager.saveVectorLayerAsImage();

    QCOMPARE(shapeLayerRequests, (KisNodeList{activeNodeValue}));
    QCOMPARE(svgFilenameRequests, 0);

    shapeLayerValue = token<KisShapeLayer>(1);
    manager.saveVectorLayerAsImage();

    QCOMPARE(svgFilenameRequests, 1);
    QVERIFY(savedSvgFilenames.isEmpty());
    QVERIFY(sortRequests.isEmpty());
}

void KisNodeManagerExportContractTest::svgExportSortsShapesAndUsesPointSize()
{
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);
    shapeLayerValue = token<KisShapeLayer>(1);
    svgFilenameValue = QStringLiteral("/tmp/layer.svg");
    imagePixelSizeValue = QSizeF(600.0, 300.0);
    imageXResolutionValue = 2.0;
    imageYResolutionValue = 3.0;
    shapeValues = {token<KoShape>(1), token<KoShape>(2)};

    manager.saveVectorLayerAsImage();

    QCOMPARE(sortRequests, (QList<QList<KoShape *>>{shapeValues}));
    QCOMPARE(savedSvgFilenames, (QStringList{svgFilenameValue}));
    QCOMPARE(savedSvgSizes, (QList<QSizeF>{QSizeF(300.0, 100.0)}));
    QCOMPARE(savedSvgShapes, (QList<QList<KoShape *>>{QList<KoShape *>{shapeValues.at(1), shapeValues.at(0)}}));
    QVERIFY(svgFailureFilenames.isEmpty());
}

void KisNodeManagerExportContractTest::svgFailureReportsFilename()
{
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);
    shapeLayerValue = token<KisShapeLayer>(1);
    svgFilenameValue = QStringLiteral("/tmp/layer.svg");
    imagePixelSizeValue = QSizeF(100.0, 100.0);
    saveSvgResult = false;

    manager.saveVectorLayerAsImage();

    QCOMPARE(svgFailureFilenames, (QStringList{svgFilenameValue}));
}

QTEST_GUILESS_MAIN(KisNodeManagerExportContractTest)

#include "KisNodeManagerExportContractTest.moc"
