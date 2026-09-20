/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_model.h"

QVariant KisNodeModel::data(const QModelIndex &index, int role) const
{
    if (!DataAccess::hasDummiesFacade(this) || !index.isValid() || !DataAccess::hasImage(this)) {
        return {};
    }

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return DataAccess::nodeName(this, index);
    case Qt::DecorationRole:
        return DataAccess::nodeIcon(this, index);
    case Qt::SizeHintRole:
        return DataAccess::imageSize(this, index);
    case Qt::ForegroundRole:
        return DataAccess::foreground(this, index);
    case Qt::FontRole:
        return DataAccess::font(this, index);
    case KisNodeModel::PropertiesRole:
        return DataAccess::properties(this, index);
    case KisNodeModel::AspectRatioRole:
        return DataAccess::aspectRatio(this, index);
    case KisNodeModel::ProgressRole:
        return DataAccess::progress(this, index);
    case KisNodeModel::ActiveRole:
        return DataAccess::active(this, index);
    case KisNodeModel::ShouldGrayOutRole:
        return DataAccess::shouldGrayOut(this, index);
    case KisNodeModel::ColorLabelIndexRole:
        return DataAccess::colorLabel(this, index);
    case KisNodeModel::DropReasonRole:
        return DataAccess::dropReason(this, index);
    case KisNodeModel::IsAnimatedRole:
        return DataAccess::isAnimated(this, index);
    default:
        return DataAccess::remainingData(this, index, role);
    }
}
