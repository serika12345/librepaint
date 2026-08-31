/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2006 Jan Hambrecht <jaham@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoShapeDeleteCommand.h"
#include "KoShapeContainer.h"
#include "KoShapeControllerBase.h"

#include <klocalizedstring.h>

namespace
{
using ParentReader = KoShapeContainer *(*)(const KoShape *);
using ParentOperation = void (*)(KoShapeContainer *, KoShape *);
using ShapeDeleter = void (*)(KoShape *);

KoShapeContainer *readParent(const KoShape *shape)
{
    return shape->parent();
}
void removeFromParent(KoShapeContainer *parent, KoShape *shape)
{
    parent->removeShape(shape);
}
void addToParent(KoShapeContainer *parent, KoShape *shape)
{
    parent->addShape(shape);
}
void deleteShape(KoShape *shape)
{
    delete shape;
}

ParentReader activeParentReader = readParent;
ParentOperation activeParentRemover = removeFromParent;
ParentOperation activeParentAdder = addToParent;
ShapeDeleter activeShapeDeleter = deleteShape;
} // namespace

#if defined(KRITAFLAKE_SHAPE_DELETE_COMMAND_CONTRACT_TESTING)
namespace KoShapeDeleteCommandTesting
{
Q_DECL_HIDDEN void
setShapeAccessForTesting(ParentReader reader, ParentOperation remover, ParentOperation adder, ShapeDeleter deleter)
{
    activeParentReader = reader;
    activeParentRemover = remover;
    activeParentAdder = adder;
    activeShapeDeleter = deleter;
}
Q_DECL_HIDDEN void resetShapeAccessForTesting()
{
    activeParentReader = readParent;
    activeParentRemover = removeFromParent;
    activeParentAdder = addToParent;
    activeShapeDeleter = deleteShape;
}
} // namespace KoShapeDeleteCommandTesting
#endif

class Q_DECL_HIDDEN KoShapeDeleteCommand::Private
{
public:
    Private(KoShapeControllerBase *c)
        : controller(c)
        , deleteShapes(false)
    {
    }

    ~Private()
    {
        if (!deleteShapes)
            return;

        Q_FOREACH (KoShape *shape, shapes)
            activeShapeDeleter(shape);
    }

    KoShapeControllerBase *controller; ///< the shape controller to use for removing/readding
    QList<KoShape *> shapes; ///< the list of shapes to delete
    QList<KoShapeContainer *> oldParents; ///< the old parents of the shapes
    bool deleteShapes; ///< shows if shapes should be deleted when deleting the command
};

KoShapeDeleteCommand::KoShapeDeleteCommand(KoShapeControllerBase *controller, KoShape *shape, KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new Private(controller))
{
    d->shapes.append(shape);
    d->oldParents.append(activeParentReader(shape));

    setText(kundo2_i18nc("Delete one shape", "Delete shape"));
}

KoShapeDeleteCommand::KoShapeDeleteCommand(KoShapeControllerBase *controller,
                                           const QList<KoShape *> &shapes,
                                           KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new Private(controller))
{
    d->shapes = shapes;
    Q_FOREACH (KoShape *shape, d->shapes) {
        d->oldParents.append(activeParentReader(shape));
    }

    setText(kundo2_i18np("Delete shape", "Delete shapes", shapes.count()));
}

KoShapeDeleteCommand::~KoShapeDeleteCommand()
{
    delete d;
}

void KoShapeDeleteCommand::redo()
{
    KUndo2Command::redo();
    if (!d->controller)
        return;

    for (int i = 0; i < d->shapes.count(); i++) {
        if (d->oldParents.at(i)) {
            activeParentRemover(d->oldParents.at(i), d->shapes[i]);
        }
    }
    d->deleteShapes = true;
}

void KoShapeDeleteCommand::undo()
{
    KUndo2Command::undo();
    if (!d->controller)
        return;

    for (int i = 0; i < d->shapes.count(); i++) {
        if (d->oldParents.at(i)) {
            activeParentAdder(d->oldParents.at(i), d->shapes[i]);
        }
    }
    d->deleteShapes = false;
}
