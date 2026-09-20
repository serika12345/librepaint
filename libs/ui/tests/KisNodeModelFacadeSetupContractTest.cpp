/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QSignalSpy>
#include <QTest>

#pragma push_macro("Q_OBJECT")
#undef Q_OBJECT
#define Q_OBJECT
#include "nodes/kis_node_model.h"
#pragma pop_macro("Q_OBJECT")

namespace
{

template<typename T>
T *token(quintptr id)
{
    return reinterpret_cast<T *>(id * 32 + 1);
}

KisDummiesFacadeBase *currentFacadeValue = nullptr;
KisShapeController *currentShapeControllerValue = nullptr;
bool currentImageAvailable = false;
QList<QString> operations;
KisShapeController *configuredShapeController = nullptr;
KisSelectionActionsAdapter *configuredSelectionActionsAdapter = nullptr;
KisNodeManager *configuredNodeManager = nullptr;
KisDummiesFacadeBase *disconnectedFacade = nullptr;

} // namespace

KisDummiesFacadeBase *KisNodeModel::FacadeSetupAccess::currentFacade(const KisNodeModel *)
{
    return currentFacadeValue;
}

KisShapeController *KisNodeModel::FacadeSetupAccess::currentShapeController(const KisNodeModel *)
{
    return currentShapeControllerValue;
}

bool KisNodeModel::FacadeSetupAccess::hasImage(const KisNodeModel *)
{
    return currentImageAvailable;
}

void KisNodeModel::FacadeSetupAccess::configureCollaborators(KisNodeModel *,
                                                             KisShapeController *shapeController,
                                                             KisSelectionActionsAdapter *selectionActionsAdapter,
                                                             KisNodeManager *nodeManager)
{
    operations.append(QStringLiteral("configureCollaborators"));
    configuredShapeController = shapeController;
    currentShapeControllerValue = shapeController;
    configuredSelectionActionsAdapter = selectionActionsAdapter;
    configuredNodeManager = nodeManager;
}

void KisNodeModel::FacadeSetupAccess::configureDisplayMode(KisNodeModel *, KisNodeManager *)
{
    operations.append(QStringLiteral("configureDisplayMode"));
}

void KisNodeModel::FacadeSetupAccess::disconnectCurrentTree(KisNodeModel *, KisDummiesFacadeBase *oldFacade)
{
    operations.append(QStringLiteral("disconnectCurrentTree"));
    disconnectedFacade = oldFacade;
}

void KisNodeModel::FacadeSetupAccess::replaceTree(KisNodeModel *,
                                                  KisDummiesFacadeBase *dummiesFacade,
                                                  const KisImageWSP &)
{
    operations.append(QStringLiteral("replaceTree"));
    currentFacadeValue = dummiesFacade;
}

void KisNodeModel::FacadeSetupAccess::connectCurrentTree(KisNodeModel *)
{
    operations.append(QStringLiteral("connectCurrentTree"));
}

KisNodeModel::KisNodeModel(QObject *parent, int)
    : QAbstractItemModel(parent)
    , m_d(nullptr)
{
}

KisNodeModel::~KisNodeModel() = default;

QModelIndex KisNodeModel::index(int, int, const QModelIndex &) const
{
    return {};
}

int KisNodeModel::rowCount(const QModelIndex &) const
{
    return 0;
}

int KisNodeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QModelIndex KisNodeModel::parent(const QModelIndex &) const
{
    return {};
}

QModelIndex KisNodeModel::sibling(int, int, const QModelIndex &) const
{
    return {};
}

QVariant KisNodeModel::data(const QModelIndex &, int) const
{
    return {};
}

Qt::ItemFlags KisNodeModel::flags(const QModelIndex &) const
{
    return Qt::NoItemFlags;
}

bool KisNodeModel::setData(const QModelIndex &, const QVariant &, int)
{
    return false;
}

QStringList KisNodeModel::mimeTypes() const
{
    return {};
}

QMimeData *KisNodeModel::mimeData(const QModelIndexList &) const
{
    return nullptr;
}

