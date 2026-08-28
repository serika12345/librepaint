/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QSet>
#include <QTest>

#include <type_traits>

#include "kis_composite_ops_model.h"
#include "kis_composite_ops_model_source_p.h"

namespace
{

using CompositeOpMap = KisCompositeOpsModelSource::CompositeOpMap;

CompositeOpMap normalOps;
CompositeOpMap layerStyleOps;
QStringList configuredFavorites;
QStringList observedDefaultFavorites;
QStringList writtenFavorites;
QSet<QString> supportedOps;
bool useDefaultFavorites = false;
int favoritesWriteCount = 0;
int warningPixmapCount = 0;

KoID category(const QString &id, const QString &name)
{
    return KoID(id, name);
}

KoID operation(const QString &id, const QString &name)
{
    return KoID(id, name);
}

void addOperation(CompositeOpMap &map,
                  const KoID &categoryId,
                  const KoID &operationId)
{
    map.insert(categoryId, operationId);
}

void resetSourceState()
{
    normalOps.clear();
    layerStyleOps.clear();
    configuredFavorites.clear();
    observedDefaultFavorites.clear();
    writtenFavorites.clear();
    supportedOps.clear();
    useDefaultFavorites = false;
    favoritesWriteCount = 0;
    warningPixmapCount = 0;
}

QModelIndex indexForCategory(const KisCompositeOpListModel &model,
                             const QString &categoryName)
{
    auto *item = model.categoriesMapper()->fetchCategory(categoryName);
    return item ? model.index(model.categoriesMapper()->rowFromItem(item)) : QModelIndex();
}

QModelIndex indexForEntry(const KisCompositeOpListModel &model,
                          const QString &categoryName,
                          const KoID &entry)
{
    auto *item = model.categoriesMapper()->fetchEntry(categoryName, entry);
    return item ? model.index(model.categoriesMapper()->rowFromItem(item)) : QModelIndex();
}

}

namespace KisCompositeOpsModelSource
{

CompositeOpMap compositeOps()
{
    return normalOps;
}

CompositeOpMap layerStyleCompositeOps()
{
    return layerStyleOps;
}

bool colorSpaceHasCompositeOp(const KoColorSpace *colorSpace, const KoID &compositeOp)
{
    return colorSpace && supportedOps.contains(compositeOp.id());
}

KoID compositeOp(const QString &id)
{
    const auto findOperation = [&id](const CompositeOpMap &ops) {
        for (auto it = ops.cbegin(); it != ops.cend(); ++it) {
            if (it.value().id() == id) {
                return it.value();
            }
        }
        return KoID(id, QStringLiteral("Resolved ") + id);
    };

    KoID result = findOperation(normalOps);
    if (!result.id().isEmpty()) {
        return result;
    }
    return findOperation(layerStyleOps);
}

QStringList favoriteCompositeOps(const QStringList &defaultFavorites)
{
    observedDefaultFavorites = defaultFavorites;
    return useDefaultFavorites ? defaultFavorites : configuredFavorites;
}

void writeFavoriteCompositeOps(const QStringList &favorites)
{
    writtenFavorites = favorites;
    ++favoritesWriteCount;
}

QPixmap warningPixmap(const QSize &size)
{
    ++warningPixmapCount;
    QPixmap pixmap(size);
    pixmap.fill(Qt::red);
    return pixmap;
}

}

class KisCompositeOpsModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void sharedInstanceInitializesOnce();
    void identifiersAndDisplayNames();
    void normalInitializationBuildsCheckableFavoritesModel();
    void layerStyleInitializationBuildsExpandedReadOnlyModel();
    void favoritesReadWriteAndCheckStateStaySynchronized();
    void validationControlsAvailabilityAndWarningDecoration();
    void sortedModelsPrioritizeFavoritesAndSelectTheirOperationSet();
};

void KisCompositeOpsModelContractTest::init()
{
    resetSourceState();
}

void KisCompositeOpsModelContractTest::sharedInstanceInitializesOnce()
{
    const KoID arithmetic = category(QStringLiteral("arithmetic"), QStringLiteral("Arithmetic"));
    const KoID multiply = operation(QStringLiteral("multiply"), QStringLiteral("Multiply"));
    addOperation(normalOps, arithmetic, multiply);
    configuredFavorites = {multiply.id()};

    KisCompositeOpListModel *first = KisCompositeOpListModel::sharedInstance();
    KisCompositeOpListModel *second = KisCompositeOpListModel::sharedInstance();

    QCOMPARE(first, second);
    QVERIFY(first->indexOf(multiply).isValid());
    QCOMPARE(first->categoriesMapper()->itemsForCategory(
                 KisCompositeOpListModel::favoriteCategory().name()).size(),
             1);
}

