/*
 *  SPDX-FileCopyrightText: 2017 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "GroupLayer.h"
#include "KoColorSpaceConstants.h"
#include "kis_assert.h"
#include "kis_types.h"
#include <kis_image.h>
#include <kis_group_layer.h>
#include <qobject.h>

GroupLayer::GroupLayer(KisImageSP image, QString name, QObject *parent) :
    Node(image, new KisGroupLayer(image, name, OPACITY_OPAQUE_U8), parent)
{

}

GroupLayer::GroupLayer(KisGroupLayerSP layer, QObject *parent):
    Node(layer->image(), layer, parent)
{

}

GroupLayer::~GroupLayer()
{

}

void GroupLayer::setPassThroughMode(bool passthrough)
{
    KisGroupLayer *group = dynamic_cast<KisGroupLayer*>(this->node().data());
    KIS_SAFE_ASSERT_RECOVER_RETURN(group);
    group->setPassThroughMode(passthrough);
}

bool GroupLayer::passThroughMode() const
{
    const KisGroupLayer *group = qobject_cast<const KisGroupLayer*>(this->node());
    return group->passThroughMode();
}

QString GroupLayer::type() const
{
    return "grouplayer";
}
