/* This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2006 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2006-2008 Jan Hambrecht <jaham@gmx.net>
 * SPDX-FileCopyrightText: 2012 Inge Wallin <inge@lysator.liu.se>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoShapeStrokeCommand.h"
#include "KoShape.h"
#include "KoShapeStrokeModel.h"
#include <KoShapeBulkActionLock.h>

#include <klocalizedstring.h>

#include "kis_command_ids.h"

namespace
{
using StrokeReader = KoShapeStrokeModelSP (*)(const KoShape *shape);
using StrokeBatchApplier = void (*)(const QList<KoShape *> &shapes, const QList<KoShapeStrokeModelSP> &strokes);

KoShapeStrokeModelSP readStroke(const KoShape *shape)
{
    return shape->stroke();
}

void applyStrokeBatch(const QList<KoShape *> &shapes, const QList<KoShapeStrokeModelSP> &strokes)
{
    KoShapeBulkActionLock lock(shapes);

    auto strokeIt = strokes.cbegin();
    for (KoShape *shape : shapes) {
        shape->setStroke(*strokeIt);
        ++strokeIt;
    }

    KoShapeBulkActionLock::bulkShapesUpdate(lock.unlock());
}

StrokeReader activeStrokeReader = readStroke;
StrokeBatchApplier activeStrokeBatchApplier = applyStrokeBatch;
} // namespace

#ifdef KRITAFLAKE_SHAPE_STROKE_COMMAND_CONTRACT_TESTING
namespace KoShapeStrokeCommandTesting
{
void setShapeAccessForTesting(StrokeReader reader, StrokeBatchApplier applier)
{
    activeStrokeReader = reader;
    activeStrokeBatchApplier = applier;
}

void resetShapeAccessForTesting()
{
    activeStrokeReader = readStroke;
    activeStrokeBatchApplier = applyStrokeBatch;
}
} // namespace KoShapeStrokeCommandTesting
#endif

class Q_DECL_HIDDEN KoShapeStrokeCommand::Private
{
public:
    Private()
    {
    }
    ~Private()
    {
    }

    void addOldStroke(KoShapeStrokeModelSP oldStroke)
    {
        oldStrokes.append(oldStroke);
    }

    void addNewStroke(KoShapeStrokeModelSP newStroke)
    {
        newStrokes.append(newStroke);
    }

    QList<KoShape *> shapes; ///< the shapes to set stroke for
    QList<KoShapeStrokeModelSP> oldStrokes; ///< the old strokes, one for each shape
    QList<KoShapeStrokeModelSP> newStrokes; ///< the new strokes to set
};

KoShapeStrokeCommand::KoShapeStrokeCommand(const QList<KoShape *> &shapes,
                                           KoShapeStrokeModelSP stroke,
                                           KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new Private())
{
    d->shapes = shapes;

    // save old strokes
    Q_FOREACH (KoShape *shape, d->shapes) {
        d->addOldStroke(activeStrokeReader(shape));
        d->addNewStroke(stroke);
    }

    setText(kundo2_i18n("Set stroke"));
}

KoShapeStrokeCommand::KoShapeStrokeCommand(const QList<KoShape *> &shapes,
                                           const QList<KoShapeStrokeModelSP> &strokes,
                                           KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new Private())
{
    Q_ASSERT(shapes.count() == strokes.count());

    d->shapes = shapes;

    // save old strokes
    Q_FOREACH (KoShape *shape, shapes)
        d->addOldStroke(activeStrokeReader(shape));
    foreach (KoShapeStrokeModelSP stroke, strokes)
        d->addNewStroke(stroke);

    setText(kundo2_i18n("Set stroke"));
}

KoShapeStrokeCommand::KoShapeStrokeCommand(KoShape *shape, KoShapeStrokeModelSP stroke, KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new Private())
{
    d->shapes.append(shape);
    d->addNewStroke(stroke);
    d->addOldStroke(activeStrokeReader(shape));

    setText(kundo2_i18n("Set stroke"));
}

KoShapeStrokeCommand::~KoShapeStrokeCommand()
{
    delete d;
}

void KoShapeStrokeCommand::redo()
{
    KUndo2Command::redo();
    activeStrokeBatchApplier(d->shapes, d->newStrokes);
}

void KoShapeStrokeCommand::undo()
{
    KUndo2Command::undo();
    activeStrokeBatchApplier(d->shapes, d->oldStrokes);
}

int KoShapeStrokeCommand::id() const
{
    return KisCommandUtils::ChangeShapeStrokeId;
}

bool KoShapeStrokeCommand::mergeWith(const KUndo2Command *command)
{
    const KoShapeStrokeCommand *other = dynamic_cast<const KoShapeStrokeCommand *>(command);

    if (!other || other->d->shapes != d->shapes) {
        return false;
    }

    d->newStrokes = other->d->newStrokes;
    return true;
}