void KisCompositeOpsModelContractTest::identifiersAndDisplayNames()
{
    static_assert(std::is_base_of_v<BaseKoIDCategorizedListModel,
                                    KisCompositeOpListModel>);
    static_assert(std::is_base_of_v<QSortFilterProxyModel,
                                    KisSortedCompositeOpListModel>);

    const KoID id(QStringLiteral("multiply"), QStringLiteral("Multiply"));
    KoIDToQStringConverter converter;
    KisCompositeOpListModel model;

    QCOMPARE(converter(id), QStringLiteral("Multiply"));
    QCOMPARE(model.categoryToString(id), QStringLiteral("Multiply"));
    QCOMPARE(model.entryToString(id), QStringLiteral("Multiply"));
    QCOMPARE(KisCompositeOpListModel::favoriteCategory().id(), QStringLiteral("favorites"));
    QVERIFY(!KisCompositeOpListModel::favoriteCategory().name().isEmpty());
    QVERIFY(!model.data(QModelIndex()).isValid());
    QVERIFY(!model.setData(QModelIndex(), Qt::Checked, Qt::CheckStateRole));
}

void KisCompositeOpsModelContractTest::normalInitializationBuildsCheckableFavoritesModel()
{
    const KoID arithmetic = category(QStringLiteral("arithmetic"), QStringLiteral("Arithmetic"));
    const KoID light = category(QStringLiteral("light"), QStringLiteral("Light"));
    const KoID multiply = operation(QStringLiteral("multiply"), QStringLiteral("Multiply"));
    const KoID screen = operation(QStringLiteral("screen"), QStringLiteral("Screen"));
    addOperation(normalOps, arithmetic, multiply);
    addOperation(normalOps, light, screen);
    configuredFavorites = {screen.id()};

    KisCompositeOpListModel model;
    model.initialize();

    const QModelIndex multiplyIndex = indexForEntry(model, arithmetic.name(), multiply);
    const QModelIndex screenIndex = indexForEntry(model, light.name(), screen);
    const QModelIndex favoritesIndex = indexForCategory(
        model, KisCompositeOpListModel::favoriteCategory().name());
    QVERIFY(multiplyIndex.isValid());
    QVERIFY(screenIndex.isValid());
    QVERIFY(favoritesIndex.isValid());
    QVERIFY(model.flags(multiplyIndex).testFlag(Qt::ItemIsUserCheckable));
    QCOMPARE(model.data(screenIndex, Qt::CheckStateRole).toInt(), int(Qt::Checked));
    QVERIFY(model.data(favoritesIndex, BaseKoIDCategorizedListModel::ExpandCategoryRole).toBool());
    QCOMPARE(model.categoriesMapper()->itemsForCategory(
                 KisCompositeOpListModel::favoriteCategory().name()).size(),
             1);
    QCOMPARE(observedDefaultFavorites,
             QStringList({QStringLiteral("normal"), QStringLiteral("erase"),
                          QStringLiteral("multiply"), QStringLiteral("burn"),
                          QStringLiteral("darken"), QStringLiteral("add"),
                          QStringLiteral("dodge"), QStringLiteral("screen"),
                          QStringLiteral("overlay"), QStringLiteral("soft_light_svg"),
                          QStringLiteral("luminize"), QStringLiteral("lighten"),
                          QStringLiteral("saturation"), QStringLiteral("color"),
                          QStringLiteral("divide")}));
}

void KisCompositeOpsModelContractTest::layerStyleInitializationBuildsExpandedReadOnlyModel()
{
    const KoID arithmetic = category(QStringLiteral("arithmetic"), QStringLiteral("Arithmetic"));
    const KoID multiply = operation(QStringLiteral("multiply"), QStringLiteral("Multiply"));
    addOperation(layerStyleOps, arithmetic, multiply);

    KisCompositeOpListModel model;
    model.initializeForLayerStyles();

    const QModelIndex categoryIndex = indexForCategory(model, arithmetic.name());
    const QModelIndex multiplyIndex = indexForEntry(model, arithmetic.name(), multiply);
    QVERIFY(categoryIndex.isValid());
    QVERIFY(multiplyIndex.isValid());
    QVERIFY(model.data(categoryIndex, BaseKoIDCategorizedListModel::ExpandCategoryRole).toBool());
    QVERIFY(!model.flags(multiplyIndex).testFlag(Qt::ItemIsUserCheckable));
    QVERIFY(!model.data(multiplyIndex, Qt::CheckStateRole).isValid());
    QVERIFY(!indexForCategory(model, KisCompositeOpListModel::favoriteCategory().name()).isValid());
    QVERIFY(observedDefaultFavorites.isEmpty());
}

