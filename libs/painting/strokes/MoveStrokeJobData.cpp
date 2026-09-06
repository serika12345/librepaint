/*
 * SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "move_stroke_strategy.h"

MoveStrokeStrategy::Data::Data(QPoint _offset)
    : KisStrokeJobData(SEQUENTIAL, NORMAL)
    , offset(_offset)
{
}

KisStrokeJobData *MoveStrokeStrategy::Data::createLodClone(int levelOfDetail)
{
    return new Data(*this, levelOfDetail);
}

MoveStrokeStrategy::Data::Data(const MoveStrokeStrategy::Data &rhs, int levelOfDetail)
    : KisStrokeJobData(rhs)
{
    KisLodTransform transform(levelOfDetail);
    offset = transform.map(rhs.offset);
}

MoveStrokeStrategy::PickLayerData::PickLayerData(QPoint _pos)
    : KisStrokeJobData(SEQUENTIAL, NORMAL)
    , pos(_pos)
{
}

KisStrokeJobData *MoveStrokeStrategy::PickLayerData::createLodClone(int levelOfDetail)
{
    return new PickLayerData(*this, levelOfDetail);
}

MoveStrokeStrategy::PickLayerData::PickLayerData(const MoveStrokeStrategy::PickLayerData &rhs, int levelOfDetail)
    : KisStrokeJobData(rhs)
{
    KisLodTransform transform(levelOfDetail);
    pos = transform.map(rhs.pos);
}

MoveStrokeStrategy::BarrierUpdateData::BarrierUpdateData(bool forceUpdate)
    : KisAsynchronousStrokeUpdateHelper::UpdateData(forceUpdate, BARRIER, EXCLUSIVE)
{
}

KisStrokeJobData *MoveStrokeStrategy::BarrierUpdateData::createLodClone(int levelOfDetail)
{
    return new BarrierUpdateData(*this, levelOfDetail);
}

MoveStrokeStrategy::BarrierUpdateData::BarrierUpdateData(const MoveStrokeStrategy::BarrierUpdateData &rhs,
                                                         int levelOfDetail)
    : KisAsynchronousStrokeUpdateHelper::UpdateData(rhs, levelOfDetail)
{
}
