/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QHash>
#include <QPixmap>
#include <QTest>

#include <type_traits>

#include "kis_paint_ops_model.h"
#include "kis_paint_ops_model_source_p.h"

namespace
{

using PaintOpState = KisPaintOpsModelSource::PaintOpState;

QHash<KisPaintOpFactory *, PaintOpState> paintOpStates;
QString stableCategoryName;

KisPaintOpFactory *factory(quintptr value)
{
    return reinterpret_cast<KisPaintOpFactory *>(value);
}

QIcon icon(const QColor &color)
{
    QPixmap pixmap(4, 4);
    pixmap.fill(color);
    return QIcon(pixmap);
}

QModelIndex indexForCategory(const KisPaintOpListModel &model,
                             const QString &category)
{
    auto *item = model.categoriesMapper()->fetchCategory(category);
    return item ? model.index(model.categoriesMapper()->rowFromItem(item)) : QModelIndex();
}

QModelIndex indexForEntry(const KisPaintOpListModel &model,
                          const QString &category,
                          const QString &id)
{
    auto *item = model.categoriesMapper()->fetchEntry(category, KisPaintOpInfo(id));
    return item ? model.index(model.categoriesMapper()->rowFromItem(item)) : QModelIndex();
}

}

namespace KisPaintOpsModelSource
{

PaintOpState paintOpState(KisPaintOpFactory *factory)
{
    return paintOpStates.value(factory);
}

QString stableCategory()
{
    return stableCategoryName;
}

}

class KisPaintOpsModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void paintOpInfoHasStableValueSemantics();
    void listModelBuildsExpandedRows();
    void itemRolesExposeNameIconAndSortKey();
    void sortedModelPrioritizesStableCategoryAndPriority();
};

void KisPaintOpsModelContractTest::init()
{
    paintOpStates.clear();
    stableCategoryName = QStringLiteral("Brush engines");
}

void KisPaintOpsModelContractTest::paintOpInfoHasStableValueSemantics()
{
    static_assert(std::is_base_of_v<BasePaintOpCategorizedListModel,
                                    KisPaintOpListModel>);
    static_assert(std::is_base_of_v<QSortFilterProxyModel,
                                    KisSortedPaintOpListModel>);

    const QIcon marker = icon(Qt::red);
    const KisPaintOpInfo empty;
    const KisPaintOpInfo byId(QStringLiteral("pixel"));
    const KisPaintOpInfo complete(QStringLiteral("pixel"),
                                  QStringLiteral("Pixel"),
                                  QStringLiteral("Brush engines"),
                                  marker,
                                  12);
    const KisPaintOpInfo sameId(QStringLiteral("pixel"),
                                QStringLiteral("Renamed"),
                                QStringLiteral("Other"),
                                icon(Qt::blue),
                                99);
    PaintOpInfoToQStringConverter converter;

    QVERIFY(empty.id.isEmpty());
    QVERIFY(empty.name.isEmpty());
    QVERIFY(empty.category.isEmpty());
    QVERIFY(empty.icon.isNull());
    QCOMPARE(empty.priority, 0);
    QCOMPARE(byId.id, QStringLiteral("pixel"));
    QVERIFY(byId.name.isEmpty());
    QCOMPARE(byId.priority, 0);
    QCOMPARE(complete.id, QStringLiteral("pixel"));
    QCOMPARE(complete.name, QStringLiteral("Pixel"));
    QCOMPARE(complete.category, QStringLiteral("Brush engines"));
    QCOMPARE(complete.icon.cacheKey(), marker.cacheKey());
    QCOMPARE(complete.priority, 12);
    QVERIFY(complete == sameId);
    QVERIFY(!(complete == KisPaintOpInfo(QStringLiteral("chalk"))));
    QCOMPARE(converter(complete), QStringLiteral("Pixel"));
}