void KisCompositeOpsModelContractTest::favoritesReadWriteAndCheckStateStaySynchronized()
{
    const KoID arithmetic = category(QStringLiteral("arithmetic"), QStringLiteral("Arithmetic"));
    const KoID multiply = operation(QStringLiteral("multiply"), QStringLiteral("Multiply"));
    const KoID divide = operation(QStringLiteral("divide"), QStringLiteral("Divide"));
    KisCompositeOpListModel model;
    auto *multiplyItem = model.categoriesMapper()->addEntry(arithmetic.name(), multiply);
    auto *divideItem = model.categoriesMapper()->addEntry(arithmetic.name(), divide);
    multiplyItem->setCheckable(true);
    divideItem->setCheckable(true);
    model.categoriesMapper()->addCategory(KisCompositeOpListModel::favoriteCategory().name());
    normalOps.insert(arithmetic, multiply);
    normalOps.insert(arithmetic, divide);
    configuredFavorites = {multiply.id()};

    model.readFavoriteCompositeOpsFromConfig();

    QCOMPARE(model.data(indexForEntry(model, arithmetic.name(), multiply),
                        Qt::CheckStateRole).toInt(),
             int(Qt::Checked));
    QCOMPARE(model.categoriesMapper()->itemsForCategory(
                 KisCompositeOpListModel::favoriteCategory().name()).size(),
             1);

    const QModelIndex divideIndex = indexForEntry(model, arithmetic.name(), divide);
    QVERIFY(model.setData(divideIndex, Qt::Checked, Qt::CheckStateRole));
    QCOMPARE(favoritesWriteCount, 1);
    QCOMPARE(writtenFavorites, QStringList({multiply.id(), divide.id()}));

    const QModelIndex multiplyIndex = indexForEntry(model, arithmetic.name(), multiply);
    QVERIFY(model.setData(multiplyIndex, Qt::Unchecked, Qt::CheckStateRole));
    QCOMPARE(favoritesWriteCount, 2);
    QCOMPARE(writtenFavorites, QStringList({divide.id()}));

    model.writeFavoriteCompositeOpsToConfig();
    QCOMPARE(favoritesWriteCount, 3);
    QCOMPARE(writtenFavorites, QStringList({divide.id()}));
}

void KisCompositeOpsModelContractTest::validationControlsAvailabilityAndWarningDecoration()
{
    const KoID arithmetic = category(QStringLiteral("arithmetic"), QStringLiteral("Arithmetic"));
    const KoID multiply = operation(QStringLiteral("multiply"), QStringLiteral("Multiply"));
    const KoID divide = operation(QStringLiteral("divide"), QStringLiteral("Divide"));
    addOperation(normalOps, arithmetic, multiply);
    addOperation(normalOps, arithmetic, divide);
    supportedOps = {multiply.id()};

    KisCompositeOpListModel model;
    model.initialize();
    model.validate(reinterpret_cast<const KoColorSpace *>(quintptr(1)));

    const QModelIndex multiplyIndex = indexForEntry(model, arithmetic.name(), multiply);
    const QModelIndex divideIndex = indexForEntry(model, arithmetic.name(), divide);
    QVERIFY(model.flags(multiplyIndex).testFlag(Qt::ItemIsEnabled));
    QVERIFY(!model.flags(divideIndex).testFlag(Qt::ItemIsEnabled));
    QVERIFY(!model.data(multiplyIndex, Qt::DecorationRole).isValid());
    const QPixmap warning = model.data(divideIndex, Qt::DecorationRole).value<QPixmap>();
    QVERIFY(!warning.isNull());
    QCOMPARE(warningPixmapCount, 1);
}

void KisCompositeOpsModelContractTest::sortedModelsPrioritizeFavoritesAndSelectTheirOperationSet()
{
    const KoID arithmetic = category(QStringLiteral("arithmetic"), QStringLiteral("Arithmetic"));
    const KoID light = category(QStringLiteral("light"), QStringLiteral("Light"));
    const KoID multiply = operation(QStringLiteral("multiply"), QStringLiteral("Multiply"));
    const KoID screen = operation(QStringLiteral("screen"), QStringLiteral("Screen"));
    addOperation(normalOps, light, screen);
    addOperation(normalOps, arithmetic, multiply);
    addOperation(layerStyleOps, arithmetic, multiply);
    configuredFavorites = {screen.id()};

    KisSortedCompositeOpListModel normalModel(false, nullptr);
    normalModel.sort(0);
    KoID firstEntry;
    bool foundFirstEntry = false;
    for (int row = 0; row < normalModel.rowCount(); ++row) {
        if (normalModel.entryAt(firstEntry, normalModel.index(row, 0))) {
            foundFirstEntry = true;
            break;
        }
    }
    QVERIFY(foundFirstEntry);
    QCOMPARE(firstEntry.id(), screen.id());
    QVERIFY(normalModel.indexOf(multiply).isValid());
    supportedOps = {multiply.id()};
    normalModel.validate(reinterpret_cast<const KoColorSpace *>(quintptr(1)));
    QVERIFY(!normalModel.flags(normalModel.indexOf(screen)).testFlag(Qt::ItemIsEnabled));

    KisSortedCompositeOpListModel layerStyleModel(true, nullptr);
    layerStyleModel.sort(0);
    QVERIFY(layerStyleModel.indexOf(multiply).isValid());
    QVERIFY(!layerStyleModel.indexOf(screen).isValid());
}

QTEST_MAIN(KisCompositeOpsModelContractTest)

#include "KisCompositeOpsModelContractTest.moc"
