/*
 * tool_transform.cc -- Part of Krita
 *
 * SPDX-FileCopyrightText: 2004 Boudewijn Rempt (boud@valdyas.org)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tool_transform.h"

#include <klocalizedstring.h>

#include <kpluginfactory.h>

#include <KoToolRegistry.h>
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qpainterpath.h>


#include "kis_default_bounds_base.h"
#include "kis_pointer_utils.h"
#include "kis_tool_transform.h"
#include "kis_transform_mask_params_factory_registry.h"
#include "kis_transform_mask_adapter.h"
#include "KisAnimatedTransformMaskParamsHolder.h"
#include "kis_transform_mask_params_interface.h"
#include "transform_transaction_properties.h"

K_PLUGIN_FACTORY_WITH_JSON(ToolTransformFactory, "kritatooltransform.json", registerPlugin<ToolTransform>();)

namespace {

KisAnimatedTransformParamsHolderInterfaceSP createAnimatedParamsHolder(KisDefaultBoundsBaseSP defaultBounds)
{
    return toQShared(new KisAnimatedTransformMaskParamsHolder(defaultBounds));
}

} // namespace



ToolTransform::ToolTransform(QObject *parent, const QVariantList &)
        : QObject(parent)
{
    KoToolRegistry::instance()->add(new KisToolTransformFactory());
    KisTransformMaskParamsFactoryRegistry::instance()->setAnimatedParamsHolderFactory(&createAnimatedParamsHolder);
    KisTransformMaskParamsFactoryRegistry::instance()->addFactory("tooltransformparams", &KisTransformMaskAdapter::fromXML);
    KisTransformMaskParamsFactoryRegistry::instance()->addFactory("dumbparams", &KisTransformMaskAdapter::fromDumbXML);
    qRegisterMetaType<TransformTransactionProperties>("TransformTransactionProperties");
    qRegisterMetaType<ToolTransformArgs>("ToolTransformArgs");
    qRegisterMetaType<QPainterPath>("QPainterPath");
}

ToolTransform::~ToolTransform()
{
}

#include "tool_transform.moc"
