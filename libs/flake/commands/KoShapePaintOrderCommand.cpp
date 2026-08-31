/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2023 Wolthera van Hövell <griffinvalley@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoShapePaintOrderCommand.h"

#include "kis_command_ids.h"
#include <klocalizedstring.h>

namespace
{
using PaintOrderReader = QVector<KoShape::PaintOrder> (*)(const KoShape *shape);
using PaintOrderWriter = void (*)(KoShape *shape, KoShape::PaintOrder first, KoShape::PaintOrder second);
using InheritPaintOrderReader = bool (*)(const KoShape *shape);
using InheritPaintOrderWriter = void (*)(KoShape *shape, bool inheritPaintOrder);
using ShapeUpdater = void (*)(const KoShape *shape);

QVector<KoShape::PaintOrder> readPaintOrder(const KoShape *shape)
{
    return shape->paintOrder();
}

void writePaintOrder(KoShape *shape, KoShape::PaintOrder first, KoShape::PaintOrder second)
{
    shape->setPaintOrder(first, second);
}

bool readInheritPaintOrder(const KoShape *shape)
{
    return shape->inheritPaintOrder();
}

void writeInheritPaintOrder(KoShape *shape, bool inheritPaintOrder)
{
    shape->setInheritPaintOrder(inheritPaintOrder);
}

void updateShape(const KoShape *shape)
{
    shape->update();
}

struct ShapeAccess {
    PaintOrderReader paintOrderReader;
    PaintOrderWriter paintOrderWriter;
    InheritPaintOrderReader inheritPaintOrderReader;
    InheritPaintOrderWriter inheritPaintOrderWriter;
    ShapeUpdater updater;
};

const ShapeAccess defaultShapeAccess{readPaintOrder,
                                     writePaintOrder,
                                     readInheritPaintOrder,
                                     writeInheritPaintOrder,
                                     updateShape};
ShapeAccess activeShapeAccess = defaultShapeAccess;
} // namespace

#if defined(KRITAFLAKE_SHAPE_PAINT_ORDER_COMMAND_CONTRACT_TESTING)
namespace KoShapePaintOrderCommandTesting
{
Q_DECL_HIDDEN void setShapeAccessForTesting(PaintOrderReader paintOrderReader,
                                            PaintOrderWriter paintOrderWriter,
                                            InheritPaintOrderReader inheritPaintOrderReader,
                                            InheritPaintOrderWriter inheritPaintOrderWriter,
                                            ShapeUpdater updater)
{
    activeShapeAccess = {paintOrderReader, paintOrderWriter, inheritPaintOrderReader, inheritPaintOrderWriter, updater};
}

Q_DECL_HIDDEN void resetShapeAccessForTesting()
{
    activeShapeAccess = defaultShapeAccess;
}
} // namespace KoShapePaintOrderCommandTesting
#endif

class Q_DECL_HIDDEN KoShapePaintOrderCommand::Private
{
public:
    Private()
    {
    }
    ~Private()
    {
    }

    QList<KoShape *> shapes;
    QList<KoShape::PaintOrder> oldFirst;
    QList<KoShape::PaintOrder> oldSecond;
    QList<bool> paintOrderInherited;
    KoShape::PaintOrder first;
    KoShape::PaintOrder second;
};

KoShapePaintOrderCommand::KoShapePaintOrderCommand(const QList<KoShape *> &shapes,
                                                   KoShape::PaintOrder first,
                                                   KoShape::PaintOrder second,
                                                   KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new Private())
{
    d->shapes = shapes;
    Q_FOREACH (KoShape *shape, d->shapes) {
        d->oldFirst.append(activeShapeAccess.paintOrderReader(shape).at(0));
        d->oldSecond.append(activeShapeAccess.paintOrderReader(shape).at(1));
        d->paintOrderInherited.append(activeShapeAccess.inheritPaintOrderReader(shape));
    }
    d->first = first;
    d->second = second;

    setText(kundo2_i18n("Set paint order"));
}

KoShapePaintOrderCommand::~KoShapePaintOrderCommand()
{
    delete d;
}

void KoShapePaintOrderCommand::redo()
{
    KUndo2Command::redo();
    Q_FOREACH (KoShape *shape, d->shapes) {
        activeShapeAccess.paintOrderWriter(shape, d->first, d->second);
        activeShapeAccess.updater(shape);
    }
}

void KoShapePaintOrderCommand::undo()
{
    KUndo2Command::undo();
    QList<KoShape::PaintOrder>::iterator firstIt = d->oldFirst.begin();
    QList<KoShape::PaintOrder>::iterator secondIt = d->oldSecond.begin();
    QList<bool>::iterator inheritIt = d->paintOrderInherited.begin();
    Q_FOREACH (KoShape *shape, d->shapes) {
        activeShapeAccess.paintOrderWriter(shape, *firstIt, *secondIt);
        activeShapeAccess.inheritPaintOrderWriter(shape, *inheritIt);
        activeShapeAccess.updater(shape);
        ++firstIt;
        ++secondIt;
        ++inheritIt;
    }
}

int KoShapePaintOrderCommand::id() const
{
    return KisCommandUtils::ChangePaintOrderCommand;
}

bool KoShapePaintOrderCommand::mergeWith(const KUndo2Command *command)
{
    const KoShapePaintOrderCommand *other = dynamic_cast<const KoShapePaintOrderCommand *>(command);

    if (!other || other->d->shapes != d->shapes) {
        return false;
    }

    d->first = other->d->first;
    d->second = other->d->second;
    return true;
}
