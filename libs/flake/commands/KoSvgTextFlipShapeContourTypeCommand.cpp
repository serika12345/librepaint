/*
 * SPDX-FileCopyrightText: 2025 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KoSvgTextFlipShapeContourTypeCommand.h"

#include <KoShapeBulkActionLock.h>
#include <KoSvgTextShape.h>

namespace
{
using ContourMembershipReader = bool (*)(const KoSvgTextShape *textShape, KoShape *shape);
using ContourMembershipBatchApplier = void (*)(KoSvgTextShape *textShape, KoShape *shape, bool inside);

bool readContourMembership(const KoSvgTextShape *textShape, KoShape *shape)
{
    return textShape->shapesInside().contains(shape);
}

void applyContourMembership(KoSvgTextShape *textShape, KoShape *shape, bool inside)
{
    KoShapeBulkActionLock lock(textShape);
    textShape->addShapeContours({shape}, inside);
    KoShapeBulkActionLock::bulkShapesUpdate(lock.unlock());
}

ContourMembershipReader contourMembershipReader = readContourMembership;
ContourMembershipBatchApplier contourMembershipBatchApplier = applyContourMembership;
} // namespace

#ifdef KRITAFLAKE_SVG_TEXT_FLIP_SHAPE_CONTOUR_TYPE_COMMAND_CONTRACT_TESTING
namespace KoSvgTextFlipShapeContourTypeCommandTesting
{
void setContourAccessForTesting(ContourMembershipReader reader, ContourMembershipBatchApplier applier)
{
    contourMembershipReader = reader;
    contourMembershipBatchApplier = applier;
}

void resetContourAccessForTesting()
{
    contourMembershipReader = readContourMembership;
    contourMembershipBatchApplier = applyContourMembership;
}
} // namespace KoSvgTextFlipShapeContourTypeCommandTesting
#endif

struct KoSvgTextFlipShapeContourTypeCommand::Private {
    Private(KoSvgTextShape *_text, KoShape *_shape)
        : textShape(_text)
        , shape(_shape)
        , isInside(contourMembershipReader(textShape, shape))
    {
    }

    KoSvgTextShape *textShape;
    KoShape *shape;
    bool isInside = false;
};

KoSvgTextFlipShapeContourTypeCommand::KoSvgTextFlipShapeContourTypeCommand(KoSvgTextShape *textShape,
                                                                           KoShape *shape,
                                                                           KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new Private(textShape, shape))
{
}

KoSvgTextFlipShapeContourTypeCommand::~KoSvgTextFlipShapeContourTypeCommand()
{
}

void KoSvgTextFlipShapeContourTypeCommand::redo()
{
    contourMembershipBatchApplier(d->textShape, d->shape, !d->isInside);
}

void KoSvgTextFlipShapeContourTypeCommand::undo()
{
    contourMembershipBatchApplier(d->textShape, d->shape, d->isInside);
}
