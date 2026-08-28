/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2010 Cyrille Berger <cberger@cberger.net>
 * SPDX-FileCopyrightText: 2011 Silvio Heinrich <plassyqweb.de>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kis_paintop_options_model.h"
#include "kis_paintop_options_model_source_p.h"

#include <klocalizedstring.h>
#include <kis_assert.h>


KisPaintOpOptionListModel::KisPaintOpOptionListModel(QObject *parent)
    : BaseOptionCategorizedListModel(parent)
{
    connect(&m_stateSignalsMapper, &QSignalMapper::mappedInt,
            this, &KisPaintOpOptionListModel::slotCheckedEnabledStateChanged);
}

QString KisPaintOpOptionListModel::categoryName(KisPaintOpOption::PaintopCategory categoryType)
{
    QString category;
    switch(categoryType) {
    case KisPaintOpOption::GENERAL:
        category = i18nc("option category", "General");
        break;
    case KisPaintOpOption::COLOR:
        category = i18nc("option category", "Color");
        break;
    case KisPaintOpOption::TEXTURE:
        category = i18nc("option category", "Texture");
        break;
    case KisPaintOpOption::FILTER:
        category = i18nc("option category", "Filter");
        break;
    case KisPaintOpOption::MASKING_BRUSH:
        category = i18nc("option category", "Masked Brush");
        break;
    };
    return category;
}

void KisPaintOpOptionListModel::addPaintOpOption(KisPaintOpOption *option, int widgetIndex, const QString &label, const QString &category) {

    DataItem *item = categoriesMapper()->addEntry(category, KisOptionInfo(option, widgetIndex, label));
    const auto state = KisPaintOpOptionsModelSource::optionState(option);

    if (state.checkable) {
        item->setCheckable(true);
        item->setChecked(state.checked);
    }

    item->setEnabled(state.enabled);
    KisPaintOpOptionsModelSource::connectStateChanges(
        option,
        &m_stateSignalsMapper,
        categoriesMapper()->rowFromItem(item),
        state.checkable);

    categoriesMapper()->expandAllCategories();
}

QVariant KisPaintOpOptionListModel::data(const QModelIndex& idx, int role) const
{
    return BaseOptionCategorizedListModel::data(idx, role);
}

bool KisPaintOpOptionListModel::setData(const QModelIndex& idx, const QVariant& value, int role)
{
    if (!idx.isValid()) return false;

    DataItem *item = categoriesMapper()->itemFromRow(idx.row());
    Q_ASSERT(item);

    if (role == Qt::CheckStateRole && item->isCheckable()) {
        KisPaintOpOptionsModelSource::setChecked(
            item->data()->option, value.toInt() == Qt::Checked);
    }

    return BaseOptionCategorizedListModel::setData(idx, value, role);
}

bool operator==(const KisOptionInfo& a, const KisOptionInfo& b)
{
    if (a.index != b.index) return false;
    if (!a.option || !b.option) return a.option == b.option;

    const auto aState = KisPaintOpOptionsModelSource::optionState(a.option);
    const auto bState = KisPaintOpOptionsModelSource::optionState(b.option);
    if (aState.objectName != bState.objectName) return false;
    if (aState.category != bState.category) return false;
    if (aState.checkable != bState.checkable) return false;
    if (aState.checked != bState.checked) return false;
    return true;
}
void KisPaintOpOptionListModel::signalDataChanged(const QModelIndex& index)
{
    Q_EMIT dataChanged(index,index);
}

void KisPaintOpOptionListModel::slotCheckedEnabledStateChanged(int row)
{
    QModelIndex idx(index(row));

    DataItem *item = categoriesMapper()->itemFromRow(row);
    KIS_SAFE_ASSERT_RECOVER_RETURN(item);
    const auto state = KisPaintOpOptionsModelSource::optionState(item->data()->option);

    if (state.enabled != item->isEnabled()) {
        item->setEnabled(state.enabled);
    }

    if (state.checked != item->isChecked()) {
        item->setChecked(state.checked);
    }

    Q_EMIT dataChanged(idx, idx);
}
