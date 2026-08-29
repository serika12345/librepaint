/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QTest>

#pragma push_macro("Q_OBJECT")
#undef Q_OBJECT
#define Q_OBJECT
#include "nodes/kis_node_model.h"
#pragma pop_macro("Q_OBJECT")

namespace
{

bool displayModeAdapterAvailable = false;
bool globalSelectionVisible = false;
QList<bool> globalSelectionRequests;
QList<int> thumbnailSizeRequests;
QList<KisIdleTasksManager *> idleTaskManagerRequests;
bool dummiesFacadeAvailable = false;

} // namespace

bool KisNodeModel::DisplayStateAccess::hasDisplayModeAdapter(const KisNodeModel *)
{
    return displayModeAdapterAvailable;
}

bool KisNodeModel::DisplayStateAccess::showGlobalSelectionMask(const KisNodeModel *)
{
    return globalSelectionVisible;
}

void KisNodeModel::DisplayStateAccess::setShowGlobalSelectionMask(KisNodeModel *, bool value)
{
    globalSelectionRequests.append(value);
}

void KisNodeModel::DisplayStateAccess::setPreferredThumbnailSize(const KisNodeModel *, int preferredSize)
{
    thumbnailSizeRequests.append(preferredSize);
}

void KisNodeModel::DisplayStateAccess::setIdleTaskManager(KisNodeModel *, KisIdleTasksManager *idleTasksManager)
{
    idleTaskManagerRequests.append(idleTasksManager);
}

bool KisNodeModel::DisplayStateAccess::hasDummiesFacade(const KisNodeModel *)
{
    return dummiesFacadeAvailable;
}

KisNodeModel::KisNodeModel(QObject *parent, int)
    : QAbstractItemModel(parent)
    , m_d(nullptr)
{
}

KisNodeModel::~KisNodeModel() = default;

int KisNodeModel::rowCount(const QModelIndex &) const
{
    return 0;
}

int KisNodeModel::columnCount(const QModelIndex &) const
{
    return 0;
}

QModelIndex KisNodeModel::index(int, int, const QModelIndex &) const
{
    return {};
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

class KisNodeModelDisplayStateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void globalSelectionDefaultsOffWithoutAdapter();
    void globalSelectionDelegatesToDisplayMode();
    void thumbnailPreferencesReachCache();
    void facadeAvailabilityReflectsState_data();
    void facadeAvailabilityReflectsState();
};

void KisNodeModelDisplayStateContractTest::init()
{
    displayModeAdapterAvailable = false;
    globalSelectionVisible = false;
    globalSelectionRequests.clear();
    thumbnailSizeRequests.clear();
    idleTaskManagerRequests.clear();
    dummiesFacadeAvailable = false;
}

void KisNodeModelDisplayStateContractTest::globalSelectionDefaultsOffWithoutAdapter()
{
    KisNodeModel model(nullptr);
    globalSelectionVisible = true;

    QCOMPARE(model.showGlobalSelection(), false);
    model.setShowGlobalSelection(true);

    QVERIFY(globalSelectionRequests.isEmpty());
}

void KisNodeModelDisplayStateContractTest::globalSelectionDelegatesToDisplayMode()
{
    KisNodeModel model(nullptr);
    displayModeAdapterAvailable = true;
    globalSelectionVisible = true;

    QCOMPARE(model.showGlobalSelection(), true);
    model.setShowGlobalSelection(false);

    QCOMPARE(globalSelectionRequests, QList<bool>({false}));
}

void KisNodeModelDisplayStateContractTest::thumbnailPreferencesReachCache()
{
    KisNodeModel model(nullptr);
    auto *idleTaskManager = reinterpret_cast<KisIdleTasksManager *>(quintptr(1));

    model.setPreferredThumnalSize(96);
    model.setIdleTaskManager(idleTaskManager);

    QCOMPARE(thumbnailSizeRequests, QList<int>({96}));
    QCOMPARE(idleTaskManagerRequests, QList<KisIdleTasksManager *>({idleTaskManager}));
}

void KisNodeModelDisplayStateContractTest::facadeAvailabilityReflectsState_data()
{
    QTest::addColumn<bool>("available");

    QTest::newRow("absent") << false;
    QTest::newRow("present") << true;
}

void KisNodeModelDisplayStateContractTest::facadeAvailabilityReflectsState()
{
    QFETCH(bool, available);
    KisNodeModel model(nullptr);
    dummiesFacadeAvailable = available;

    QCOMPARE(model.hasDummiesFacade(), available);
}

QTEST_MAIN(KisNodeModelDisplayStateContractTest)

#include "KisNodeModelDisplayStateContractTest.moc"
