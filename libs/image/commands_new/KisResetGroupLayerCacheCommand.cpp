/*
 *  SPDX-FileCopyrightText: 2021 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisResetGroupLayerCacheCommand.h"

#include "kis_command_utils.h"
#include "kis_group_layer.h"
#include "kis_types.h"

KisResetGroupLayerCacheCommand::KisResetGroupLayerCacheCommand(KisGroupLayerSP layer, const KoColorSpace *dstColorSpace, KisCommandUtils::FlipFlopCommand::State state)
    : KisCommandUtils::FlipFlopCommand(state)
    , m_layer(layer)
    , m_dstColorSpace(dstColorSpace)
{
}

void KisResetGroupLayerCacheCommand::partB()
{
    m_layer->resetCache(m_dstColorSpace);
}
