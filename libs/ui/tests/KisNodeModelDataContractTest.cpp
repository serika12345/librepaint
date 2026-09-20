/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QColor>
#include <QHash>
#include <QTest>

#include "nodes/kis_node_model.h"

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

class TestNodeModel : public KisNodeModel
{
public:
    TestNodeModel()
        : KisNodeModel(nullptr)
    {
    }

    QModelIndex testIndex(int row = 2, int column = 0, quintptr id = 1) const
    {
        return createIndex(row, column, token<void>(id));
    }
};

bool facadeAvailable = true;
bool imageAvailable = true;
QStringList dataRequests;
QHash<QString, QVariant> dataResults;

QVariant requestedValue(const QString &request)
{
    dataRequests.append(request);
    return dataResults.value(request);
}

} // namespace

void *KisNodeModel::LifecycleAccess::createPrivateState(int)
{
    return token<void>(1);
}

void KisNodeModel::LifecycleAccess::connectUpdateCompressor(KisNodeModel *, void *)
{
}

void KisNodeModel::LifecycleAccess::connectThumbnailCache(KisNodeModel *, void *)
{
}

void KisNodeModel::LifecycleAccess::destroyPrivateState(void *)
{
}

bool KisNodeModel::DataAccess::hasDummiesFacade(const KisNodeModel *)
{
    return facadeAvailable;
}

bool KisNodeModel::DataAccess::hasImage(const KisNodeModel *)
{
    return imageAvailable;
}

QVariant KisNodeModel::DataAccess::nodeName(const KisNodeModel *, const QModelIndex &)
{
    return requestedValue(QStringLiteral("name"));
}

QVariant KisNodeModel::DataAccess::nodeIcon(const KisNodeModel *, const QModelIndex &)
{
    return requestedValue(QStringLiteral("icon"));
}

QVariant KisNodeModel::DataAccess::imageSize(const KisNodeModel *, const QModelIndex &)
{
    return requestedValue(QStringLiteral("size"));
}

QVariant KisNodeModel::DataAccess::foreground(const KisNodeModel *, const QModelIndex &)
{
    return requestedValue(QStringLiteral("foreground"));
}

QVariant KisNodeModel::DataAccess::font(const KisNodeModel *, const QModelIndex &)
{
    return requestedValue(QStringLiteral("font"));
}

QVariant KisNodeModel::DataAccess::properties(const KisNodeModel *, const QModelIndex &)
{
    return requestedValue(QStringLiteral("properties"));
}

QVariant KisNodeModel::DataAccess::aspectRatio(const KisNodeModel *, const QModelIndex &)
{
    return requestedValue(QStringLiteral("aspect-ratio"));
}

QVariant KisNodeModel::DataAccess::progress(const KisNodeModel *, const QModelIndex &)
{
    return requestedValue(QStringLiteral("progress"));
}

QVariant KisNodeModel::DataAccess::active(const KisNodeModel *, const QModelIndex &)
{
    return requestedValue(QStringLiteral("active"));
}

QVariant KisNodeModel::DataAccess::shouldGrayOut(const KisNodeModel *, const QModelIndex &)
{
    return requestedValue(QStringLiteral("gray-out"));
}

QVariant KisNodeModel::DataAccess::colorLabel(const KisNodeModel *, const QModelIndex &)
{
    return requestedValue(QStringLiteral("color-label"));
}

QVariant KisNodeModel::DataAccess::dropReason(const KisNodeModel *, const QModelIndex &)
{
    return requestedValue(QStringLiteral("drop-reason"));
}

QVariant KisNodeModel::DataAccess::isAnimated(const KisNodeModel *, const QModelIndex &)
{
    return requestedValue(QStringLiteral("animated"));
}

QVariant KisNodeModel::DataAccess::remainingData(const KisNodeModel *, const QModelIndex &, int role)
{
    return requestedValue(QStringLiteral("remaining:%1").arg(role));
}

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

void KisNodeModel::setShowGlobalSelection(bool)
{
}

void KisNodeModel::slotBeginInsertDummy(KisNodeDummy *, int, const QString &)
{
}

void KisNodeModel::slotEndInsertDummy(KisNodeDummy *)
{
}

void KisNodeModel::slotBeginRemoveDummy(KisNodeDummy *)
{
}

void KisNodeModel::slotEndRemoveDummy()
{
}

void KisNodeModel::slotDummyChanged(KisNodeDummy *)
{
}

void KisNodeModel::slotIsolatedModeChanged()
{
}

void KisNodeModel::slotNodeDisplayModeChanged(bool, bool)
{
}

void KisNodeModel::processUpdateQueue()
{
}

void KisNodeModel::progressPercentageChanged(int, const KisNodeSP)
{
}

void KisNodeModel::slotLayerThumbnailUpdated(KisNodeSP)
{
}

class KisNodeModelDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void unreadableStateReturnsNoData_data();
    void unreadableStateReturnsNoData();
    void standardRolesDispatchToTheirObservableValues_data();
    void standardRolesDispatchToTheirObservableValues();
    void specializedRolesPreserveTheRequestedRole_data();
    void specializedRolesPreserveTheRequestedRole();
};

