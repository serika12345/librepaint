/*
 *  SPDX-FileCopyrightText: 2009 Cyrille Berger <cberger@cberger.net>
 *  SPDX-FileCopyrightText: 2011 Silvio Heinrich <plassy@web.de>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_composite_ops_model.h"

#include <QApplication>
#include <QStyle>
#include <QStyleOptionButton>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KoCompositeOp.h>
#include <KoCompositeOpRegistry.h>
#include <kis_icon.h>

#include "kis_debug.h"

namespace {

const QStringList defaultFavoriteCompositeOps = QStringLiteral(
    "normal,erase,multiply,burn,darken,add,dodge,screen,overlay,soft_light_svg,luminize,lighten,saturation,color,divide")
                                                    .split(QLatin1Char(','));

KConfigGroup applicationConfig()
{
    return KSharedConfig::openConfig()->group(QString());
}

}

KoID KisCompositeOpListModel::favoriteCategory() {
    static KoID category("favorites", ki18n("Favorites"));
    return category;
}

void KisCompositeOpListModel::initialize()
{
    auto ops = KoCompositeOpRegistry::instance().getCompositeOps();
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QMapIterator<KoID, KoID> it(ops);
#else
    QMultiMapIterator<KoID, KoID> it(ops);
#endif


    while (it.hasNext()) {
        KoID op = *it.next();
        KoID category = it.key();
        BaseKoIDCategorizedListModel::DataItem *item = categoriesMapper()->addEntry(category.name(), op);
        item->setCheckable(true);
    }

    BaseKoIDCategorizedListModel::DataItem *item = categoriesMapper()->addCategory(favoriteCategory().name());
    item->setExpanded(true);

    readFavoriteCompositeOpsFromConfig();
}

void KisCompositeOpListModel::initializeForLayerStyles()
{
    auto ops = KoCompositeOpRegistry::instance().getLayerStylesCompositeOps();
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QMapIterator<KoID, KoID> it(ops);
#else
    QMultiMapIterator<KoID, KoID> it(ops);
#endif


    while (it.hasNext()) {
        KoID op = *it.next();
        KoID category = it.key();
        BaseKoIDCategorizedListModel::DataItem *item = categoriesMapper()->addEntry(category.name(), op);
        item->setCheckable(false);
    }

   categoriesMapper()->expandAllCategories();
}

KisCompositeOpListModel* KisCompositeOpListModel::sharedInstance()
{
    static KisCompositeOpListModel *model = 0;

    if (!model) {
        model = new KisCompositeOpListModel();
        model->initialize();
    }

    return model;
}

void KisCompositeOpListModel::validate(const KoColorSpace *cs)
{
    for (int i = 0, size = categoriesMapper()->rowCount(); i < size; i++) {
        DataItem *item = categoriesMapper()->itemFromRow(i);

        if (!item->isCategory()) {
            bool value = KoCompositeOpRegistry::instance().colorSpaceHasCompositeOp(cs, *item->data());
            item->setEnabled(value);
        }
    }
}

bool KisCompositeOpListModel::setData(const QModelIndex& idx, const QVariant& value, int role)
{
    if (!idx.isValid()) return false;

    bool result = BaseKoIDCategorizedListModel::setData(idx, value, role);

    DataItem *item = categoriesMapper()->itemFromRow(idx.row());
    Q_ASSERT(item);


    if(role == Qt::CheckStateRole) {
        if (item->isChecked()) {
            addFavoriteEntry(*item->data());
        } else {
            removeFavoriteEntry(*item->data());
        }

        writeFavoriteCompositeOpsToConfig();
    }

    return result;
}

QVariant KisCompositeOpListModel::data(const QModelIndex& idx, int role) const
{
    if (!idx.isValid()) return QVariant();

    if(role == Qt::DecorationRole) {
        DataItem *item = categoriesMapper()->itemFromRow(idx.row());
        Q_ASSERT(item);

        if (!item->isCategory() && !item->isEnabled()) {
            QStyle *style = QApplication::style();
            QStyleOptionButton so;
            QSize size = style->sizeFromContents(QStyle::CT_CheckBox, &so, QSize(), 0);
            const QPixmap pixmap = KisIconUtils::loadIcon("warning").pixmap(size);
            return pixmap;
        }
    }

    return BaseKoIDCategorizedListModel::data(idx, role);
}

void KisCompositeOpListModel::addFavoriteEntry(const KoID &entry)
{
    DataItem *item = categoriesMapper()->addEntry(favoriteCategory().name(), entry);
    item->setCheckable(false);
}

void KisCompositeOpListModel::removeFavoriteEntry(const KoID &entry)
{
    categoriesMapper()->removeEntry(favoriteCategory().name(), entry);
}

void KisCompositeOpListModel::readFavoriteCompositeOpsFromConfig()
{
    const QStringList favoriteCompositeOps =
        applicationConfig().readEntry(QStringLiteral("favoriteCompositeOps"), defaultFavoriteCompositeOps);

    Q_FOREACH (const QString &op, favoriteCompositeOps) {
        KoID entry = KoCompositeOpRegistry::instance().getKoID(op);

        DataItem *item = categoriesMapper()->fetchOneEntry(entry);
        if (item) {
            item->setChecked(true);
        }

        addFavoriteEntry(entry);
    }
}

void KisCompositeOpListModel::writeFavoriteCompositeOpsToConfig() const
{
    QStringList list;
    QVector<DataItem*> filteredItems =
        categoriesMapper()->itemsForCategory(favoriteCategory().name());

    Q_FOREACH (DataItem *item, filteredItems) {
        list.append(item->data()->id());
    }

    KConfigGroup config = applicationConfig();
    config.writeEntry(QStringLiteral("favoriteCompositeOps"), list);
    config.sync();
}
