/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceItemChooserButtonSource_p.h"

#include "KisResourceItemListView.h"

#include <KisResourceLoaderRegistry.h>
#include <KisResourceUserOperations.h>
#include <KisTagFilterResourceProxyModel.h>
#include <KoFileDialog.h>

#include <QFileInfo>
#include <QStandardPaths>

#include <klocalizedstring.h>

namespace KisResourceItemChooserButtonSource
{
QStringList mimeTypes(const QString &resourceType)
{
    return KisResourceLoaderRegistry::instance()->mimeTypes(resourceType);
}

QStringList importFilenames(const QStringList &mimeTypes)
{
    KoFileDialog dialog(nullptr, KoFileDialog::OpenFiles, "OpenDocument");
    dialog.setMimeTypeFilters(mimeTypes);
    dialog.setDefaultDir(
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    dialog.setCaption(i18nc("@title:window", "Choose File to Add"));
    return dialog.filenames();
}

bool fileIsReadable(const QString &filename)
{
    const QFileInfo fileInfo(filename);
    return fileInfo.exists() && fileInfo.isReadable();
}

KoResourceSP importResource(QWidget *parent,
                            const QString &resourceType,
                            const QString &filename)
{
    return KisResourceUserOperations::importResourceFileWithUserInput(
        parent, QString(), resourceType, filename);
}

QModelIndex indexForResource(KisTagFilterResourceProxyModel *model,
                             KoResourceSP resource)
{
    return model->indexForResource(resource);
}

QModelIndex currentIndex(KisResourceItemListView *view)
{
    return view->currentIndex();
}

void setResourceInactive(KisTagFilterResourceProxyModel *model,
                         const QModelIndex &index)
{
    model->setResourceInactive(index);
}

QModelIndex index(KisTagFilterResourceProxyModel *model, int row, int column)
{
    return model->index(row, column);
}

void sort(KisTagFilterResourceProxyModel *model, int role)
{
    model->sort(role);
}
}