void KisNodeModelDataContractTest::init()
{
    facadeAvailable = true;
    imageAvailable = true;
    dataRequests.clear();
    dataResults.clear();
}

void KisNodeModelDataContractTest::unreadableStateReturnsNoData_data()
{
    QTest::addColumn<bool>("facade");
    QTest::addColumn<bool>("validIndex");
    QTest::addColumn<bool>("image");

    QTest::newRow("no-facade") << false << true << true;
    QTest::newRow("invalid-index") << true << false << true;
    QTest::newRow("no-image") << true << true << false;
}

void KisNodeModelDataContractTest::unreadableStateReturnsNoData()
{
    QFETCH(bool, facade);
    QFETCH(bool, validIndex);
    QFETCH(bool, image);
    TestNodeModel model;
    facadeAvailable = facade;
    imageAvailable = image;

    const QModelIndex index = validIndex ? model.testIndex() : QModelIndex();
    QVERIFY(!model.data(index, Qt::DisplayRole).isValid());
    QVERIFY(dataRequests.isEmpty());
}

void KisNodeModelDataContractTest::standardRolesDispatchToTheirObservableValues_data()
{
    QTest::addColumn<int>("role");
    QTest::addColumn<QString>("request");
    QTest::addColumn<QVariant>("value");

    QTest::newRow("display") << int(Qt::DisplayRole) << QStringLiteral("name") << QVariant(QStringLiteral("Layer"));
    QTest::newRow("decoration") << int(Qt::DecorationRole) << QStringLiteral("icon") << QVariant(17);
    QTest::newRow("edit") << int(Qt::EditRole) << QStringLiteral("name") << QVariant(QStringLiteral("Renamed"));
    QTest::newRow("size") << int(Qt::SizeHintRole) << QStringLiteral("size") << QVariant(QSize(640, 480));
    QTest::newRow("foreground") << int(Qt::ForegroundRole) << QStringLiteral("foreground")
                                << QVariant(QColor(Qt::gray));
    QTest::newRow("font") << int(Qt::FontRole) << QStringLiteral("font") << QVariant(QStringLiteral("bold"));
    QTest::newRow("properties") << int(KisNodeModel::PropertiesRole) << QStringLiteral("properties") << QVariant(23);
    QTest::newRow("aspect-ratio") << int(KisNodeModel::AspectRatioRole) << QStringLiteral("aspect-ratio")
                                  << QVariant(4.0 / 3.0);
    QTest::newRow("progress") << int(KisNodeModel::ProgressRole) << QStringLiteral("progress") << QVariant(57);
    QTest::newRow("active") << int(KisNodeModel::ActiveRole) << QStringLiteral("active") << QVariant(true);
    QTest::newRow("gray-out") << int(KisNodeModel::ShouldGrayOutRole) << QStringLiteral("gray-out") << QVariant(false);
    QTest::newRow("color-label") << int(KisNodeModel::ColorLabelIndexRole) << QStringLiteral("color-label")
                                 << QVariant(6);
    QTest::newRow("drop-reason") << int(KisNodeModel::DropReasonRole) << QStringLiteral("drop-reason")
                                 << QVariant(QStringLiteral("unsupported"));
    QTest::newRow("animated") << int(KisNodeModel::IsAnimatedRole) << QStringLiteral("animated") << QVariant(true);
}

void KisNodeModelDataContractTest::standardRolesDispatchToTheirObservableValues()
{
    QFETCH(int, role);
    QFETCH(QString, request);
    QFETCH(QVariant, value);
    TestNodeModel model;
    dataResults.insert(request, value);

    QCOMPARE(model.data(model.testIndex(), role), value);
    QCOMPARE(dataRequests, QStringList({request}));
}

void KisNodeModelDataContractTest::specializedRolesPreserveTheRequestedRole_data()
{
    QTest::addColumn<int>("role");

    QTest::newRow("info") << int(KisNodeModel::InfoTextRole);
    QTest::newRow("filter-color") << int(KisNodeModel::FilterMaskColorRole);
    QTest::newRow("overlay-color") << int(KisNodeModel::LayerColorOverlayColorRole);
    QTest::newRow("thumbnail") << int(KisNodeModel::BeginThumbnailRole + 96);
    QTest::newRow("unknown") << int(Qt::WhatsThisRole);
}

void KisNodeModelDataContractTest::specializedRolesPreserveTheRequestedRole()
{
    QFETCH(int, role);
    TestNodeModel model;
    const QString request = QStringLiteral("remaining:%1").arg(role);
    const QVariant value = QStringLiteral("specialized");
    dataResults.insert(request, value);

    QCOMPARE(model.data(model.testIndex(), role), value);
    QCOMPARE(dataRequests, QStringList({request}));
}

QTEST_GUILESS_MAIN(KisNodeModelDataContractTest)

#include "KisNodeModelDataContractTest.moc"
