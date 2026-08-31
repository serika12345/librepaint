/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Jan Hambrecht <jaham@gmx.net>
 * SPDX-FileCopyrightText: 2007 Thomas Zander <zander@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kis_command_ids.h"

#include "KoShape.h"
#include "KoShapeTransformCommand.h"
#include <KoShapeBulkActionLock.h>

#include <QList>
#include <QTransform>

#include <FlakeDebug.h>

namespace
{
using TransformationBatchApplier = void (*)(const QList<KoShape *> &shapes, const QList<QTransform> &transformations);

void applyTransformationBatch(const QList<KoShape *> &shapes, const QList<QTransform> &transformations)
{
    KoShapeBulkActionLock lock(shapes);

    const int shapeCount = shapes.count();
    for (int i = 0; i < shapeCount; ++i) {
        shapes[i]->setTransformation(transformations[i]);
    }

    KoShapeBulkActionLock::bulkShapesUpdate(lock.unlock());
}

TransformationBatchApplier activeTransformationBatchApplier = applyTransformationBatch;
} // namespace

#if defined(KRITAFLAKE_SHAPE_TRANSFORM_COMMAND_CONTRACT_TESTING)
namespace KoShapeTransformCommandTesting
{
Q_DECL_HIDDEN void setTransformationBatchApplierForTesting(TransformationBatchApplier applier)
{
    activeTransformationBatchApplier = applier;
}

Q_DECL_HIDDEN void resetTransformationBatchApplierForTesting()
{
    activeTransformationBatchApplier = applyTransformationBatch;
}
} // namespace KoShapeTransformCommandTesting
#endif

class Q_DECL_HIDDEN KoShapeTransformCommand::Private
{
public:
    Private(const QList<KoShape *> &list)
        : shapes(list)
    {
    }
    QList<KoShape *> shapes;
    QList<QTransform> oldState;
    QList<QTransform> newState;
};

KoShapeTransformCommand::KoShapeTransformCommand(const QList<KoShape *> &shapes,
                                                 const QList<QTransform> &oldState,
                                                 const QList<QTransform> &newState,
                                                 KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new Private(shapes))
{
    Q_ASSERT(shapes.count() == oldState.count());
    Q_ASSERT(shapes.count() == newState.count());
    d->oldState = oldState;
    d->newState = newState;
}

KoShapeTransformCommand::~KoShapeTransformCommand()
{
    delete d;
}

void KoShapeTransformCommand::redo()
{
    KUndo2Command::redo();
    activeTransformationBatchApplier(d->shapes, d->newState);
}

void KoShapeTransformCommand::undo()
{
    KUndo2Command::undo();
    activeTransformationBatchApplier(d->shapes, d->oldState);
}

int KoShapeTransformCommand::id() const
{
    return KisCommandUtils::TransformShapeId;
}

bool KoShapeTransformCommand::mergeWith(const KUndo2Command *command)
{
    const KoShapeTransformCommand *other = dynamic_cast<const KoShapeTransformCommand *>(command);

    if (!other || other->d->shapes != d->shapes || other->text() != text()) {
        return false;
    }

    d->newState = other->d->newState;
    return true;
}
