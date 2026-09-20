/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2011 Jan Hambrecht <jaham@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoShapeClipCommand.h"
#include "KoClipPath.h"
#include "KoPathShape.h"
#include "KoShape.h"
#include "KoShapeContainer.h"
#include "KoShapeControllerBase.h"

#include <klocalizedstring.h>

#include "kis_pointer_utils.h"

namespace
{
using ClipReader = KoClipPath *(*)(KoShape *);
using ParentReader = KoShapeContainer *(*)(KoPathShape *);
using ClipFactory = KoClipPath *(*)(const QList<KoPathShape *> &);
using ClipSetter = void (*)(KoShape *, KoClipPath *);
using ShapeOperation = void (*)(KoShape *);
using ParentOperation = void (*)(KoShapeContainer *, KoPathShape *);
using ClipDeleter = void (*)(KoClipPath *);
KoClipPath *readClip(KoShape *s)
{
    return s->clipPath();
}
KoShapeContainer *readParent(KoPathShape *s)
{
    return s->parent();
}
KoClipPath *makeClip(const QList<KoPathShape *> &s)
{
    return new KoClipPath(implicitCastList<KoShape *>(s), KoFlake::UserSpaceOnUse);
}
void setClip(KoShape *s, KoClipPath *p)
{
    s->setClipPath(p);
}
void updateShape(KoShape *s)
{
    s->update();
}
void removeShape(KoShapeContainer *p, KoPathShape *s)
{
    p->removeShape(s);
}
void addShape(KoShapeContainer *p, KoPathShape *s)
{
    p->addShape(s);
}
void deleteClip(KoClipPath *p)
{
    delete p;
}
ClipReader clipReader = readClip;
ParentReader parentReader = readParent;
ClipFactory clipFactory = makeClip;
ClipSetter clipSetter = setClip;
ShapeOperation shapeUpdater = updateShape;
ParentOperation parentRemover = removeShape, parentAdder = addShape;
ClipDeleter clipDeleter = deleteClip;
} // namespace
#ifdef KRITAFLAKE_SHAPE_CLIP_COMMAND_CONTRACT_TESTING
namespace KoShapeClipCommandTesting
{
Q_DECL_HIDDEN void setAccess(ClipReader cr,
                             ParentReader pr,
                             ClipFactory f,
                             ClipSetter s,
                             ShapeOperation u,
                             ParentOperation r,
                             ParentOperation a,
                             ClipDeleter d)
{
    clipReader = cr;
    parentReader = pr;
    clipFactory = f;
    clipSetter = s;
    shapeUpdater = u;
    parentRemover = r;
    parentAdder = a;
    clipDeleter = d;
}
Q_DECL_HIDDEN void resetAccess()
{
    setAccess(readClip, readParent, makeClip, setClip, updateShape, removeShape, addShape, deleteClip);
}
} // namespace KoShapeClipCommandTesting
#endif

class Q_DECL_HIDDEN KoShapeClipCommand::Private
{
public:
    Private(KoShapeControllerBase *c)
        : controller(c)
        , executed(false)
    {
    }

    ~Private()
    {
        if (executed) {
            for (KoClipPath *p : oldClipPaths)
                clipDeleter(p);
        } else {
            for (KoClipPath *p : newClipPaths)
                clipDeleter(p);
        }
    }

    QList<KoShape *> shapesToClip;
    QList<KoClipPath *> oldClipPaths;
    QList<KoPathShape *> clipPathShapes;
    QList<KoClipPath *> newClipPaths;
    QList<KoShapeContainer *> oldParents;
    KoShapeControllerBase *controller;
    bool executed;
};

KoShapeClipCommand::KoShapeClipCommand(KoShapeControllerBase *controller,
                                       const QList<KoShape *> &shapes,
                                       const QList<KoPathShape *> &clipPathShapes,
                                       KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new Private(controller))
{
    d->shapesToClip = shapes;
    d->clipPathShapes = clipPathShapes;

    Q_FOREACH (KoShape *shape, d->shapesToClip) {
        d->oldClipPaths.append(clipReader(shape));
        d->newClipPaths.append(clipFactory(clipPathShapes));
    }

    Q_FOREACH (KoPathShape *path, clipPathShapes) {
        d->oldParents.append(parentReader(path));
    }

    setText(kundo2_i18n("Clip Shape"));
}

KoShapeClipCommand::KoShapeClipCommand(KoShapeControllerBase *controller,
                                       KoShape *shape,
                                       const QList<KoPathShape *> &clipPathShapes,
                                       KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new Private(controller))
{
    d->shapesToClip.append(shape);
    d->clipPathShapes = clipPathShapes;
    d->oldClipPaths.append(clipReader(shape));
    d->newClipPaths.append(clipFactory(clipPathShapes));

    Q_FOREACH (KoPathShape *path, clipPathShapes) {
        d->oldParents.append(parentReader(path));
    }

    setText(kundo2_i18n("Clip Shape"));
}

KoShapeClipCommand::~KoShapeClipCommand()
{
    delete d;
}

void KoShapeClipCommand::redo()
{
    const uint shapeCount = d->shapesToClip.count();
    for (uint i = 0; i < shapeCount; ++i) {
        clipSetter(d->shapesToClip[i], d->newClipPaths[i]);
        shapeUpdater(d->shapesToClip[i]);
    }

    const uint clipPathCount = d->clipPathShapes.count();
    for (uint i = 0; i < clipPathCount; ++i) {
        if (d->oldParents.at(i)) {
            parentRemover(d->oldParents.at(i), d->clipPathShapes[i]);
        }
    }

    d->executed = true;

    KUndo2Command::redo();
}

void KoShapeClipCommand::undo()
{
    KUndo2Command::undo();

    const uint shapeCount = d->shapesToClip.count();
    for (uint i = 0; i < shapeCount; ++i) {
        clipSetter(d->shapesToClip[i], d->oldClipPaths[i]);
        shapeUpdater(d->shapesToClip[i]);
    }

    const uint clipPathCount = d->clipPathShapes.count();
    for (uint i = 0; i < clipPathCount; ++i) {
        if (d->oldParents.at(i)) {
            parentAdder(d->oldParents.at(i), d->clipPathShapes[i]);
        }
    }

    d->executed = false;
}
