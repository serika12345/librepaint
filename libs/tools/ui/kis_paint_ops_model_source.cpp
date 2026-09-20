/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_paint_ops_model_source_p.h"

#include <brushengine/kis_paintop_factory.h>

namespace KisPaintOpsModelSource
{

PaintOpState paintOpState(KisPaintOpFactory *factory)
{
    return {factory->id(),
            factory->name(),
            factory->category(),
            factory->icon(),
            factory->priority()};
}

QString stableCategory()
{
    return KisPaintOpFactory::categoryStable();
}

}
