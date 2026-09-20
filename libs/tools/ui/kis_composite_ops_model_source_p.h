/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_COMPOSITE_OPS_MODEL_SOURCE_P_H
#define KIS_COMPOSITE_OPS_MODEL_SOURCE_P_H

#include <QMultiMap>
#include <QPixmap>
#include <QSize>
#include <QStringList>

#include <KoID.h>

class KoColorSpace;

namespace KisCompositeOpsModelSource
{

using CompositeOpMap = QMultiMap<KoID, KoID>;

CompositeOpMap compositeOps();
CompositeOpMap layerStyleCompositeOps();
bool colorSpaceHasCompositeOp(const KoColorSpace *colorSpace, const KoID &compositeOp);
KoID compositeOp(const QString &id);
QStringList favoriteCompositeOps(const QStringList &defaultFavorites);
void writeFavoriteCompositeOps(const QStringList &favorites);
QPixmap warningPixmap(const QSize &size);

}

#endif
