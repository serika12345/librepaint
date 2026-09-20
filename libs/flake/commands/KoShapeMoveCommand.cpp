/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2006 Jan Hambrecht <jaham@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoShapeMoveCommand.h"

#include "kis_command_ids.h"
#include <KoShape.h>
#include <KoShapeBulkActionLock.h>
#include <kis_assert.h>
#include <klocalizedstring.h>

namespace
{
using PositionReader = QPointF (*)(const KoShape *shape, KoFlake::AnchorPosition anchor);
using PositionBatchApplier = void (*)(const QList<KoShape *> &shapes,
                                      const QList<QPointF> &positions,
                                      KoFlake::AnchorPosition anchor);

QPointF readPosition(const KoShape *shape, KoFlake::AnchorPosition anchor)
{
    return shape->absolutePosition(anchor);
}

void applyPositionBatch(const QList<KoShape *> &shapes, const QList<QPointF> &positions, KoFlake::AnchorPosition anchor)
{
    KoShapeBulkActionLock lock(shapes);

    for (int i = 0; i < shapes.count(); ++i) {
        shapes.at(i)->setAbsolutePosition(positions.at(i), anchor);
    }

    KoShapeBulkActionLock::bulkShapesUpdate(lock.unlock());
}

PositionReader activePositionReader = readPosition;
PositionBatchApplier activePositionBatchApplier = applyPositionBatch;
} // namespace

#if defined(KRITAFLAKE_SHAPE_MOVE_COMMAND_CONTRACT_TESTING)
namespace KoShapeMoveCommandTesting
{
Q_DECL_HIDDEN void setShapeAccessForTesting(PositionReader reader, PositionBatchApplier applier)
{
    activePositionReader = reader;
    activePositionBatchApplier = applier;
}

Q_DECL_HIDDEN void resetShapeAccessForTesting()
{
    activePositionReader = readPosition;
    activePositionBatchApplier = applyPositionBatch;
}
} // namespace KoShapeMoveCommandTesting
#endif

class Q_DECL_HIDDEN KoShapeMoveCommand::Private
{
public:
    QList<KoShape *> shapes;
    QList<QPointF> previousPositions, newPositions;
    KoFlake::AnchorPosition anchor;
};

KoShapeMoveCommand::KoShapeMoveCommand(const QList<KoShape *> &shapes,
                                       QList<QPointF> &previousPositions,
                                       QList<QPointF> &newPositions,
                                       KoFlake::AnchorPosition anchor,
                                       KUndo2Command *parent)
    : KUndo2Command(kundo2_i18n("Move shapes"), parent)
    , d(new Private())
{
    d->shapes = shapes;
    d->previousPositions = previousPositions;
    d->newPositions = newPositions;
    d->anchor = anchor;
    Q_ASSERT(d->shapes.count() == d->previousPositions.count());
    Q_ASSERT(d->shapes.count() == d->newPositions.count());
}

KoShapeMoveCommand::KoShapeMoveCommand(const QList<KoShape *> &shapes, const QPointF &offset, KUndo2Command *parent)
    : KUndo2Command(kundo2_i18n("Move shapes"), parent)
    , d(new Private())
{
    d->shapes = shapes;
    d->anchor = KoFlake::Center;

    Q_FOREACH (KoShape *shape, d->shapes) {
        const QPointF pos = activePositionReader(shape, KoFlake::Center);

        d->previousPositions << pos;
        d->newPositions << pos + offset;
    }
}

KoShapeMoveCommand::~KoShapeMoveCommand()
{
    delete d;
}

void KoShapeMoveCommand::redo()
{
    KUndo2Command::redo();
    activePositionBatchApplier(d->shapes, d->newPositions, d->anchor);
}

void KoShapeMoveCommand::undo()
{
    KUndo2Command::undo();
    activePositionBatchApplier(d->shapes, d->previousPositions, d->anchor);
}

int KoShapeMoveCommand::id() const
{
    return KisCommandUtils::MoveShapeId;
}

bool KoShapeMoveCommand::mergeWith(const KUndo2Command *command)
{
    const KoShapeMoveCommand *other = dynamic_cast<const KoShapeMoveCommand *>(command);
    KIS_ASSERT(other);

    if (other->d->shapes != d->shapes || other->d->anchor != d->anchor) {
        return false;
    }

    d->newPositions = other->d->newPositions;
    return true;
}
