/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisStorageChooserDelegate.h"

#include <QApplication>
#include <QPainter>
#include <QStyle>

#include <KoIcon.h>

#include "KisStorageModel.h"

KisStorageChooserDelegate::KisStorageChooserDelegate(QObject *parent)
    : QAbstractItemDelegate(parent)
{
}

void KisStorageChooserDelegate::paint(QPainter *painter,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }

    painter->save();

    QString name = index.sibling(index.row(), KisStorageModel::DisplayName).data(Qt::DisplayRole).value<QString>();
    QString location = index.sibling(index.row(), KisStorageModel::Location).data(Qt::DisplayRole).value<QString>();
    bool active = index.data(Qt::UserRole + KisStorageModel::Active).value<bool>();
    QString storageType = index.data(Qt::UserRole + KisStorageModel::StorageType).value<QString>();

    QImage thumbnail = index.data(Qt::UserRole + KisStorageModel::Thumbnail).value<QImage>();

    qreal devicePixelRatioF = painter->device()->devicePixelRatioF();

    if (thumbnail.isNull()) {
        thumbnail = koIcon("warning").pixmap(option.decorationSize).toImage();
        if (storageType == "Folder") {
            thumbnail = koIcon("document-open").pixmap(option.decorationSize).toImage();
        } else if (storageType == "Adobe Style Library") {
            thumbnail = koIcon("layer-style-enabled").pixmap(option.decorationSize).toImage();
            if (!thumbnail.isNull()) {
                thumbnail = thumbnail.scaled(option.decorationSize, Qt::KeepAspectRatio, Qt::FastTransformation);
            }
        } else if (storageType == "Adobe Brush Library") {
            thumbnail = koIcon("select-all").pixmap(option.decorationSize).toImage();
        } else if (storageType == "Memory") {
            if (location != "memory") {
                thumbnail = koIcon("document-new").pixmap(option.decorationSize).toImage();
            } else {
                thumbnail = koIcon("drive-harddisk").pixmap(option.decorationSize).toImage();
            }
        } else if (storageType == "Bundle") {
            thumbnail = koIcon("bundle_archive").pixmap(option.decorationSize).toImage();
        }
    } else {
        if (!thumbnail.isNull()) {
            thumbnail = thumbnail.scaled(option.decorationSize * devicePixelRatioF,
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
        }
        thumbnail.setDevicePixelRatio(devicePixelRatioF);
    }

    QColor penColor(option.palette.text().color());

    QStyleOptionViewItem opt = option;

    if (active) {
        opt.state = QStyle::State_Sunken;
    }

    QApplication::style()->drawPrimitive(QStyle::PE_PanelButtonTool, &opt, painter);

    painter->setPen(penColor);
    painter->drawImage(option.rect.topLeft() + QPoint(4, 4), thumbnail, thumbnail.rect());

    QStyleOptionButton checkboxOption;
    checkboxOption.rect = QRect(option.rect.x() + option.decorationSize.width() + 8,
                                option.rect.y() + option.rect.height() - 30,
                                20,
                                20);
    checkboxOption.state = active ? QStyle::State_On : QStyle::State_Off;
    QApplication::style()->drawControl(QStyle::CE_CheckBox, &checkboxOption, painter);

    QRect text = option.rect;
    text.setLeft(text.left() + option.decorationSize.width() + 8);
    text.setTop(text.top() + 4);
    painter->drawText(text, Qt::TextWordWrap, name.split("_").join(" "));

    painter->restore();
}

QSize KisStorageChooserDelegate::sizeHint(const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QSize(200, option.decorationSize.height() + 8);
}