bool KisNodeModel::dropMimeData(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &)
{
    return false;
}

bool KisNodeModel::canDropMimeData(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &) const
{
    return false;
}

Qt::DropActions KisNodeModel::supportedDragActions() const
{
    return Qt::IgnoreAction;
}

Qt::DropActions KisNodeModel::supportedDropActions() const
{
    return Qt::IgnoreAction;
}

KisModelIndexConverterBase *KisNodeModel::createIndexConverter()
{
    return nullptr;
}

class KisNodeModelFacadeSetupContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void firstTreeAssignmentResetsModel();
    void sameTreeReconnectsWithoutReset();
    void clearingTreeDisconnectsAndResetsModel();
};

void KisNodeModelFacadeSetupContractTest::init()
{
    currentFacadeValue = nullptr;
    currentShapeControllerValue = nullptr;
    currentImageAvailable = false;
    operations.clear();
    configuredShapeController = nullptr;
    configuredSelectionActionsAdapter = nullptr;
    configuredNodeManager = nullptr;
    disconnectedFacade = nullptr;
}

void KisNodeModelFacadeSetupContractTest::firstTreeAssignmentResetsModel()
{
    KisNodeModel model(nullptr);
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    auto *facade = token<KisDummiesFacadeBase>(1);
    auto *shapeController = token<KisShapeController>(2);
    auto *selectionActionsAdapter = token<KisSelectionActionsAdapter>(3);
    auto *nodeManager = token<KisNodeManager>(4);

    model.setDummiesFacade(facade, {}, shapeController, selectionActionsAdapter, nodeManager);

    QCOMPARE(operations,
             QList<QString>({QStringLiteral("configureCollaborators"),
                             QStringLiteral("configureDisplayMode"),
                             QStringLiteral("replaceTree"),
                             QStringLiteral("connectCurrentTree")}));
    QCOMPARE(configuredShapeController, shapeController);
    QCOMPARE(configuredSelectionActionsAdapter, selectionActionsAdapter);
    QCOMPARE(configuredNodeManager, nodeManager);
    QCOMPARE(currentFacadeValue, facade);
    QCOMPARE(resetSpy.count(), 1);
}

void KisNodeModelFacadeSetupContractTest::sameTreeReconnectsWithoutReset()
{
    KisNodeModel model(nullptr);
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    auto *facade = token<KisDummiesFacadeBase>(1);
    auto *shapeController = token<KisShapeController>(2);
    currentFacadeValue = facade;
    currentShapeControllerValue = shapeController;
    currentImageAvailable = true;

    model.setDummiesFacade(facade, {}, shapeController, nullptr, nullptr);

    QCOMPARE(operations,
             QList<QString>({QStringLiteral("configureCollaborators"),
                             QStringLiteral("configureDisplayMode"),
                             QStringLiteral("disconnectCurrentTree"),
                             QStringLiteral("replaceTree"),
                             QStringLiteral("connectCurrentTree")}));
    QCOMPARE(disconnectedFacade, facade);
    QCOMPARE(resetSpy.count(), 0);
}

void KisNodeModelFacadeSetupContractTest::clearingTreeDisconnectsAndResetsModel()
{
    KisNodeModel model(nullptr);
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    auto *facade = token<KisDummiesFacadeBase>(1);
    currentFacadeValue = facade;
    currentShapeControllerValue = token<KisShapeController>(2);
    currentImageAvailable = true;

    model.setDummiesFacade(nullptr, {}, nullptr, nullptr, nullptr);

    QCOMPARE(operations,
             QList<QString>({QStringLiteral("configureCollaborators"),
                             QStringLiteral("configureDisplayMode"),
                             QStringLiteral("disconnectCurrentTree"),
                             QStringLiteral("replaceTree")}));
    QCOMPARE(disconnectedFacade, facade);
    QVERIFY(!currentFacadeValue);
    QCOMPARE(resetSpy.count(), 1);
}

QTEST_MAIN(KisNodeModelFacadeSetupContractTest)

#include "KisNodeModelFacadeSetupContractTest.moc"
