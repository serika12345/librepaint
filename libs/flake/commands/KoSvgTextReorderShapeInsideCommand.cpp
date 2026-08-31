/*
 * SPDX-FileCopyrightText: 2025 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KoSvgTextReorderShapeInsideCommand.h"
#include <KoShapeBulkActionLock.h>
#include <KoSvgTextShape.h>

namespace
{
using MementoReader = KoSvgTextShapeMementoSP (*)(KoSvgTextShape *);
using InsideShapesReader = QList<KoShape *> (*)(const KoSvgTextShape *);
using MoveInsideWriter = void (*)(KoSvgTextShape *, KoShape *, int);
using MementoRestorer = void (*)(KoSvgTextShape *, const KoSvgTextShapeMementoSP &);

KoSvgTextShapeMementoSP readMemento(KoSvgTextShape *shape)
{
    return shape->getMemento();
}

QList<KoShape *> readInsideShapes(const KoSvgTextShape *shape)
{
    return shape->shapesInside();
}

void moveInside(KoSvgTextShape *textShape, KoShape *shape, int index)
{
    textShape->moveShapeInsideToIndex(shape, index);
}

void restoreMemento(KoSvgTextShape *shape, const KoSvgTextShapeMementoSP &memento)
{
    shape->setMemento(memento);
}

MementoReader activeMementoReader = readMemento;
InsideShapesReader activeInsideShapesReader = readInsideShapes;
MoveInsideWriter activeMoveInsideWriter = moveInside;
MementoRestorer activeMementoRestorer = restoreMemento;
} // namespace

#if defined(KRITAFLAKE_SVG_TEXT_REORDER_SHAPE_INSIDE_COMMAND_CONTRACT_TESTING)
namespace KoSvgTextReorderShapeInsideCommandTesting
{
Q_DECL_HIDDEN void setShapeAccessForTesting(MementoReader mementoReader,
                                            InsideShapesReader insideShapesReader,
                                            MoveInsideWriter moveInsideWriter,
                                            MementoRestorer mementoRestorer)
{
    activeMementoReader = mementoReader;
    activeInsideShapesReader = insideShapesReader;
    activeMoveInsideWriter = moveInsideWriter;
    activeMementoRestorer = mementoRestorer;
}

Q_DECL_HIDDEN void resetShapeAccessForTesting()
{
    activeMementoReader = readMemento;
    activeInsideShapesReader = readInsideShapes;
    activeMoveInsideWriter = moveInside;
    activeMementoRestorer = restoreMemento;
}
} // namespace KoSvgTextReorderShapeInsideCommandTesting
#endif

struct KoSvgTextReorderShapeInsideCommand::Private {
    Private(KoSvgTextShape *_text, QList<KoShape *> _shapes, KoSvgTextReorderShapeInsideCommand::MoveShapeType _type)
        : textShape(_text)
        , memento(activeMementoReader(textShape))
        , shapes(_shapes)
        , type(_type)
    {
        std::sort(shapes.begin(), shapes.end(), [this](KoShape *a, KoShape *b) {
            const QList<KoShape *> insideShapes = activeInsideShapesReader(this->textShape);
            return insideShapes.indexOf(a) < insideShapes.indexOf(b);
        });
        Q_FOREACH (KoShape *shape, shapes) {
            oldIndices.append(activeInsideShapesReader(textShape).indexOf(shape));
        }
    }

    KoSvgTextShape *textShape;
    KoSvgTextShapeMementoSP memento;
    QList<KoShape *> shapes;
    QList<int> oldIndices;
    KoSvgTextReorderShapeInsideCommand::MoveShapeType type;
};

KoSvgTextReorderShapeInsideCommand::KoSvgTextReorderShapeInsideCommand(KoSvgTextShape *textShape,
                                                                       QList<KoShape *> shapes,
                                                                       MoveShapeType type,
                                                                       KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new Private(textShape, shapes, type))
{
}

KoSvgTextReorderShapeInsideCommand::~KoSvgTextReorderShapeInsideCommand()
{
}

void KoSvgTextReorderShapeInsideCommand::redo()
{
    KoShapeBulkActionLock lock(d->textShape);

    int newIndex = activeInsideShapesReader(d->textShape).indexOf(d->shapes.first());
    const int max = activeInsideShapesReader(d->textShape).size() - 1;
    if (d->type == MoveEarlier || d->type == BringToFront) {
        if (d->type == MoveEarlier) {
            newIndex = qMax(0, newIndex - 1);
        } else {
            newIndex = 0;
        }
        Q_FOREACH (KoShape *shape, d->shapes) {
            const int index = activeInsideShapesReader(d->textShape).indexOf(shape);
            if (index == newIndex)
                continue;

            activeMoveInsideWriter(d->textShape, shape, newIndex);
            newIndex += 1;
        }
    } else {
        if (d->type == MoveLater) {
            newIndex = qMin(max, newIndex + d->shapes.size());
        } else {
            newIndex = max;
        }
        auto end = std::make_reverse_iterator(d->shapes.begin());
        auto begin = std::make_reverse_iterator(d->shapes.end());
        for (auto it = begin; it != end; it++) {
            KoShape *shape = *it;

            const int index = activeInsideShapesReader(d->textShape).indexOf(shape);
            if (index == newIndex)
                continue;
            if (newIndex < 0)
                continue;

            activeMoveInsideWriter(d->textShape, shape, newIndex);
            newIndex -= 1;
        }
    }

    KoShapeBulkActionLock::bulkShapesUpdate(lock.unlock());
}

void KoSvgTextReorderShapeInsideCommand::undo()
{
    KoShapeBulkActionLock lock(d->textShape);

    if (d->type == MoveEarlier || d->type == BringToFront) {
        for (int i = d->oldIndices.size() - 1; i >= 0; i--) {
            activeMoveInsideWriter(d->textShape, d->shapes.at(i), d->oldIndices.at(i));
        }
    } else {
        for (int i = 0; i < d->oldIndices.size(); i++) {
            activeMoveInsideWriter(d->textShape, d->shapes.at(i), d->oldIndices.at(i));
        }
    }
    activeMementoRestorer(d->textShape, d->memento);

    KoShapeBulkActionLock::bulkShapesUpdate(lock.unlock());
}