void KisPaintOpsModelContractTest::listModelBuildsExpandedRows()
{
    QObject parent;
    KisPaintOpListModel model(&parent);
    paintOpStates.insert(factory(1),
                         {QStringLiteral("pixel"),
                          QStringLiteral("Pixel"),
                          stableCategoryName,
                          icon(Qt::red),
                          12});
    paintOpStates.insert(factory(2),
                         {QStringLiteral("chalk"),
                          QStringLiteral("Chalk"),
                          QStringLiteral("Texture"),
                          icon(Qt::blue),
                          3});

    model.fill({factory(1), factory(2)});

    QCOMPARE(model.parent(), &parent);
    QCOMPARE(model.rowCount(QModelIndex()), 4);
    const QModelIndex stableCategory = indexForCategory(model, stableCategoryName);
    const QModelIndex textureCategory = indexForCategory(model, QStringLiteral("Texture"));
    const QModelIndex pixel = indexForEntry(model, stableCategoryName, QStringLiteral("pixel"));
    const QModelIndex chalk = indexForEntry(model, QStringLiteral("Texture"), QStringLiteral("chalk"));
    QVERIFY(stableCategory.isValid());
    QVERIFY(textureCategory.isValid());
    QVERIFY(pixel.isValid());
    QVERIFY(chalk.isValid());
    QVERIFY(model.data(stableCategory,
                       BasePaintOpCategorizedListModel::ExpandCategoryRole).toBool());
    QVERIFY(model.data(textureCategory,
                       BasePaintOpCategorizedListModel::ExpandCategoryRole).toBool());

    KisPaintOpInfo entry;
    QVERIFY(model.entryAt(entry, pixel));
    QCOMPARE(entry.id, QStringLiteral("pixel"));
    QCOMPARE(entry.name, QStringLiteral("Pixel"));
    QVERIFY(model.indexOf(KisPaintOpInfo(QStringLiteral("chalk"))).isValid());
}

void KisPaintOpsModelContractTest::itemRolesExposeNameIconAndSortKey()
{
    KisPaintOpListModel model(nullptr);
    const QIcon marker = icon(Qt::green);
    paintOpStates.insert(factory(1),
                         {QStringLiteral("ink"),
                          QStringLiteral("Ink"),
                          QStringLiteral("Drawing"),
                          marker,
                          7});
    model.fill({factory(1)});
    const QModelIndex category = indexForCategory(model, QStringLiteral("Drawing"));
    const QModelIndex ink = indexForEntry(model, QStringLiteral("Drawing"),
                                          QStringLiteral("ink"));

    QVERIFY(!model.data(QModelIndex()).isValid());
    QCOMPARE(model.data(category, Qt::DisplayRole).toString(), QStringLiteral("Drawing"));
    QVERIFY(!model.data(category, Qt::DecorationRole).isValid());
    QCOMPARE(model.data(ink, Qt::DisplayRole).toString(), QStringLiteral("Ink"));
    QCOMPARE(model.data(ink, Qt::DecorationRole).value<QIcon>().cacheKey(),
             marker.cacheKey());
    QCOMPARE(model.data(ink, BasePaintOpCategorizedListModel::SortRole).toString(),
             QStringLiteral("%1%2%3")
                 .arg(QStringLiteral("Drawing"))
                 .arg(7, 4)
                 .arg(QStringLiteral("Ink")));
}

void KisPaintOpsModelContractTest::sortedModelPrioritizesStableCategoryAndPriority()
{
    paintOpStates.insert(factory(1),
                         {QStringLiteral("texture"),
                          QStringLiteral("Texture Op"),
                          QStringLiteral("Texture"),
                          icon(Qt::yellow),
                          1});
    paintOpStates.insert(factory(2),
                         {QStringLiteral("zeta"),
                          QStringLiteral("Zeta"),
                          stableCategoryName,
                          icon(Qt::red),
                          20});
    paintOpStates.insert(factory(3),
                         {QStringLiteral("alpha"),
                          QStringLiteral("Alpha"),
                          stableCategoryName,
                          icon(Qt::blue),
                          3});

    QObject parent;
    KisSortedPaintOpListModel model(&parent);
    model.fill({factory(1), factory(2), factory(3)});
    model.sort(0);

    QCOMPARE(model.parent(), &parent);
    QCOMPARE(model.rowCount(), 5);
    QStringList labels;
    for (int row = 0; row < model.rowCount(); ++row) {
        labels.append(model.data(model.index(row, 0), Qt::DisplayRole).toString());
    }
    QCOMPARE(labels,
             QStringList({QStringLiteral("Brush engines"),
                          QStringLiteral("Alpha"),
                          QStringLiteral("Zeta"),
                          QStringLiteral("Texture"),
                          QStringLiteral("Texture Op")}));
    QVERIFY(model.indexOf(KisPaintOpInfo(QStringLiteral("alpha"))).isValid());
    KisPaintOpInfo entry;
    QVERIFY(model.entryAt(entry, model.index(1, 0)));
    QCOMPARE(entry.id, QStringLiteral("alpha"));
}

QTEST_MAIN(KisPaintOpsModelContractTest)

#include "KisPaintOpsModelContractTest.moc"
