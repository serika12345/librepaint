/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2008 Thorsten Zachmann <zachmann@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoShapeRenameCommand.h"

#include "KoShape.h"
#include <QString>
#include <klocalizedstring.h>

namespace
{
using ShapeNameReader = QString (*)(const KoShape *shape);
using ShapeNameWriter = void (*)(KoShape *shape, const QString &name);

QString readShapeName(const KoShape *shape)
{
    return shape->name();
}

void writeShapeName(KoShape *shape, const QString &name)
{
    shape->setName(name);
}

struct ShapeAccess {
    ShapeNameReader reader;
    ShapeNameWriter writer;
};

const ShapeAccess defaultShapeAccess{readShapeName, writeShapeName};
ShapeAccess activeShapeAccess = defaultShapeAccess;
} // namespace

#if defined(KRITAFLAKE_SHAPE_RENAME_COMMAND_CONTRACT_TESTING)
namespace KoShapeRenameCommandTesting
{
Q_DECL_HIDDEN void setShapeAccessForTesting(ShapeNameReader reader, ShapeNameWriter writer)
{
    activeShapeAccess = {reader, writer};
}

Q_DECL_HIDDEN void resetShapeAccessForTesting()
{
    activeShapeAccess = defaultShapeAccess;
}
} // namespace KoShapeRenameCommandTesting
#endif

class Q_DECL_HIDDEN KoShapeRenameCommand::Private
{
public:
    Private(KoShape *shape, const QString &newName)
        : shapeAccess(activeShapeAccess)
        , shape(shape)
        , newName(newName)
        , oldName(shapeAccess.reader(shape))
    {
    }

    ShapeAccess shapeAccess;
    KoShape *shape;
    QString newName;
    QString oldName;
};

KoShapeRenameCommand::KoShapeRenameCommand(KoShape *shape, const QString &newName, KUndo2Command *parent)
    : KUndo2Command(kundo2_i18n("Rename Shape"), parent)
    , d(new Private(shape, newName))
{
}

KoShapeRenameCommand::~KoShapeRenameCommand()
{
    delete d;
}

void KoShapeRenameCommand::redo()
{
    KUndo2Command::redo();
    d->shapeAccess.writer(d->shape, d->newName);
}

void KoShapeRenameCommand::undo()
{
    KUndo2Command::undo();
    d->shapeAccess.writer(d->shape, d->oldName);
}
