/*
 *  SPDX-FileCopyrightText: 2018 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#ifndef KISTOOLPAINTFACTORYBASE_H
#define KISTOOLPAINTFACTORYBASE_H

#include <KoToolFactoryBase.h>

#include "kritatools_export.h"

class KRITATOOLS_EXPORT KisToolPaintFactoryBase : public KoToolFactoryBase
{
public:
    explicit KisToolPaintFactoryBase(const QString &id);
    ~KisToolPaintFactoryBase() override;
protected:
    QList<QAction *> createActionsImpl() override;

};

#endif // KISTOOLPAINTFACTORYBASE_H
