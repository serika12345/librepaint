/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Jan Hambrecht <jaham@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoPathFillRuleCommand.h"
#include "KoPathShape.h"

#include <klocalizedstring.h>

namespace
{
using FillRuleReader = Qt::FillRule (*)(const KoPathShape *shape);
using FillRuleWriter = void (*)(KoPathShape *shape, Qt::FillRule fillRule);
using UpdateRequester = void (*)(KoPathShape *shape);

Qt::FillRule readFillRule(const KoPathShape *shape)
{
    return shape->fillRule();
}

void writeFillRule(KoPathShape *shape, Qt::FillRule fillRule)
{
    shape->setFillRule(fillRule);
}

void requestUpdate(KoPathShape *shape)
{
    shape->update();
}

struct ShapeAccess
{
    FillRuleReader reader;
    FillRuleWriter writer;
    UpdateRequester updater;
};

const ShapeAccess defaultShapeAccess {readFillRule, writeFillRule, requestUpdate};
ShapeAccess activeShapeAccess = defaultShapeAccess;
}

#if defined(KRITAFLAKE_PATH_FILL_RULE_COMMAND_CONTRACT_TESTING)
namespace KoPathFillRuleCommandTesting
{
Q_DECL_HIDDEN void setShapeAccessForTesting(FillRuleReader reader, FillRuleWriter writer, UpdateRequester updater)
{
    activeShapeAccess = {reader, writer, updater};
}

Q_DECL_HIDDEN void resetShapeAccessForTesting()
{
    activeShapeAccess = defaultShapeAccess;
}
}
#endif

class Q_DECL_HIDDEN KoPathFillRuleCommand::Private
{
public:
    Private(Qt::FillRule fillRule)
        : shapeAccess(activeShapeAccess)
        , newFillRule(fillRule)
    {
    }

    QList<KoPathShape*> shapes;       ///< the shapes to set fill rule for
    QList<Qt::FillRule> oldFillRules; ///< the old fill rules, one for each shape
    ShapeAccess shapeAccess;
    Qt::FillRule newFillRule;         ///< the new fill rule to set
};

KoPathFillRuleCommand::KoPathFillRuleCommand(const QList<KoPathShape*> &shapes, Qt::FillRule fillRule, KUndo2Command *parent)
        : KUndo2Command(parent)
        , d(new Private(fillRule))
{
    d->shapes = shapes;
    Q_FOREACH (KoPathShape *shape, d->shapes)
        d->oldFillRules.append(d->shapeAccess.reader(shape));

    setText(kundo2_i18n("Set fill rule"));
}

KoPathFillRuleCommand::~KoPathFillRuleCommand()
{
    delete d;
}

void KoPathFillRuleCommand::redo()
{
    KUndo2Command::redo();
    Q_FOREACH (KoPathShape *shape, d->shapes) {
        d->shapeAccess.writer(shape, d->newFillRule);
        d->shapeAccess.updater(shape);
    }
}

void KoPathFillRuleCommand::undo()
{
    KUndo2Command::undo();
    QList<Qt::FillRule>::iterator ruleIt = d->oldFillRules.begin();
    Q_FOREACH (KoPathShape *shape, d->shapes) {
        d->shapeAccess.writer(shape, *ruleIt);
        d->shapeAccess.updater(shape);
        ++ruleIt;
    }
}
