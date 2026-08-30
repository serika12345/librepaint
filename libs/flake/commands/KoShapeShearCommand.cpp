/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2006 Jan Hambrecht <jaham@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoShapeShearCommand.h"
#include "KoShape.h"

#include <klocalizedstring.h>

namespace
{
using ShapeUpdater = void (*)(const KoShape *shape);
using ShapeShearer = void (*)(KoShape *shape, qreal shearX, qreal shearY);

void updateShape(const KoShape *shape)
{
    shape->update();
}

void shearShape(KoShape *shape, qreal shearX, qreal shearY)
{
    shape->shear(shearX, shearY);
}

struct ShapeAccess {
    ShapeUpdater updater;
    ShapeShearer shearer;
};

const ShapeAccess defaultShapeAccess{updateShape, shearShape};
ShapeAccess activeShapeAccess = defaultShapeAccess;
} // namespace

#if defined(KRITAFLAKE_SHAPE_SHEAR_COMMAND_CONTRACT_TESTING)
namespace KoShapeShearCommandTesting
{
Q_DECL_HIDDEN void setShapeAccessForTesting(ShapeUpdater updater, ShapeShearer shearer)
{
    activeShapeAccess = {updater, shearer};
}

Q_DECL_HIDDEN void resetShapeAccessForTesting()
{
    activeShapeAccess = defaultShapeAccess;
}
} // namespace KoShapeShearCommandTesting
#endif

class KoShapeShearCommandPrivate
{
public:
    QList<KoShape *> shapes;
    QList<qreal> previousShearXs;
    QList<qreal> previousShearYs;
    QList<qreal> newShearXs;
    QList<qreal> newShearYs;
};

KoShapeShearCommand::KoShapeShearCommand(const QList<KoShape *> &shapes,
                                         const QList<qreal> &previousShearXs,
                                         const QList<qreal> &previousShearYs,
                                         const QList<qreal> &newShearXs,
                                         const QList<qreal> &newShearYs,
                                         KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new KoShapeShearCommandPrivate())
{
    d->shapes = shapes;
    d->previousShearXs = previousShearXs;
    d->previousShearYs = previousShearYs;
    d->newShearXs = newShearXs;
    d->newShearYs = newShearYs;

    Q_ASSERT(d->shapes.count() == d->previousShearXs.count());
    Q_ASSERT(d->shapes.count() == d->previousShearYs.count());
    Q_ASSERT(d->shapes.count() == d->newShearXs.count());
    Q_ASSERT(d->shapes.count() == d->newShearYs.count());

    setText(kundo2_i18n("Shear shapes"));
}

KoShapeShearCommand::~KoShapeShearCommand()
{
    delete d;
}

void KoShapeShearCommand::redo()
{
    KUndo2Command::redo();
    for (int i = 0; i < d->shapes.count(); i++) {
        activeShapeAccess.updater(d->shapes.at(i));
        activeShapeAccess.shearer(d->shapes.at(i), d->newShearXs.at(i), d->newShearYs.at(i));
        activeShapeAccess.updater(d->shapes.at(i));
    }
}

void KoShapeShearCommand::undo()
{
    KUndo2Command::undo();
    for (int i = 0; i < d->shapes.count(); i++) {
        activeShapeAccess.updater(d->shapes.at(i));
        activeShapeAccess.shearer(d->shapes.at(i), d->previousShearXs.at(i), d->previousShearYs.at(i));
        activeShapeAccess.updater(d->shapes.at(i));
    }
}
