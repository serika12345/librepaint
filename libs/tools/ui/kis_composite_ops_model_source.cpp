/*
 *  SPDX-FileCopyrightText: 2009 Cyrille Berger <cberger@cberger.net>
 *  SPDX-FileCopyrightText: 2011 Silvio Heinrich <plassy@web.de>
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_composite_ops_model_source_p.h"

#include <QApplication>
#include <QStyle>
#include <QStyleOptionButton>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KoCompositeOpRegistry.h>
#include <kis_icon.h>

namespace
{

KConfigGroup applicationConfig()
{
    return KSharedConfig::openConfig()->group(QString());
}

}

namespace KisCompositeOpsModelSource
{

CompositeOpMap compositeOps()
{
    return KoCompositeOpRegistry::instance().getCompositeOps();
}

CompositeOpMap layerStyleCompositeOps()
{
    return KoCompositeOpRegistry::instance().getLayerStylesCompositeOps();
}

bool colorSpaceHasCompositeOp(const KoColorSpace *colorSpace, const KoID &compositeOp)
{
    return KoCompositeOpRegistry::instance().colorSpaceHasCompositeOp(colorSpace, compositeOp);
}

KoID compositeOp(const QString &id)
{
    return KoCompositeOpRegistry::instance().getKoID(id);
}

QStringList favoriteCompositeOps(const QStringList &defaultFavorites)
{
    return applicationConfig().readEntry(QStringLiteral("favoriteCompositeOps"), defaultFavorites);
}

void writeFavoriteCompositeOps(const QStringList &favorites)
{
    KConfigGroup config = applicationConfig();
    config.writeEntry(QStringLiteral("favoriteCompositeOps"), favorites);
    config.sync();
}

QPixmap warningPixmap(const QSize &size)
{
    return KisIconUtils::loadIcon("warning").pixmap(size);
}

}
